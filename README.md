# OBD-II Proxy

This project provides **bidirectional CAN bus proxying** between two MCP2515 breakout boards as well as OBD-II emulation capabilities.

## Overview

The CAN proxy system allows you to:

- **Forward all CAN traffic** between two separate CAN buses
- **Maintain OBD-II emulation** on one of the CAN buses
- **Monitor and debug** traffic on both buses via UDP broadcast and web interface
- **Update firmware** over-the-air (OTA)

## Hardware Setup

### Required Components

- **ESP32 DevKitC** (or compatible ESP32 board)
- **2x MCP2515 CAN Controller Breakout Boards** with TJA1050 transceivers and 8MHz crystals (one per MCP2515 board)

### Pin Connections

#### Shared SPI Bus

- **MOSI**: GPIO23
- **MISO**: GPIO19  
- **SCK**: GPIO18

#### CAN1 (OBD-II Interface)

- **CS**: GPIO5
- **INT**: GPIO4
- **CANH/CANL**: Connect to OBD-II scanner or vehicle

#### CAN2 (Proxy Interface)

- **CS**: GPIO14
- **INT**: GPIO13
- **CANH/CANL**: Connect to target CAN bus

### Debug Output Configuration

- **Serial**: 115200 baud (for monitoring and debugging)
- **Web Server**: Port 23002 (plain text output)
- **UDP Broadcast**: 192.168.101.255:23000

### Pin Assignment Summary

| ESP32 Pin | Function | Connected To         | Description                   |
| --------- | -------- | -------------------- | ----------------------------- |
| 3.3V      | Power    | CAN1 VCC, CAN2 VCC   | 3.3V power supply             |
| GND       | Ground   | CAN1 GND, CAN2 GND   | Common ground                 |
| GPIO18    | SPI CLK  | CAN1 SCK, CAN2 SCK   | SPI clock signal              |
| GPIO19    | SPI MISO | CAN1 MISO, CAN2 MISO | SPI data from CAN controllers |
| GPIO23    | SPI MOSI | CAN1 MOSI, CAN2 MOSI | SPI data to CAN controllers   |
| GPIO5     | CAN1 CS  | CAN1 CS              | Chip select for CAN1          |
| GPIO4     | CAN1 INT | CAN1 INT             | Interrupt from CAN1           |
| GPIO15    | CAN2 CS  | CAN2 CS              | Chip select for CAN2          |
| GPIO16    | CAN2 INT | CAN2 INT             | Interrupt from CAN2           |

## Circuit Diagram

```
                    OBD-II CAN Proxy Device
                    ┌─────────────────────────────────────────────────────────┐
                    │                                                         │
                    │  ┌─────────────────────────────────────────────────┐    │
                    │  │                ESP32 DevkitC                    │    │
                    │  │  ┌─────────────────────────────────────────┐    │    │
                    │  │  │              Power Supply               │    │    │
                    │  │  │  5V ──────────────────────────────-     │    │    │
                    │  │  │  GND  ──────────────────────────────    │    │    │
                    │  │  └─────────────────────────────────────────┘    │    │
                    │  │                                                 │    │
                    │  │  ┌─────────────────────────────────────────┐    │    │
                    │  │  │              SPI Bus                    │    │    │
                    │  │  │  GPIO18 ── CLK ───────────────────────  │    │    │
                    │  │  │  GPIO19 ── MISO ──────────────────────  │    │    │
                    │  │  │  GPIO23 ── MOSI ──────────────────────  │    │    │
                    │  │  └─────────────────────────────────────────┘    │    │
                    │  │                                                 │    │
                    │  │  ┌─────────────────────────────────────────┐    │    │
                    │  │  │            Control Lines                │    │    │
                    │  │  │  GPIO5  ── CAN1 CS ──────────────────   │    │    │
                    │  │  │  GPIO4  ── CAN1 INT ─────────────────   │    │    │
                    │  │  │  GPIO15 ── CAN2 CS ──────────────────   │    │    │
                    │  │  │  GPIO16 ── CAN2 INT ─────────────────   │    │    │
                    │  │  └─────────────────────────────────────────┘    │    │
                    │  └─────────────────────────────────────────────────┘    │
                    │                                                         │
                    │  ┌─────────────────┐    ┌─────────────────┐             │
                    │  │  CAN1 Module    │    │  CAN2 Module    │             │
                    │  │  (MCP2515)      │    │  (MCP2515)      │             │
                    │  │                 │    │                 │             │
                    │  │  ┌───────────┐  │    │  ┌───────────┐  │             │
                    │  │  │ MCP2515   │  │    │  │ MCP2515   │  │             │
                    │  │  │ Controller│  │    │  │ Controller│  │             │
                    │  │  └───────────┘  │    │  └───────────┘  │             │
                    │  │        │        │    │        │        │             │
                    │  │  ┌───────────┐  │    │  ┌───────────┐  │             │
                    │  │  │ TJA1050   │  │    │  │ TJA1050   │  │             │
                    │  │  │Transceiver│  │    │  │Transceiver│  │             │
                    │  │  └───────────┘  │    │  └───────────┘  │             │
                    │  └─────────────────┘    └─────────────────┘             │
                    │                                                         │
                    │  ┌─────────────────┐    ┌─────────────────┐             │
                    │  │   OBD-II Port   │    │   OBD-II Port   │             │
                    │  │   (CAN1)        │    │   (CAN2)        │             │
                    │  └─────────────────┘    └─────────────────┘             │
                    └─────────────────────────────────────────────────────────┘
```

