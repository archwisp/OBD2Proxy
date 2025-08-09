// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "MCP2515.h"

#define REG_BFPCTRL                0x0c
#define REG_TXRTSCTRL              0x0d

#define REG_CANSTAT                0x0e
#define REG_CANCTRL                0x0f

#define REG_CNF3                   0x28
#define REG_CNF2                   0x29
#define REG_CNF1                   0x2a

#define REG_CANINTE                0x2b
#define REG_CANINTF                0x2c

#define FLAG_RXnIE(n)              (0x01 << n)
#define FLAG_RXnIF(n)              (0x01 << n)
#define FLAG_TXnIF(n)              (0x04 << n)

#define REG_RXFnSIDH(n)            (0x00 + (n * 4))
#define REG_RXFnSIDL(n)            (0x01 + (n * 4))
#define REG_RXFnEID8(n)            (0x02 + (n * 4))
#define REG_RXFnEID0(n)            (0x03 + (n * 4))

#define REG_RXMnSIDH(n)            (0x20 + (n * 0x04))
#define REG_RXMnSIDL(n)            (0x21 + (n * 0x04))
#define REG_RXMnEID8(n)            (0x22 + (n * 0x04))
#define REG_RXMnEID0(n)            (0x23 + (n * 0x04))

#define REG_TXBnCTRL(n)            (0x30 + (n * 0x10))
#define REG_TXBnSIDH(n)            (0x31 + (n * 0x10))
#define REG_TXBnSIDL(n)            (0x32 + (n * 0x10))
#define REG_TXBnEID8(n)            (0x33 + (n * 0x10))
#define REG_TXBnEID0(n)            (0x34 + (n * 0x10))
#define REG_TXBnDLC(n)             (0x35 + (n * 0x10))
#define REG_TXBnD0(n)              (0x36 + (n * 0x10))

#define REG_RXBnCTRL(n)            (0x60 + (n * 0x10))
#define REG_RXBnSIDH(n)            (0x61 + (n * 0x10))
#define REG_RXBnSIDL(n)            (0x62 + (n * 0x10))
#define REG_RXBnEID8(n)            (0x63 + (n * 0x10))
#define REG_RXBnEID0(n)            (0x64 + (n * 0x10))
#define REG_RXBnDLC(n)             (0x65 + (n * 0x10))
#define REG_RXBnD0(n)              (0x66 + (n * 0x10))

#define FLAG_IDE                   0x08
#define FLAG_SRR                   0x10
#define FLAG_RTR                   0x40
#define FLAG_EXIDE                 0x08

#define FLAG_RXM0                  0x20
#define FLAG_RXM1                  0x40

#define CMD_WRITE				   0x02
#define CMD_READ				   0x03
#define CMD_UPDATE				   0x05
#define CMD_RESET				   0xC0

#define CANSTAT_NORMAL 			   0x00
#define CANSTAT_CONFIG 			   0x80

#define FLAG_TX0IF 0x04  // Bit 2
#define FLAG_TX1IF 0x08  // Bit 3
#define FLAG_TX2IF 0x10  // Bit 4

#define FLAG_ERRIF  0x20 // bit 5 of REG_CANINTF (0x2C) 
#define REG_EFLG    0x2D // Error Flag Register
#define REG_TEC     0x1C // Transmit Error Counter
#define REG_REC     0x1D // Receive Error Counter

// Constructor
MCP2515Class::MCP2515Class(const int instance_id, Stream* debug): 
    CANControllerClass(instance_id, debug)
{
  if (debug) debug->printf("MCP2515Class configuring instance %d\n", instance_id);
  // CANControllerClass(instance_id, debug);

  _csPin = -1;
  _intPin = -1;
  _spiSckPin = -1;
  _spiMisoPin = -1;
  _spiMosiPin = -1;
  _clockFrequency = 8E6;
  _spiInitialized = false;
}

void MCP2515Class::setSPIPins(int sck, int miso, int mosi, int cs, int irq)
{
  _spiSckPin = sck;
  _spiMisoPin = miso;
  _spiMosiPin = mosi;
  _csPin = cs;
  _intPin = irq;
}

void MCP2515Class::setSPISettings(uint32_t frequency, uint8_t bitOrder, uint8_t mode)
{
  _spiSettings = SPISettings(frequency, bitOrder, mode);
}

