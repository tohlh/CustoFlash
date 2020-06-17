#include "SerialFlashLayout.h"

void SerialFlashLayout::init() {
  begin(DEVICE_SELECT, CHIP_PIN);
  searchActiveSector();
  readSectorFlags();
  i = getNextRecordIndexForSector(k);
}

void SerialFlashLayout::writeRecord(const void *record, uint8_t recordSize) {
  if (isBlankState(flags.active_flag)) {
    activateBlankSector(recordSize);
  } else if (isActiveState(flags.active_flag)) {
    if (flags.s != recordSize) {
      if (i == 0) {
        reactivateCurrentSector(recordSize);
      } else {
        activateNextSector(recordSize);
      }
    }
  } else {
    // something wrong
    Serial.println(F("writeRecord ERROR: FILESYSTEM CORRUPTED"));
  }

  uint32_t recordAddr = k * SECTOR_SIZE + i * flags.s;
  write(recordAddr, record, recordSize);
  incrementRecordIndex();
}

uint16_t SerialFlashLayout::readRecord(RecordAddress_t recordAddress, void *buf) {
  if (recordAddress.sectorIndex >= MAX_SECTOR) {
    Serial.println(F("readRecord ERROR: INVALID SECTOR INDEX"));
    return INVALID_ADDRESS;
  }

  SectorFlags_t temp = retrieveSectorFlag(recordAddress.sectorIndex);
  uint16_t numberOfRecords = getNextRecordIndexForSector(recordAddress.sectorIndex);

  if (isBlankState(temp.active_flag)) {
    printf("ERROR: INVALID SECTOR ADDRESS \n");
    return INVALID_ADDRESS;
  } else {
    if (recordAddress.recordIndex >= numberOfRecords) {
      printf("ERROR: INVALID RECORD INDEX \n");
      return INVALID_ADDRESS;
    }
  }

  uint32_t recordAddr = recordAddress.sectorIndex * SECTOR_SIZE + recordAddress.recordIndex * temp.s;

  read(recordAddr, buf, temp.s);
  return temp.s;
}

uint16_t SerialFlashLayout::readRecords(RecordAddress_t* recordAddresses, uint16_t length, void *buf) {
  uint16_t recordSize;

  uint8_t *data = (uint8_t *) buf;

  for (int i = 0; i < length; i++) {
    recordSize = readRecord(recordAddresses[i], (data + i*recordSize));
  }

  return (recordSize * length);
}

void SerialFlashLayout::markRecordSent(RecordAddress_t recordAddr) {
  SectorFlags_t temp = retrieveSectorFlag(recordAddr.sectorIndex);

  uint16_t l = ceiling(temp.n, 8);                  // length of state bits in bytes
  uint32_t a = recordAddr.sectorIndex * SECTOR_SIZE;           // sector start address
  uint32_t addr = a + SECTOR_SIZE - (5 + (2 * l));  // record bits start address
  uint16_t offset = recordAddr.recordIndex / 8;                // position byte offset
  uint32_t byteAddress = addr + offset;             // position byte address
  uint8_t bitPosition = recordAddr.recordIndex % 8;            // position of bit to program

  uint8_t buf;
  read(byteAddress, &buf, 1);                       // replace with actual read
  buf &= ~((1 << bitPosition) & 0xFF);              // set 0 at the bit position
  write(byteAddress, &buf, 1);                      // replace with actual write

  if (!isActiveState(temp.active_flag) && !isBlankState(temp.active_flag)) {
    uint16_t recordsWritten = getNextRecordIndexForSector(recordAddr.sectorIndex);
    uint16_t lengthOfUsedBytes = ceiling(recordsWritten, 8);

    if (lengthOfUsedBytes == 0) {
      //nothing is written in this sector
      return;
    }

    uint8_t unsentBitMask[lengthOfUsedBytes];
    read(addr, &unsentBitMask, lengthOfUsedBytes);

    uint8_t remainderBits = recordsWritten % CHAR_BIT;
    uint8_t unusedBits = remainderBits == 0 ? 0 : CHAR_BIT - remainderBits;

    if (remainderBits != 0) {
      *(unsentBitMask + lengthOfUsedBytes - 1) &= ~((0xFF << remainderBits) & 0xFF); // set leading unused bits to 0
    }

    for (int i = 0; i < lengthOfUsedBytes; i++) {
      if (unsentBitMask[i] != 0x00) {
        return;
      }
    }

    markSectorSent(recordAddr.sectorIndex);
  }
}

