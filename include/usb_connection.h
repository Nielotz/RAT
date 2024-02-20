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

    bool sendPacket(const Packet &packet) const;

    /* Receive packet or nullptr when no packet available. */
    std::unique_ptr<Packet> receivePacket() const;

    void setUsbCallback();

    static void usbEventCallback(void *arg, esp_event_base_t eventBase, int32_t eventId, void *eventData);

    static UsbConnection* debugUsbConnection;
private:
    bool callbackSet = false;
};


#endif //SERIALCONNECTION_H
