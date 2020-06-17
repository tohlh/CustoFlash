#ifndef INCLUDE_SERIAL_FLASH_LAYOUT
#define INCLUDE_SERIAL_FLASH_LAYOUT

#include "SerialFlashChip.h"
#define SECTOR_SIZE           4096
#define MAX_SECTOR            512     //last sector index is 7,actual is 512
#define CHAR_BIT              8
#define NO_BACKLOG_SECTOR     (uint16_t) -1   //no latest or earliest backlog sector
#define NO_BACKLOG_RECORD     (uint16_t) -1   //no latest or earliest backlog records in the sector
#define BLANK_STATE           (uint16_t) -1   //when we request latest written record in a blank state
#define NO_WRITTEN_RECORD     (uint16_t) -1   //no latest written record in sector
#define INVALID_ADDRESS       (uint16_t) -1   //invalid address to read record
#define NO_ACTIVE_SECTOR      (uint16_t) -1
#define CORRUPTED_FILESYSTEM  (uint16_t) -1
#define MAX_PAYLOAD_SIZE    	242

#define DEVICE_SELECT					SPI1
#define CHIP_PIN							32

static const char LogTable256[256] =
{
	#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
	LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

typedef struct SectorFlags {
	uint16_t n;           // maximum number of records that can be stored in this sector
	uint8_t s;            // record size for this sector
	uint8_t unsent_flag;  // unsent flag to indicate if sector has unsent records
	uint8_t active_flag;  // active flag to indicate if this sector is active
} SectorFlags_t;

typedef struct SectorState {
	uint8_t unsent;
	uint8_t active;
} SectorState_t;

typedef struct RecordAddress {
	uint16_t sectorIndex;
	uint16_t recordIndex;
} RecordAddress_t;

class SerialFlashLayout : public SerialFlashChip
{
private:
	SectorFlags_t flags = {
		.n = 0xFFFF,
		.s = 0xFF,
		.unsent_flag = 0xFF,
		.active_flag = 0xFF
	};

	uint16_t k = 0;     //sector index
	uint16_t i = 0;     //record index

public:
	void init();
	void writeRecord(const void *record, uint8_t recordSize);
	uint16_t readRecord(RecordAddress_t recordAddress, void *buf);
	uint16_t readRecords(RecordAddress_t* recordAddresses, uint16_t length, void *buf);
	void markRecordSent(RecordAddress_t recordAddr);
	void markRecordsSent(RecordAddress_t* recordAddresses, uint16_t length);
	void markLatestWrittenRecordSent();
	uint16_t getLatestWrittenRecordSector();
	uint16_t getLatestWrittenRecordIndex(uint16_t sectorIndex);
	uint16_t getEarliestBacklogSector();
	uint16_t getEarliestBacklogIndex(uint16_t sectorIndex);
	uint16_t getLatestBacklogSector();
	uint16_t getLatestBacklogIndex(uint16_t sectorIndex);
	uint16_t getLatestBacklogIndex(uint16_t sectorIndex, uint16_t preceding);
	uint16_t retrieveLatestBacklogsAddresses(uint8_t payloadSize, RecordAddress_t *addresses);

	uint16_t getNextRecordIndexForSector(uint16_t sectorIndex);
	uint16_t getCurrentSectorIndex();
	uint16_t getNextRecordIndex();

protected:
	bool isBlankState(uint8_t flag);
	bool isActiveState(uint8_t flag);
	uint8_t activateState(uint8_t flag);
	uint8_t deactivateState(uint8_t flag);

	//Functions related to flags
	void readSectorFlags();
	void setFlags(uint16_t recordSize, uint8_t unsentState, uint8_t activeState);
	uint8_t getActiveFlag(uint16_t sectorIndex);
	SectorFlags_t retrieveSectorFlag(uint16_t sectorIndex);

	//Functions related to sectors
	void searchActiveSector();
	void sequentialSearchActiveSector();
	void deactivateCurrentSector();
	void activateSector(uint16_t sector, SectorFlags_t sectorFlags);
	void activateBlankSector(uint8_t recordSize);
	void activateNextSector(uint8_t recordSize);
	void reactivateCurrentSector(uint8_t recordSize);

	//Functions related to record index
	void incrementRecordIndex();
	void updateRecordPositionBit(uint16_t pos);

	//Functions related to unsent sector and backlog
	uint16_t nextLatestBacklogIndex(uint32_t addr, uint16_t preceding);
	void markSectorSent(uint16_t sectorIndex);

	//Basic functions
	uint16_t ctz(uint8_t x);
	uint16_t clz(uint8_t x);
	uint16_t countTrailingZeroes(uint8_t* positionBytes, uint32_t length);
	uint16_t countLeadingZeroes(uint8_t* positionBytes, uint32_t length);
	uint16_t ceiling(uint16_t numerator, uint16_t denominator);
};

#endif