void SerialFlashLayout::markRecordsSent(RecordAddress_t* recordAddresses, uint16_t length) {
  for (int i = 0; i < length; i++) {
    markRecordSent(recordAddresses[i]);
  }
}

void SerialFlashLayout::markLatestWrittenRecordSent() {
  uint16_t sectorIndex = getLatestWrittenRecordSector();
  uint16_t recordIndex = getLatestWrittenRecordIndex(sectorIndex);
  RecordAddress_t recordAddr = {
    .sectorIndex = sectorIndex,
    .recordIndex = recordIndex
  };
  markRecordSent(recordAddr);
}

uint16_t SerialFlashLayout::getLatestWrittenRecordSector() {
  if (i == 0) {
    //move to the previous sector
    uint16_t sectorIndex = k < 1 ? SECTOR_SIZE - 1 : k - 1;

    SectorFlags_t temp = retrieveSectorFlag(sectorIndex);

    if (isBlankState(temp.active_flag)) {
      //something is wrong
      return BLANK_STATE;
    } else {
      return sectorIndex;
    }

  } else {
    //current sector stores the latest record
    return k;
  }
}

uint16_t SerialFlashLayout::getLatestWrittenRecordIndex(uint16_t sectorIndex) {
  //recordIndex will never be < 0
  if (sectorIndex == NO_BACKLOG_SECTOR) {
    return NO_WRITTEN_RECORD;
  }
  uint16_t recordIndex = getNextRecordIndexForSector(sectorIndex) - 1;

  return recordIndex;
}

uint16_t SerialFlashLayout::getEarliestBacklogSector() {
  uint16_t seek_k = k + 1 >= MAX_SECTOR ? 0 : k + 1;
  uint16_t track = 0;

  while (track < MAX_SECTOR) {
    uint32_t a = seek_k * SECTOR_SIZE;

    uint32_t sectorStateAddr = a + (SECTOR_SIZE - 2);
    SectorState_t sectorState;
    read(sectorStateAddr, &sectorState, 2);

    if (sectorState.active != sectorState.unsent) {
      if (seek_k == k) {
        uint16_t recordsWritten = getNextRecordIndexForSector(k);
        return recordsWritten == 0 ? NO_BACKLOG_SECTOR : seek_k;
      } else {
        return seek_k;
      }
    }
    seek_k++;

    if (seek_k >= MAX_SECTOR) {
      seek_k = 0;
    }
    track++;
  }
  return BLANK_STATE;
}

uint16_t SerialFlashLayout::getEarliestBacklogIndex(uint16_t sectorIndex) {
  SectorFlags_t temp = retrieveSectorFlag(sectorIndex);

  uint16_t l = ceiling(temp.n, 8);  // length of state bits in bytes
  uint32_t a = sectorIndex * SECTOR_SIZE;             // sector start address
  uint32_t addr = a + SECTOR_SIZE - (5 + (2 * l));      // record bits start address

  uint16_t recordsWritten = getNextRecordIndexForSector(sectorIndex);
  uint16_t lengthOfUsedBytes = ceiling(recordsWritten, 8);

  if (lengthOfUsedBytes == 0) {
    //nothing is written in this sector
    return NO_BACKLOG_RECORD;
  }

  uint8_t buf[lengthOfUsedBytes];
  read(addr, &buf, lengthOfUsedBytes);

  uint16_t pos = countTrailingZeroes(buf, lengthOfUsedBytes); // first set bit

  if (pos >= recordsWritten) {
    Serial.println(F("Filesystem corrupted"));
    return NO_BACKLOG_RECORD;
  }
  return pos;
}

uint16_t SerialFlashLayout::getLatestBacklogSector() {
  uint16_t recordsWritten = getNextRecordIndexForSector(k);
  uint16_t prevSectorIndex = k < 1 ? MAX_SECTOR - 1 : k - 1;
  int16_t seek_k = recordsWritten == 0 ? prevSectorIndex : k;
  uint16_t track = 0;

  while (track < MAX_SECTOR) {
    uint32_t a = seek_k * SECTOR_SIZE;
    uint32_t sectorStateAddr = a + (SECTOR_SIZE - 2);

    SectorState_t sectorState;
    read(sectorStateAddr, &sectorState, 2);

    if (sectorState.active != sectorState.unsent) {
      return seek_k;
    }

    if (isBlankState(sectorState.active)) {
      break;
    }

    seek_k--;

    if (seek_k < 0) {
      seek_k = MAX_SECTOR - 1;
    }
    track++;
  }
  return NO_BACKLOG_SECTOR;
}