### Board Layout

```
Board Layout (Top View):
┌─────────────────────────────────────────────────────────────────┐
│                                  Scanner        ECU             │
│  ESP32 DevKitC                   CAN1 Module    CAN2 Module     │
│  ┌─────────────┐                ┌─────────────┐ ┌─────────────┐ │
│  │ 3.3V ───────┼────────────────┼── VCC       ┼─┼── VCC       │ │
│  │ GND  ───────┼────────────────┼── GND       ┼─┼── GND       │ │
│  │ GPIO18 ─────┼────────────────┼── SCK       ┼─┼── SCK       │ │
│  │ GPIO19 ─────┼────────────────┼── MISO      ┼─┼── MISO      │ │
│  │ GPIO23 ─────┼────────────────┼── MOSI      ┼─┼── MOSI      │ │
│  │ GPIO5  ─────┼────────────────┼── CS        │ │             │ │
│  │ GPIO4  ─────┼────────────────┼── INT       │ │             │ │
│  │ GPIO15 ─────┼────────────────┼─────────────┼─┼── CS        │ │
│  │ GPIO16 ─────┼────────────────┼─────────────┼─┼── INT       │ │
|  └─────────────┘                |             | |             | |
│                                 |   CANH      | |   CANH      | |
|                                 |   CANL      | |   CANL      | |
|                                 └─────────────┘ └─────────────┘ |
└─────────────────────────────────────────────────────────────────┘
```

## Building and Flashing

### Prerequisites

- PlatformIO IDE or CLI
- ESP32 development environment

### Build Commands

#### Single CAN Version (OBD-II Emulator)

```bash
pio run -e esp32-emulator
```

#### CAN Proxy Version (Dual CAN)

```bash
pio run -e esp32-proxy
```

### Upload

```bash
# For CAN proxy version
pio run -e esp32-proxy --target upload

# For OBD-II emulator version
pio run -e esp32-emulator --target upload
```

### Monitor

```bash
# For CAN proxy version
pio run -e esp32-proxy --target monitor

# For OBD-II emulator version
pio run -e esp32-emulator --target monitor
```

## Configuration

### CAN Bus Settings

Both CAN buses are configured for:

- **Baud Rate**: 500 kbps
- **Clock Frequency**: 8 MHz
- **Frame Buffer Size**: 32 frames per bus
- **SPI Configuration**:
  - **SCK Pin**: GPIO18
  - **MISO Pin**: GPIO19
  - **MOSI Pin**: GPIO23
  - **SPI Frequency**: 1 MHz
  - **SPI Bit Order**: MSBFIRST
  - **SPI Mode**: SPI_MODE0

### Monitoring

Connect to the UDP broadcast to monitor system status:

```bash
tcpdump -ni wlp1s0 port 23000 and not src 192.168.101.255 -A
```

Or access the web interface at `http://[ESP32_IP]:23002` for real-time debug output.

## Status Output

The system provides comprehensive status information via UDP broadcast:

