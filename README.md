# ESP32 Lights Controller

A comprehensive toolkit for controlling LED strips with ESP32 microcontrollers using WebBluetooth, WiFi, and ESP-NOW protocols. This project combines hardware programming with modern web technologies to create interactive light displays and real-time audio visualizations.

## 🌟 Features

- **Multiple Control Methods**: WebBluetooth, WiFi, and ESP-NOW communication
- **Interactive Web Dashboard**: Browser-based control panel for LED patterns
- **Audio Visualization**: Real-time audio analysis for music-reactive lighting
- **Multiple Animation Modes**: 
  - Color swipe and transitions
  - Theater chase effects
  - Rainbow patterns
  - Multi-color sizzle and wipe effects
- **DJ Integration**: Web-based DJ panel for audio-reactive lighting
- **Jupyter Notebook Documentation**: Interactive documentation and development notes

## 🚀 Quick Start

### Hardware Requirements

- ESP32 development board
- WS2812B/NeoPixel LED strip
- Power supply appropriate for your LED strip
- Jumper wires for connections

### Software Requirements

- [PlatformIO](https://platformio.org/) or Arduino IDE
- Modern web browser with WebBluetooth support (Chrome, Edge)
- Python (for Jupyter notebooks)

### Installation

1. **Clone the repository**:
   ```bash
   git clone https://github.com/carloskarpati/lights.git
   cd lights
   ```

2. **Install dependencies**:
   ```bash
   npm install
   ```

3. **Flash the ESP32**:
   - Connect your ESP32 via USB
   - Upload the code using PlatformIO:
     ```bash
     pio run --target upload
     ```
   - Or use the Arduino IDE with the code from `lightstrip/main.cpp`

4. **Open the web interface**:
   - Open `index.html` in a WebBluetooth-compatible browser
   - Or serve the files locally for development

## 📱 Usage

### Web Dashboard

1. Open the web interface in your browser
2. Click "BT Connect" to connect to your ESP32 via Bluetooth
3. Use the control panel to:
   - Set custom colors
   - Adjust brightness and animation speed
   - Select different lighting patterns
   - Generate random color combinations

### Available Light Modes

- **Set Color**: Static color display
- **Color Swipe**: Smooth color transitions
- **Theater Chase**: Classic theater-style chasing lights
- **Rainbow**: Cycling rainbow pattern
- **Theater Chase Rainbow**: Rainbow with theater chase effect
- **Multi-Color Effects**: Various multi-color patterns including sizzle and wipe animations

### Audio Visualization

Access the [DJ Panel](https://carlos-a-diez.com/music/dj.html) for real-time audio analysis and music-reactive lighting effects.

## 🗂️ Project Structure

```
├── src/                    # Main PlatformIO source code
├── lightstrip/            # LED strip control implementation
├── ipynb/                 # Jupyter notebooks with documentation
├── samples/               # Example code and audio processing samples
├── web.js                 # Web interface JavaScript
├── rsc/                   # Resources and reference materials
└── include/               # Header files
```

## 📚 Documentation

The project includes extensive Jupyter notebook documentation:

- **lights.ipynb**: Main project overview and interactive controls
- **esp32.ipynb**: ESP32 development notes and examples
- **Audio Notes.ipynb**: Audio processing and visualization techniques
- **communications.ipynb**: Communication protocols and implementation
- **platformio_espnow_jlcpcb.ipynb**: PlatformIO and ESP-NOW setup

## 🔧 Development

### Building from Source

```bash
# Install PlatformIO dependencies
pio lib install

# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

### Web Development

The web interface uses vanilla JavaScript with WebBluetooth API. No build process required - simply serve the HTML files.

## 🤝 Contributing

Contributions are welcome! Please feel free to submit issues, feature requests, or pull requests.

## 📄 License

This project is open source. Please check the individual files for specific licensing information.

## 🔗 Related Projects

- [WLED](https://github.com/Aircoookie/WLED) - Advanced LED strip control
- [FastLED](https://github.com/FastLED/FastLED) - Arduino LED library

## 💡 Keywords

3D Printing, Crypto, Metaverse, Navigation, ESP32, Arduino, LED Control, WebBluetooth, IoT, Audio Visualization

---

*For detailed technical documentation and interactive examples, explore the Jupyter notebooks in the `ipynb/` directory.*