uint16_t SerialFlashLayout::getLatestBacklogIndex(uint16_t sectorIndex) {
  if (sectorIndex == NO_BACKLOG_SECTOR) {
    return NO_BACKLOG_RECORD;
  }

  SectorFlags_t temp = retrieveSectorFlag(sectorIndex);

  uint16_t l = ceiling(temp.n, 8);                  // length of state bits in bytes
  uint32_t a = sectorIndex * SECTOR_SIZE;           // sector start address
  uint32_t addr = a + SECTOR_SIZE - (5 + (2 * l));  // unsent bits start address

  uint16_t recordsWritten = getNextRecordIndexForSector(sectorIndex);

  if (recordsWritten == 0) {
    //nothing is written in this sector
    return NO_BACKLOG_RECORD;
  }

  return nextLatestBacklogIndex(addr, recordsWritten);
}

uint16_t SerialFlashLayout::getLatestBacklogIndex(uint16_t sectorIndex, uint16_t preceding) {
  SectorFlags_t temp = retrieveSectorFlag(sectorIndex);

  uint16_t l = ceiling(temp.n, 8);                  // length of state bits in bytes
  uint32_t a = sectorIndex * SECTOR_SIZE;           // sector start address
  uint32_t addr = a + SECTOR_SIZE - (5 + (2 * l));  // unsent bits start address

  if (preceding == 0) {
    //nothing is written in this sector
    return NO_BACKLOG_RECORD;
  }

  return nextLatestBacklogIndex(addr, preceding);
}

uint16_t SerialFlashLayout::retrieveLatestBacklogsAddresses(uint8_t payloadSize, RecordAddress_t *addresses) {
  uint16_t latestBacklogSector = getLatestBacklogSector();
  uint16_t latestBacklogIndex = getLatestBacklogIndex(latestBacklogSector);

  SectorFlags_t temp = retrieveSectorFlag(latestBacklogSector);

  uint8_t totalRecordsSize = temp.s;
  uint16_t firstLatestBacklogSector = latestBacklogSector;
  uint16_t index = -1;
  bool terminate = latestBacklogSector == NO_BACKLOG_SECTOR || latestBacklogIndex == NO_BACKLOG_RECORD;

  while (totalRecordsSize <= payloadSize && !terminate) {
    index++;
    (addresses + index)->sectorIndex = latestBacklogSector;
    (addresses + index)->recordIndex = latestBacklogIndex;
    totalRecordsSize += temp.s;

    latestBacklogIndex = getLatestBacklogIndex(latestBacklogSector, latestBacklogIndex);

    while (latestBacklogIndex == NO_BACKLOG_RECORD) {
      latestBacklogSector = latestBacklogSector > 0 ? latestBacklogSector - 1 : MAX_SECTOR - 1;
      if (firstLatestBacklogSector == latestBacklogSector) {  // full cycle
        terminate = true;
        break;
      }

      SectorFlags_t temp2 = retrieveSectorFlag(latestBacklogSector);
      if (temp2.s != temp.s || isBlankState(temp2.active_flag)) {
        terminate = true;
        break;
      }

      latestBacklogIndex = getLatestBacklogIndex(latestBacklogSector);
    }
  }
  return (index + 1);
}

uint16_t SerialFlashLayout::getCurrentSectorIndex() {
  return k;
}

uint16_t SerialFlashLayout::getNextRecordIndex() {
  return i;
}

//Functions related to states
bool SerialFlashLayout::isBlankState(uint8_t flag) {
  return flag == 0xFF;    //0xFF means blank
}

bool SerialFlashLayout::isActiveState(uint8_t flag) {
  if (flag == 0x00) {
    return false;
  }
  uint8_t msn = (flag & 0xF0) >> 4;   //most significant nibble
  uint8_t lsn = (flag & 0x0F);        //least significant nibble
  return ((msn << 1) & 0x0F) == lsn;
}

uint8_t SerialFlashLayout::activateState(uint8_t flag) {
  if (flag == 0x00) {
    return 0xFE;
  }
  return (flag & 0xF0) | ((flag << 1) & 0x0F);
}

uint8_t SerialFlashLayout::deactivateState(uint8_t flag) {
  return (flag & 0xF0) << 1 | (flag & 0x0F);
}

//Functions related to flags
void SerialFlashLayout::readSectorFlags() {
  //retrive flags from flash memory to flags struct
  uint32_t addr = (k + 1) * SECTOR_SIZE;
  read(addr - 5, &flags, 5);
}

