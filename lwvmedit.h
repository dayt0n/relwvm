#define A_LIST 1
#define A_EDIT 2

#define LwVM_NAME_LEN 0x24

bool ignore_errors = false;

bool human_readable = false; //I'm evil.

struct HFSPlusVolumeHeader {
	uint16_t signature;
	uint16_t version;
	uint32_t attributes;
	uint32_t lastMountedVersion;
	uint32_t journalInfoBlock;
	
	uint32_t createDate;
	uint32_t modifyDate;
	uint32_t backupDate;
	uint32_t checkedDate;
	
	uint32_t fileCount;
	uint32_t folderCount;
	
	uint32_t blockSize;
	uint32_t totalBlocks;
	uint32_t freeBlocks;
	
	uint32_t nextAllocation;
	uint32_t rsrcClumpSize;
	uint32_t dataClumpSize;
	uint32_t nextCatalogID;
	
	uint32_t writeCount;
	uint64_t encodingsBitmap;
	
	uint32_t finderInfo[6];
	uint64_t volumeUUID;
	/*
	HFSPlusForkData allocationFile;
	HFSPlusForkData extentsFile;
	HFSPlusForkData catalogFile;
	HFSPlusForkData attributesFile;
	HFSPlusForkData startupFile;*/
} __attribute__((packed));

typedef struct _LwVMPartitionRecord {
	uint64_t type[2];
	uint64_t guid[2];
	uint64_t begin;
	uint64_t end;
	uint64_t attribute; // 0 == unencrypted; 0x1000000000000 == encrypted
	char partitionName[0x48];
} __attribute__ ((packed)) LwVMPartitionRecord;

typedef struct _LwVM {
	uint64_t type[2];
	uint64_t guid[2];
	uint64_t mediaSize;
	uint32_t numPartitions;
	uint32_t crc32;
	uint8_t unkn[464];
	LwVMPartitionRecord partitions[12];
	uint16_t chunks[1024]; // chunks[0] should be 0xF000
} __attribute__ ((packed)) LwVM;

static const char LwVMType[] = { 0x6A, 0x90, 0x88, 0xCF, 0x8A, 0xFD, 0x63, 0x0A, 0xE3, 0x51, 0xE2, 0x48, 0x87, 0xE0, 0xB9, 0x8B };

static const char LwVMType_noCRC[] = { 0xB1, 0x89, 0xA5, 0x19, 0x4F, 0x59, 0x4B, 0x1D, 0xAD, 0x44, 0x1E, 0x12, 0x7A, 0xAF, 0x45, 0x39 };
