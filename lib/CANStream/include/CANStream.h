// vim: ts=4:sw=4:et

#ifndef CAN_STREAM_H
#define CAN_STREAM_H

#include <Arduino.h>
#include <MCP2515.h>

// Maximum number of CANStream instances supported
#define MAX_CAN_STREAM_INSTANCES 4

// Configuration for CAN controller
struct CANConfig {
    // Index for reading/writing this instance's state data to the static store
    int instance_id;
    
    int cs_pin;
    int irq_pin;
    long baud_rate;
    long clock_frequency;
    const char* name;
    
    // SPI pin configuration
    int spi_sck_pin;   // SPI clock pin
    int spi_miso_pin;  // SPI MISO pin
    int spi_mosi_pin;  // SPI MOSI pin
    uint32_t spi_frequency; // SPI frequency in Hz
    
    // SPI mode configuration
    uint8_t spi_bit_order;  // SPI bit order (MSBFIRST or LSBFIRST)
    uint8_t spi_mode;       // SPI mode (SPI_MODE0, SPI_MODE1, SPI_MODE2, SPI_MODE3)
};

// Frame data structure for CAN messages
// struct CANFrame {
    // unsigned long id;
    // bool is_extended;
    // bool is_remote;
    // bool is_retransmit;
    // int data_len;
    // char data[8];
    // unsigned long timestamp;
// };

// For tuning purposes, scanner sends a frame about every 75-100 milliseconds
// Only the most recent frame in the buffer gets processed, so the buffer can be small
struct CANStreamState {
    CANFrame frame_buffer[128];  // Static array instead of pointer
	unsigned int delay_after_receive = 50; // microseconds
    unsigned int frames_buffered = 0;
    unsigned int frames_processed = 0;
    unsigned int dropped_frames = 0;
    unsigned int frame_buffer_size = 20;
    unsigned int buffer_head = 0;  // Ring buffer head index
    unsigned int buffer_tail = 0;  // Ring buffer tail index
    unsigned long error_count = 0;
    unsigned long interrupt_count = 0;
    unsigned long frames_received = 0;
    unsigned long frames_sent = 0;
};

class CANStream {
private:
    MCP2515Class _can;
    CANConfig _config;
    CANStreamState _state;
    
    // Static instance pointer for interrupt callbacks
    static CANStream* _instances[MAX_CAN_STREAM_INSTANCES];
    
    // Instance callback function for receiving frames
    void _onReceive();
    
    // Debug output
    static Stream* _debug;
    
    // Internal methods
    void _handleInterrupt();
    
public:
    CANStream(const CANConfig& config, Stream* debug = nullptr);
    
    // Initialization
    int begin();
    void end();
    
    // Frame handling
    bool available();
    CANFrame read();
    CANFrame getLastFrame();
    void clearBuffer();
   
    // Sending frames
    int sendFrame(const CANFrame& frame);
    
    // Statistics
    void printStats();
    
    // Configuration
    void dumpRegisters();
    
    // Hardware detection
    bool detectHardware();
    void printHardwareStatus();
    void printFrameData(const CANFrame &frame);
    
    // Direct access to MCP2515 (for advanced usage)
    MCP2515Class* getCANController() { return &_can; }
    
    // Callback function for frame reception
    static void onReceive(const int instance_id);
};

#endif // CAN_STREAM_H 
