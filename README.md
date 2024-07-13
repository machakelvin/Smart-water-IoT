
# Smart Water Meter

This project implements a smart water meter system using ESP32/ESP8266, Firebase, and the SIM800L GSM module. The system tracks water usage and sends notifications based on the remaining units.

## Features

- **Wi-Fi Connectivity**: Connects to multiple Wi-Fi networks for reliable connectivity.
- **Firebase Integration**: Stores and retrieves data from Firebase Realtime Database.
- **Flow Sensor Readings**: Measures water flow and calculates total volume used.
- **GSM Notifications**: Sends SMS notifications using the SIM800L GSM module.
- **LCD Display**: Displays real-time data on a 16x2 LCD screen.
- **Time Synchronization**: Synchronizes with NTP servers for accurate timekeeping.

## Hardware Requirements

- ESP32 or ESP8266 microcontroller
- SIM800L GSM module
- Flow sensors (for user and station)
- 16x2 I2C LCD display
- Wi-Fi network access
- Active Firebase project

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software)
- [Firebase ESP Client Library](https://github.com/mobizt/Firebase-ESP-Client)
- [LiquidCrystal I2C Library](https://github.com/johnrickman/LiquidCrystal_I2C)

## Configuration

1. **Wi-Fi Credentials**: Update the Wi-Fi SSID and password in the `WifiSetup` function.
    ```cpp
    #define WIFI_SSID1 "your_SSID1"
    #define WIFI_PASSWORD1 "your_PASSWORD1"
    #define WIFI_SSID2 "your_SSID2"
    #define WIFI_PASSWORD2 "your_PASSWORD2"
    ```

2. **Firebase Configuration**: Update the Firebase API key and database URL in the `FirebaseSetup` function.
    ```cpp
    #define API_KEY "your_firebase_API_key"
    #define DATABASE_URL "your_firebase_database_URL"
    ```

3. **NTP Configuration**: Update the NTP server information and time zone in the `timeSetup` function.
    ```cpp
    const char* ntpServer1 = "pool.ntp.org";
    const char* ntpServer2 = "time.nist.gov";
    const long gmtOffset_sec = 10800;  // GMT+3 hours
    const int daylightOffset_sec = 0;  // No daylight saving time
    const char* time_zone = "EAT-3";  // TimeZone rule for East Africa Time (UTC+3)
    ```

4. **User Configuration**: Update the `Username` variable with the username you want to use for Firebase.
    ```cpp
    String Username = "your_username";
    ```

## Setup

1. **Connect Hardware**: Connect the flow sensors, LCD, and SIM800L module to the ESP32/ESP8266 as per the pin configurations in the code.

2. **Install Libraries**: Install the required libraries using the Arduino Library Manager.

3. **Upload Code**: Upload the provided code to your ESP32/ESP8266 using the Arduino IDE.

4. **Monitor Serial Output**: Open the Serial Monitor to view the connection status and real-time data.

## Usage

- The system will automatically connect to the available Wi-Fi networks.
- Water usage will be measured and displayed on the LCD.
- Data will be sent to Firebase every 15 seconds.
- SMS notifications will be sent if tampering is detected or when water usage reaches certain thresholds.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

Feel free to modify the README as per your specific requirements.
