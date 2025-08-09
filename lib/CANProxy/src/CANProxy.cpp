// vim: ts=4:sw=4:et

#include <CANProxy.h>
#include <SPI.h>

// Static debug output
Stream* CANProxy::_debug = nullptr;
int CANProxy::_obd2_responder_gpio_pin;
bool CANProxy::_obd2_responder_gpio_enabled = true; // Enabled by default


CANProxy::CANProxy(const CANConfig& config1, const CANConfig& config2, Stream* debug) 
    : _can1(config1, debug), _can2(config2, debug) {
    
    // Initialize statistics
    memset(&_stats, 0, sizeof(_stats));
    
    // Set debug output
    _debug = debug;
}

CANProxy::~CANProxy() {
    if (_obd2_responder) {
        delete _obd2_responder;
        _obd2_responder = nullptr;
    }
}

int CANProxy::begin() {
    if (_debug) {
        _debug->println("CANProxy: Initializing dual CAN proxy");
    }
    
    // Initialize CAN1
    int result1 = _can1.begin();
    if (result1 != 1) {
        if (_debug) {
            _debug->print("CANProxy: Failed to initialize CAN1 with error ");
            _debug->println(result1);
        }
        return -10 + result1; // Return error code for CAN1
    }
    
    // Initialize CAN2
    int result2 = _can2.begin();
    if (result2 != 1) {
        if (_debug) {
            _debug->print("CANProxy: Failed to initialize CAN2 with error ");
            _debug->println(result2);
        }
        return -20 + result2; // Return error code for CAN2
    }
    
    if (_debug) {
        _debug->println("CANProxy: Both CAN controllers initialized successfully");
    }
    
    return 1; // Both CAN controllers initialized successfully
}

void CANProxy::end() {
    _can1.end();
    _can2.end();
    if (_debug) {
        _debug->println("CANProxy: Ended both CAN controllers");
    }
}

// Enables responding to specific frames instead of forwarding them
void CANProxy::activateOBD2Responder(int gpio_pin) {
    // Validate GPIO pin number
    if (gpio_pin < 0 || gpio_pin > 39) {
        if (_debug) {
            _debug->print("CANProxy: Invalid GPIO pin number: ");
            _debug->println(gpio_pin);
        }
        return;
    }
    
    _obd2_responder = new OBD2Responder(_can1, _debug);
    _obd2_responder->init();
    
    // All ready: { 0x06, 0x41, 0x01, 0x00, 0x07, 0xFF, 0x00, 0xCC }
    CANFrame monitor_status_frame = {
        .id = 0x7e8,
        .is_extended = false,
        .is_remote = false,
        .is_retransmit = false,
        .data_len = 8,
        .data = { 0x06, 0x41, 0x01, 0x00, 0x07, 0xFF, 0x00, 0xCC },
        .timestamp = millis()
    };

    _obd2_responder->setMonitorStatusFrame(monitor_status_frame);

    _obd2_responder_gpio_pin = gpio_pin;
    pinMode(_obd2_responder_gpio_pin, INPUT_PULLDOWN);
    attachInterrupt(digitalPinToInterrupt(_obd2_responder_gpio_pin), CANProxy::handleOBD2ResponderGPIOEnable, CHANGE);
    
    if (_debug) {
        _debug->printf("CANProxy: OBD2Responder activated with GPIO pin %d\n", gpio_pin);
    }
}

void CANProxy::handleOBD2ResponderGPIOEnable() {
    // Check GPIO pin state - only process frames when pin is HIGH
    if (digitalRead(_obd2_responder_gpio_pin) == LOW) {
        if (_debug) _debug->println("CANProxy: OBD2Responder gpio is LOW, disabling");
        _obd2_responder_gpio_enabled = false;
    } else {
        if (_debug) _debug->println("CANProxy: OBD2Responder gpio is HIGH, enabling");
        _obd2_responder_gpio_enabled = true;
    }
}

