// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifdef ESPSJA1000

#include "CANController.h"

struct SJA1000Status {
    uint32_t apb_freq;
    uint8_t mode, clk_div, btr0, btr1;
};

class ESP32SJA1000Class : protected CANControllerClass {

public:
  // Constructor requires instance_id and debug parameters
  ESP32SJA1000Class(const int instance_id = 0, Stream* debug = nullptr);
  virtual ~ESP32SJA1000Class();

  virtual int begin(long baudRate);
  virtual void end();

  virtual int endFrame();
  virtual int parseFrame();

  using CANControllerClass::filter;
  virtual int filter(int id, int mask);
  using CANControllerClass::filterExtended;
  virtual int filterExtended(long id, long mask);
  
  void setPins(int rx, int tx);

  void dumpRegisters(Stream& out);
  uint8_t readRegister(uint8_t address);
  bool isTxBufferFree();
  bool isTxComplete();
  bool isBusRecessive();
  bool isReadyToTransmit();
  SJA1000Status getStatus();

protected:
  void handleInterrupt();

private:
  gpio_num_t _rxPin;
  gpio_num_t _txPin;
  bool _loopback;
  intr_handle_t _intrHandle;
  bool _transmitting;

  void reset();
  void modifyRegister(uint8_t address, uint8_t mask, uint8_t value);
  void writeRegister(uint8_t address, uint8_t value);
};

#endif
