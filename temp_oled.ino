#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <WifiUDP.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>

// Define NTP properties
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "1.de.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);
String date;
String t;

// OLED Settings
#define OLED_ADDR  0x3C
#define OLED_WIDTH 128
#define CHAR_WIDTH 6

// Define the OLED and BME objects
Adafruit_SSD1306 display(-1);
Adafruit_BME280 bme; // I2C

// Check if OLED lib is correct
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

bool connect_wifi(const char* ssid, const char* password, int retry) {
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Connecting WiFi");
  display.setCursor(0,16);
  display.print("SSID: ");
  printRight(ssid, 16);
  display.display();
  WiFi.begin(ssid, password);
  int trycount = 0;
  while ( WiFi.status() != WL_CONNECTED && trycount != retry )
  {
    delay(500);
    trycount += 1;
  }
  if ( WiFi.status() == WL_CONNECTED ) {
    return true;
  } else {
    return false;
  }
}

bool connect_loop() {
  // Main network
  const char* ssid = "********";
  const char* password = "********";
  if ( connect_wifi(ssid, password, 30) ) {
    return true;
  } else {
    // Fallback network
    ssid = "********";
    password = "********";
    if ( connect_wifi(ssid, password, 30) ) {
      return true;
    } else {
      printCenter("Could not connect", 32);
      return false;
    }
  }
}

void display_ip() {
  display.setCursor(0,32);
  display.print("Connected!");
  display.setCursor(0,48);
  display.print(WiFi.localIP());
  display.display();
  delay(3000);
  display.clearDisplay();
  display.display();
}

void setup() {
  timeClient.begin();   // Start the NTP UDP client
  // Define pin 0 and 2 for i2c
  Wire.pins(0, 2);
  Wire.begin();

  // initialize and clear display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();

  // Connect Wifi
  while ( true ) {
    if ( !connect_loop() ) {
      delay(30000);
    } else {
      break;
    }
  }
  display_ip();

  // Initialize and check BME280 sensor
  bool status = bme.begin(0x76);  
  if (!status) {
    display.setCursor(0,0);
    display.print("BME-280");
    display.setCursor(0,16);
    display.print("Not found!");
    display.setCursor(0,32);
    display.print("Check wiring");
    display.display();
    while (1);
  }
}

// right align text
void printRight( const char* data, int line ) {
  display.setCursor((OLED_WIDTH - (strlen(data) * CHAR_WIDTH)), line);
  display.print(data);
}

// center size 2 text
void printCenterBig( const char* data, int line ) {
  display.setCursor(((OLED_WIDTH / 2) - (strlen(data) * CHAR_WIDTH)), line);
  display.print(data);
}

// center size 1 text
void printCenter( const char* data, int line ) {
  display.setCursor(((OLED_WIDTH / 2) - ((strlen(data) * CHAR_WIDTH) / 2)), line);
  display.print(data);
}

void loop() {
  date = "";  // clear the variables
  t = "";
  
  // update the NTP client and get the UNIX UTC timestamp 
  timeClient.update();
  unsigned long epochTime =  timeClient.getEpochTime();

  // convert received time stamp to time_t object
  time_t local, utc;
  utc = epochTime;

  // Then convert the UTC UNIX timestamp to local time
  TimeChangeRule MET = {"CEST", First, Sun, Nov, 2, +120};   //UTC + 2 hours
  TimeChangeRule CEST = {"MET", Second, Sun, Nov, 2, +60};   //UTC + 1 hour
  Timezone europe(MET, CEST);
  local = europe.toLocal(utc);

  // now format the Time variables into strings
  date += day(local);
  date += ".";
  date += month(local);
  date += ".";
  date += year(local);

  // format the time to 24-hour format and no seconds
  if(hour(local) < 10)
    t += "0";
  t += hour(local);
  t += ":";
  if(minute(local) < 10)  // add a zero if minute is under 10
    t += "0";
  t += minute(local);
  
  // Read data from the bme280 sensor
  float humi = bme.readHumidity();
  float temp = bme.readTemperature();
  float pres = (bme.readPressure() / 100.0F);

  // display date and time
  display.setTextSize(1);
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(date);
  const char* t_char = t.c_str();
  printRight(t_char, 0);

  // display sensor data (first 2 lines large)
  display.setTextSize(2);
  String t_text =  String(temp)+" "+String(char(0xF7))+"C"; // 0xF7 = Degree Symbol
  const char* t_data = t_text.c_str();
  printCenterBig(t_data, 16);
  String h_text =  String(humi)+" %";
  const char* h_data = h_text.c_str();
  printCenterBig(h_data, 34);
  display.setTextSize(1);
  String p_text =  String(pres)+" hPa";
  const char* p_data = p_text.c_str();
  printCenter(p_data, 56);
  display.display();
  // sleep for 30 secs.
  delay(30000);
}


