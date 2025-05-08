# Smart Radiator Thermostat Regulation

## Description
This project implements a smart radiator temperature control system. The system allows manual temperature adjustment via a rotary encoder, as well as remote control through a web interface. It uses ESP32 and ESP32-C3 modules, a 28BYJ-48 5V stepper motor for precise temperature regulation, an OLED display (SSD1306), and a DHT11 sensor for measuring ambient temperature. A press button is used to switch between different radiators in the system.

## Features

Manual Temperature Control: Rotate the rotary encoder to adjust the temperature setting.
Web Interface: Use the web server to control radiator temperature remotely via a browser.
Radiator Selection: Press the button to toggle between different radiators.
Real-Time Temperature Monitoring: Displays current temperature readings from the DHT11 sensor on the OLED display.
Precise Valve Control: Uses a stepper motor to adjust radiator valves according to the set temperature.
LIVE thermostat updates when sending new requests, showing status via display.


## Setup

### Hardware
 - Connect the Stepper Motor (28BYJ-48) to the ESP32 using the appropriate motor driver (e.g., ULN2003).
 - OLED Display (SSD1306) connects via I2C to the ESP32.
 - Rotary Encoder connects to two GPIO pins for reading rotational data.
 - DHT11 Sensor connects to a GPIO pin for temperature sensing.
 - Press Button connects to a GPIO pin for switching between radiators.
### Software
 - Install the ESP32 Board support in the Arduino IDE.
 - Install the following libraries:
1. Adafruit_SSD1306 for the OLED display.
2. DHT for the DHT11 sensor.
3. ESPAsyncWebServer for the web server.
4. AsyncTCP
5. LittleFS
 - To Upload the code to your ESP32 use the Arduino IDE.

⚠️ **DON'T FORGET TO!** ⚠️
For uploading WEB files use LittleFS:

ARDUINO IDE -> (CMD + SHIFT + P) -> UPLOAD LittleFS to...

#### Web Interface
Once the system is up and running, connect your device to the same network as the ESP32. Access the web interface by entering the ESP32's IP address in a web browser. Use the interface to adjust the radiator temperatures remotely.
The IP address to access the server is being printed to serial port.

#### Manual Control
Rotate the rotary encoder to adjust the desired temperature for the selected radiator. The OLED display will show the current temperature and the selected radiator. Use the button to switch between different radiators.

# Contributors
- [Ričards Bubišs](https://github.com/Richard0exe?tab=followers)
- [Markuss Birznieks](https://github.com/Markuss-B)
