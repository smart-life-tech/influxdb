I need help on finalizing Freezer Temp code

Equipment Used;
- ProS3 ESP32-S3 Dev board
- Polymer Lithium Ion Battery (LiPo) 3.7V 1100mAh
- DS18B20 Digital temperature sensor
- Gravity: Digital RGB LED Module

AIM:

- Using ESP32 to read temperature from a DS18B20 sensor and battery voltage via the UMS3 library
- Sending the readings to a influxd cloud server
- Send the readings when the usb power is on ( if usb_presence=1)
- Wanna have led module code to be function when the usb power is on (usb_presence=1)
- Store data on ESP32 until if there’s NO a WiFi connection
- Store data on ESP32 until if there’s NO a influxdb connection
- Disable WiFi when the ESP32 is running on battery mode when the usb presence is =0
- Shut down the ESP32 if the battery reaches 3.60V or after 4 hours (whichever is first) if the usb presence is =0

I already have the base code for reading & sending data to influxdb
Also have the led module code want this module run when there is power on via USB.

Wanna hear thoughts on the aim?

Like i have been recommended using Serialization to store data. https://www.boost.org/doc/libs/1_36_0/libs/serialization/doc/index.html Example using serilization; https://www.boost.org/doc/libs/1_36_0/libs/serialization/example/demo.cpp