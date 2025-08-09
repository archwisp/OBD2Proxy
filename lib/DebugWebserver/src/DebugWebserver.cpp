#include <DebugWebserver.h>
#include <cstring>

// Static member initialization
httpd_uri_t DebugWebserver::uri_get = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = DebugWebserver::handleGetRequest,
    .user_ctx = NULL
};

DebugWebserver* DebugWebserver::instance = nullptr;
bool DebugWebserver::instance_created = false;

DebugWebserver::DebugWebserver() : port(80) {
    if (!instance_created) {
        instance = this;
        instance_created = true;
    }
    buffer_pos = 0;
    server = NULL;
}

DebugWebserver::DebugWebserver(uint16_t port) : port(port) {
    if (!instance_created) {
        instance = this;
        instance_created = true;
    }
    buffer_pos = 0;
    server = NULL;
}

int DebugWebserver::start() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        return 1;
    }
    return 0;
}

void DebugWebserver::stop() {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
}

void DebugWebserver::send(const char* message) {
    send(message, strlen(message));
}

void DebugWebserver::send(const char* message, const unsigned int length) {
    write((const uint8_t*)message, length);
}

// Stream interface implementation
size_t DebugWebserver::write(uint8_t byte) {
    if (buffer_pos >= BUFFER_SIZE - 1) {
        flush();
    }
    buffer[buffer_pos++] = byte;
    return 1;
}

size_t DebugWebserver::write(const uint8_t *data, size_t size) {
    size_t written = 0;
    while (size > 0) {
        size_t space = BUFFER_SIZE - buffer_pos - 1;
        size_t to_copy = (size < space) ? size : space;
        memcpy(buffer + buffer_pos, data, to_copy);
        buffer_pos += to_copy;
        written += to_copy;
        data += to_copy;
        size -= to_copy;

        if (buffer_pos >= BUFFER_SIZE - 1) {
            flush();
        }
    }
    return written;
}

void DebugWebserver::flush() {
    if (buffer_pos == 0) return;

    // Null-terminate the buffer for web display
    buffer[buffer_pos] = '\0';
    
    // Update the web server content
    updateWebContent();
    
    buffer_pos = 0;
}

void DebugWebserver::updateWebContent() {
    // This method is called during flush() but we don't need to do anything here
    // The actual HTML generation happens in handleGetRequest() when the page is accessed
    // This prevents stack overflow from large local arrays
}

esp_err_t DebugWebserver::handleGetRequest(httpd_req_t *req) {
    if (!instance) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // Set content type to plain text
    httpd_resp_set_type(req, "text/plain");
    
    // Send debug content as plain text
    httpd_resp_send(req, instance->buffer, strlen(instance->buffer));
    
    return ESP_OK;
} 