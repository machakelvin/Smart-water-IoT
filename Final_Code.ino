// WIFI
////////////////////////////////////////////////////////////
#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"
// Insert your network credentials
#define WIFI_SSID1 "100 Plain Pages"
#define WIFI_PASSWORD1 "Machakelvin"
#define WIFI_SSID2 "younger"
#define WIFI_PASSWORD2 "makeit114."
// #define WIFI_SSID3 "COICT-BLOCK D2"
// #define WIFI_PASSWORD3 "Udsm12345!"
// #define WIFI_SSID4 "3D lab"
// #define WIFI_PASSWORD4 "123456789"


// FIREBASE
////////////////////////////////////////////////////////////
// Insert Firebase project API Key
// #define API_KEY "AIzaSyBUuLBvjU8ovA1F7H1bSxelOMja7Jwt_CM"
#define API_KEY "AIzaSyCPwEQAg8TJ2slBNmKQcipf9tX-fsjTARs"
// Insert RTDB URL
#define DATABASE_URL "https://final-smart-meter-default-rtdb.firebaseio.com/"
// #define DATABASE_URL "https://smart-water-meter-e42cd-default-rtdb.firebaseio.com/"
// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

// FIREBASE DATA
////////////////////////////////////////////////////////////
int userOdometer;
int stationOdometer;
float onBootUnits;
float units = 0;
String Username = "mtui";
int x = 0;
int z = 0;
int y = 0;


// FLOW
////////////////////////////////////////////////////////////
float accumulatedVolumeUser = 0.0;
float accumulatedVolumeStation = 0.0;
unsigned long lastUpdateMillis = 0;
volatile int flow_frequency_user; // Measures flow sensor pulses for user
volatile int flow_frequency_station; // Measures flow sensor pulses for station
unsigned int l_hour_user; // Calculated litres/hour for user
unsigned int l_hour_station; // Calculated litres/hour for station
#define flowsensor_user 19 // Sensor Input for user
#define flowsensor_station 23 // Sensor Input for station
unsigned long currentTime;
unsigned long cloopTime;
float total_volume_user = 0; // Total volume of water for user (in liters)
float total_volume_station = 0; // Total volume of water for station (in liters)
float lastUserVolume;
float lastStationVolume;
float unitsUsedUser;
float unitsUsedStation;
void flow_user() // Interrupt function for user sensor
{
   flow_frequency_user++;
}
void flow_station() // Interrupt function for station sensor
{
   flow_frequency_station++;
}


// LCD
////////////////////////////////////////////////////////////
# include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);
# include <SoftwareSerial.h>
// SoftwareSerial arduinoSerial(7, 8);


// GSM SIM800L
////////////////////////////////////////////////////////////
SoftwareSerial gsm(34,33);
String lastNumber;
String lastMessage;


// TIME
////////////////////////////////////////////////////////////
#include "time.h"
#include "sntp.h"
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 10800;  // GMT+3 hours
const int   daylightOffset_sec = 0; // No daylight saving time
const char* time_zone = "EAT-3";  // TimeZone rule for East Africa Time (UTC+3)
String getLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    // Serial.println("No time available (yet)");
    return "";
  }

  // Format time as index = "YYYY_MM_DD_HH:MM:SS"
  char timeStringBuff[25]; // Buffer to store the formatted time
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y_%m_%d/%H:%M:%S", &timeinfo);
  String index = String(timeStringBuff);

  // Print the formatted time
  // Serial.println(index);

  return index;
}
// Callback function (gets called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  // Serial.println("Got time adjustment from NTP!");
  getLocalTime();
}


// OTHER PINS
////////////////////////////////////////////////////////////
#define pumpPin 15

void LCDSetup(){
  lcd.init();
  lcd.backlight();
}

void WifiSetup() {
  const char* ssids[] = {WIFI_SSID1, WIFI_SSID2};
  const char* passwords[] = {WIFI_PASSWORD1, WIFI_PASSWORD2};

  Serial.print("Connecting to Wi-Fi");
  
  for (int i = 0; i < 2; i++) {
    WiFi.begin(ssids[i], passwords[i]);

    int attemptCounter = 0;
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(300);

      attemptCounter++;
      if (attemptCounter >= 10) { // Try each network for 10 attempts (3 seconds each)
        break;
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.print("Connected to ");
      Serial.print(ssids[i]);
      Serial.print(" with IP: ");
      Serial.println(WiFi.localIP());
      Serial.println();

      // Display on the LCD
      lcd.clear();
      lcd.print("Connected to ");
      lcd.print(ssids[i]);
      lcd.setCursor(0, 1); // Move to the second line of the LCD
      lcd.print("IP: ");
      lcd.print(WiFi.localIP());

      delay(2000); // Delay for 2 seconds

      return;
    }
  }

  Serial.println();
  Serial.println("Failed to connect to any Wi-Fi network.");
}

