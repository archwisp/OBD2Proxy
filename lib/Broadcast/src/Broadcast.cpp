#include <Broadcast.h>

Broadcast::Broadcast(const char* ip, const uint port) {
    this->port = port;
    strncpy(this->ip, ip, 16);
}

int Broadcast::connect() {
    return this->udp.begin(0);
}

void Broadcast::send(const char* message, const unsigned int length) {
    write((const uint8_t*)message, length);
}

void Broadcast::send(const char* message) {
    send(message, strlen(message));
}

// Stream interface implementation
size_t Broadcast::write(uint8_t byte) {
    // if (buffer_pos >= BUFFER_SIZE) {
        // flush();
    // }
    buffer[buffer_pos++] = byte;
    return 1;
}

size_t Broadcast::write(const uint8_t *data, size_t size) {
    size_t written = 0;
    while (size > 0) {
        size_t space = BUFFER_SIZE - buffer_pos;
        size_t to_copy = (size < space) ? size : space;
        memcpy(buffer + buffer_pos, data, to_copy);
        buffer_pos += to_copy;
        written += to_copy;
        data += to_copy;
        size -= to_copy;

        // if (buffer_pos == BUFFER_SIZE) {
            // flush();
        // }
    }
    return written;
}

void Broadcast::flush() {
    if (buffer_pos == 0) return;

    udp.beginPacket(this->ip, this->port);
    udp.write(buffer, buffer_pos);
    udp.endPacket();

    buffer_pos = 0;
}
