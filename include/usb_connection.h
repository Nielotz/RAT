#ifndef SERIALCONNECTION_H
#define SERIALCONNECTION_H
#include <esp_event_base.h>
#include <USBCDC.h>

#include "packet.h"

class UsbConnection {
public:
    USBCDC& usb;
    uint32_t syn = -1;
    explicit UsbConnection(USBCDC &usb);
    bool sendPacket(const std::unique_ptr<Packet> &packet) const;

    /* Receive packet or nullptr when no packet available. */
    std::unique_ptr<Packet> receivePacket() const;

    static void usbEventCallback(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);

    /**
     * \brief Handle handshake procedure.
     * \param packet
     * \return success
     */
    bool handleHanshakePacket(const std::unique_ptr<HandshakePacket> &packet);

    void handleUsb(int packetsAmountLimit = 5);

    static UsbConnection* debugUsbConnection;
private:
    bool callbackSet = false;
};


#endif //SERIALCONNECTION_H
