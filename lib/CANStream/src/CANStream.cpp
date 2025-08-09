// vim: ts=4:sw=4:et

#include <Arduino.h>
#include <SPI.h>
#include <BinaryString.h>
#include <HexString.h>
#include <CANStream.h>

// Static instance pointer for interrupt callbacks
CANStream* CANStream::_instances[MAX_CAN_STREAM_INSTANCES] = {nullptr};
Stream* CANStream::_debug = nullptr;

CANStream::CANStream(const CANConfig& config, Stream* debug) : _config(config), _can(config.instance_id, debug) {
    // Initialize ring buffer indices
    _state.buffer_head = 0;
    _state.buffer_tail = 0;
    _state.frames_buffered = 0;
    
    // Set instance pointer for interrupt callbacks
    if (config.instance_id >= 0 && config.instance_id < MAX_CAN_STREAM_INSTANCES) {
        _instances[config.instance_id] = this;
    }

    // Set debug output
    _debug = debug;
}

int CANStream::begin() {
    if (_debug) {
        _debug->print("CANStream: Initializing ");
        _debug->println(_config.name);
    }
    
    // Configure CAN controller
    _can.setSPIPins(_config.spi_sck_pin, _config.spi_miso_pin, 
                    _config.spi_mosi_pin, _config.cs_pin, _config.irq_pin);
    _can.setClockFrequency(_config.clock_frequency);
    _can.setSPISettings(_config.spi_frequency, _config.spi_bit_order, _config.spi_mode);
    
    // Set up interrupt callback
    _can.configureCallback(onReceive);
    
    // Initialize CAN with SPI initialization
    int result = _can.begin(_config.baud_rate, true);
    
    if (result == 1) {
        if (_debug) {
            _debug->print("CANStream: Successfully initialized ");
            _debug->println(_config.name);
        }
    } else {
        if (_debug) {
            _debug->print("CANStream: Failed to initialize ");
            _debug->print(_config.name);
            _debug->print(" with error ");
            _debug->println(result);
        }
        _state.error_count++;
    }
    
    return result;
}

bool CANStream::available() {
    return _state.buffer_head != _state.buffer_tail;
}

// When responding to CAN frames, only respond to the last one received
CANFrame CANStream::getLastFrame() {
    delayMicroseconds(_state.delay_after_receive); // Wait for ACK, EOF, and IFS to fully elapse

    if (!available()) {
        // Return empty frame if no data available
        CANFrame empty_frame = {0, false, false, false, 0, {0}, 0};
        return empty_frame;
    }

    // Get the most recent frame (last frame added to buffer)
    unsigned int last_index = (_state.buffer_head - 1 + _state.frame_buffer_size) % _state.frame_buffer_size;
    return _state.frame_buffer[last_index];
}

// After responding to a frame, clear the buffer because any old frames are not
// longer valid 
void CANStream::clearBuffer() {
    _state.buffer_head = 0;
    _state.buffer_tail = 0;
    _state.frames_buffered = 0;
}

CANFrame CANStream::read() {
    delayMicroseconds(_state.delay_after_receive); // Wait for ACK, EOF, and IFS to fully elapse
    
    if (!available()) {
        // Return empty frame if no data available
        CANFrame empty_frame = {0, false, false, false, 0, {0}, 0};
        return empty_frame;
    }
    
    // Get the oldest frame from the ring buffer (FIFO order)
    CANFrame frame = _state.frame_buffer[_state.buffer_tail];
    
    // Advance the tail pointer
    _state.buffer_tail = (_state.buffer_tail + 1) % _state.frame_buffer_size;
    _state.frames_buffered--;
    
    return frame;
}

int CANStream::sendFrame(const CANFrame& frame) {
    int result = _can.transmitFrame(frame);
    if (result == 1) {
        _state.frames_sent++;
        if (_debug) {
            _debug->print("CANStream: Sent frame\n");
            printFrameData(frame);
        }
    } else {
        _state.error_count++;
        if (_debug) {
            _debug->print("CANStream: Failed to send frame, error ");
            _debug->println(result);
        }
    }
    
    return result;
}

