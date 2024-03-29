#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <UMS3.h>
#include <esp_bt.h>
#include <SPIFFS.h>
#include <Adafruit_NeoPixel.h>
#include <FS.h>

#define PIN 2      // Input your RGB LED module GPIO pin
#define NUM_LEDS 1 // Number of LEDs in your module

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

int DELAYVAL = 500; // Time (in milliseconds) to pause between pixels

UMS3 ums3;

WiFiMulti wifiMulti;

#define DEVICE "eps32-s3"

#define WIFI_SSID "SSID_OF_NETWORK"
#define WIFI_PASSWORD "PASSWORD_OF_NETWORK"

#define WIFI_SSID1 "SSID_OF_NETWORK1"
#define WIFI_PASSWORD1 "PASSWORD_OF_NETWORK1"
#define WIFI_SSID2 "SSID_OF_NETWORK2"
#define WIFI_PASSWORD2 "PASSWORD_OF_NETWORK2"
#define WIFI_SSID3 "SSID_OF_NETWORK3"
#define WIFI_PASSWORD3 "PASSWORD_OF_NETWORK3"

#define INFLUXDB_URL "http://10.20.2.50:5006"
#define INFLUXDB_TOKEN "3n56566UfmFARbfTey0xLliyjQaNmFNIe0gv0qX6Ls7U7EqVocMEWTbihsA=="
#define INFLUXDB_ORG "928c8dd365c43aa5"
#define INFLUXDB_BUCKET "farm"
#define TZ_INFO "AEST-10AEDT,M10.1.0,M4.1.0/3"

InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);

Point sensorReadings("measurements");

#define ONE_WIRE_BUS 1
#define DATA_FILE "/data.txt"
// Global variables to track time and battery voltage
unsigned long startMillis = 0;
float initialBatteryVoltage = 0.0;
int fileCount = 0;
struct DataPoint
{
  float temperature;
  float batteryVoltage;
};

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Input a value 0 to 255 to get a color value.
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}
void rainbow(uint8_t wait)
{
  uint16_t i, j;

  for (j = 0; j < 256 * 2; j++)
  { // repeat twice for a smoother transition
    for (i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel((i + j) & 255));
    }
    strip.show();
    delay(wait); // we acnt dekay here any more we will use the general 30 secs delay
    // break;
  }
}
void saveDataToSPIFFS(float temperature, float batteryVoltage)
{
  File file = SPIFFS.open(DATA_FILE, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  DataPoint dataPoint = {temperature, batteryVoltage};

  file.write((uint8_t *)&dataPoint, sizeof(DataPoint));
  file.close();
  Serial.println("data saved to spiffs");
}
void uploadDataFromSPIFFS()
{
  File file = SPIFFS.open(DATA_FILE, FILE_READ);
  if (!file)
  {
    Serial.println("No data to upload from SPIFFS");
    return;
  }

  while (file.available())
  {

      if (client.validateConnection())
    {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    }
    else
    {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }
    DataPoint dataPoint;
    size_t bytesRead = file.read((uint8_t *)&dataPoint, sizeof(DataPoint));

    if (bytesRead == sizeof(DataPoint))
    {
      
      sensorReadings.addField("temperature", dataPoint.temperature);
      sensorReadings.addField("battery_voltage", dataPoint.batteryVoltage);

      Serial.print("Writing data from SPIFFS: ");
      Serial.println(client.pointToLineProtocol(sensorReadings));

      client.writePoint(sensorReadings);
       Serial.println(client.writePoint(sensorReadings));
      sensorReadings.clearFields();
    }
    else
    {
      Serial.println("Error reading data from SPIFFS");
    }
    delay(1000);
  }

  file.close();

  // Delete data file from SPIFFS after upload
  SPIFFS.remove(DATA_FILE);
}
void initDS18B20()
{
  sensors.begin();
}
int trial = 0;
void setup()
{
  Serial.begin(115200);
  ums3.begin();
  sensors.begin();
  WiFi.mode(WIFI_STA);

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  wifiMulti.addAP(WIFI_SSID1, WIFI_PASSWORD1);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASSWORD2);
  wifiMulti.addAP(WIFI_SSID3, WIFI_PASSWORD3);
  btStop();

  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
    trial++;
    if (trial > 10)
    {
      trial = 0;
      break;
    }
  }
  Serial.println();

  initDS18B20();

  sensorReadings.addTag("device", DEVICE);
  sensorReadings.addTag("location", "farm");

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }
  SPIFFS.remove(DATA_FILE);
  startMillis = millis();                           // Store the current time
  initialBatteryVoltage = ums3.getBatteryVoltage(); // Store initial battery voltage
  strip.begin();                                    // Initialize the NeoPixel library
  strip.show();                                     // Initialize all pixels to 'off'
}

