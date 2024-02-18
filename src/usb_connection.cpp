// ReSharper disable CppDFAUnusedValue
#include "usb_connection.h"

#include <control.h>
#include <converters.h>
#include <USB.h>

UsbConnection *UsbConnection::debugUsbConnection = nullptr;

UsbConnection::UsbConnection(USBCDC &usb)
    : usb(usb) {
}

// TODO: Add reason of failire.
bool UsbConnection::sendPacket(const std::unique_ptr<Packet> &packet) const {
    if (!usb.availableForWrite())
        return false;

    if (this->usb.write(static_cast<uint8_t>(packet->packetType)) != sizeof(uint8_t))
        return false;

    const auto payloadSizeBuff = convert32bitTo4<uint32_t, char>(packet->payload.size());
    if (this->usb.write(payloadSizeBuff.data(), sizeof(uint32_t)) != sizeof(uint32_t))
        return false;
    if (this->usb.write(packet->payload.data(), packet->payload.size()) != packet->payload.size())
        return false;

    return true;
}

std::unique_ptr<Packet> UsbConnection::receivePacket() const {
    if (!usb || !usb.available())
        return nullptr;

    PacketType packetType;
    if (const uint8_t readByte = usb.read(); readByte == -1)
        return nullptr;
    else if (readByte >= static_cast<uint8_t>(PacketType::UNDEFINED))
        packetType = PacketType::UNDEFINED;
    else
        packetType = static_cast<PacketType>(readByte);

    std::vector<char> buffer(sizeof(uint32_t));

    this->usb.readBytes(buffer.data(), sizeof(uint32_t));
    const uint32_t payloadSize = convert4x8BitsTo32<char, uint32_t>(buffer);

    buffer.resize(payloadSize);
    this->usb.readBytes(buffer.data(), payloadSize);

    return std::make_unique<Packet>(packetType, buffer);
}

// TODO: Handle failed to send.
// ReSharper disable once CppParameterMayBeConst
void UsbConnection::usbEventCallback(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData) {
    bool failed = false;
    if (eventBase == ARDUINO_USB_EVENTS) {
        switch (eventId) {
            case ARDUINO_USB_STARTED_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket("USB PLUGGED").pack());
                break;
            case ARDUINO_USB_STOPPED_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket("USB UNPLUGGED").pack());
                break;
            case ARDUINO_USB_SUSPEND_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket(
                    std::string("USB SUSPENDED: remote_wakeup_en: ") + (
                        static_cast<arduino_usb_event_data_t *>(eventData)->suspend.remote_wakeup_en
                            ? "yes"
                            : "no")
                ).pack());
                break;
            case ARDUINO_USB_RESUME_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket("USB RESUMED").pack());
                break;
            default:
                break;
        }
    } else if (eventBase == ARDUINO_USB_CDC_EVENTS) {
        const auto *data = static_cast<arduino_usb_cdc_event_data_t *>(eventData);
        switch (eventId) {
            case ARDUINO_USB_CDC_CONNECTED_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket("CDC CONNECTED").pack());
                break;
            case ARDUINO_USB_CDC_DISCONNECTED_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket("CDC DISCONNECTED").pack());
                break;
            case ARDUINO_USB_CDC_LINE_STATE_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket(
                        std::string("CDC LINE STATE: dtr: ") + (data->line_state.dtr ? "yes" : "no") +
                        " rts: " + (data->line_state.rts ? "yes" : "no")
                    ).pack()
                );
                break;
            case ARDUINO_USB_CDC_LINE_CODING_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket(
                    std::string("CDC LINE CODING:") +
                    " bit_rate: ," + std::to_string(data->line_coding.bit_rate) +
                    " data_bits: , " + std::to_string(data->line_coding.data_bits) +
                    "stop_bits: , " + std::to_string(data->line_coding.stop_bits) +
                    "parity: " + std::to_string(data->line_coding.parity)
                ).pack());
                break;
            case ARDUINO_USB_CDC_RX_EVENT: {
                failed = debugUsbConnection->sendPacket(
                    DebugPacket("CDC RX " + std::to_string(data->rx.len)).pack());
                uint8_t buf[data->rx.len];
                // const size_t len = usbSerial.read(buf, data->rx.len);

                // usb.sendPacket(DebugPacket(std::s(buf, len)).pack());

                // usbSerial.write(buf, data->rx.len);
            }
            break;
            case ARDUINO_USB_CDC_RX_OVERFLOW_EVENT:
                failed = debugUsbConnection->sendPacket(DebugPacket("CDC RX Overflow").pack());
            // sendPacket(debugSerial, DebugPacket("CDC RX Overflow of %d bytes",
            // data->rx_overflow.dropped_bytes).pack());
                break;

            default:
                break;
        }
    }
}

bool UsbConnection::handleHanshakePacket(const std::unique_ptr<HandshakePacket> &handshakePacket) {
    switch (handshakePacket->stage) {
        case HandshakePacket::HandshakeStage::SYN:
            this->syn = 0;
            return this->sendPacket(HandshakePacket(HandshakePacket::HandshakeStage::SYN_ACK, this->syn++).pack());
        case HandshakePacket::HandshakeStage::SYN_ACK:
            return this->sendPacket(DebugPacket("uC side SYN_ACK not supported").pack());
        case HandshakePacket::HandshakeStage::ACK:
            return this->sendPacket(DebugPacket("Handshake ACK OK").pack());
        case HandshakePacket::HandshakeStage::INVALID:
        default:
            return this->sendPacket(DebugPacket("Received invalid handshake").pack());
    }
}

/* Handle usb.
 *
 * Recives up to *packetsAmountLimit* packets, and handles them.
 */
void UsbConnection::handleUsb(int packetsAmountLimit) {
    if (!usb)
        return;
    if (!callbackSet) {
        usb.onEvent(usbEventCallback);
        callbackSet = true;
    }

    while (--packetsAmountLimit) {
        const std::unique_ptr<Packet> packet = receivePacket();
        if (packet == nullptr)
            return;

        sendPacket(DebugPacket("Received packet: " + packet->str()).pack());
        switch (packet->packetType) {
            case PacketType::HANDSHAKE: {
                sendPacket(DebugPacket("Packet is a handshake.").pack());
                const auto &handshakePacket = HandshakePacket::unpack(packet);
                this->sendPacket(DebugPacket(handshakePacket->str()).pack());
                    // ReSharper disable once CppExpressionWithoutSideEffects

                const bool success = handleHanshakePacket(handshakePacket);
                sendPacket(
                    DebugPacket(std::string("Handled handshake: ") + (success ? "success" : "failed")).pack());
            }

            break;
            case PacketType::DEBUG:
            case PacketType::UNDEFINED:
            default: ;
        }
    }
    return ;
}