void CANStream::printFrameData(const CANFrame &frame) {
    // Safety check - ensure _debug is valid
    if (_debug == nullptr) {
        return; // Skip if not properly initialized
    }
    
    const int output_buffer_len = 256; 
    char output_buffer[output_buffer_len];
    memset(output_buffer, 0x0, output_buffer_len);

    int data_hex_len = (8 * 2) + 1;
    char data_hex[data_hex_len];
    memset(data_hex, 0x0, data_hex_len); 
    byte_array_to_hex(data_hex, data_hex_len, frame.data, frame.data_len);

    int data_binary_len = (8 * 8) + 1;
    char data_binary[data_binary_len];
    memset(data_binary, 0x0, data_binary_len); 
    byte_array_to_bits(data_binary, data_binary_len, frame.data, frame.data_len);  

    memset(output_buffer, 0x0, output_buffer_len);

    snprintf(output_buffer, output_buffer_len, 
        "Frame data: time: %lu, id=%x length: %u, hex: %s, binary: %s\n", 
        frame.timestamp, frame.id, frame.data_len, data_hex, data_binary
    );

    _debug->print(output_buffer);
}

void CANStream::printStats() {
    if (_debug) {
        _debug->print("CANStream ");
        _debug->print(_config.name);
        _debug->println(" Statistics:");
        _debug->print("  Frames received: ");
        _debug->println(_state.frames_received);
        _debug->print("  Frames buffered: ");
        _debug->println(_state.frames_buffered);
        _debug->print("  Ring buffer head: ");
        _debug->println(_state.buffer_head);
        _debug->print("  Ring buffer tail: ");
        _debug->println(_state.buffer_tail);
        _debug->print("  Frames sent: ");
        _debug->println(_state.frames_sent);
        _debug->print("  Frames dropped: ");
        _debug->println(_state.dropped_frames);
        _debug->print("  Errors: ");
        _debug->println(_state.error_count);
        _debug->print("  Interrupts: ");
        _debug->println(_state.interrupt_count);
    }
}

void CANStream::dumpRegisters() {
    if (_debug) {
        _debug->print("CANStream ");
        _debug->print(_config.name);
        _debug->println(" Register Dump:");
        _can.dumpRegisters();
    }
}

bool CANStream::detectHardware() {
    // Try to read a register to detect if hardware is present
    // This is a basic detection method
    return _can.begin(_config.baud_rate, false) == 1;
}

void CANStream::printHardwareStatus() {
    _debug->print("CANStream ");
    _debug->print(_config.name);
    _debug->print(" Hardware: ");
    _debug->println(detectHardware() ? "DETECTED" : "NOT DETECTED");
}

// Static lookup and call of instance callback handler
void CANStream::onReceive(const int instance_id) {
    // if (_debug) _debug->printf("CANStream interrupt on instance %d\n", instance_id);

    if (instance_id >= 0 && instance_id < MAX_CAN_STREAM_INSTANCES && _instances[instance_id]) {
        _instances[instance_id]->_onReceive();
    } else {
        // if (_debug) _debug->printf("CANStream invalid instance %d or null pointer\n", instance_id);
    }
}

// Write to the buffer as quickly as possible because this 
// is triggered by an interrupt
void CANStream::_onReceive() {
    _state.interrupt_count++;

    CANFrame frame;
    int receive_len = _can.receiveFrame(&frame);

    // Only process the frame if we actually received data
    if (receive_len <= 0) {
        return;
    }

    // Store frame in ring buffer
    unsigned int next_head = (_state.buffer_head + 1) % _state.frame_buffer_size;
    if (next_head != _state.buffer_tail) {
        // Buffer has space, store the frame
        _state.frame_buffer[_state.buffer_head] = frame;
        _state.buffer_head = next_head;
        _state.frames_buffered++;
        _state.frames_received++;
    } else {
        // Buffer is full, drop the frame
        _state.dropped_frames++;
    }
}
