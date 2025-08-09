// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef ESPSJA1000

#include "esp_intr.h"
#include "soc/dport_reg.h"
#include "driver/gpio.h"

// These are to read the APB clock speed
#include <soc/rtc_cntl_reg.h>
#include <soc/soc.h>
#include <esp32/rom/rtc.h>

#include "ESP32SJA1000.h"

#define REG_BASE                   0x3ff6b000

#define REG_MOD                    0x00
#define REG_CMR                    0x01
#define REG_SR                     0x02
#define REG_IR                     0x03
#define REG_IER                    0x04

#define REG_BTR0                   0x06
#define REG_BTR1                   0x07
#define REG_OCR                    0x08

#define REG_ALC                    0x0b
#define REG_ECC                    0x0c
#define REG_EWLR                   0x0d
#define REG_RXERR                  0x0e
#define REG_TXERR                  0x0f
#define REG_SFF                    0x10
#define REG_EFF                    0x10
#define REG_ACRn(n)                (0x10 + n)
#define REG_AMRn(n)                (0x14 + n)

#define REG_EIR                    0x1C
#define REG_CDR                    0x1F

#define IR_EI 0x02 // Error Interrupt flag

#define SR_TCS 0x08  // Transmit Complete Status
#define SR_TBS 0x04  // Transmit Buffer Status (free)

ESP32SJA1000Class::ESP32SJA1000Class(const int instance_id, Stream* debug) :
  CANControllerClass(instance_id, debug),
  _rxPin(DEFAULT_CAN_RX_PIN),
  _txPin(DEFAULT_CAN_TX_PIN),
{
}

