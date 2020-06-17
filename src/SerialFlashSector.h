#ifndef INCLUDE_SERIAL_FLASH_SECTOR
#define INCLUDE_SERIAL_FLASH_SECTOR

#include "SerialFlashLayout.h"
#include "SerialFlashRecord.h"

#define PAGE_LENGTH   256

class SerialFlashSector : virtual public SerialFlashLayout {

private:
  uint16_t sectorIndex;
  SectorFlags_t flags;

public:
  SerialFlashSector(uint16_t index) {
    sectorIndex = index;
    flags = retrieveSectorFlag(index);
  }

  uint8_t getActiveFlag() {
    return flags.active_flag;
  }

  uint8_t getUnsentFlag() {
    return flags.unsent_flag;
  }

  uint8_t getRecordSize() {
    return flags.s;
  }

  uint16_t getMaxCount() {
    return flags.n;
  }

  uint16_t getWrittenCount() {
    return getNextRecordIndexForSector(sectorIndex);
  }

  uint16_t getUnsentCount() {
    uint16_t unsentCount = 0;
    uint16_t recordIndex = getLatestBacklogIndex(sectorIndex);

    while(recordIndex != NO_BACKLOG_RECORD) {
      unsentCount++;
      recordIndex = getLatestBacklogIndex(sectorIndex, recordIndex);
    }
    return unsentCount;
  }

  bool isActive() {
    return isActiveState(flags.active_flag);
  }

  bool isBlank() {
    return isBlankState(flags.active_flag);
  }

  bool verifyBlank() {
    uint8_t buf[PAGE_LENGTH];
    for (int i = 0; i < SECTOR_SIZE; i += PAGE_LENGTH) {
      uint32_t checkAddr = (sectorIndex * SECTOR_SIZE) + i;
      read(checkAddr, buf, PAGE_LENGTH);

      for (int i = 0; i < PAGE_LENGTH; i++) {
        if (buf[i] != 0xFF) {
          return false;
        }
      }
    }
    return true;
  }

  //Functions instantiating record class
  SerialFlashRecord getRecord(uint16_t recordIndex) {
    SerialFlashRecord record(sectorIndex, recordIndex);
    return record;
  }
};

#endif //INCLUDE_SERIAL_FLASH_SECTOR