void loop()
{
  // Check USB presence
  int usbPresence = ums3.getVbusPresent();

  if (usbPresence == 0 && WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);
    WiFi.reconnect();
    wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
    wifiMulti.addAP(WIFI_SSID1, WIFI_PASSWORD1);
    wifiMulti.addAP(WIFI_SSID2, WIFI_PASSWORD2);
    wifiMulti.addAP(WIFI_SSID3, WIFI_PASSWORD3);
    btStop();
    while (wifiMulti.run() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(500);
      trial++;
      if (trial > 10)
      {
        trial = 0;
        break;
      }
    }
    Serial.println("wifi connected");
  }
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  float batteryVoltage = ums3.getBatteryVoltage();
  delay(1000);
  sensorReadings.addField("temperature", temperature);
  sensorReadings.addField("battery_voltage", batteryVoltage);
  sensorReadings.addField("usb_presence", usbPresence);
  // Upload data from SPIFFS if WiFi connection is restored
  if (client.validateConnection())
  {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // client.writePoint(sensorReadings);

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi connection lost");
    //  Save data to SPIFFS if USB power is not present
    saveDataToSPIFFS(temperature, batteryVoltage);
  }
  else
  {
    // Upload data from SPIFFS if WiFi connection is restored
    if (client.validateConnection())
    {
      Serial.print("Connected to InfluxDB: ");
      Serial.println(client.getServerUrl());
    }
    else
    {
      Serial.print("InfluxDB connection failed: ");
      Serial.println(client.getLastErrorMessage());
    }
    // Upload data from SPIFFS if WiFi connection is restored
    uploadDataFromSPIFFS();
    Serial.print("Writing: ");
    Serial.println(client.pointToLineProtocol(sensorReadings));
    client.writePoint(sensorReadings);
    Serial.println(client.writePoint(sensorReadings));
    sensorReadings.clearFields();
  }
  // Read battery voltage
  float currentBatteryVoltage = ums3.getBatteryVoltage();

  // Check if battery voltage is below 3.60V or 4 hours have elapsed (whichever comes first)
  if ((currentBatteryVoltage < 3.60) || (millis() - startMillis >= 4 * 60 * 60 * 1000 && !ums3.getVbusPresent()))
  {
    // Shut down the ESP32
    // esp_deep_sleep_start();
    Serial.println("esp 32 will turn off now");
  }
  // Send data to InfluxDB if USB power is present
  if (ums3.getVbusPresent() == 0)
  {
    if (fileCount == 0)
    {
      fileCount++;
      rainbow(60 * 1000 / 256); // 60 seconds divided by 256-color spectrum
      strip.clear();
      strip.show();
      if (fileCount >= 60)
        fileCount = 0; // do thid every 60 seconds
    }
  }
  else
  {
    fileCount = 0;
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    Serial.println("wifi disconnected successfully");
    // wifiMulti.disconnect();
    //  Save data to SPIFFS if USB power is not present
    saveDataToSPIFFS(temperature, batteryVoltage);
  }
  Serial.println("Wait 30s");
  delay(30000);
}