int ESP32SJA1000Class::begin(long baudRate)
{
  CANControllerClass::begin(baudRate);

  _loopback = false;

  DPORT_CLEAR_PERI_REG_MASK(DPORT_PERIP_RST_EN_REG, DPORT_CAN_RST);
  DPORT_SET_PERI_REG_MASK(DPORT_PERIP_CLK_EN_REG, DPORT_CAN_CLK_EN);

  // RX pin
  gpio_set_direction(_rxPin, GPIO_MODE_INPUT);
  gpio_matrix_in(_rxPin, CAN_RX_IDX, 0);
  gpio_pad_select_gpio(_rxPin);

  // TX pin
  gpio_set_direction(_txPin, GPIO_MODE_OUTPUT);
  gpio_matrix_out(_txPin, CAN_TX_IDX, 0, 0);
  gpio_pad_select_gpio(_txPin);

  modifyRegister(REG_CDR, 0x80, 0x80); // pelican mode

  // Masks:
  // 0xc0: bits 6-7
  // 0x3f: bits 0-5
  //
  // 0x80: bit 7
  // 0x70: bits 4-6
  // 0x0f: bits 0-3
  //
  // Validate with python:
  // format(ord(chr(0xff & ~0x07 | 0x40)), 'b').zfill(8)

  // Final SJW, BRP, TSEG1, and TSEG2 values are regval + 1
  uint8_t sjw, brp, ts, tseg1, tseg2;
  
  switch (baudRate) {
    case (long)500E3:
      // Target config is 500kbps, 75% sample point, triple sampling disabled
      // ESP32 has an 80 Mhz APB clock (79998976 hz to be exact), 
      // 80E6 / 500E3 = 160
      // So theoretically, (BRP +1) * (1 + TSEG1+1 + TSEG2+1) must equal 160
	  // The TWAI library 500k settings are brp=8, tseg_1=15, tseg_2=4, sjw=3, triple_sampling=false
      // Which is 8 * (1 + 15 + 4) = 160
      // But that doesn't seem to work at all 
      // The ESP32-WROOM-32E datasheet says the crystal on this board is 40 Mhz 
      // The TJA1000 datasheet says the clock is 24 Mhz
	  // The clock divider is set to 0x00
	  // The TJA1000 datasheet says CLKDIV 0x00 = 2 and 0x01 = 1
	  // So let's do some experiments

	  // 20 Mhz = 20E6/500E3 = 40 = 2x20; brp=2; 20x.75=15 (75% sample point); tseg1=15; 20-15=5; tseg2=5;
	  // Acks first scanner frame, sends response, scaner acks, triggers CAN error on next scanner tx bit
	  sjw = 2; ts = 0; brp = 2, tseg1 = 15, tseg2 = 5;
	  
	  // Same: sjw = 2; ts = 0; brp = 2, tseg1 = 14, tseg2 = 6;
	  // Nope: sjw = 2; ts = 0; brp = 2, tseg1 = 15, tseg2 = 6;
	  // sjw = 2; ts = 0; brp = 2, tseg1 = 16, tseg2 = 4;

	  // 24 Mhz = 24E6/500E3 = 48 = 3x16; brp=3; 16x.75=12, tseg1=12; 16-12=4; tseg2=4;
	  // Triggers error on first scanner frame CRC
	  // sjw = 3; ts = 0; brp = 3, tseg1 = 12, tseg2 = 4;
	  
	  // 12 Mhz = 12E6/500E3 = 24 = 1x24; brp=1; 24*.75=18; tseg1=18; 24-18=6; tseg2=6; 
      // Triggers CAN error after first scanner tx bit
	  // sjw = 3; ts = 0; brp = 1, tseg1 = 18, tseg2 = 6;
	  
	  // 40 Mhz = 40E6/500E3 = 80 = 4x20; brp=4; 20x.75=15 (75% sample point); tseg1=15; 20-15=5; tseg2=5;
	  // Triggers error after first scanner tx data bit
	  // sjw = 3; ts = 0; brp = 4, tseg1 = 15, tseg2 = 5;

	  // 80 Mhz = 80E6/500E3 = 160 = 8x20; brp=8; 20x.75=15; tseg1=15; 20-15=5; tseg2=5;
	  // Triggers error after first scanner tx data bit
	  // sjw = 3; ts = 0; brp = 8, tseg1 = 15, tseg2 = 5;

      modifyRegister(REG_BTR0, 0xc0, sjw -1 << 6);   // SJW (bits 6-7) -1 because final value is regval+1
      modifyRegister(REG_BTR0, 0x3f, brp -1);        // BRP (bits 0-5) -1 because final value is regval+1
      modifyRegister(REG_BTR1, 0x80, ts << 7);       // Triple Sampling (bit 7)
      modifyRegister(REG_BTR1, 0x70, tseg2 -1 << 4); // TSEG2 (bits 4-6) -1 because final value is regval+1
      modifyRegister(REG_BTR1, 0x0f, tseg1 -2);      // TSEG1 (bits 0-3) -2 to account for regval+1 + SYNC bit
      break;

    default:
      return 0;
      break;
  }

  writeRegister(REG_IER, 0xff); // enable all interrupts

  // set filters to allow anything
  writeRegister(REG_ACRn(0), 0x00);
  writeRegister(REG_ACRn(1), 0x00);
  writeRegister(REG_ACRn(2), 0x00);
  writeRegister(REG_ACRn(3), 0x00);

  writeRegister(REG_AMRn(0), 0xff);
  writeRegister(REG_AMRn(1), 0xff);
  writeRegister(REG_AMRn(2), 0xff);
  writeRegister(REG_AMRn(3), 0xff);

  // normal output mode
  modifyRegister(REG_OCR, 0x03, 0x02); 
  
  // reset error counters
  writeRegister(REG_TXERR, 0x00);
  writeRegister(REG_RXERR, 0x00);

  // clear errors and interrupts
  readRegister(REG_ECC);
  readRegister(REG_IR);

  // 0x17 = 0b00010111 → mask for:
  // Bit 0: RM (Reset Mode)
  // Bit 1: LOM (Listen Only Mode)
  // Bit 2: STM (Self-Test Mode)
  // Bit 4: AFM (Acceptance Filter Mode)
  // Value 0x00 sets them all to 0x00
  modifyRegister(REG_MOD, 0x17, 0x00);

  return 1;
}

bool ESP32SJA1000Class::_isTxComplete()
{
    return readRegister(REG_SR) & SR_TCS;
}

bool ESP32SJA1000Class::_isTxBufferFree()
{
    return readRegister(REG_SR) & SR_TBS;
} 

bool ESP32SJA1000Class::_isBusRecessive()
{
    return (readRegister(REG_SR) & (1 << 4)) != 0;  // Bit 4 is SR_RX
} 

bool ESP32SJA1000Class::_isReadyToTransmit()
{
	return _isTxBufferFree() && _isBusRecessive();
}

int ESP32SJA1000Class::transmitFrame(CANFrame& frame)
{
  int dataReg;

  if (frameExtended()) {
    writeRegister(REG_EFF, 0x80 | (_txRtr ? 0x40 : 0x00) | (0x0f & _txLength));
    writeRegister(REG_EFF + 1, _txId >> 21);
    writeRegister(REG_EFF + 2, _txId >> 13);
    writeRegister(REG_EFF + 3, _txId >> 5);
    writeRegister(REG_EFF + 4, _txId << 3);

    dataReg = REG_EFF + 5;
  } else {
    writeRegister(REG_SFF, (_txRtr ? 0x40 : 0x00) | (0x0f & _txLength));
    writeRegister(REG_SFF + 1, _txId >> 3);
    writeRegister(REG_SFF + 2, _txId << 5);

    dataReg = REG_SFF + 3;
  }

  for (int i = 0; i < _txLength; i++) {
    writeRegister(dataReg + i, _txData[i]);
  }

  if ( _loopback) {
    // self reception request
    modifyRegister(REG_CMR, 0x1f, 0x10);
  } else {
    while (!_isReadyToTransmit()) {
      // yield(); // Allow background tasks to happen
    }

    // transmit request
    modifyRegister(REG_CMR, 0x1f, 0x01);
  }

  // wait for TX complete, then return
  while (!isTxComplete()) {
	  // yield();
  }
  
  _transmitting = true;
  return 1;
}

