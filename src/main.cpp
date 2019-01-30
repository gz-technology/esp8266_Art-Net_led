#define DEBUG_HARDWARE_SERIAL                                                   // if commented, not defined, serial debug info will be off
#define SERIAL_SPEED 115200
#define CODE_VERSION 1.5
#define HOSTNAME "costume"                                                      //TODO change to assign dynamicly based on ip

#define LED_OUT    13

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "credentials.h"                                                        //ignored by git to keep the network details in separate file private
/* credentials.h example:
const char* ssid = "network_name";
const char* password = "password";
IPAddress gateway(10,0,100,1);
 */

 #include <ArduinoOTA.h>
//   |--------------|-------|---------------|--|--|--|--|--|
//   ^              ^       ^               ^     ^
//   Sketch    OTA update   File system   EEPROM  WiFi config (SDK)

extern "C"{
 #include "user_interface.h"                                                    //NOTE needed for esp_system_info Since the include file from SDK is a plain C not a C++
}
#include "devices.h"                                                            //list of MAC addresses of devices for self assigning static IPs
IPAddress deviceip;
int unit_ID;

//------------------------ functions -------------------------------------------
void blink(int tOn, int tOff){                                                  // for testing led
static int timer=tOn;
  static long previousMillis;
  if ((millis() - previousMillis) >= timer) {
    if (digitalRead(LED_OUT) == HIGH) {
      timer = tOff;
    } else {
      timer = tOn;
    }
digitalWrite(LED_OUT, !digitalRead(LED_OUT));
     previousMillis = millis();
  }
}
//------------------------------------------------------------------------------

void setup() {
  pinMode(LED_OUT, OUTPUT);
  analogWrite(LED_OUT, 0);

  #ifdef DEBUG_HARDWARE_SERIAL
    Serial.begin(SERIAL_SPEED);
    Serial.println(" ");  Serial.println(" ");  Serial.println(" ");            //Some compilation and other info 
    Serial.println("---------------------------------------");
    Serial.println("Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
    Serial.print("Code version: "); Serial.println(CODE_VERSION);
    Serial.println("---------------------------------------");
    Serial.println("ESP Info: ");
    Serial.print( F("Heap: ") ); Serial.println(system_get_free_heap_size());
    Serial.print( F("Boot Vers: ") ); Serial.println(system_get_boot_version());
    Serial.print( F("CPU: ") ); Serial.println(system_get_cpu_freq());
    Serial.print( F("SDK: ") ); Serial.println(system_get_sdk_version());
    Serial.print( F("Chip ID: ") ); Serial.println(system_get_chip_id());
    Serial.print( F("Flash ID: ") ); Serial.println(spi_flash_get_id());
    Serial.print( F("Flash Size: ") ); Serial.println(ESP.getFlashChipRealSize());
    Serial.printf("Sketch size: %u\n", ESP.getSketchSize());
    Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());
    Serial.print( F("Vcc: ") ); Serial.println(ESP.getVcc());
    Serial.println(" ");
    Serial.println("Starting Setup");
  #endif

//---------------------------- WiFi --------------------------------------------
// determine IP address based on devices.h definitions, each device has exactly the same code and address itself accordingly to mac table
  int chip_id = ESP.getChipId();
  const device_details *device = devices;
  for (; device->esp_chip_id != 0; device++) {
    #ifdef DEBUG_HARDWARE_SERIAL
      Serial.printf("chip_id %X = %X?\n", chip_id, device->esp_chip_id);
    #endif
    if (device->esp_chip_id == chip_id)
      break;
  }
  if (device->esp_chip_id == 0) {
    while(1) {
      #ifdef DEBUG_HARDWARE_SERIAL
        Serial.println("Could not obtain a chipId we know. Means we dont know what id/IP address to asign. Fail");
        Serial.printf("This ESP8266 Chip id = 0x%08X\n", chip_id);
      #endif
    }
  }
  deviceip = IPAddress(gateway);
  deviceip[3] = device->id;
  #ifdef DEBUG_HARDWARE_SERIAL
    Serial.print("found in devices list ID: "); Serial.println(deviceip[3]);
  #endif

  WiFi.mode(WIFI_STA);
  WiFi.config(deviceip, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    #ifdef DEBUG_HARDWARE_SERIAL
      Serial.println("Connection Failed! Rebooting...");
    #endif
    delay(5000);
    ESP.restart();
  }

  #ifdef DEBUG_HARDWARE_SERIAL
    Serial.print("unit MAC address: "); Serial.println(WiFi.macAddress());
    Serial.print("assigned IP address: "); Serial.println(WiFi.localIP());
  #endif

// --------------------------- OTA ---------------------------------------------
  ArduinoOTA.setHostname(HOSTNAME);
  #ifdef DEBUG_HARDWARE_SERIAL
    Serial.print("Hostname: "); Serial.println(HOSTNAME);
  #endif

  ArduinoOTA.onStart([]() {
  #ifdef DEBUG_HARDWARE_SERIAL
    Serial.println("Uploading...");
  #endif
  });
  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG_HARDWARE_SERIAL
      Serial.println("\nEnd");
    #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef DEBUG_HARDWARE_SERIAL
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });
  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG_HARDWARE_SERIAL
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    #endif
  });
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
  blink(500,500);                                                               // for testing led
}
