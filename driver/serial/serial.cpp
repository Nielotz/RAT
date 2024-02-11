#include <iostream>
#include <unistd.h>
#include <cstring>

#include "serial.h"
#include "converters.h"
#include "packet.h"

Serial::Serial(std::string path, const BaudRate baudRate)
    : path(std::move(path)), baudRate(baudRate) {
}

bool Serial::open() {
    fd = ::open(path.c_str(), O_RDWR | O_NOCTTY);

    std::cout << "File descriptor: " << fd << std::endl;
    if (fd == -1)
        return false;

    /* Error Handling */
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return false;
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
    tty.c_cc[VTIME] = 5; // Timeout in tenths of a second to a next data byte income.
    tty.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines

    /* Make raw */
    cfmakeraw(&tty);

    /* Flush Port, then applies attributes */
    tcflush(fd, TCIFLUSH);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcsetattr" << std::endl;
        return false;
    }

    return true;
}

bool Serial::writePacket(const std::unique_ptr<Packet> &packet) const {
    const auto packetType = static_cast<uint8_t>(packet->packetType);
    const auto payloadSize = static_cast<uint32_t>(packet->payload.size());
    const auto payloadSizeAsChars = convert32bitTo4<uint32_t, char>(payloadSize);

    std::cout << "Sending packet: '" << packet->str() << "', raw: 0x"
    << std::hex
    << static_cast<uint32_t>(packetType) << " " << payloadSize << " " << "..." << std::endl
    << std::dec;

    if (::write(fd, &packetType, 1) != sizeof(uint8_t))
        return false;
    if (::write(fd, payloadSizeAsChars.data(), sizeof(uint32_t)) != sizeof(uint32_t))
        return false;
    if (::write(fd, packet->payload.data(), payloadSize) != payloadSize)
        return false;

    return true;
}

std::unique_ptr<Packet> Serial::readPacket() const {
    uint8_t packetTypeBuff;
    auto readBytes = ::read(fd, &packetTypeBuff, 1);
    if (readBytes == 0) {
        std::cout << "There is no packet in the queue." << std::endl;
        return nullptr;
    }

    if (readBytes != 1) {
        std::cerr << "Error reading packet type: " << strerror(errno) << std::endl;
        throw;
    }

    PacketType packetType;
    if (packetTypeBuff >= static_cast<uint8_t>(PacketType::UNDEFINED))
        packetType = PacketType::UNDEFINED;
    else
        packetType = static_cast<PacketType>(packetTypeBuff);

    std::vector<char> buff(4);
    readBytes = ::read(fd, buff.data(), 4);
    const uint32_t payloadSize = convert4x8BitsTo32<char, uint32_t>(buff);

    if (readBytes != 4) {
        std::cerr << "Error reading packet length: " << strerror(errno) << std::endl;
        throw;
    }
    std::cout << "Read payload size: " << payloadSize << ", ";

    std::vector<char> payload(payloadSize);
    readBytes = ::read(fd, payload.data(), payloadSize);
    if (readBytes != payloadSize) {
        std::cerr << "Error reading packet data, read: " << readBytes << "/" << payloadSize
        << ", error:" << strerror(errno) << std::endl;
        throw;
    }
    auto readPacket = std::make_unique<Packet>(packetType, payload);
    std::cout << readPacket->str() << "'" << std::endl;
    return readPacket;
}
