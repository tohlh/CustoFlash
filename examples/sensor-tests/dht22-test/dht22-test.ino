/*
 * This sketch uses DHT library by AdaFruit,
 * please make sure you have the library installed.
 */
#include <DHT.h>
#include "CustoFlash.h"

#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define CHECK_INTERVAL 60000

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("Device Started"));
  pinMode(LORA_RESET, OUTPUT);
  digitalWrite(LORA_RESET, LOW);

  dht.begin();
  CustoFlash.beginWork();
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));

  int humid = round(h * 100);
  int temp = round(t * 100);
  uint8_t humidLSB = humid & 0x00FF;
  uint8_t humidMSB = (humid & 0xFF00) >> 8;
  uint8_t tempLSB = temp & 0x00FF;
  uint8_t tempMSB = (temp & 0xFF0) >> 8;
  uint8_t record[] = { humidLSB, humidMSB, tempLSB, tempMSB };

  Serial.println();
  Serial.print(F("Record: "));
  for (int i = 0; i < 4; i++) {
    Serial.print(record[i], HEX);
    Serial.print(" ");
  }
  RecordAddress_t recordAddr = CustoFlash.writeRecord(record, 4);
  Serial.print(" Sector Index: ");
  Serial.print(recordAddr.sectorIndex);
  Serial.print(" Record Index: ");
  Serial.println(recordAddr.recordIndex);
  delay(CHECK_INTERVAL);
}
