/*

    Web Server Stick v4
    Author: Toby Chui

    This firmware load and serve web content
    from microSD card.

    The following firmware config are recommended
    Board: Wemos D1 Mini
    CPU clockspeed: 160Mhz
    IwIP Varient: v2 Higher Bandwidth

    Require external library:
    - ESPAsyncTCP (https://github.com/me-no-dev/ESPAsyncTCP)
    - ESPAsyncWebServer (https://github.com/me-no-dev/ESPAsyncWebServer)
    - ArduinoJson (https://github.com/bblanchon/ArduinoJson)
    - ESPping (https://github.com/dvarrel/ESPping)
    - Wake On LAN (https://github.com/a7md0/WakeOnLan)
*/

//WiFi library
#include <ESP8266WiFi.h>

//SD cards library
#include <SPI.h>
#include <SD.h>
#include <FS.h>

//Web server library
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

//Discovery related library
#include <ESP8266mDNS.h>
#include <ESPping.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>


/* Hardware Configurations */
#define CS_PIN D0

/* SD Card SPI Speed Definations */
#define SDSPI_HALF_SPEED 4000000
#define SDSPI_DEFAULT_SPEED 32000000
#define SDSPI_HIGH_SPEED 64000000

/* Software Global Variables */
AsyncWebServer server(80);
String adminUsername = "";
String adminPassword = "";
String mdnsName = "webstick";
String authSession = ""; //Session key for admin
int SDCardInitSpeed = SDSPI_HIGH_SPEED;

/* Time Keeping */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

/* Wake On Lan */
WakeOnLan WOL(ntpUDP);

/* Debug variables */

/* Function definations */
String loadWiFiInfoFromSD();


void setup() {
  // Setup Debug Serial Port
  Serial.begin(115200);

  //Try Initialize SD card (blocking)
  while (!SD.begin(CS_PIN, SDCardInitSpeed)) {
    if (SDCardInitSpeed == SDSPI_HIGH_SPEED) {
      //Fallback to default speed
      SDCardInitSpeed = SDSPI_DEFAULT_SPEED;
      Serial.println("SD card initialization failed. Falling back to default speed");
    } else if (SDCardInitSpeed == SDSPI_DEFAULT_SPEED) {
      //Fallback to half speed (legacy mode)
      SDCardInitSpeed = SDSPI_HALF_SPEED;
      Serial.println("SD card initialization failed. Falling back to legacy SPI_HALF_SPEED");
    }else{
      Serial.println("SD card initialization failed. Retrying in 3 seconds...");
    }
    delay(3000);
  }
  Serial.println("SD card initialized");
  Serial.println("\n\nStorage Info:");
  Serial.println("----------------------");
  getSDCardTotalSpace();
  getSDCardUsedSpace();
  Serial.println("----------------------");
  Serial.println();

  //Connect to wifi based on settings (cfg/wifi.txt)
  initWiFiConn();

  //Load admin credentials from SD card (cfg/admin.txt)
  initAdminCredentials();

  //Start mDNS service
  initmDNSName();
  if (!MDNS.begin(mdnsName)) {
    Serial.println("mDNS Error. Skipping.");
  } else {
    Serial.println("mDNS started. Connect to your webstick using http://" + mdnsName + ".local");
    MDNS.addService("http", "tcp", 80);
  }

  //Start NTP time client
  timeClient.begin();
  Serial.print("Requesting time from NTP (unix timestamp): ");
  timeClient.update();
  Serial.println(getTime());

  //Wake on Lan Settings
  WOL.setRepeat(3, 100);
  WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());

  //Initialize database
  DBInit();

  //Resume login session if any
  initLoginSessionKey();

  // Start listening to HTTP Requests
  initWebServer();
}


void loop() {
  MDNS.update();
  timeClient.update();
}