int ESP32SJA1000Class::receiveFrame(CANFrame &frame_output)
{
  long rxId = 0;
  bool rxExtended = false;
  bool rxRtr = false;
  int rxDlc = 0;
  int rxLength = 0;
  int rxIndex = 0;
  uint8_t rxData[8];

  if ((readRegister(REG_SR) & 0x01) != 0x01) {
    // no frame
    return 0;
  }

  rxExtended = (readRegister(REG_SFF) & 0x80) ? true : false;
  rxRtr = (readRegister(REG_SFF) & 0x40) ? true : false;
  rxDlc = (readRegister(REG_SFF) & 0x0f);
  rxIndex = 0;

  int dataReg;

  if (rxExtended) {
    rxId = (readRegister(REG_EFF + 1) << 21) |
            (readRegister(REG_EFF + 2) << 13) |
            (readRegister(REG_EFF + 3) << 5) |
            (readRegister(REG_EFF + 4) >> 3);

    dataReg = REG_EFF + 5;
  } else {
    rxId = (readRegister(REG_SFF + 1) << 3) | ((readRegister(REG_SFF + 2) >> 5) & 0x07);

    dataReg = REG_SFF + 3;
  }

  if (rxRtr) {
    rxLength = 0;
  } else {
    rxLength = rxDlc;

    for (int i = 0; i < rxLength; i++) {
      rxData[i] = readRegister(dataReg + i);
    }
  }

  // Save the data
  _saveFrameState(rxId, rxExtended, rxRtr, rxDlc, rxLength, rxData[8]);
  // release RX buffer
  modifyRegister(REG_CMR, 0x04, 0x04);

  return rxDlc;
}

void ESP32SJA1000Class::setPins(int rx, int tx)
{
  _rxPin = (gpio_num_t)rx;
  _txPin = (gpio_num_t)tx;
}

void ESP32SJA1000Class::dumpRegisters(Stream& out)
{
  for (int i = 0; i < 32; i++) {
    byte b = readRegister(i);

    out.print("0x");
    if (i < 16) {
      out.print('0');
    }
    out.print(i, HEX);
    out.print(": 0x");
    if (b < 16) {
      out.print('0');
    }
    out.println(b, HEX);
  }
}

void ESP32SJA1000Class::handleInterrupt()
{
  uint8_t ir;

  // Loop to catch all interrupts
  do {
  	  uint8_t ir = readRegister(REG_IR); // Read interrupts
  
	  if (ir & IR_EI) { // Check error interrupt
		  uint8_t ecc = readRegister(REG_ECC); // Get error code
		  if (ecc != 0x00) {
			  // _onError(ir, ecc); // Trigger error buffering callback
		  }
	  }

	  if (ir & 0x01) {
		  // received frame, parse and call callback
		  parseFrame();
		  _doCallback(); // Trigger frame buffering callback
	  } 
  } while (ir); 
}

uint8_t ESP32SJA1000Class::readRegister(uint8_t address)
{
  volatile uint32_t* reg = (volatile uint32_t*)(REG_BASE + address * 4);

  return *reg;
}

void ESP32SJA1000Class::modifyRegister(uint8_t address, uint8_t mask, uint8_t value)
{
  volatile uint32_t* reg = (volatile uint32_t*)(REG_BASE + address * 4);

  *reg = (*reg & ~mask) | value;
}

void ESP32SJA1000Class::writeRegister(uint8_t address, uint8_t value)
{
  volatile uint32_t* reg = (volatile uint32_t*)(REG_BASE + address * 4);

  *reg = value;
}

SJA1000Status ESP32SJA1000Class::getStatus() 
{
    SJA1000Status status;
    status.apb_freq = ((READ_PERI_REG(RTC_APB_FREQ_REG)) & UINT16_MAX) << 12;
    status.mode = readRegister(REG_MOD);
    status.clk_div = readRegister(REG_CDR) & 0x07; // Bits 0-2 of CDR
    status.btr0 = readRegister(REG_BTR0);
    status.btr1 = readRegister(REG_BTR1);
    // char bitrate_message[80];
    // sprintf(bitrate_message, 
        // "Mode: 0x%02x, APB: %i, CLK DIV: 0x%02x, BTR0: 0x%02x, BTR1: 0x%02x", 
        // mode, apb_freq, clock_divider, btr0, btr1
    // );
    return status;
}

#endif
