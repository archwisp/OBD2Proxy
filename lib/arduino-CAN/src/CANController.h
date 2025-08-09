// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <Arduino.h>

// Maximum number of CAN controller instances supported
#define MAX_CAN_CONTROLLER_INSTANCES 4

typedef void (*TCallback)(int);
typedef void (*TErrorCallback)(int, int);

struct CANFrame {
    unsigned long id;
    bool is_extended;
    bool is_remote;
    bool is_retransmit;
    int data_len;
    char data[8];
    unsigned long timestamp;
};

class CANControllerClass {

public:
  CANControllerClass(const int instance_id, Stream* debug);

  void configureCallback(TCallback callback);

  // Has a frame been recieved
  int available();
  
  static void onInterrupt0();
  static void onInterrupt1();
  static void onInterrupt2();
  static void onInterrupt3();

protected:
  // The hardware specifics are handled by the derived classes
  virtual void configureHardwareInterrupt(void (*interruptHandler)()) = 0;
  virtual void handleInterrupt() = 0;

  // These are used by the derived classes
  void doCallback();
  static Stream* _debug;

private:
  // Constructor requires instance_id and debug parameters
  static CANControllerClass* _instances[MAX_CAN_CONTROLLER_INSTANCES];
  int _instance_id;
  TCallback _onReceive;
  TErrorCallback _onError;

  void _attachInterruptToInstance();      
};
