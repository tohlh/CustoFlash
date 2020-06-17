#ifndef INCLUDE_SERIAL_FLASH_RECORD
#define INCLUDE_SERIAL_FLASH_RECORD

#include "SerialFlashLayout.h"

class SerialFlashRecord : virtual public SerialFlashLayout {

private:
  RecordAddress_t address;
  uint8_t buf[242];
  uint8_t recordSize;

public:
  SerialFlashRecord(uint16_t sectorIndex, uint16_t recordIndex) {
    address.sectorIndex = sectorIndex;
    address.recordIndex = recordIndex;
    recordSize = readRecord(address, buf);
  }

  bool hasBeenSent() {
    uint16_t recordIndex = getLatestBacklogIndex(address.sectorIndex);

    while(recordIndex != NO_BACKLOG_RECORD) {
      if(recordIndex == address.recordIndex) {
        return false;
      }
      recordIndex = getLatestBacklogIndex(address.sectorIndex, recordIndex);
    }

    return true;
  }

  void markSent() {
    markRecordSent(address);
  }

  uint8_t getRecordSize() {
    return recordSize;
  }

  uint8_t readContent(uint8_t *record) {
    for (int i = 0; i < recordSize; i++) {
      record[i] = buf[i];
    }
    return recordSize;
  }

  String getHexString() {
    String ret = "";
    for (int i = 0; i < recordSize; i++) {
      String hex = String(buf[i], HEX);
      if (hex.length() == 1) {
        ret += "0";
        ret += hex;
      } else {
        ret += hex;
      }
    }
    return ret;
  }

};

#endif
