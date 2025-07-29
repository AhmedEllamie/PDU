# PDU (Power Distribution Unit) Controller

## Overview
This project implements a Power Distribution Unit (PDU) controller using an ESP32 microcontroller. The PDU manages power distribution across multiple channels with features like temperature monitoring, voltage problem detection, and both web-based and serial interfaces for control.

## Features
- 4-Channel power distribution control
- Edge problem detection and auto-recovery
- Temperature monitoring with threshold-based control
- Battery voltage monitoring
- Web interface for remote control
- Serial command interface
- Settings persistence in flash memory
- Configurable safety thresholds
- WiFi Access Point for easy connection

## Hardware Requirements
- ESP32 (WROVER) board
- Temperature sensor (DS18B20)
- Voltage divider for battery measurement
- 4 output channels (CH1-CH4)
- 4 fuse indicators (F1-F4)
- Relay control
- IGN (Ignition) input
- VP (Voltage Problem) detection input

### Pin Configuration
```
F1_PIN  = 14  // Fuse indicator CH1 (input)
F2_PIN  = 35  // Fuse indicator CH2 (input)
F3_PIN  = 18  // Fuse indicator CH3 (input)
F4_PIN  = 25  // Fuse indicator CH4 (input)
IGN_PIN = 23  // IGN (ON/OFF) input
VP_PIN  = 36  // Edge problem detection input
TEMP_PIN = 21 // Temperature sensor (OneWire)
BAT_PIN = 34  // Battery measurement (analog)
CH1_PIN = 26  // CH1 Edge Control output
CH2_PIN = 27  // CH2 Control output
CH3_PIN = 32  // CH3 Control output
CH4_PIN = 33  // CH4 Control output
RELAY_PIN = 13 // Relay Control output
```

## Software Architecture
The project follows a modular architecture with three main classes:

### 1. PDUController
Core functionality for PDU operations:
- Channel control and monitoring
- Temperature and battery monitoring
- VP fault detection and recovery
- Settings management
- Power sequencing

### 2. PDUWebServer
Web interface implementation:
- Real-time status monitoring
- Channel control
- Threshold configuration
- Basic authentication
- RESTful API endpoints

### 3. SerialCommandHandler
Serial interface for command processing:
- Channel control commands
- Relay control
- Threshold settings
- Status reporting

## Setup and Installation

1. **Development Environment**
   - Install PlatformIO
   - Clone this repository
   - Open the project in PlatformIO

2. **Dependencies**
   ```ini
   # platformio.ini
   lib_deps =
     OneWire
     DallasTemperature
     ESP32 Preferences
   ```

3. **WiFi Configuration**
   ```cpp
   SSID: "TAT_PDU_AP"
   Password: "password123"
   ```

4. **Web Interface Credentials**
   ```cpp
   Username: "admin"
   Password: "password"
   ```

## Usage

### Web Interface
1. Connect to the PDU's WiFi network (TAT_PDU_AP)
2. Navigate to the web interface (typically http://192.168.4.1)
3. Log in using the credentials
4. Monitor and control the PDU through the interface

### Serial Commands
Available commands:
- `SET_CH[1-4] [0/1]` - Control channels
- `SET_RELAY [0/1]` - Control relay
- `SET_TSTemp [value]` - Set temperature threshold
- `SET_TSTime [minutes]` - Set time threshold

### Safety Features
1. **VP (Voltage Problem) Protection**
   - Monitors CH1 for voltage problems
   - Auto-recovery with configurable retry limit
   - Permanent shutdown after max retries

2. **Temperature Protection**
   - Automatic shutdown above temperature threshold
   - Configurable threshold via web/serial interface

3. **Time-based Protection**
   - Shutdown after configurable time when IGN is LOW
   - Prevents battery drainage

## API Endpoints
- GET `/api/status` - System status
- POST `/api/control` - Channel and relay control
- POST `/api/setTemp` - Temperature threshold
- POST `/api/setTime` - Time threshold

## State Persistence
Settings stored in flash memory:
- Channel states
- Temperature threshold
- Time threshold
- Relay flag

## Development and Maintenance

### Debug Output
- Serial monitoring at 9600 baud
- Status updates for VP_PIN and CH1_PIN states
- Fault detection and recovery logging

### Code Organization
```
src/
├── PDUWifi_cmd.ino     - Main program
├── pdu_controller.h    - PDU control logic
├── pdu_web_server.h   - Web interface
├── SerialCommandHandler.h - Serial interface
└── pdu_config.h       - Configuration
```

### Troubleshooting
1. Channel Issues:
   - Check fuse indicators
   - Monitor VP_PIN state
   - Review serial debug output

2. Web Interface:
   - Verify WiFi connection
   - Check authentication
   - Review browser console

3. Settings Reset:
   - Use serial commands
   - Clear flash memory if needed
   - Restore default values

## Contributing
1. Fork the repository
2. Create a feature branch
3. Submit a pull request

## License
This project is proprietary software for Tatweer use only.

## Authors
- Ahmed Ellamie

## Version History
- v1.0.0 (5/7/2025) - Initial release
- v1.1.0 (5/8/2025) - Added Preferences storage
- v2.0.0 (7/28/2025) - Refactored to modular structure
