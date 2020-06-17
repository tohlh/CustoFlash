#include "CustoFlash.h"

#define RECORDS_SHOWN   5
#define PAGE_LENGTH     256

uint16_t sectorIndex = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("CUSTOFLASH SECTOR DIAGNOSTIC UTILITY"));

  pinMode(LORA_RESET, OUTPUT);
  digitalWrite(LORA_RESET, LOW);
  flash.beginWork();
}

void loop() {
  if (sectorIndex >= MAX_SECTOR) {
    Serial.println();
    Serial.println("Sector diagnostic completed,");
    Serial.println("Device halted.");
    while(true);
  }

  Serial.println();
  Serial.print("SECTOR INDEX: ");
  Serial.println(sectorIndex);

  SerialFlashSector sector = flash.getSector(sectorIndex);

  if (sector.isBlank()) {
    Serial.println("Blank sector check...");
    if (sector.verifyBlank()) {
      Serial.println("Sector is OK.");
    } else {
      Serial.println("Sector corrupted!");
    }
  } else {
    Serial.print("Active flag: ");
    Serial.println(sector.getActiveFlag(), HEX);
    Serial.print("Unsent flag: ");
    Serial.println(sector.getUnsentFlag(), HEX);
    Serial.print("Record size: ");
    Serial.println(sector.getRecordSize());
    Serial.print("Written records: ");
    Serial.print(sector.getWrittenCount());
    Serial.print("/");
    Serial.println(sector.getMaxCount());
    Serial.print("Unsent records: ");
    Serial.print(sector.getUnsentCount());
    Serial.print("/");
    Serial.println(sector.getWrittenCount());

    if (sector.getWrittenCount() >= RECORDS_SHOWN) {
      Serial.print("First 5 records of sector ");
      Serial.print(sectorIndex);
      Serial.println(":");
      for (int i = 0; i < RECORDS_SHOWN; i++) {
        SerialFlashRecord record = sector.getRecord(i);
        uint8_t recordSize = record.getRecordSize();
        uint8_t buf[recordSize];
        record.readContent(buf);

        Serial.print("Record ");
        Serial.print(i);
        Serial.print(": ");
        for (int j = 0; j < recordSize; j++) {
          Serial.print(buf[j], HEX);
          Serial.print(" ");
        }
        Serial.println();
      }
    } else {
      Serial.print("First ");
      Serial.print(sector.getWrittenCount());
      Serial.print(" records of sector ");
      Serial.print(sectorIndex);
      Serial.println(":");

      for (int i = 0; i < sector.getWrittenCount(); i++) {
        SerialFlashRecord record = sector.getRecord(i);
        uint8_t recordSize = record.getRecordSize();
        uint8_t buf[recordSize];
        record.readContent(buf);

        Serial.print("Record ");
        Serial.print(i);
        Serial.print(": ");
        for (int j = 0; j < recordSize; j++) {
          Serial.print(buf[j], HEX);
          Serial.print(" ");
        }
        Serial.println();
      }
    }
  }
  sectorIndex++;
}