void MCP2515Class::setClockFrequency(long clockFrequency)
{
  _clockFrequency = clockFrequency;
}

int MCP2515Class::begin(long baudRate, bool initializeSPI)
{
  pinMode(_csPin, OUTPUT);
  pinMode(_intPin, INPUT_PULLUP);

  // Initialize SPI only if requested and not already initialized
  if (initializeSPI && !_spiInitialized) {
    SPI.begin(_spiSckPin, _spiMisoPin, _spiMosiPin, _csPin);
    _spiInitialized = true;
    delay(500);
  }

  // Reset and verify that CANSTAT is in Config mode	
  if (_sendReset() != 1) {
	return -4;
  }

  const struct {
    long clockFrequency;
    long baudRate;
    uint8_t cnf[3];
  } CNF_MAPPER[] = {
    {  (long)8E6, (long)1000E3, { 0x00, 0x80, 0x00 } },
    {  (long)8E6, (long)666666, { 0xC0, 0xB8, 0x01 } },
    {  (long)8E6,  (long)500E3, { 0x00, 0x90, 0x02 } },
    {  (long)8E6,  (long)250E3, { 0x00, 0xb1, 0x05 } },
    {  (long)8E6,  (long)200E3, { 0x00, 0xb4, 0x06 } },
    {  (long)8E6,  (long)125E3, { 0x01, 0xb1, 0x05 } },
    {  (long)8E6,  (long)100E3, { 0x01, 0xb4, 0x06 } },
    {  (long)8E6,   (long)80E3, { 0x01, 0xbf, 0x07 } },
    {  (long)8E6,   (long)50E3, { 0x03, 0xb4, 0x06 } },
    {  (long)8E6,   (long)40E3, { 0x03, 0xbf, 0x07 } },
    {  (long)8E6,   (long)20E3, { 0x07, 0xbf, 0x07 } },
    {  (long)8E6,   (long)10E3, { 0x0f, 0xbf, 0x07 } },
    {  (long)8E6,    (long)5E3, { 0x1f, 0xbf, 0x07 } },

    { (long)16E6, (long)1000E3, { 0x00, 0xd0, 0x82 } },
    { (long)16E6, (long)666666, { 0xC0, 0xF8, 0x81 } },
    { (long)16E6,  (long)500E3, { 0x00, 0xf0, 0x86 } },
    { (long)16E6,  (long)250E3, { 0x41, 0xf1, 0x85 } },
    { (long)16E6,  (long)200E3, { 0x01, 0xfa, 0x87 } },
    { (long)16E6,  (long)125E3, { 0x03, 0xf0, 0x86 } },
    { (long)16E6,  (long)100E3, { 0x03, 0xfa, 0x87 } },
    { (long)16E6,   (long)80E3, { 0x03, 0xff, 0x87 } },
    { (long)16E6,   (long)50E3, { 0x07, 0xfa, 0x87 } },
    { (long)16E6,   (long)40E3, { 0x07, 0xff, 0x87 } },
    { (long)16E6,   (long)20E3, { 0x0f, 0xff, 0x87 } },
    { (long)16E6,   (long)10E3, { 0x1f, 0xff, 0x87 } },
    { (long)16E6,    (long)5E3, { 0x3f, 0xff, 0x87 } },
  };

  const uint8_t* cnf = NULL;

  for (unsigned int i = 0; i < (sizeof(CNF_MAPPER) / sizeof(CNF_MAPPER[0])); i++) {
	if (CNF_MAPPER[i].clockFrequency == _clockFrequency && CNF_MAPPER[i].baudRate == baudRate) {
	  cnf = CNF_MAPPER[i].cnf;
	  break;
	}
  }

  if (cnf == NULL) {
	return -2;
  }

  writeRegister(REG_CNF1, cnf[0]);
  if (readRegister(REG_CNF1) != cnf[0]) { return -5; }
  writeRegister(REG_CNF2, cnf[1]);
  if (readRegister(REG_CNF2) != cnf[1]) { return -6; }
  writeRegister(REG_CNF3, cnf[2]);
  if (readRegister(REG_CNF3) != cnf[2]) { return -7; }

  writeRegister(REG_CANINTE, FLAG_RXnIE(1) | FLAG_RXnIE(0));
  writeRegister(REG_BFPCTRL, 0x00);
  writeRegister(REG_TXRTSCTRL, 0x00);
  writeRegister(REG_RXBnCTRL(0), FLAG_RXM1 | FLAG_RXM0);
  writeRegister(REG_RXBnCTRL(1), FLAG_RXM1 | FLAG_RXM0);

  // 0x00 = Normal mode
  writeRegister(REG_CANCTRL, CANSTAT_NORMAL);

  if ((readRegister(REG_CANSTAT) & 0xE0) != CANSTAT_NORMAL) {
    return -3;
  }
  

  return 1;
}