void CANProxy::handleFrames() {
    // Process frames from CAN1 buffer and forward to CAN2
    if (_can1.available()) {
        // If OBD2Responder has been activated, let it process the frame first
        if (_obd2_responder && _obd2_responder_gpio_enabled) {
            FrameResultCode result = _obd2_responder->handleNextFrame();
            
            // If OBD2Responder handled the frame (responded to it), skip forwarding
            // Positive values indicate the frame was handled
            if (result > 0) {
                _stats.frames_received_can1++;
                if (_debug) {
                    _debug->println("CANProxy: OBD2Responder handled frame, not forwarding");
                }
                return;
            }
        }

        // OBD2Responder didn't handle this frame, so forward it
        CANFrame can_frame = _can1.read();
        _stats.frames_received_can1++;
        
        // Forward frame to CAN2
        int result = _can2.sendFrame(can_frame);
        if (result == 1) {
            _stats.frames_forwarded_can1_to_can2++;
            if (_debug) {
                _debug->print("CANProxy: Forwarded frame from CAN1 to CAN2, ID: 0x");
                _debug->println(can_frame.id, HEX);
            }
        } else {
            _stats.errors_can1++;
            if (_debug) {
                _debug->print("CANProxy: Failed to forward frame from CAN1 to CAN2, error: ");
                _debug->println(result);
            }
        }
        
        // Wait up to 50ms for response
        unsigned long tx_time = millis();
        while (!_can2.available() && millis() - tx_time < 50) { }
        
        // If we got a response, forward it
        if (_can2.available()) {
            // Process frames from CAN2 buffer and forward to CAN1
            CANFrame can_frame_2 = _can2.read();
            _stats.frames_received_can2++;
            
            // Forward frame to CAN1
            result = _can1.sendFrame(can_frame_2);
            if (result == 1) {
                _stats.frames_forwarded_can2_to_can1++;
                if (_debug) {
                    _debug->print("CANProxy: Forwarded frame from CAN2 to CAN1, ID: 0x");
                    _debug->println(can_frame_2.id, HEX);
                }
            } else {
                _stats.errors_can2++;
                if (_debug) {
                    _debug->print("CANProxy: Failed to forward frame from CAN2 to CAN1, error: ");
                    _debug->println(result);
                }
            }
        }
    }
}

void CANProxy::resetStats() {
    memset(&_stats, 0, sizeof(_stats));
}

void CANProxy::printStats() {
    _debug->println("CANProxy Statistics:");
    _debug->print("  CAN1 frames received: ");
    _debug->println(_stats.frames_received_can1);
    _debug->print("  CAN2 frames received: ");
    _debug->println(_stats.frames_received_can2);
    _debug->print("  Frames forwarded CAN1->CAN2: ");
    _debug->println(_stats.frames_forwarded_can1_to_can2);
    _debug->print("  Frames forwarded CAN2->CAN1: ");
    _debug->println(_stats.frames_forwarded_can2_to_can1);
    _debug->print("  CAN1 errors: ");
    _debug->println(_stats.errors_can1);
    _debug->print("  CAN2 errors: ");
    _debug->println(_stats.errors_can2);
    
    if (_obd2_responder) {
        _debug->print("  OBD2 Responder: ");
        _debug->println(isOBD2ResponderEnabled() ? "ENABLED" : "DISABLED");
        _debug->print("  OBD2 GPIO Pin: ");
        _debug->println(_obd2_responder_gpio_pin);
    }
}

void CANProxy::dumpRegisters() {
    if (_debug) _debug->println("CANProxy Register Dumps:");
    if (_debug) _debug->println("CAN1 Registers:");
    _can1.dumpRegisters();
    if (_debug) _debug->println("CAN2 Registers:");
    _can2.dumpRegisters();
}

bool CANProxy::detectHardware() {
    return _can1.detectHardware() && _can2.detectHardware();
}

void CANProxy::printHardwareStatus() {
    _debug->println("CANProxy Hardware Status:");
    _debug->print("  CAN1: ");
    _debug->println(_can1.detectHardware() ? "DETECTED" : "NOT DETECTED");
    _debug->print("  CAN2: ");
    _debug->println(_can2.detectHardware() ? "DETECTED" : "NOT DETECTED");
} 

bool CANProxy::isOBD2ResponderEnabled() const {
    if (!_obd2_responder) {
        return false;
    }
    return digitalRead(_obd2_responder_gpio_pin) == HIGH;
}