void FirebaseSetup(){
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    // Serial.println("ok");
    signupOK = true;
  } else {
    // Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void pumpControlbyUnits(int units){
  if(units>0){
    digitalWrite(pumpPin,HIGH);
  }
  else{
    digitalWrite(pumpPin,LOW);
  }
}

void getUpdates(){
  if (Firebase.ready() && signupOK) {
    // Retrieve odometer values
    if (Firebase.RTDB.getFloat(&fbdo, "users/"+Username+"/Odometer")) {
      userOdometer = fbdo.floatData();
      // Serial.print("USER_ODOMETER:");
      // Serial.println(userOdometer);
    } else {
      // Serial.println(fbdo.errorReason());
    }

    if (Firebase.RTDB.getFloat(&fbdo, "users/"+Username+"/Station/Odometer")) {
      stationOdometer = fbdo.floatData();
      // Serial.print("STATION_ODOMETER:");
      // Serial.println(stationOdometer);
    } else {
      // Serial.println(fbdo.errorReason());
    }

    // Retrieve units value
    if (Firebase.RTDB.getFloat(&fbdo, "users/"+Username+"/Units")) {
      units = fbdo.intData();
      onBootUnits = fbdo.intData();
      // Serial.print("UNITS:");
      // Serial.println(units);
    } else {
      // Serial.println(fbdo.errorReason());
    }

    // Update local variables
    total_volume_user = userOdometer;
    lastUserVolume = userOdometer;
    total_volume_station = stationOdometer;
    lastStationVolume = stationOdometer;
    pumpControlbyUnits(units);

    // Wait a bit before the next retrieval
    delay(1000);  // Adjust the delay as needed
  }
}

void flowSetup(){
  pinMode(flowsensor_user, INPUT);
  digitalWrite(flowsensor_user, HIGH); // Optional Internal Pull-Up

  pinMode(flowsensor_station, INPUT);
  digitalWrite(flowsensor_station, HIGH); // Optional Internal Pull-Up
  
  attachInterrupt(digitalPinToInterrupt(flowsensor_user), flow_user, RISING); // Setup Interrupt for user sensor
  attachInterrupt(digitalPinToInterrupt(flowsensor_station), flow_station, RISING); // Setup Interrupt for station sensor
  
  sei(); // Enable interrupts

  currentTime = millis();
  cloopTime = currentTime;
}

void takeflowReadings() {
  currentTime = millis();
  // Every second, calculate and print litres/hour
  if (currentTime >= (cloopTime + 1000)) {
    cloopTime = currentTime; // Updates cloopTime

    // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
    l_hour_user = (flow_frequency_user * 60 / 7.5); // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
    l_hour_station = (flow_frequency_station * 60 / 7.5); // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour

    // Update total volume (liters)
    float volumeUser = (flow_frequency_user / 7.5) / 60.0; // Convert to liters by dividing by 7.5 (pulses/L) and 60 (min)
    float volumeStation = (flow_frequency_station / 7.5) / 60.0; // Convert to liters by dividing by 7.5 (pulses/L) and 60 (min)

    total_volume_user += volumeUser;
    total_volume_station += volumeStation;

    // Accumulate volume
    accumulatedVolumeUser += volumeUser;
    accumulatedVolumeStation += volumeStation;

    // Reset Counter
    flow_frequency_user = 0;
    flow_frequency_station = 0;

    // Reduce from readings
    units -= unitsUsedUser;
  }
}

void lcdDisplayToArduino(){
  // String Status;
  // Status = "Station: ";
  // Status += String(total_volume_station);
  // Status += " Lr";
  // Status += String(total_volume_user);
  // Status += Status + " Lr";
  // Status += " Lr\n";

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Station: ");
  lcd.print(total_volume_station);
  lcd.print("Lr");

  lcd.setCursor(0,1);
  lcd.print("User   : ");
  lcd.print(total_volume_user);
  lcd.print("Lr");

  delay(300);

  // arduinoSerial.print(Status);
}

void timeSetup(){
  sntp_set_time_sync_notification_cb(timeavailable);
  sntp_servermode_dhcp(1);    // (optional)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  configTzTime(time_zone, ntpServer1, ntpServer2);
}