void MCP2515Class::configureHardwareInterrupt(void (*interruptHandler)())
{
    if (_debug) _debug->printf("MCP2515Class attaching hardware interrupt to pin %d\n", _intPin);

    if (!_intPin) {
        if (_debug) _debug->printf("MCP2515Class invalid interrupt pin %d\n", _intPin);
        return;
    }

    if (!interruptHandler) {
        if (_debug) _debug->printf("MCP2515Class invalid interrupt handler\n");
        return;
    }

    attachInterrupt(digitalPinToInterrupt(_intPin), interruptHandler, FALLING);
}

void MCP2515Class::handleInterrupt()
{
  // if (_debug) _debug->printf("MCP2515Class handleInterrupt called\n");
  if (!(readRegister(REG_CANINTF) & (FLAG_RXnIF(0) | FLAG_RXnIF(1)))) {
    return; // no frame in RXB0 or RXB1
  }

  doCallback();
}

int MCP2515Class::transmitFrame(const CANFrame frame)
{
  // 1) pick a free mailbox
  int n = -1;
  for (int i = 0; i < 3; i++) {
	  if (!(readRegister(REG_TXBnCTRL(i)) & 0x08)) {  // TXREQ==0?
		  n = i;
		  break;
	  }
  }

  if (n < 0) {
	  // all three mailboxes busy!
	  return 0;
  }

  // 2) load ID/DLC/data
  if (frame.is_extended) {
    writeRegister(REG_TXBnSIDH(n), frame.id >> 21);
    writeRegister(REG_TXBnSIDL(n), (((frame.id >> 18) & 0x07) << 5) | FLAG_EXIDE | ((frame.id >> 16) & 0x03));
    writeRegister(REG_TXBnEID8(n), (frame.id >> 8) & 0xff);
    writeRegister(REG_TXBnEID0(n), frame.id & 0xff);
  } else {
    writeRegister(REG_TXBnSIDH(n), frame.id >> 3);
    writeRegister(REG_TXBnSIDL(n), frame.id << 5);
    writeRegister(REG_TXBnEID8(n), 0x00);
    writeRegister(REG_TXBnEID0(n), 0x00);
  }

  if (frame.is_retransmit) {
    writeRegister(REG_TXBnDLC(n), 0x40 | frame.data_len);
  } else {
    writeRegister(REG_TXBnDLC(n), frame.data_len);

    for (int i = 0; i < frame.data_len; i++) {
      writeRegister(REG_TXBnD0(n) + i, frame.data[i]);
    }
  }

  // 3) request transmit
  writeRegister(REG_TXBnCTRL(n), 0x08);

  // 4) wait for TXREQ to clear, or abort on MLOA/ABTF/timeout
  unsigned long start = millis();
  bool aborted = false;
  while (readRegister(REG_TXBnCTRL(n)) & 0x08) {
    uint8_t s = readRegister(REG_TXBnCTRL(n));
    if (s & (0x10 /*ABTF*/ | 0x20 /*MLOA*/)) {
      aborted = true;
      break;
    }
    if (millis() - start > 10) {
      // force‐abort if no ACK
      modifyRegister(REG_CANCTRL, 0x10, 0x10); // ABAT=1
      while (readRegister(REG_TXBnCTRL(n)) & 0x08) // yield();
      modifyRegister(REG_CANCTRL, 0x10, 0x00); // ABAT=0
      aborted = true;
      break;
    }
    // yield();
  }

  // Print the status
  uint8_t s = readRegister(REG_TXBnCTRL(n));
  if (_debug) _debug->printf("TXB%d CTRL=0x%02X  TXREQ=%d  ABTF=%d  MLOA=%d\n",
          n, s,
          !!(s & 0x08),
          !!(s & 0x10),
          !!(s & 0x20));

  // 5) clear interrupts & return
  modifyRegister(REG_CANINTF, FLAG_TXnIF(n), 0x00);
  return aborted ? 0 : 1;
}

