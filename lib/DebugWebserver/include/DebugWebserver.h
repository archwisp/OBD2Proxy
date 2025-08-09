#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_http_server.h>

class DebugWebserver : public Stream {
    httpd_handle_t server;
    uint16_t port;
    
    static const size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    size_t buffer_pos = 0;
    
    static httpd_uri_t uri_get;
    static DebugWebserver* instance;
    static bool instance_created;
    
    public:
    DebugWebserver();
    DebugWebserver(uint16_t port);
    int start();
    void stop();
    void send(const char* message);
    void send(const char* message, const unsigned int length);

    // Stream interface
    virtual size_t write(uint8_t byte) override;
    virtual size_t write(const uint8_t *data, size_t size) override;
    virtual void flush() override;

    virtual int available() override { return 0; }
    virtual int read() override { return -1; }
    virtual int peek() override { return -1; }

    using Print::write; // Pull in Print::write overrides
    
    private:
    static esp_err_t handleGetRequest(httpd_req_t *req);
    void updateWebContent();
}; 
