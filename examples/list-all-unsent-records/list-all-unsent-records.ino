#include "CustoFlash.h"

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("CUSTOFLASH LIST BACKLOG UTILITY"));
  Serial.println(F("LISTING ALL BACKLOGS: "));
  Serial.println();

  pinMode(LORA_RESET, OUTPUT);
  digitalWrite(LORA_RESET, LOW);
  CustoFlash.beginWork();
}

void loop() {
  RecordAddress_t recordAddr;
  uint16_t recordSize = CustoFlash.getNextBacklogAddress(&recordAddr);

  if (recordSize != NO_BACKLOG_RECORD) {
    uint8_t buf[recordSize];
    CustoFlash.readRecord(recordAddr, buf);
    Serial.print(F("Sector Index = "));
    Serial.print(recordAddr.sectorIndex);
    Serial.print(F(", Record Index = "));
    Serial.print(recordAddr.recordIndex);
    Serial.print(F(", content:"));
    for (int i = 0; i < recordSize; i++) {
      Serial.print(" ");
      Serial.print(buf[i], HEX);
    }
    Serial.println();
  } else {
    Serial.println();
    Serial.println(F("No more backlog."));
    Serial.println(F("Device halted."));
    while (true);
  }
}
