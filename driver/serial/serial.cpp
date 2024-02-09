#include <iostream>
#include <unistd.h>

#include <cstring>

#include "serial.h"
#include "packet.h"

Serial::Serial(std::string path, const BaudRate baudRate)
    : path(std::move(path)), baudRate(baudRate) {
}

void Serial::open() {
    fd = ::open(path.c_str(), O_RDWR | O_NOCTTY);

    std::cout << "File descriptor: " << fd << std::endl;
    if (fd == -1) {
        exit(fd);
    }

    /* Error Handling */
    if (tcgetattr(fd, &tty) != 0) {
        std::cout << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
    }

    /* Set Baud Rate */
    cfsetospeed(&tty, static_cast<speed_t>(this->baudRate));
    cfsetispeed(&tty, static_cast<speed_t>(this->baudRate));

    /* Setting other Port Stuff */
    tty.c_cflag &= ~PARENB; // Make 8n1
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cflag &= ~CRTSCTS; // no flow control
    tty.c_cc[VMIN] = 1; // Minimal bytes received to be able to return from read.
    tty.c_cc[VTIME] = 2; // Timeout in tenths of a second to a next data byte income.
    tty.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tty);

    /* Flush Port, then applies attributes */
    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cout << "Error " << errno << " from tcsetattr" << std::endl;
    }
}

bool Serial::writePacket(const Packet *packet) const {
    if (::write(fd, reinterpret_cast<void *>(static_cast<uint8_t>(packet->packetType)), 1) != sizeof(uint8_t))
        return false;
    if (::write(fd, &packet->payloadSize, sizeof(uint32_t)) != sizeof(uint32_t))
        return false;
    if (::write(fd, packet->payload, packet->payloadSize) != packet->payloadSize)
        return false;

    return true;
}

Packet *Serial::readPacket() const {
    uint8_t packetTypeByte;
    PacketType packetType;
    auto readBytes = ::read(fd, &packetTypeByte, 1);
    if (readBytes < 0) {
        std::cerr << "Error reading packet type: " << strerror(errno) << std::endl;
        throw;
    }
    if (packetTypeByte >= static_cast<uint8_t>(PacketType::UNDEFINED))
        packetType = PacketType::UNDEFINED;
    else
        packetType = static_cast<PacketType>(packetTypeByte);

    uint32_t packetLength;
    readBytes = ::read(fd, &packetLength, 4);
    if (readBytes < 0) {
        std::cerr << "Error reading packet length: " << strerror(errno) << std::endl;
        throw;
    }
    std::cout << "Read packet length: " << packetLength << std::endl;

    auto *packetData = new char[packetLength];
    readBytes = ::read(fd, packetData, packetLength);
    if (readBytes < 0) {
        std::cerr << "Error reading packet data, read: " << readBytes << "/" << packetLength
                << ", error:" << strerror(errno) << std::endl;
        throw;
    }
    std::cout << "Read packet: " << packetData << std::endl;;
    return new Packet(packetLength, packetType, packetData);
}
