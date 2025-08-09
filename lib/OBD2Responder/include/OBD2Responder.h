// vim: ts=4:sw=4:et

typedef enum {
    PACKET_RESULT_TROUBLE_CODES = 3,
    PACKET_RESULT_MONITOR_STATUS = 2,
    PACKET_RESULT_PIDS = 1,
    PACKET_RESULT_OXYGEN_SENSOR = 4,
    PACKET_RESULT_NONE = 0,
    PACKET_RESULT_RTR = -1,
    PACKET_RESULT_EXTENDED = -2,
    PACKET_RESULT_SELF = -3,
    PACKET_RESULT_UNKNOWN = -4,
    PACKET_RESULT_DUPLICATE = -5,
    PACKET_RESULT_BUS_ERROR = -6,
} FrameResultCode;

// Forward declarations
class CANStream;

class OBD2Responder {
	static Stream* _debug;
	CANStream* _can_stream;
	
	public:

    OBD2Responder(
        CANStream& can_stream,
        Stream* debug = nullptr
    );
    
    void setMonitorStatusFrame(CANFrame frame);
    int init();
	
    FrameResultCode handleNextFrame();
	void sendSupportedPIDs();
	void sendMonitorStatus();
	void sendTroubleCodes();
	void sendOxygenSensor();

    private:

    // All ready: { 0x06, 0x41, 0x01, 0x00, 0x07, 0xFF, 0x00, 0xCC }
    // All but O2 sensor: { 0x06, 0x41, 0x01, 0x00, 0x07, 0xFF, 0x20, 0xCC }
    CANFrame _monitor_status_frame = {
        .id = 0x7e8,
        .is_extended = false,
        .is_remote = false,
        .is_retransmit = false,
        .data_len = 8,
        .data = { 0x06, 0x41, 0x01, 0x00, 0x07, 0xFF, 0x20, 0xCC },
        .timestamp = millis()
    };
}; 
