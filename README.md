# 🎣 SunPhish - ESP8266 Phishing Awareness System

[![Platform](https://img.shields.io/badge/Platform-ESP8266-blue.svg)](https://www.espressif.com/en/products/socs/esp8266)
[![Framework](https://img.shields.io/badge/Framework-Arduino-green.svg)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

A low-cost, solar-powered, energy self-sufficient IoT device designed to **educate people about phishing attacks** through hands-on experience. When users connect to the fake Wi-Fi network and attempt to "login" via social media, they're immediately shown an awareness page explaining what just happened and how to protect themselves.

This project was developed for educational purposes as part of my Computer Engineering degree at PUCRS (Pontifícia Universidade Católica do Rio Grande do Sul), Brazil, in 2025. Always obtain proper authorization before conducting security testing.
Read the original document [here](https://github.com/radzki/SunPhish/blob/main/Essay.pdf).

> ⚠️ **Disclaimer:** This project is intended for **educational and authorized security awareness purposes only**. Always obtain proper authorization before deploying. Never use this to collect actual credentials.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [How It Works](#how-it-works)
- [Hardware Requirements](#hardware-requirements)
- [Software Architecture](#software-architecture)
- [Installation](#installation)
- [Configuration](#configuration)
- [Customization](#customization)
- [Technical Deep Dive](#technical-deep-dive)
- [Field Testing Results](#field-testing-results)
- [Future Improvements](#future-improvements)
- [Academic Background](#academic-background)
- [License](#license)

## Overview

The democratization of the Internet has connected billions of people to the network, but cybersecurity education hasn't kept pace. Public Wi-Fi networks are prime targets for social engineering attacks like phishing.

This project demonstrates how easy it is to create a convincing fake login portal, then immediately educates the "victim" about:
- What just happened
- How to identify phishing attempts
- Best practices for public Wi-Fi security

**Key principle:** The system counts awareness completions but **never stores actual credentials**.

## Features

- 🌐 **Captive Portal** - Automatically opens on connection (works on iOS, Android, Windows, macOS)
- 🔐 **Multiple Login Pages** - Cloned Facebook, Instagram, Gmail, and Twitter login pages
- 📚 **Educational Awareness Page** - Explains the attack and provides security tips
- ☀️ **Solar Powered** - Fully autonomous with 4x 6V/1W solar panels
- 🔋 **Smart Energy Management** - Deep sleep mode, voltage monitoring, scheduled operation
- 📡 **OTA Updates** - Update firmware wirelessly over Wi-Fi
- 🔄 **Auto-Disconnect** - Disconnects users after awareness flow to free up connections
- 🌡️ **Temperature Compensation** - Accurate voltage readings across temperature ranges
- ⚡ **Low Power Consumption** - Optimized to run indefinitely on solar power

## How It Works

```
┌─────────────────────────────────────────────────────────────────┐
│                         USER FLOW                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. User sees "#NET-ESCURO-WIFI" network                        │
│                    ↓                                             │
│  2. User connects (no password required)                        │
│                    ↓                                             │
│  3. Captive portal auto-opens with social login options         │
│                    ↓                                             │
│  4. User selects Facebook/Instagram/Gmail/Twitter               │
│                    ↓                                             │
│  5. Convincing cloned login page appears                        │
│                    ↓                                             │
│  6. User enters credentials and clicks "Login"                  │
│                    ↓                                             │
│  7. ⚠️ AWARENESS PAGE displays explaining the attack            │
│                    ↓                                             │
│  8. User is automatically disconnected after 60 seconds         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Hardware Requirements

### Core Components

| Component | Specification | Notes |
|-----------|--------------|-------|
| NodeMCU V2/V3 | ESP-12E (ESP8266) | Main microcontroller |
| Solar Panels | 6V/1W × 4 | Connected in parallel |
| Li-ion Battery | 3.7V, 10000mAh | From recycled power bank |
| Charge Controller | TP4056 or similar | Handles solar charging |

### Shield Components (for energy monitoring)

| Component | Quantity | Purpose |
|-----------|----------|---------|
| Resistors (10kΩ, 15kΩ) | Various | Voltage dividers |
| MOSFET (IRLB8721) | 2 | Switching voltage dividers |
| Optocouplers (PC817) | 2 | Gate drivers for MOSFETs |
| Diodes (1N4148) | 4 | Isolation and virtual ground |
| LM35 | 1 | Temperature sensor |
| Capacitor (100µF) | 1 | Smoothing |

### Wiring Diagram

```
                    ┌─────────────┐
    Solar Panels ──►│   Charge    │
    (6V parallel)   │  Controller │
                    └──────┬──────┘
                           │
              ┌────────────┼────────────┐
              │            │            │
              ▼            ▼            ▼
         ┌────────┐   ┌────────┐   ┌────────┐
         │Battery │   │NodeMCU │   │ Shield │
         │ 3.7V   │◄─►│  Vin   │◄─►│  ADC   │
         └────────┘   └────────┘   └────────┘
```

### Pin Configuration

```cpp
#define ADC_IN      A0   // Analog input (multiplexed)
#define TEMP_SENSOR D5   // Temperature sensor enable
#define VD_PANEL    D1   // Solar panel voltage divider enable
#define VD_BATTERY  D7   // Battery voltage divider enable
```

## Software Architecture

```
ESPPhishing/
├── src/
│   └── main.cpp           # Main firmware
├── include/
│   └── config.h           # Configuration constants
├── data/                  # LittleFS filesystem (web content)
│   ├── index.html         # Main captive portal
│   ├── fb.html            # Facebook login clone
│   ├── instagram.html     # Instagram login clone
│   ├── gmail.html         # Gmail login clone
│   ├── whathappened.html  # Awareness/education page
│   ├── config.html        # Admin configuration page
│   └── rdz_css/           # Stylesheets
│       ├── bootstrap.min.css
│       ├── fontawesome.min.css
│       └── instagram.css
├── platformio.ini         # PlatformIO configuration
└── README.md
```

### Key Libraries

- **ESPAsyncWebServer** - Non-blocking web server for handling multiple connections
- **ESPAsyncTCP** - Async TCP library for ESP8266
- **LittleFS** - Flash filesystem for storing web content
- **ArduinoOTA** - Over-the-air firmware updates
- **Ticker** - Software timers for periodic tasks

## Installation

### Prerequisites

1. [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
2. USB cable for initial flashing
3. Python 3.x (for PlatformIO)

### Using PlatformIO (Recommended)

```bash
# Clone the repository
git clone https://github.com/yourusername/phishable.git
cd phishable/ESPPhishing

# Build and upload firmware
pio run --target upload

# Upload filesystem (web content)
pio run --target uploadfs
```

### Using Arduino IDE

1. Install ESP8266 board support
2. Install required libraries:
   - ESP Async WebServer
   - ESPAsyncTCP
   - LittleFS
3. Select board: "NodeMCU 1.0 (ESP-12E Module)"
4. Set Flash Size: "4MB (FS:1MB OTA:~1019KB)"
5. Upload sketch and filesystem separately

### OTA Updates (After Initial Flash)

```bash
# Uncomment these lines in platformio.ini:
upload_port = esp-rdz.local
upload_protocol = espota

# Then upload normally
pio run --target upload
pio run --target uploadfs
```

## Configuration

### Runtime Configuration

Access the configuration page at `http://8.8.8.8/config` when connected to the network:

- **Time Sync** - Synchronize the device's software RTC
- **Sleep Hours** - Configure active hours (default: 8AM-8PM)
- **Cutoff Voltage** - Battery protection threshold (default: 3.2V)

### Compile-Time Configuration (`config.h`)

```cpp
// Enable serial debugging
#define DEBUG false

// Deep sleep duration (30 minutes)
#define SLEEPTIMEuS 1800e6

// Initial timestamp (Unix time)
#define RTC_START_UNIXTIME 849182400

// Operating hours
#define SLEEP_MAX_HOUR 20  // 8 PM
#define SLEEP_MIN_HOUR 8   // 8 AM

// Battery cutoff voltage (mV)
#define CUTOFF_VOLTAGE 3200
```

## Customization

### Adding New Login Pages

1. Create your HTML file in `data/` folder
2. Clone the target login page's essential CSS
3. Ensure the form POSTs to `/login` with `login` and `password` fields:

```html
<form method="post" action="/login">
    <input name="login" type="text" placeholder="Email">
    <input name="password" type="password" placeholder="Password">
    <button type="submit">Login</button>
</form>
```

4. Add a link to your new page in `index.html`
5. Upload the new filesystem: `pio run --target uploadfs`

### Changing the Network Name

In `main.cpp`, modify:

```cpp
WiFi.softAP("#YOUR-NETWORK-NAME");
```

### Customizing the Awareness Page

Edit `data/whathappened.html` to include:
- Your organization's branding
- Specific security policies
- Links to security training resources
- Contact information

## Technical Deep Dive

### Captive Portal Detection

Different devices check for internet connectivity using different endpoints. The firmware handles all common ones:

```cpp
// Android/Chrome
server.on("/generate_204", ...);
server.on("/gen_204", ...);

// Apple iOS/macOS
server.on("/hotspot-detect.html", ...);

// Windows
server.on("/connecttest.txt", ...);
```

**Pro tip:** The gateway IP is set to `8.8.8.8` because some Samsung devices have Google's DNS hardcoded.

### User Disconnection via Deauthentication

The ESP8266 SDK removed the `wifi_send_pkt_freedom()` function to prevent abuse. Through reverse engineering with Ghidra, an undocumented function was discovered:

```cpp
extern "C" {
    bool wifi_softap_deauth(uint8 mac[6]);
}
```

This allows disconnecting users after they complete the awareness flow, freeing up connection slots (ESP8266 supports max 8 simultaneous clients).

### Energy Management

The system implements smart power management:

1. **Voltage Monitoring** - Measures solar panel and battery voltage every 60 seconds
2. **Temperature Compensation** - Adjusts readings based on ambient temperature
3. **Deep Sleep** - Enters low-power mode during configured hours or when battery is low
4. **RTC Persistence** - Maintains time across deep sleep cycles using RTC memory

```
Daily Energy Balance:
━━━━━━━━━━━━━━━━━━━━━━
Consumption = (85mA × 12h) + (5mA × 12h) = 1,080 mAh
Generation  = 200mA × 6h = 1,200 mAh
━━━━━━━━━━━━━━━━━━━━━━
Surplus     = +120 mAh/day ✓
```

## Field Testing Results

| Metric | Result |
|--------|--------|
| Test Duration | 1 week outdoor |
| Weather Conditions | Sun, clouds, rain |
| Battery Stability | Maintained healthy levels |
| Victim Rate | ~30% completed flow |
| System Crashes | 0 |
| Unexpected Restarts | 0 |

The 30% completion rate in a small sample demonstrates the real-world effectiveness of phishing attacks and the need for user education.

## Future Improvements

- [ ] Replace AMS1117 linear regulator with DC-DC converter (reduce deep sleep current)
- [ ] Add INA219 current sensor for precise power monitoring
- [ ] Integrate LoRaWAN/Sigfox for remote monitoring and configuration
- [ ] Design custom PCB to reduce size
- [ ] Add more login page templates (LinkedIn, Microsoft, etc.)
- [ ] Implement statistics dashboard with historical data
- [ ] Add multi-language support for awareness page

## Academic Background

This project was developed as a **Bachelor's Thesis (TCC)** for the Computer Engineering degree at **Pontifícia Universidade Católica do Rio Grande do Sul (PUCRS)**, Brazil, in 2025.

**Title:** "Sistema de Conscientização para Ataques de Phishing de Baixo Custo e Energeticamente Autossustentável"  
*(Low-Cost and Energy Self-Sufficient Phishing Awareness System)*

**Advisor:** Prof. Julio César Marques de Lima

**Grade:** Highest marks (A)

### Key Topics Covered

- IoT Development (ESP8266/NodeMCU)
- Network Security & Social Engineering
- Reverse Engineering (Ghidra)
- Solar Energy Systems
- Embedded Systems Design
- Web Development (HTML/CSS/JS)

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [ESP8266 Arduino Core](https://github.com/esp8266/Arduino)
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [PlatformIO](https://platformio.org/)
- [Ghidra](https://ghidra-sre.org/) - NSA's reverse engineering framework
- Projeto Recondicionar (Polo Marista) - For inspiring the upcycling philosophy

---

<p align="center">
  <b>Built with ☀️ and ♻️ recycled components</b><br>
  <i>Because security awareness shouldn't cost the earth</i>
</p>
