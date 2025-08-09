// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "CANController.h"

// Static instance management
CANControllerClass* CANControllerClass::_instances[MAX_CAN_CONTROLLER_INSTANCES] = {nullptr};
Stream* CANControllerClass::_debug = nullptr;

CANControllerClass::CANControllerClass(const int instance_id, Stream* debug)
{
    if (instance_id >= 0 && instance_id < MAX_CAN_CONTROLLER_INSTANCES) {
        if (_debug) _debug->printf("CANControllerClass configuring instance %d\n", instance_id);
        _instance_id = instance_id;
        _instances[instance_id] = this;
    }
    _debug = debug;
}

void CANControllerClass::configureCallback(TCallback callback)
{
    _onReceive = callback;
    _attachInterruptToInstance();
}

void CANControllerClass::doCallback() 
{
    // if (_debug) _debug->printf("CANControllerClass instance %d handling interrupt\n", _instance_id);

    if (_instance_id < 0 || _instance_id >= MAX_CAN_CONTROLLER_INSTANCES) {
        // if (_debug) _debug->printf("CANControllerClass invalid instance %d while handling interrupt\n", _instance_id);
        return;
    }

    if (_onReceive) {
        _onReceive(_instance_id);
    } else {
        // if (_debug) _debug->printf("CANControllerClass no callback registered for instance %d\n", _instance_id);
    }
}

void CANControllerClass::_attachInterruptToInstance()
{
    if (_instance_id < 0 || _instance_id >= MAX_CAN_CONTROLLER_INSTANCES) {
        if (_debug) _debug->printf("CANControllerClass invalid instance ID %d\n", _instance_id);
        return;
    }

    if (_debug) _debug->printf("CANControllerClass Attaching intterupt to instance %d\n", _instance_id);
    
    // Select the appropriate static interrupt handler based on instance_id
    void (*interruptHandler)() = nullptr;
    switch(_instance_id) {
        case 0:
            interruptHandler = onInterrupt0;
            break;
        case 1:
            interruptHandler = onInterrupt1;
            break;
        case 2:
            interruptHandler = onInterrupt2;
            break;
        case 3:
            interruptHandler = onInterrupt3;
            break;
        default:
            if (_debug) _debug->printf("CANControllerClass invalid instance ID %d for interrupt handler selection\n", _instance_id);
            return;
    }

    configureHardwareInterrupt(interruptHandler);
}

// Static interrupt handlers for each instance
void CANControllerClass::onInterrupt0()
{
    // if (_debug) _debug->printf("CANControllerClass interrupt on instance 0\n");
    if (_instances[0]) {
        _instances[0]->handleInterrupt();
    }
}

void CANControllerClass::onInterrupt1()
{
    if (_instances[1]) {
        _instances[1]->handleInterrupt();
    }
}

void CANControllerClass::onInterrupt2()
{
    if (_instances[2]) {
        _instances[2]->handleInterrupt();
    }
}

void CANControllerClass::onInterrupt3()
{
    if (_instances[3]) {
        _instances[3]->handleInterrupt();
    }
}
