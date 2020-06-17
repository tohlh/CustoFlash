#include "CustoFlash.h"

int userResponse;
uint8_t buf;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("CUSTOFLASH ERASE UTILITY"));
  delay(1000);

  pinMode(LORA_RESET, OUTPUT);
  digitalWrite(LORA_RESET, LOW);
  CustoFlash.beginWork();

  Serial.println();
  Serial.println(F("This utility will erase everything on the flash memory."));
  Serial.println(F("Are you sure to erase the memory?"));
  Serial.println(F("WARNING: Erased records CANNOT be recovered."));
  delay(2000);

  Serial.println();
  Serial.println(F("Enter '1' to confirm, '0' to cancel."));

  while(!Serial.available());
  userResponse = Serial.read();

  if (userResponse == '1') {
    Serial.println();
    Serial.println(F("Starting to erase entire flash memory."));
    CustoFlash.eraseAll();
  } else {
    Serial.println();
    Serial.println(F("Erase process cancelled."));
    Serial.println(F("Device halted."));
    while (true);
  }
}

void loop() {
  Serial.println("Memory erased, checking flash.");
  for(uint32_t i = 0; i < MAX_SECTOR * SECTOR_SIZE; i++) {
    CustoFlash.read(i, &buf, 1);
    if (buf != 0xFF) {
      Serial.println(F("Erase process failed! Please try again."));
      while (true);
    }
    switch (i) {
      case (MAX_SECTOR * SECTOR_SIZE * 10) / 100:
        Serial.println(F("Checked: 10%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 20) / 100:
        Serial.println(F("Checked: 20%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 30) / 100:
        Serial.println(F("Checked: 30%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 40) / 100:
        Serial.println(F("Checked: 40%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 50) / 100:
        Serial.println(F("Checked: 50%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 60) / 100:
        Serial.println(F("Checked: 60%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 70) / 100:
        Serial.println(F("Checked: 70%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 80) / 100:
        Serial.println(F("Checked: 80%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE * 90) / 100:
        Serial.println(F("Checked: 90%"));
        break;
      case (MAX_SECTOR * SECTOR_SIZE - 1):
        Serial.println(F("Checked: 100%"));
        break;
    }
  }

  Serial.println();
  Serial.println(F("Erase process successful."));
  while (true);

}
