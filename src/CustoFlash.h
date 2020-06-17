#include "SerialFlashLayout.h"
#include "SerialFlashSector.h"
#include "SerialFlashRecord.h"

class CustoFlash {
private:
  SerialFlashLayout layout;

  //For getNextBacklogAddress
  uint16_t sectorIndex;
  uint16_t recordIndex;
  uint16_t terminateSector;

public:
  void beginWork() {
    layout.init();
    layout.wakeup();
    resetIndexes();
  }
  void endWork() {
    layout.sleep();
  }
  RecordAddress_t writeRecord(const void *record, uint8_t recordSize) {
    RecordAddress_t ret;
    ret.sectorIndex = layout.getCurrentSectorIndex();
    ret.recordIndex = layout.getNextRecordIndex();
    layout.writeRecord(record, recordSize);
    return ret;
  }
  uint16_t readRecord(RecordAddress_t recordAddress, void *buf) {
    return layout.readRecord(recordAddress, buf);
  }
  uint16_t readRecords(RecordAddress_t* recordAddresses, uint16_t length, void *buf) {
    return layout.readRecords(recordAddresses, length, buf);
  }
  void markRecordSent(RecordAddress_t recordAddr) {
    layout.markRecordSent(recordAddr);
  }
  void markRecordsSent(RecordAddress_t* recordAddresses, uint16_t length) {
    layout.markRecordsSent(recordAddresses, length);
  }
  void markLatestWrittenRecordSent() {
    layout.markLatestWrittenRecordSent();
  }
  uint16_t getLatestWrittenRecordSector() {
    return layout.getLatestWrittenRecordSector();
  }
  uint16_t getLatestWrittenRecordIndex(uint16_t sectorIndex) {
    return layout.getLatestWrittenRecordIndex(sectorIndex);
  }
  uint16_t getEarliestBacklogSector() {
    return layout.getEarliestBacklogSector();
  }
  uint16_t getEarliestBacklogIndex(uint16_t sectorIndex) {
    return layout.getEarliestBacklogIndex(sectorIndex);
  }
  uint16_t getLatestBacklogSector() {
    return layout.getLatestBacklogSector();
  }
  uint16_t getLatestBacklogIndex(uint16_t sectorIndex) {
    return layout.getLatestBacklogIndex(sectorIndex);
  }
  uint16_t getLatestBacklogIndex(uint16_t sectorIndex, uint16_t preceding) {
    return layout.getLatestBacklogIndex(sectorIndex, preceding);
  }
  uint16_t retrieveLatestBacklogsAddresses(uint8_t payloadSize, RecordAddress_t *addresses) {
    return layout.retrieveLatestBacklogsAddresses(payloadSize, addresses);
  }
  uint16_t getNextRecordIndexForSector(uint16_t sectorIndex) {
    return layout.getNextRecordIndexForSector(sectorIndex);
  }
  uint16_t getNextBacklogAddress(RecordAddress_t* recordAddr) {
    uint16_t backlogIndex = layout.getLatestBacklogIndex(sectorIndex, recordIndex);

    while (backlogIndex == NO_BACKLOG_RECORD) {
      sectorIndex = sectorIndex < 1 ? MAX_SECTOR - 1 : sectorIndex - 1;
      backlogIndex = layout.getLatestBacklogIndex(sectorIndex);

      if (sectorIndex == terminateSector) {
        resetIndexes();
        return NO_BACKLOG_RECORD;
      }
    }

    SectorFlags_t flags;
    uint32_t flagAddress = (sectorIndex + 1) * SECTOR_SIZE - 5;
    layout.read(flagAddress, &flags, 5);

    recordIndex = backlogIndex;
    recordAddr -> sectorIndex = sectorIndex;
    recordAddr -> recordIndex = backlogIndex;

    return flags.s;
  }

  //From SerialFlashChip
  void read(uint32_t addr, void *buf, uint32_t len) {
    layout.read(addr, buf, len);
  }
  void write(uint32_t addr, const void *buf, uint32_t len) {
    layout.write(addr, buf, len);
  }
  void eraseAll() {
    layout.eraseAll();
  }
  void eraseSector(uint32_t addr) {
    layout.eraseSector(addr);
  }
  void eraseBlock(uint32_t addr) {
    layout.eraseBlock(addr);
  }

  //Functions instantiating other classes
  SerialFlashSector getSector(uint16_t sectorIndex) {
    SerialFlashSector sector(sectorIndex);
    return sector;
  }
  SerialFlashSector getActiveSector() {
    uint16_t activeSector = layout.getCurrentSectorIndex();
    SerialFlashSector sector(activeSector);
    return sector;
  }

private:
  void resetIndexes() {
    sectorIndex = layout.getCurrentSectorIndex();
    recordIndex = layout.getNextRecordIndexForSector(sectorIndex);
    terminateSector = sectorIndex + 1 >= MAX_SECTOR ? 0 : sectorIndex + 1;
  }

};

CustoFlash flash;
