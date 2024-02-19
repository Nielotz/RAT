// ReSharper disable CppDFAUnusedValue
#include "usb_connection.h"

#include <control.h>
#include <converters.h>
#include <USB.h>

UsbConnection *UsbConnection::debugUsbConnection = nullptr;

UsbConnection::UsbConnection(USBCDC &usb)
    : usb(usb) {
    usb.setRxBufferSize(512);
}

// TODO: Add reason of failire.
bool UsbConnection::sendPacket(const std::unique_ptr<Packet> &packet) const {
    if (!usb.availableForWrite())
        return false;

    const auto packetSizeBuff = convert32bitTo4<uint32_t, char>(
        sizeof(uint8_t) + packet->payload.size()
    );

    if (this->usb.write(packetSizeBuff.data(), sizeof(uint32_t)) != sizeof(uint32_t))
        return false;

    if (this->usb.write(static_cast<uint8_t>(packet->packetType)) != sizeof(uint8_t))
        return false;
    if (this->usb.write(packet->payload.data(), packet->payload.size()) != packet->payload.size())
        return false;

    return true;
}

std::unique_ptr<Packet> UsbConnection::receivePacket() const {
    if (!usb || !usb.available())
        return nullptr;

    std::vector<char> buffer(sizeof(uint32_t));

    this->usb.readBytes(buffer.data(), sizeof(uint32_t));
    const uint32_t packetSize = convert4x8BitsTo32<char, uint32_t>(buffer);

    PacketType packetType;
    if (const uint8_t readByte = usb.read(); readByte == -1)
        return nullptr;
    else if (readByte >= static_cast<uint8_t>(PacketType::UNDEFINED))
        packetType = PacketType::UNDEFINED;
    else
        packetType = static_cast<PacketType>(readByte);

    const auto payloadSize = packetSize - sizeof(uint8_t);
    buffer.resize(payloadSize);
    this->usb.readBytes(buffer.data(), payloadSize);

    return std::make_unique<Packet>(packetType, buffer);
}

void UsbConnection::setUsbCallback() {
    if (!callbackSet) {
        // usb.onEvent(usbEventCallback);
        callbackSet = true;
    }
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