```
=== OBD-II CAN Proxy Status ===
Frames buffered: 0, Processed frames: 5, Dropped frames: 0
=== CAN Proxy Statistics ===
CAN1 Received: 15
CAN2 Received: 12
Forwarded CAN1->CAN2: 15
Forwarded CAN2->CAN1: 12
Dropped CAN1: 0
Dropped CAN2: 0
Errors CAN1: 0
Errors CAN2: 0
CAN1 Buffer: 0/32
CAN2 Buffer: 0/32
===========================
```

## Troubleshooting

### Common Issues

#### CAN Bus Not Initializing

- Check SPI connections (MOSI, MISO, SCK)
- Verify CS and INT pin connections
- Check power supply to MCP2515 boards
- Ensure CS pins are pulled HIGH during idle (add `pinMode(cs_pin, INPUT_PULLUP)`)

#### Frame Dropping

- Increase frame buffer size if needed
- Check CAN bus termination
- Verify baud rate matches target devices
- Monitor interrupt frequency

#### WiFi Connection Issues

- Verify SSID and password in code (currently hardcoded to "Netdrop")
- Check WiFi signal strength
- Ensure network allows UDP broadcast

### Debug Commands

The system provides several debug features:

- **Register dumping**: `can_proxy.dumpRegisters(stream)`
- **Statistics reset**: `can_proxy.resetStats()`
- **Frame data logging**: Automatic via UDP broadcast
- **Web interface**: Access at `http://[ESP32_IP]:23002`

## Performance Considerations

### Frame Latency

- **Typical latency**: < 1ms for frame forwarding
- **Buffer overflow**: Monitored via statistics
- **Interrupt handling**: Optimized for minimal latency

### Memory Usage

- **Frame buffers**: ~2KB total (32 frames × 2 buses)
- **UDP buffers**: 256 bytes
- **Web server buffer**: 4KB
- **Stack usage**: Minimal due to interrupt optimization

### Power Consumption

- **ESP32**: ~150mA at 160MHz
- **MCP2515 boards**: ~50mA each
- **Total**: ~250mA typical

## Advanced Configuration

### Custom Baud Rates

Modify the `CANConfig` structures in `src/OBD2Proxy.cpp`:

```cpp
CANConfig can1_config = {
    .instance_id = 0,
    .cs_pin = 5,
    .irq_pin = 4,
    .baud_rate = 250000,  // Custom baud rate
    .clock_frequency = 8000000,
    .name = "CAN1",
    .spi_sck_pin = 18,    // SPI clock pin
    .spi_miso_pin = 19,   // SPI MISO pin
    .spi_mosi_pin = 23,   // SPI MOSI pin
    .spi_frequency = 1000000,  // SPI frequency in Hz
    .spi_bit_order = MSBFIRST, // SPI bit order
    .spi_mode = SPI_MODE0      // SPI mode
};
```

### Buffer Sizes

Adjust frame buffer sizes in the CANStream library configuration.

### GPIO Pins

Change pin assignments as needed (ensure no conflicts):

```cpp
CANConfig can2_config = {
    .instance_id = 1,
    .cs_pin = 17,   // Alternative CS pin
    .irq_pin = 18,  // Alternative INT pin
    .spi_sck_pin = 18,    // SPI clock pin
    .spi_miso_pin = 19,   // SPI MISO pin
    .spi_mosi_pin = 23,   // SPI MOSI pin
    .spi_frequency = 1000000,  // SPI frequency in Hz
    .spi_bit_order = MSBFIRST, // SPI bit order
    .spi_mode = SPI_MODE0,     // SPI mode
    // ... other settings
};
```

## Single CAN Mode (OBD-II Emulator)

### Hardware Setup
- Connect **1x MCP2515 breakout board** to the ESP32
- **CAN**: GPIO5 (CS), GPIO4 (INT) - OBD-II interface
- **SPI**: GPIO23 (MOSI), GPIO19 (MISO), GPIO18 (SCK)

### Build and Flash
```bash
# Build single CAN version
pio run -e esp32-emulator

# Upload to ESP32
pio run -e esp32-emulator --target upload

# Monitor output
pio run -e esp32-emulator --target monitor
```

### Features
- **OBD-II emulation** for diagnostic scanners
- **Real-time monitoring** via UDP broadcast
- **Over-the-air updates** supported

## ESP32 Specifications

```
esptool.py v4.5.1
Serial port /dev/ttyUSB0
Connecting....
Detecting chip type... Unsupported detection protocol, switching and trying again...
Connecting....
Detecting chip type... ESP32
Chip is ESP32-D0WD-V3 (revision v3.1)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: f0:24:f9:e6:d4:58
```