int MCP2515Class::receiveFrame(CANFrame* frame)
{
  int n; // which rx buffer
  uint8_t intf = readRegister(REG_CANINTF);

  if (intf & FLAG_RXnIF(0)) { // CAN message is available in RX buffer 0
    n = 0;
  } else if (intf & FLAG_RXnIF(1)) { // CAN message is available in RX buffer 1
    n = 1;
  } else {
    return 0;
  }
  
  frame->is_extended = (readRegister(REG_RXBnSIDL(n)) & FLAG_IDE) ? true : false;

  uint32_t idA = ((readRegister(REG_RXBnSIDH(n)) << 3) & 0x07f8) | ((readRegister(REG_RXBnSIDL(n)) >> 5) & 0x07);
  if (frame->is_extended) {
    uint32_t idB = (((uint32_t)(readRegister(REG_RXBnSIDL(n)) & 0x03) << 16) & 0x30000) | ((readRegister(REG_RXBnEID8(n)) << 8) & 0xff00) | readRegister(REG_RXBnEID0(n));

    frame->id = (idA << 18) | idB;
    frame->is_retransmit = (readRegister(REG_RXBnDLC(n)) & FLAG_RTR) ? true : false;
  } else {
    frame->id = idA;
    frame->is_retransmit = (readRegister(REG_RXBnSIDL(n)) & FLAG_SRR) ? true : false;
  }

  frame->data_len = readRegister(REG_RXBnDLC(n)) & 0x0f;

  if (!frame->is_retransmit) {
    for (int i = 0; i < frame->data_len; i++) {
      frame->data[i] = readRegister(REG_RXBnD0(n) + i);
    }
  }

  // Release RX buffer
  modifyRegister(REG_CANINTF, FLAG_RXnIF(n), 0x00);
  return frame->data_len;
}

void MCP2515Class::dumpErrors()
{
	uint8_t intf = readRegister(REG_CANINTF);
	if (intf & FLAG_ERRIF) {
		uint8_t eflg = readRegister(REG_EFLG);
		uint8_t tec  = readRegister(REG_TEC);
		uint8_t rec  = readRegister(REG_REC);
		if (_debug) _debug->printf("ERRIF! EFLG=0x%02X  TEC=%u  REC=%u\n", eflg, tec, rec);
		modifyRegister(REG_CANINTF, FLAG_ERRIF, 0x00);
	}
}

void MCP2515Class::dumpRegisters()
{
  if (!_debug) return;
  for (int i = 0; i < 128; i++) {
    byte b = readRegister(i);

    _debug->print("0x");
    if (i < 16) {
      _debug->print('0');
    }
    _debug->print(i, HEX);
    _debug->print(":0x");
    if (b < 16) {
      _debug->print('0');
    }
    _debug->print(b, HEX);
    _debug->print(" ");
    if (i % 8 == 7) {
      _debug->println();
    }
  }
}

int MCP2515Class::_sendReset()
{
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(CMD_RESET);
  digitalWrite(_csPin, HIGH);
  delay(10);
  
  // Read CANSTAT (cmd 0x03, address 0x0E)
  digitalWrite(_csPin, LOW);
  SPI.transfer(CMD_READ);     // READ command
  SPI.transfer(REG_CANSTAT);     // CANSTAT address
  uint8_t canstat = SPI.transfer(0x00);  // read value
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();

  // CANSTAT 0x80 == config mode
  if (canstat != CANSTAT_CONFIG) {
	return -1;
  }

  return 1;
}

uint8_t MCP2515Class::readRegister(uint8_t address)
{
  uint8_t value;
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(CMD_READ);
  SPI.transfer(address);
  value = SPI.transfer(0x00);
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();
  return value;
}

void MCP2515Class::modifyRegister(uint8_t address, uint8_t mask, uint8_t value)
{
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(CMD_UPDATE);
  SPI.transfer(address);
  SPI.transfer(mask);
  SPI.transfer(value);
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();
}

void MCP2515Class::writeRegister(uint8_t address, uint8_t value)
{
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_csPin, LOW);
  SPI.transfer(CMD_WRITE);
  SPI.transfer(address);
  SPI.transfer(value);
  digitalWrite(_csPin, HIGH);
  SPI.endTransaction();
}