void SerialFlashLayout::setFlags(uint16_t recordSize, uint8_t unsentState, uint8_t activeState) {
  //set flags struct before writing the flags to the flash memory
  flags.s = recordSize;
  if (flags.s == 0xFF) {
    flags.n = 0xFFFF;
  } else {
    flags.n = ((8 * SECTOR_SIZE) - 47) / (2 * (1 + 4*flags.s));
  }
  flags.unsent_flag = unsentState;
  flags.active_flag = activeState;
}

uint8_t SerialFlashLayout::getActiveFlag(uint16_t sectorIndex) {
  uint32_t flagAddr = (sectorIndex + 1) * SECTOR_SIZE - 1;
  uint8_t flag;
  read(flagAddr, &flag, 1);
  return flag;
}

SectorFlags_t SerialFlashLayout::retrieveSectorFlag(uint16_t sectorIndex) {
  SectorFlags_t sectorFlags;
  uint32_t sectorAddress = (sectorIndex + 1) * SECTOR_SIZE - 5;
  read(sectorAddress, &sectorFlags, 5);
  return sectorFlags;
}

//Functions related to sectors
void SerialFlashLayout::searchActiveSector() {
  uint16_t lower = 0;
  uint16_t upper = MAX_SECTOR;
  uint8_t lowerFlag = getActiveFlag(lower);

  if (isActiveState(lowerFlag) || isBlankState(lowerFlag)) {
    k = lower;
    return;
  }
  k = NO_ACTIVE_SECTOR;
  int round = 0;
  do {
    uint16_t mid = (lower + upper) / 2;
    if (lower == mid) {
      break;
    }

    uint8_t midFlag = getActiveFlag(mid);

    if (isActiveState(midFlag)) {
      k = mid;
      return;
    }

    if (midFlag == lowerFlag) {
      lower = mid;
    } else {
      upper = mid;
    }

    round++;
  } while ((1 << round) < MAX_SECTOR);
}

void SerialFlashLayout::sequentialSearchActiveSector() {
  //Sequential search approach, TODO: Implement binary search
  uint16_t seek_k = 0;

  do {
    uint32_t a = seek_k * SECTOR_SIZE;
    uint32_t seek_addr = a + (SECTOR_SIZE - 1);

    uint8_t flag;
    read(seek_addr, &flag, 1);

    if (isActiveState(flag) || isBlankState(flag)) {
      k = seek_k;
      break;
    } else {
      seek_k++;
    }
  } while (seek_k < MAX_SECTOR);

  if (seek_k >= MAX_SECTOR) {
    //something is very wrong
    Serial.println(F("The filesystem is corrupted."));
    return;
  }
}

void SerialFlashLayout::deactivateCurrentSector() {
  uint32_t a = k * SECTOR_SIZE;
  uint32_t addr = a + (SECTOR_SIZE - 1);
  uint8_t flag;
  read(addr, &flag, 1);
  if (isActiveState(flag)) {
    flag = deactivateState(flag);
    write(addr, &flag, 1);
  } else {
    Serial.println(F("This flag is not active."));
    Serial.println(F("Deactivate current sector ERROR!"));
  }
}

void SerialFlashLayout::activateSector(uint16_t sector, SectorFlags_t sectorFlags) {
  uint32_t a = sector * SECTOR_SIZE;
  uint32_t addr = a + (SECTOR_SIZE - 5);
  write(addr, &sectorFlags, 5);
}

void SerialFlashLayout::activateBlankSector(uint8_t recordSize) {
  uint8_t flag = activateState(flags.active_flag);
  setFlags(recordSize, flags.unsent_flag, flag);
  activateSector(k, flags);
}

void SerialFlashLayout::activateNextSector(uint8_t recordSize) {
  deactivateCurrentSector();
  k++;
  if (k >= MAX_SECTOR) {
    k = 0;
  }
  uint32_t a = k * SECTOR_SIZE;
  uint32_t addr = a + (SECTOR_SIZE - 1);
  uint8_t flag;
  read(addr, &flag, 1);
  eraseSector(a);
  setFlags(recordSize, flags.unsent_flag, activateState(flag));
  activateSector(k, flags);
  i = 0;  // restart record index in new sector
}

void SerialFlashLayout::reactivateCurrentSector(uint8_t recordSize) {
  setFlags(recordSize, flags.unsent_flag, flags.active_flag);
  uint32_t a = k * SECTOR_SIZE;
  eraseSector(a);
  activateSector(k, flags);
  i = 0;
}

