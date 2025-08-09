// vim: ts=4:sw=4:et
#include <Arduino.h>
#include <CANStream.h>
#include <OBD2Responder.h>

Stream* OBD2Responder::_debug = nullptr;

// Constructor - attaches to a CANStream
OBD2Responder::OBD2Responder(
    CANStream& can_stream,
    Stream* debug
) {
    _debug = debug;
    _can_stream = &can_stream;
}

int OBD2Responder::init() {
    if (_can_stream) {
        if (_debug) _debug->println("OBD2Responder: Attached to CANStream");
        return 1;
    } else {
        if (_debug) _debug->println("OBD2Responder: ERROR - No CANStream attached");
        return -1;
    }
}

// Only respond to the most recent frame
// If we respond, clear the buffer
// Leave other packets in the buffer so they can be forwarded elsewhere
FrameResultCode OBD2Responder::handleNextFrame() {
    if (!_can_stream) {
        return PACKET_RESULT_BUS_ERROR;
    }
    
    if (!_can_stream->available()) {
        return PACKET_RESULT_NONE;
    }

	if (_debug) _debug->println();
    
    CANFrame frame = _can_stream->getLastFrame();
    if (_debug) _debug->println("Processing frame.");
    _can_stream->printFrameData(frame);

    FrameResultCode retcode;

    if (frame.is_retransmit) {
        retcode = PACKET_RESULT_RTR;
    } else if (frame.is_extended) {
        retcode = PACKET_RESULT_EXTENDED;
    } else if (frame.id == 0x7e8) {
        retcode = PACKET_RESULT_SELF;
    } else if (frame.id == 0x7df && frame.data[0] == 0x02 && frame.data[1] == 0x01 && frame.data[2] == 0x00) {
        if (_debug) _debug->println("Received request for supported PIDs");
        _can_stream->printFrameData(frame);
        sendSupportedPIDs();
        _can_stream->clearBuffer();
        retcode = PACKET_RESULT_PIDS;
    } else if (frame.id == 0x7df && frame.data[0] == 0x02 && frame.data[1] == 0x01 && frame.data[2] == 0x01) {
        if (_debug) _debug->println("Received request for monitor status");
        _can_stream->printFrameData(frame);
        sendMonitorStatus();
        _can_stream->clearBuffer();
        retcode = PACKET_RESULT_MONITOR_STATUS;
    } else if (frame.id == 0x7df && frame.data[0] == 0x01 && frame.data[1] == 0x03) {
        if (_debug) _debug->println("Received request for trouble codes");
        _can_stream->printFrameData(frame);
        sendTroubleCodes();
        _can_stream->clearBuffer();
        retcode = PACKET_RESULT_TROUBLE_CODES;
    } else if (frame.id == 0x7df && frame.data[0] == 0x01 && frame.data[1] == 0x07) {
        if (_debug) _debug->println("Received request for pending trouble codes");
        _can_stream->printFrameData(frame);
        sendTroubleCodes();
        _can_stream->clearBuffer();
        retcode = PACKET_RESULT_TROUBLE_CODES;
    } else if (frame.id == 0x7df && frame.data[0] == 0x02 && frame.data[1] == 0x01 && frame.data[2] == 0x11) {
        if (_debug) _debug->println("Received request for oxygen sensor");
        _can_stream->printFrameData(frame);
        sendOxygenSensor();
        _can_stream->clearBuffer();
        retcode = PACKET_RESULT_OXYGEN_SENSOR;
    } else {
        if (_debug) _debug->println("Unknown frame");
        _can_stream->printFrameData(frame);
        retcode = PACKET_RESULT_UNKNOWN;
    }

    return retcode;
}

// { 0x06, 0x41, 0x00, 0xbe, 0x1f, 0xe8, 0x1b, 0xCC  }
void OBD2Responder::sendSupportedPIDs() {
    if (!_can_stream) return;
    if (_debug) _debug->println("Sending supported PIDs");

    CANFrame frame = {
        .id = 0x7e8,
        .is_extended = false,
        .is_remote = false,
        .is_retransmit = false,
        .data_len = 8,
        .data = {0x06, 0x41, 0x00, 0xbe, 0x1f, 0xe8, 0x1b, 0xCC},
        .timestamp = millis()
    };

    _can_stream->sendFrame(frame);
}

void OBD2Responder::sendOxygenSensor() {
    if (!_can_stream) return;
    if (_debug) _debug->println("Sending oxygen sensor data");
    
    CANFrame frame = {
        .id = 0x7e8,
        .is_extended = false,
        .is_remote = false,
        .is_retransmit = false,
        .data_len = 8,
        .data = { 0x04, 0x41, 0x11, 0x80, 0x80, 0xCC, 0xCC, 0xCC },
        .timestamp = millis()
    };

    _can_stream->sendFrame(frame);
}
    
void OBD2Responder::setMonitorStatusFrame(CANFrame frame) {
    _monitor_status_frame = frame;
}

void OBD2Responder::sendMonitorStatus() {
    if (!_can_stream) return;
    if (_debug) _debug->println("Sending monitor status");
    _can_stream->sendFrame(_monitor_status_frame);
}

// { 0x02, 0x43, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC  }
void OBD2Responder::sendTroubleCodes() {
    if (!_can_stream) return;
    if (_debug) _debug->println("Sending monitor status");
    
    CANFrame frame = {
        .id = 0x7e8,
        .is_extended = false,
        .is_remote = false,
        .is_retransmit = false,
        .data_len = 8,
        .data = { 0x02, 0x43, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC },
        .timestamp = millis()
    };

    _can_stream->sendFrame(frame);
}