- Internal 8 MHz oscillator with calibration
- Internal RC oscillator with calibration
- External 2 MHz ~ 60 MHz crystal oscillator (40 MHz only for Wi-Fi/Bluetooth functionality)
- External 32 kHz crystal oscillator for RTC with calibration

## CAN Specifications

- **CANH and CANL** are identical but inverted signals around the DC level
- **Baud Rate**: 500 kbps (configurable)
- **Clock Frequency**: 8 MHz (configurable)

### Timing Configuration

Timing is configured using the BRP, SJW, TSEG1, TESEG2, and Triple Sampling registers.

The following formula is used to set the timing:

```
bitrate = clock / (BRP × (1 + TSEG1 + TSEG2))
```

- **BRP (Baudrate Prescaler)**: Can be any even number from 2 to 128
- **A single bit contains**:
  - **Synchronization Segment**: Consists of a single time quanta
  - **Timing Segment 1**: Consists of 1 to 16 time quanta before sample point
  - **Timing Segment 2**: Consists of 1 to 8 time quanta after sample point
- **Sample point**: Located at the intersection of Timing Segment 1 and 2
- **Triple Sampling**: Enables 3 time quanta to be sampled per bit instead of 1
- **SJW (Synchronization Jump Width)**: Maximum number of time quanta a single bit time can be lengthened/shortened for synchronization purposes (1 to 4)

## CAN Voltages

### Scanner Interface

- **CANH and CANL**: Operate at 2.4V DC level
- **CANH signals**: +1.3V (3.7V)
- **CANL signals**: -1.3V (1.1V)

### Vehicle Interface

- **CANH and CANL**: Operate at 2.4V DC level
- **CANH signals**: +0.8V (3.2V) with spikes to +1.3V (3.7V)

## Hardware Analysis

### Scanner Components

- Voltage buck circuit
- 78M05 Voltage Regulator
- TJA1050 Transceiver
- LM393 Comparator
- 1AM transistors
- ATMEL CPU
- 1001 (1k), 1002 (10k), 1004 (1M), 2212 (22.1k)
- Hysteresis: 1004 (1M)
- Vref: 1002 (10k)
- Pullup: 1001 (1k)

### Freematics Components

- MP1582EN step-down regulator
- SS14 schottky diode
- TJA1040 transceiver
- 1AM transistor
- AMS1117 voltage regulator
- STM32F CPU

## References

- [OBD-II PIDs](https://en.wikipedia.org/wiki/OBD-II_PIDs)
- [On-board diagnostics](https://en.wikipedia.org/wiki/On-board_diagnostics)
- [ESP32 CAN Bus Tutorial](https://lastminuteengineers.com/esp32-can-bus-tutorial/)
- [Arduino CAN Library](https://github.com/sandeepmistry/arduino-CAN)
- [TJA1050 Datasheet](https://www.nxp.com/docs/en/data-sheet/TJA1050.pdf)
- [SJA1000 Datasheet](https://www.nxp.com/docs/en/data-sheet/SJA1000.pdf)
- [ESP32 OTA Pull Library](https://github.com/mikalhart/ESP32-OTA-Pull)
- [std::function and std::bind](https://stackoverflow.com/questions/6610046/stdfunction-and-stdbind-what-are-they-and-when-should-they-be-used)
- [OBD2 Explained](https://www.csselectronics.com/pages/obd2-explained-simple-intro)
- [OBD2 PID Table](https://www.csselectronics.com/pages/obd2-pid-table-on-board-diagnostics-j1979)
- [DC Coupling](https://northcoastsynthesis.com/news/more-about-dc-coupling/)
- [CAN Bus Errors Tutorial](https://www.csselectronics.com/pages/can-bus-errors-intro-tutorial)
- [ISO 15765-4](https://www.iso.org/standard/63139.html) (search for PDF)
- [CANopen Bus Interface](https://www.hybridservos.com/news/canopen-bus-interface-circuit-principle-and-de-13390315.html)
- [6N137 Datasheet](https://radiolux.com.ua/files/pdf/6N137.pdf)
- [ESP32 TWAI Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/twai.html)
- [ESP32-WROOM-32E Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32e_esp32-wroom-32ue_datasheet_en.pdf)
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
