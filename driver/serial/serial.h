// https://stackoverflow.com/questions/18108932/reading-and-writing-to-serial-port-in-c-on-linux

#ifndef SERIAL_H
#define SERIAL_H
#include <fcntl.h>
#include <string>
#include <termios.h>

#include "packet.h"

enum struct BaudRate {
    BR_9600 = B9600,
    BR_19200 = B19200,
    BR_38400 = B38400,
    BR_57600 = B57600,
    BR_115200 = B115200,
    BR_230400 = B230400,
    BR_460800 = B460800,
    BR_500000 = B500000,
    BR_576000 = B576000,
    BR_921600 = B921600,
    BR_1000000 = B1000000,
    BR_1152000 = B1152000,
    BR_1500000 = B1500000,
    BR_2000000 = B2000000,
    BR_2500000 = B2500000,
    BR_3000000 = B3000000,
    BR_3500000 = B3500000,
    BR_4000000 = B4000000,
};

class Serial {
public:
    Serial(std::string path, BaudRate baudRate);

    bool open();

    bool writePacket(const Packet &packet) const;

    bool writePacket(const std::unique_ptr<Packet> &packet) const;

    std::unique_ptr<Packet> readPacket() const;

private:
    termios tty = {};
    int fd = -1;
    const std::string path;
    const BaudRate baudRate;
};


#endif //SERIAL_H