void makeUpdates(){
  String userUnitsUsedPathString = "users/"+Username+"/Usage/";
  userUnitsUsedPathString += getLocalTime();
  const char* userUnitsUsedPath = userUnitsUsedPathString.c_str();  // Convert String to const char*

  String userUnitsPathString = "users/"+Username+"/Units";
  const char* userUnitsPath = userUnitsPathString.c_str();  // Convert String to const char*

  String userOdometerPathString = "users/"+ Username+"/Odometer";
  const char*  userOdometerPath = userOdometerPathString.c_str();  // Convert String to const char*

  String stationUnitsUsedPathString = "users/"+Username+"/Station/Usage/";
  stationUnitsUsedPathString += getLocalTime();
  const char* stationUnitsUsedPath = stationUnitsUsedPathString.c_str();  // Convert String to const char*

  String stationOdometerPathString = "users/"+Username+"/Station/Odometer";
  const char*  stationOdometerPath = stationOdometerPathString.c_str();  // Convert String to const char*

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
    String messagePathString = "users/"+Username+"/tamper_notifications/message";
    const char*  messagePath = messagePathString.c_str();  // Convert String to const char*
    String message = "Tamper detected at meter 01";

    sendDataPrevMillis = millis();
    // Update user units used
    if (Firebase.RTDB.setInt(&fbdo, userUnitsUsedPath, unitsUsedUser)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }
    
    // Update users units
    if (Firebase.RTDB.setInt(&fbdo, userUnitsPath, units)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }

    // Update users units
    if (Firebase.RTDB.setInt(&fbdo, userOdometerPath, total_volume_user)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }

    // Update station units used
    if (Firebase.RTDB.setInt(&fbdo, stationUnitsUsedPath, unitsUsedStation)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }

    // Update station odometer
    if (Firebase.RTDB.setInt(&fbdo, stationOdometerPath, total_volume_station)){
      // Serial.println("PASSED");
      // Serial.println("PATH: " + String(fbdo.dataPath()));
      // Serial.println("TYPE: " + String(fbdo.dataType()));
    }
    else {
      // Serial.println("FAILED");
      // Serial.println("REASON: " + String(fbdo.errorReason()));
    }

    if ((total_volume_station-total_volume_user)>1){
      sendSMS("+255622109047", "Tamper detected at meter 01");
      // Update tamper messages
      if (Firebase.RTDB.setString(&fbdo, messagePath, message)){
        // Serial.println("PASSED");
        // Serial.println("PATH: " + String(fbdo.dataPath()));
        // Serial.println("TYPE: " + String(fbdo.dataType()));
      }
      else {
        // Serial.println("FAILED");  
        // Serial.println("REASON: " + String(fbdo.errorReason()));
      }
    }
  }
}

void sendSMS(String number, String message) {

  if(!(lastNumber==number && lastMessage==message)){
    lastNumber = number;
    lastMessage = message;

    // Command to set the recipient number
    gsm.print("AT+CMGS=\"");
    gsm.print(number);
    gsm.println("\"");

    // Wait for the module to respond
    // delay(200);

    // Send the message
    gsm.print(message);

    // End SMS and send Ctrl+Z (ASCII 26)
    gsm.write(0x1A);

    // Wait for the module to send the message
    delay(200);

    // Print response from gsm module
    while (gsm.available()) {
      Serial.write(gsm.read());
    }
  }
}

bool isTimeToUpdate() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return false; // Unable to get time
  }
  int currentSeconds = timeinfo.tm_sec;
  return (currentSeconds == 0 || currentSeconds == 30);
}

void sendDataToDatabase() {
  // Assuming makeUpdates() handles all the data update logic
  makeUpdates();
}

void sendAccumulatedDataIfNeeded() {
  unsigned long currentMillis = millis();
  if (currentMillis - lastUpdateMillis >= 30000) { // 30 seconds have passed
    lastUpdateMillis = currentMillis;

    // Assuming makeUpdates() handles all the data update logic
    // Use accumulated volumes for this update
    lastUserVolume = total_volume_user - accumulatedVolumeUser;
    lastStationVolume = total_volume_station - accumulatedVolumeStation;

    // Call the function to send data to the database
    sendDataToDatabase();

    // Reset accumulated volumes
    accumulatedVolumeUser = 0.0;
    accumulatedVolumeStation = 0.0;
  }
}

void setup() {
  Serial.begin(115200);
  gsm.begin(9600);
  pinMode(pumpPin, OUTPUT);

  LCDSetup();
  
  WifiSetup();

  FirebaseSetup();

  flowSetup();
  
  getUpdates();

  timeSetup();
}

void loop() {
  if (units==0.00){

    takeflowReadings();

    lcdDisplayToArduino();

    pumpControlbyUnits(units);

    getUpdates();

    if (x%50==0){
            //Finding units used difference
      unitsUsedUser = total_volume_user - lastUserVolume;
      unitsUsedStation = total_volume_station - lastStationVolume;
      
      makeUpdates();

      //Finding units used difference
      lastUserVolume = total_volume_user;
      lastStationVolume = total_volume_station;
    }
    x++;
  }
  else if (units > 0){
    takeflowReadings();

    lcdDisplayToArduino();

    pumpControlbyUnits(units);

    if(y%100 == 0){
      //Finding units used difference
      unitsUsedUser = total_volume_user - lastUserVolume;
      unitsUsedStation = total_volume_station - lastStationVolume;
      
      makeUpdates();

      //Finding units used difference
      lastUserVolume = total_volume_user;
      lastStationVolume = total_volume_station;
    }
    y++;
  }

  if (units < 0.25*onBootUnits && units > 0.2*onBootUnits){
    sendSMS("+255622109047", "You have more than 75% of the units you bought");
  }

  if (units == 0.00){
    sendSMS("+255622109047", "You have used all of your units bought");
  }
}