#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiMulti.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <UMS3.h>
#include <esp_bt.h>

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

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void initDS18B20() {
  sensors.begin();
}

void setup() {
  Serial.begin(115200);
  ums3.begin();
  sensors.begin();
  WiFi.mode(WIFI_STA);

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  wifiMulti.addAP(WIFI_SSID1, WIFI_PASSWORD1);
  wifiMulti.addAP(WIFI_SSID2, WIFI_PASSWORD2);
  wifiMulti.addAP(WIFI_SSID3, WIFI_PASSWORD3);
  btStop();
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  initDS18B20();

  sensorReadings.addTag("device", DEVICE);
  sensorReadings.addTag("location", "farm");

  timeSync(TZ_INFO, "pool.ntp.org", "time.nis.gov");

  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
  sensors.requestTemperatures();

  float temperature = sensors.getTempCByIndex(0);
  float batteryVoltage = ums3.getBatteryVoltage();
  delay(1000);
  int usbpresence = ums3.getVbusPresent();

  sensorReadings.addField("temperature", temperature);
  sensorReadings.addField("battery_voltage", batteryVoltage);
  sensorReadings.addField("usb_presence", usbpresence);

  Serial.print("Writing: ");
  Serial.println(client.pointToLineProtocol(sensorReadings));

  client.writePoint(sensorReadings);

  sensorReadings.clearFields();

  if (wifiMulti.run() != WL_CONNECTED) {
    Serial.println("Wifi connection lost");
  }

  Serial.println("Wait 30s");
  delay(30000);
}
