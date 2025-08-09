// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <SPI.h>
#include "CANController.h"

class MCP2515Class : public CANControllerClass {

public:
  MCP2515Class(const int instance_id = 0, Stream* debug = nullptr);
  
  void setSPIPins(int sck, int miso, int mosi, int cs, int irq);
  void setSPISettings(uint32_t frequency, uint8_t bitOrder, uint8_t mode);
  void setClockFrequency(long clockFrequency);
  
  int begin(long baudRate, bool initializeSPI = true);

  int receiveFrame(CANFrame* frame);
  int transmitFrame(const CANFrame frame);
 
  void dumpRegisters();
  void dumpErrors();

protected:
  // These are called by CanContrøllerClass
  void configureHardwareInterrupt(void (*interruptHandler)());
  void handleInterrupt();

private:
  SPISettings _spiSettings;
  int _csPin;
  int _intPin;
  int _spiSckPin;
  int _spiMisoPin;
  int _spiMosiPin;
  long _clockFrequency;
  bool _spiInitialized = false;
  
  int _sendReset();
  uint8_t readRegister(uint8_t address);
  void modifyRegister(uint8_t address, uint8_t mask, uint8_t value);
  void writeRegister(uint8_t address, uint8_t value);
};