//Functions related to record index
void SerialFlashLayout::incrementRecordIndex() {
  updateRecordPositionBit(i);
  i++;                            // increment record index
  if (i >= flags.n) {             // ensure record index doesn't exceed capacity
  activateNextSector(flags.s);
}
}

uint16_t SerialFlashLayout::getNextRecordIndexForSector(uint16_t sectorIndex) {
  SectorFlags_t temp = retrieveSectorFlag(sectorIndex);

  if (temp.n == 0xFFFF) {
    return 0;
  }

  uint32_t l = ceiling(temp.n, 8);            // length of state bits in bytes
  uint32_t a = sectorIndex * SECTOR_SIZE;     // sector start address
  uint32_t addr = a + SECTOR_SIZE - (5 + l);  // record bits start address

  uint8_t buf[l];                             // array to store record bits
  read(addr, &buf, l);

  uint16_t pos = countTrailingZeroes(buf, l); // first set bit

  if (pos > temp.n) {
    Serial.println(F("ERROR: FILESYSTEM CORRUPTED"));
    return CORRUPTED_FILESYSTEM;
  }
  return pos;
}

void SerialFlashLayout::updateRecordPositionBit(uint16_t pos) {
  uint16_t l = ceiling(flags.n, 8);           // length of state bits in bytes
  uint32_t a = k * SECTOR_SIZE;               // sector start address
  uint32_t addr = a + SECTOR_SIZE - (5 + l);  // record bits start address
  uint16_t offset = pos / 8;                  // position byte offset
  uint32_t byteAddress = addr + offset;       // position byte address
  uint8_t bitPosition = pos % 8;              // position of bit to program

  uint8_t buf;
  read(byteAddress, &buf, 1);                 // replace with actual read
  buf &= ~(1 << bitPosition);                 // set 0 at the bit position
  write(byteAddress, &buf, 1);                // replace with actual write
}

//Functions related to unsent sector and backlog
uint16_t SerialFlashLayout::nextLatestBacklogIndex(uint32_t addr, uint16_t preceding) {
  uint16_t lengthOfUsedBytes = ceiling(preceding, 8);
  uint8_t buf[lengthOfUsedBytes];
  read(addr, &buf, lengthOfUsedBytes);

  uint8_t remainderBits = preceding % CHAR_BIT;
  uint8_t unusedBits = remainderBits == 0 ? 0 : CHAR_BIT - remainderBits;

  if (remainderBits != 0) {
    *(buf + lengthOfUsedBytes - 1) &= ~((0xFF << remainderBits) & 0xFF); // set leading unused bits to 0
  }

  uint16_t lzExcludingUnusedBits = countLeadingZeroes(buf, lengthOfUsedBytes) - unusedBits;

  uint16_t pos = preceding - lzExcludingUnusedBits - 1; // last set bit

  return pos;
}

void SerialFlashLayout::markSectorSent(uint16_t sectorIndex) {
  uint32_t sectorStateAddr = (sectorIndex + 1) * SECTOR_SIZE - 2;

  SectorState_t sectorState;
  read(sectorStateAddr, &sectorState, 2);

  if (isBlankState(sectorState.unsent)) {
    // write active flag value to unsent flag
    write(sectorStateAddr, &sectorState.active, 1);
  } else {
    //something wrong
    Serial.println(F("Mark sector sent error!"));
  }
}

//Basic functions
uint16_t SerialFlashLayout::ctz(uint8_t x) {
  return x == 0 ? 8 : (uint16_t) LogTable256[x & (int8_t)(-x)];
}

uint16_t SerialFlashLayout::clz(uint8_t x) {
  return x == 0 ? 8 : (uint16_t) 7 - LogTable256[x];
}

uint16_t SerialFlashLayout::countTrailingZeroes(uint8_t* positionBytes, uint32_t length) {
  uint8_t* ptr;
  uint16_t i = 0;
  for (ptr = positionBytes; i < length - 1 && *ptr == 0x00; ptr++, i++);
  uint16_t pos = i * CHAR_BIT + ctz(*ptr);
  return pos;
}

uint16_t SerialFlashLayout::countLeadingZeroes(uint8_t* positionBytes, uint32_t length) {
  uint8_t* ptr;
  uint16_t i = 0;
  for (ptr = (positionBytes + length - 1); *ptr == 0x00 && i < length - 1; ptr--, i++);
  uint16_t pos = i * CHAR_BIT + clz(*ptr);
  return pos;
}

uint16_t SerialFlashLayout::ceiling(uint16_t numerator, uint16_t denominator) {
  return (numerator + denominator - 1) / denominator;
}
