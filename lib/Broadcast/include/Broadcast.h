#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

class Broadcast : public Stream {
    WiFiUDP udp;
    uint port;
    char ip[16];

    static const size_t BUFFER_SIZE = 4096;
    uint8_t buffer[BUFFER_SIZE];
    size_t buffer_pos = 0;


    public:
    Broadcast(){};
    Broadcast(const char* ip, const uint port);
    int connect();
    void send(const char* message);
    void send(const char* message, const unsigned int length);

    // Stream interface
    virtual size_t write(uint8_t byte) override;
    virtual size_t write(const uint8_t *buffer, size_t size) override;
    virtual void flush() override;


    virtual int available() override { return 0; }
    virtual int read() override { return -1; }
    virtual int peek() override { return -1; }

    using Print::write; // Pull in Print::write overrides
};
