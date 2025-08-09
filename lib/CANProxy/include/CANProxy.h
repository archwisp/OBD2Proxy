// vim: ts=4:sw=4:et

#ifndef CAN_PROXY_H
#define CAN_PROXY_H

#include <Arduino.h>
#include <CANStream.h>
#include <OBD2Responder.h>

// Statistics tracking
struct CANProxyStats {
    unsigned long frames_received_can1;
    unsigned long frames_received_can2;
    unsigned long frames_forwarded_can1_to_can2;
    unsigned long frames_forwarded_can2_to_can1;
    unsigned long errors_can1;
    unsigned long errors_can2;
};

class CANProxy {
private:
    CANStream _can1;
    CANStream _can2;
 
    OBD2Responder* _obd2_responder = nullptr;
    static int _obd2_responder_gpio_pin;
    static bool _obd2_responder_gpio_enabled;
    static void handleOBD2ResponderGPIOEnable();

    CANProxyStats _stats;
    
    // Debug output
    static Stream* _debug;
    
public:
    CANProxy(const CANConfig& config1, const CANConfig& config2, Stream* debug = nullptr);
    ~CANProxy();
    static Stream* getDebugOutput();
    
    // Initialization
    int begin();
    void end();
    
    // Frame handling
    void handleFrames();
    
    // Statistics
    void resetStats();
    void printStats();
    CANProxyStats getStats() const { return _stats; }
    
    // Direct CAN access (for OBD-II emulation)
    CANStream* getCAN1() { return &_can1; }
    CANStream* getCAN2() { return &_can2; }

    // Activate OBD2 responder with GPIO control
    // gpio_pin: GPIO pin number to control responder enable/disable (HIGH=enable, LOW=disable)
    void activateOBD2Responder(int gpio_pin); 
    
    // Configuration
    void dumpRegisters();
    
    // GPIO status
    bool isOBD2ResponderEnabled() const;
    
    // Hardware detection
    bool detectHardware();
    void printHardwareStatus();
};

#endif // CAN_PROXY_H 
