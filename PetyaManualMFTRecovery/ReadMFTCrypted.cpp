#include <iostream>
#include <fstream>
#include  <iomanip>
#include <codecvt>
#include <locale>
#include <string>
#include "util.h"
#include "salsa20.h"
#include <sstream>
#include <unordered_map>
// #include <boost/filesystem.hpp>

// "C:\Users\procrash\Documents\Visual Studio 2013\Projects\PetyaManualMFTRecovery\Release\PetyaManualMFTRecovery.exe" "G:\image.dd\image - Copy.dd" CsxxHCxxNDxxBmxx "G:\image.dd\UweMFT" "G:\image.dd\UweInfo.txt"

// Problem @Found OWSHLP10.CHM @3470786560
//                              

// @1449873408
// @1484521472 crash
using namespace std;

unsigned char bootsector[512];

/*
union MFTHeaderTag{
    uint8_t fileStr[4] = {'F', 'I', 'L', 'E' };
    uint8_t badStr[4] =  {'B', 'A', 'A', 'D' };
};
*/

/*
struct MFTEntry {
    uint8_t fileStr[4] = {'F', 'I', 'L', 'E' };
    uint16_t offsetToUpdateSequence;
    uint16_t nrOfUpdateEntries;
    
    
    uint16_t sequenceNumber; // Byte 16+17 = How many times record has been used
    uint16_t linkCount;      // Byte 18+19
    uint16_t firstAttributePosition; // No sure if this is a word or a byte
    uint16_t flags; // 00 02 entry in use 00 01 directory? 

    uint64_t fileReferenceToBaseFileEntry; // If all are 0's this is the first entry
    uint16_t nextAttributeId;
    
    uint16_t fixupArray; // always 8c06 last two bytes of every sector of the record  The real last couple of bytes in all the sectors are stored in the fixup array that follows, namely all zeroes. The idea behind the fixup array is to recognize clusters that have been partially written.

    

    
    
    
};
*/

// The standard header is always part of an MFT Block!!!
struct StandardInformationBlockType {
    uint64_t fileCreationTime;        // @00  The time values are given in 100 nanoseconds since January 1, 1601, UTC.
    uint64_t fileModificationTime;    // @08
    uint64_t mftModificationTime;     // @10
    
    uint64_t fileReadTime;            // @18
    uint32_t dosFilePermissions;      // @20
    uint32_t maximumNumberOfVersions; // @24
    uint32_t versionNumber;           // @28
    uint32_t classId;                 // @2c
    uint32_t ownerId;                 // @30
    uint32_t securityId;              // @34
    uint64_t quotaCharged;            // @38
    uint64_t updateSequenceNumber;    // @40
};

struct ResidentFileType {
    uint32_t attributeIdentifier;  // @00
    uint32_t lengthOfAttribute;    // @04
    uint8_t  residentFlag;         // @08
    uint8_t  lengthOfName;         // @09
    uint16_t offsetToName;         // @0A
    uint16_t flags;                // @0C
    
    uint16_t attributeIdentifier2; // @0E
    uint32_t sizeOfContent;        // @10
    uint16_t offsetToContent;      // @15
    
};


struct NonResidentFileType {
    uint32_t attributeIdentifier;                  // @00
    uint32_t lengthOfAttribute;                    // @04
    uint8_t  residentFlag;                         // @08
    uint8_t  lengthOfName;                         // @09
    uint16_t offsetToName;                         // @0A
    uint16_t flags;                                // @0C
    
    uint16_t attributeIdentifier2;                 // @0E
    uint64_t startingVirtualClusterOfTheRunlist;   // @10
    uint64_t endingVirtualClusterOfTheRunlist;     // @18
    
    uint16_t offsetToTheRunList;                   // @20
    uint16_t compressionUnitSize;                  // @22
    uint32_t ununsed;                              // @24
    
    uint64_t allocatedSizeOfTheAttributeContent;   // @28
    uint64_t actualSizeOfTheAttributeContent;      // @30
    uint64_t initializedSizeOfTheAttributeContent; // @38
};


struct FilenameType {
    uint64_t fileReferenceToParentDirectory;  // @00
    uint64_t fileCreationTime;                // @08
    uint64_t fileModificationTime;            // @10
    uint64_t mftModificationTime;             // @18
    uint64_t fileAccessTime;                  // @20
    uint64_t allocatedSizeOfFile;             // @28
    uint64_t realSizeOfFile;                  // @30   
    uint32_t flags;                           // @38
    uint32_t easAndReparseInformation;        // @3C
    uint32_t securityId;                      // @34 <-- Error here TODO: Check
    uint8_t filenameLengthInUnicodeCharacters;// 0x40
    uint8_t filenameNamespace;                // 0x41
    uint8_t filenameInUnicode;                // 0x42
    
};

// End Marker 0xFFFFFFFF


enum PartitionType {
	empty = 0x00,
	fat12 = 0x01,
	fat16_small = 0x04, // < 32MB	
	extendedPartition = 0x05,
	fat16_big = 0x06, //	FAT16 > 32 MiB
	ntfs = 0x07,
	fat32 = 0x0B,
	fat32_lba = 0x0C,
	fat16_lba_big = 0x0E,
	extended_lba = 0x0F,
	diagnostic = 0x12,
	windows_re = 0x27,
	dynamicDataStorage = 0x42,
	linuxSwap = 0x82,
	linuxNative = 0x83,
	linuxLvm = 0x8E,
	freeBsd = 0xA5,
	openBsd = 0xA6,
	netBsd = 0xA9,
	legacyEfiHeader = 0xEE,
	efiFilesystem = 0xEF
};


void printPartitionType(unsigned char type) {
	switch (type) {
	case empty: cout << "Empty Partition" << endl << endl; break;
	case fat12: cout << "Fat12 Partition" << endl; break;
	case fat16_small: cout << "Fat16 small Partition" << endl; break;
	case extendedPartition: cout << "Extended Partition" << endl; break;
	case fat16_big: cout << "Fat16 big Partition" << endl; break;
	case ntfs: cout << "NTFS Partition" << endl; break;
	case fat32: cout << "Fat32 Partition" << endl; break;
	case fat32_lba: cout << "Fat32 LBA Partition" << endl; break;
	case fat16_lba_big: cout << "Fat32 LBA big Partition" << endl; break;
	case extended_lba: cout << "Extended LBA Partition" << endl; break;
	case diagnostic: cout << "Diagnostic Partition" << endl; break;
	case windows_re: cout << "Windows RE Partition" << endl; break;
	case dynamicDataStorage: cout << "Dynamic Data Storage Partition" << endl; break;
	case linuxSwap: cout << "Linux Swap Partition" << endl; break;
	case linuxNative: cout << "Linux Native Partition" << endl; break;
	case linuxLvm: cout << "Linux LVM Partition" << endl; break;
	case freeBsd: cout << "Free BSD Partition" << endl; break;
	case openBsd: cout << "Open BSD Partition" << endl; break;
	case netBsd: cout << "Net BSD Partition" << endl; break;
	case legacyEfiHeader: cout << "Legacy EFI Partition" << endl; break;
	case efiFilesystem: cout << "EFI Filesystem Partition" << endl; break;

	default: cout << "Unknown Partition Type (" << hex << (unsigned int)type << dec << endl; break;

	}
}

bool checkBootsector(unsigned char* bootsector) {
	if (!(bootsector[0x1fe] == (unsigned char)0x55) ||
		!(bootsector[0x1ff] == (unsigned char)0xAA)) {
		cout << "No valid bootsector found" << endl;

		cout << "Bytes are ";

		cout << hex << (unsigned int)(bootsector[0x1fe]);
		cout << dec << " ";

		cout << hex << (unsigned int)(bootsector[0x1ff]) << endl;

		return 0;
	}
	return -1;
}



uint64_t getPartitionStartPosition(int partitionNumber, unsigned char*bootsector) {
	uint64_t result = 0;

	unsigned char* partition = bootsector + partitionNumber * 16 + 0x1BE;

	unsigned char lbaStartSector[4];
	lbaStartSector[0] = partition[0x08];
	lbaStartSector[1] = partition[0x09];
	lbaStartSector[2] = partition[0x0A];
	lbaStartSector[3] = partition[0x0B];

	uint64_t lbaStartSectorNum = (lbaStartSector[3] << 24) +
		(lbaStartSector[2] << 16) +
		(lbaStartSector[1] << 8) +
		(lbaStartSector[0]);


	result = lbaStartSectorNum * 512;

	return result;
}


uint64_t getPartitionEndPosition(int partitionNumber, unsigned char*bootsector) {
	uint64_t result = 0;

	unsigned char* partition = bootsector + partitionNumber * 16 + 0x1BE;

	unsigned char lbaStartSector[4];
	lbaStartSector[0] = partition[0x08];
	lbaStartSector[1] = partition[0x09];
	lbaStartSector[2] = partition[0x0A];
	lbaStartSector[3] = partition[0x0B];

	uint64_t lbaStartSectorNum = (lbaStartSector[3] << 24) +
		(lbaStartSector[2] << 16) +
		(lbaStartSector[1] << 8) +
		(lbaStartSector[0]);

    unsigned char  lbaSectors[4];
    lbaSectors[0] = partition[0xC];
    lbaSectors[1] = partition[0xD];
    lbaSectors[2] = partition[0xE];
    lbaSectors[3] = partition[0xF];
    
    uint64_t nrLbaSectors = (lbaSectors[3] << 24) +
                            (lbaSectors[2] << 16) +
                            (lbaSectors[1] << 8) +
                            (lbaSectors[0]);
    
	result = lbaStartSectorNum * 512+nrLbaSectors*512;

	return result;
}

uint64_t getPartitionBootsectorBackupPosition(int partitionNumber, unsigned char*bootsector)
{
    return getPartitionEndPosition(partitionNumber, bootsector)-512;
}


unsigned char getPartitionType(int partitionNumber, unsigned char*bootsector) {
	unsigned char* partition = bootsector + partitionNumber * 16 + 0x1BE;
	unsigned char partitionType = partition[0x04];
	return partitionType;
}


// 16 Bytes
void printPartition(unsigned char*bootsector, int partitionNumber) {
	unsigned char* partition = bootsector + partitionNumber * 16 + 0x1BE;

	cout << endl;
	cout << endl;
	cout << "Partition " << (partitionNumber + 1) << ":" << endl;
	if (*partition == 0x80) {
		cout << "Partition is bootable" << endl;
	}
	else {
		cout << "Partition is not bootable (" << hex << (unsigned int)(partition[0x00]) << dec << ")" << endl;
	}

	// Cylinder Head Sector information
	unsigned char firstSectorChs[3];
	firstSectorChs[0] = partition[0x01];
	firstSectorChs[1] = partition[0x02];
	firstSectorChs[2] = partition[0x03];

	unsigned int firstSectorCylinder = firstSectorChs[2] + (firstSectorChs[1] >> 6) * 256;
	unsigned int firstSector = firstSectorChs[1] & 0x3F; // first 6 bit;
	unsigned int firstSectorHead = firstSectorChs[0];

	unsigned char partitionType = getPartitionType(partitionNumber, bootsector);

	unsigned char lastSectorChs[3];
	lastSectorChs[0] = partition[0x05];
	lastSectorChs[1] = partition[0x06];
	lastSectorChs[2] = partition[0x07];

	unsigned int lastSectorCylinder = lastSectorChs[2] + (firstSectorChs[1] >> 6) * 256;
	unsigned int lastSector = lastSectorChs[1] & 0x3F; // first 6 bit;
	unsigned int lastSectorHead = lastSectorChs[0];


	unsigned char lbaStartSector[4];
	lbaStartSector[0] = partition[0x08];
	lbaStartSector[1] = partition[0x09];
	lbaStartSector[2] = partition[0x0A];
	lbaStartSector[3] = partition[0x0B];

    /*
	cout << "Hex Bytes of StartSectors are:" << endl;
	hexdump((char*)lbaStartSector, 4);
    */
    
	uint64_t lbaStartSectorNum = getPartitionStartPosition(partitionNumber, bootsector) / 512;

	

	uint64_t partitionStartPos = lbaStartSectorNum * 512;
	cout << "Partition start at file position " << partitionStartPos << endl;

	unsigned char numLbaSectors[4];
	numLbaSectors[0] = partition[0x0C];
	numLbaSectors[1] = partition[0x0D];
	numLbaSectors[2] = partition[0x0E];
	numLbaSectors[3] = partition[0x0F];

	uint64_t lbaSectors = (numLbaSectors[3] << 24) +
		(numLbaSectors[2] << 16) +
		(numLbaSectors[1] << 8) +
		(numLbaSectors[0]);

    cout << "Start Lba Sector: " << lbaStartSectorNum << " End Lba Sector: " << (lbaStartSectorNum+lbaSectors) << endl;  // in Blocks (512 bytes)

	cout << "Number of LBA Sectors " << lbaSectors << endl;
    

	printPartitionType(partitionType);
}



bool checkNTFSSector(unsigned char* sector, uint8_t *chiffreStream, uint64_t chiffreStreamSize, int64_t sectorSize=1024) {
	if (!(sector[0x00] == (unsigned char)0xEB) ||
		!(sector[0x01] == (unsigned char)0x52) ||

		!(sector[0x02] == (unsigned char)0x90) || // 
		!(sector[0x03] == (unsigned char)0x4E) || // N
		!(sector[0x04] == (unsigned char)0x54) || // T
		!(sector[0x05] == (unsigned char)0x46) || // F
		!(sector[0x06] == (unsigned char)0x53) || // S
		!(sector[0x07] == (unsigned char)0x20) || // SPACE
		!(sector[0x08] == (unsigned char)0x20) || // SPACE
		!(sector[0x09] == (unsigned char)0x20) || // SPACE
		!(sector[0x0A] == (unsigned char)0x20)    // SPACE

		) {
		// cout << "No valid NTFS Partition found" << endl;
		// cout << "Decrypting..." << endl;



		// cout << "Data is decrypred, looking for chiffre stream...." << endl;

		uint8_t chiffreToLookFor[14];

		// EB 52 90 4E 54 46 53 20 20 20 20 00 02 08
		// Start of NTFS Partition...
		chiffreToLookFor[0] = sector[0x00] ^ (unsigned char)0xEB;
		chiffreToLookFor[1] = sector[0x01] ^ (unsigned char)0x52;
		chiffreToLookFor[2] = sector[0x02] ^ (unsigned char)0x90;
		chiffreToLookFor[3] = sector[0x03] ^ (unsigned char)0x4E;
		chiffreToLookFor[4] = sector[0x04] ^ (unsigned char)0x54;
		chiffreToLookFor[5] = sector[0x05] ^ (unsigned char)0x46;
		chiffreToLookFor[6] = sector[0x06] ^ (unsigned char)0x53;
		chiffreToLookFor[7] = sector[0x07] ^ (unsigned char)0x20;
		chiffreToLookFor[8] = sector[0x08] ^ (unsigned char)0x20;
		chiffreToLookFor[9] = sector[0x09] ^ (unsigned char)0x20;
		chiffreToLookFor[10] = sector[0x0A] ^ (unsigned char)0x20;
		chiffreToLookFor[11] = sector[0x0B] ^ (unsigned char)0x00;
		chiffreToLookFor[12] = sector[0x0C] ^ (unsigned char)0x02;
		chiffreToLookFor[13] = sector[0x0D] ^ (unsigned char)0x08;


        /*
		cout << "Looking for ";
		hexdump((char*)chiffreToLookFor, 14);
		cout << endl;
		cout << "Encrypted Partition Data is ";
		hexdump((char*)sector, 14);
		cout << endl;
		*/

		bool found;
		
		uint64_t chiffrePos = 0;

		do {

			found = false;

			// NTFS Partition Start...
			if (chiffreStream[(chiffrePos+ 0)%chiffreStreamSize] == chiffreToLookFor[0] &&
				chiffreStream[(chiffrePos+ 1)%chiffreStreamSize] == chiffreToLookFor[1] &&
				chiffreStream[(chiffrePos+ 2)%chiffreStreamSize] == chiffreToLookFor[2] &&
				chiffreStream[(chiffrePos+ 3)%chiffreStreamSize] == chiffreToLookFor[3] &&
				chiffreStream[(chiffrePos+ 4)%chiffreStreamSize] == chiffreToLookFor[4] &&
				chiffreStream[(chiffrePos+ 5)%chiffreStreamSize] == chiffreToLookFor[5] &&
				chiffreStream[(chiffrePos+ 6)%chiffreStreamSize] == chiffreToLookFor[6] &&
				chiffreStream[(chiffrePos+ 7)%chiffreStreamSize] == chiffreToLookFor[7] &&
				chiffreStream[(chiffrePos+ 8)%chiffreStreamSize] == chiffreToLookFor[8] &&
				chiffreStream[(chiffrePos+ 9)%chiffreStreamSize] == chiffreToLookFor[9] &&
				chiffreStream[(chiffrePos+10)%chiffreStreamSize] == chiffreToLookFor[10] &&
				chiffreStream[(chiffrePos+11)%chiffreStreamSize] == chiffreToLookFor[11] &&
				chiffreStream[(chiffrePos+12)%chiffreStreamSize] == chiffreToLookFor[12] &&
				chiffreStream[(chiffrePos+13)%chiffreStreamSize] == chiffreToLookFor[13]
				)
			{
				found = true;
			}

			if (found == true) break;
            
			chiffrePos++;

		} while (!found);


		// cout << "Chiffre Found at position " << chiffrePos << " " << endl;
		
		/*
		// Could overflow by 5 bytes!
		
		uint8_t debug[5];
		
		for (uint64_t i = 0; i<5; i++) {
		  debug[i] = chiffreStream[(chiffrePos+i)%chiffreStreamSize];
		}
		
		hexdump((char*)debug, 5);
		cout << endl;
        */

		for (int i = 0; i<sectorSize; i++) { // 1024 because Petya encryptes always 2 sectors...
			sector[i] ^= chiffreStream[(chiffrePos+ i)%chiffreStreamSize];
		}

		return 1;


		cout << "Bytes are ";

		cout << hex << (unsigned int)(sector[0x00]);
		cout << hex << (unsigned int)(sector[0x01]);
		cout << hex << (unsigned int)(sector[0x02]);
		cout << hex << (unsigned int)(sector[0x03]);
		cout << hex << (unsigned int)(sector[0x04]);
		cout << hex << (unsigned int)(sector[0x05]);
		cout << hex << (unsigned int)(sector[0x06]);
		cout << hex << (unsigned int)(sector[0x07]);
		cout << hex << (unsigned int)(sector[0x08]);
		cout << hex << (unsigned int)(sector[0x09]);
		cout << hex << (unsigned int)(sector[0x0A]);
		cout << endl;

		return 0;
	}
	return -1;

}

unsigned int getPartitionClusterSizeInBytes(unsigned char* partitionStartSector) {
	unsigned int blocksize = partitionStartSector[0xC] * 256 + partitionStartSector[0xB];
	unsigned int clusterSizeInBytes = partitionStartSector[0xD] * blocksize; // should be 512
	return clusterSizeInBytes;
}



uint64_t getMFTLength(unsigned char* partitionStartSector, uint64_t partitionStartPosition) {

	uint64_t mftTotalSectors = (((uint64_t)partitionStartSector[0x22])) +
                               (((uint64_t)partitionStartSector[0x23]) << 8) +
                               (((uint64_t)partitionStartSector[0x24]) << 16) +
                               (((uint64_t)partitionStartSector[0x25]) << 24) +
                               (((uint64_t)partitionStartSector[0x26]) << 32) +
                               (((uint64_t)partitionStartSector[0x27]) << 40) +
                               (((uint64_t)partitionStartSector[0x28]) << 48) +
                               (((uint64_t)partitionStartSector[0x29]) << 56);

	return mftTotalSectors;
}


uint64_t getMFTStartPosition(unsigned char* partitionStartSector, uint64_t partitionStartPosition) {

	unsigned int clusterSizeInBytes = getPartitionClusterSizeInBytes(partitionStartSector);

	uint64_t dollarMFTPositionCluster = partitionStartSector[0x30] +
                            (((uint64_t)partitionStartSector[0x31]) << 8) +
                            (((uint64_t)partitionStartSector[0x32]) << 16) +
                            (((uint64_t)partitionStartSector[0x33]) << 24) +
                            (((uint64_t)partitionStartSector[0x34]) << 32) +
                            (((uint64_t)partitionStartSector[0x35]) << 40) +
                            (((uint64_t)partitionStartSector[0x36]) << 48) +
                            (((uint64_t)partitionStartSector[0x37]) << 56);


	uint64_t dollarMFTStartBytePosition = dollarMFTPositionCluster*clusterSizeInBytes;


	dollarMFTStartBytePosition += partitionStartPosition;

	return dollarMFTStartBytePosition;
}

uint64_t getMFTCopyStartPosition(unsigned char* partitionStartSector, uint64_t partitionStartPosition) {

	unsigned int clusterSizeInBytes = getPartitionClusterSizeInBytes(partitionStartSector);

	uint64_t dollarMFTPositionCluster = partitionStartSector[0x30+8] +
                            (((uint64_t)partitionStartSector[0x31+8]) << 8) +
                            (((uint64_t)partitionStartSector[0x32+8]) << 16) +
                            (((uint64_t)partitionStartSector[0x33+8]) << 24) +
                            (((uint64_t)partitionStartSector[0x34+8]) << 32) +
                            (((uint64_t)partitionStartSector[0x35+8]) << 40) +
                            (((uint64_t)partitionStartSector[0x36+8]) << 48) +
                            (((uint64_t)partitionStartSector[0x37+8]) << 56);


	uint64_t dollarMFTStartBytePosition = dollarMFTPositionCluster*clusterSizeInBytes;


	dollarMFTStartBytePosition += partitionStartPosition;

	return dollarMFTStartBytePosition;
}

/*
void printNTFSInformation(unsigned char* sector, uint64_t partitionStartPosition, int partitionNr) {


	cout << "NTFS  Cluster Size in Bytes " << getPartitionClusterSizeInBytes(sector) << endl;

	uint64_t dollarMFTPosition = getMFTStartPosition(sector, partitionStartPosition);

	cout << "$MFT starts at file position " << dollarMFTPosition << endl;
	
	cout <<  "Total sectors of MFT " << getMFTLength(sector, partitionStartPosition) << endl;
}*/


bool createDirectory(string directoryName) {
        /*
    	boost::filesystem::path dir(directoryName);
    	
        if(boost::filesystem::create_directory(dir)) {
            return true;
        } else {
            return false;
        }*/
        return false;
}

void extractDataNonResident(uint64_t partitionStartPosition, uint64_t dataAttributePosition, uint8_t* data, string ddFilename, string dataFilename) {
    
    cout << endl << endl;
    cout << "@"<< dataAttributePosition << endl;
    hexdump((char*)data, 150);
    cout << endl << endl;
    
    uint32_t typeId;
    uint32_t length;
    uint8_t formCode;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;  // 0x00FF = compressed, 0x8000 Sparse, 0x4000 Encrypted
    uint16_t attributeId;

    uint64_t startVirtualClusterNumber;
    uint64_t endVirtualClusterNumber;
    uint16_t runListOffset;
    uint16_t compressionUnitSize;
    uint32_t _4zeros;
    uint64_t sizeOfAttributeContent;
    uint64_t sizeOnDiskOfAttributeContent;
    uint64_t initializedSizeOfAttributeContent;
    // Data runlists start here...        
    
    typeId = (((uint64_t)data[0]))+
             (((uint64_t)data[1])<<8)+
             (((uint64_t)data[2])<<16)+
             (((uint64_t)data[3])<<24);

    length = (((uint64_t)data[4]))+
             (((uint64_t)data[5])<<8)+
             (((uint64_t)data[6])<<16)+
             (((uint64_t)data[7])<<24);
    
    formCode = data[8];
    
    nameLength = data[9];

    nameOffset = (((uint64_t)data[10]))+
                 (((uint64_t)data[11])<<8);
                 
    flags = (((uint64_t)data[12]))+
            (((uint64_t)data[13])<<8);
            
    
    attributeId = (((uint64_t)data[14]))+
                  (((uint64_t)data[15])<<8);
                  


    startVirtualClusterNumber = (((uint64_t)data[16]))+
                                (((uint64_t)data[17])<<8)+
                                (((uint64_t)data[18])<<16)+
                                (((uint64_t)data[19])<<24)+
                                (((uint64_t)data[20])<<32)+
                                (((uint64_t)data[21])<<40)+
                                (((uint64_t)data[22])<<48)+
                                (((uint64_t)data[23])<<56);

    endVirtualClusterNumber = (((uint64_t)data[16]))+
                              (((uint64_t)data[17])<<8)+
                              (((uint64_t)data[18])<<16)+
                              (((uint64_t)data[19])<<24)+
                              (((uint64_t)data[20])<<32)+
                              (((uint64_t)data[21])<<40)+
                              (((uint64_t)data[22])<<48)+
                              (((uint64_t)data[23])<<56);
                              
    runListOffset = (((uint64_t)data[24]))+
                     (((uint64_t)data[25])<<8);

    runListOffset = (((uint64_t)data[32]))+
                    (((uint64_t)data[33])<<8);
                    
    compressionUnitSize = (((uint64_t)data[26]))+
                          (((uint64_t)data[27])<<8);               
                    
    _4zeros = (((uint64_t)data[28]))+
              (((uint64_t)data[29])<<8)+
              (((uint64_t)data[30])<<16)+
              (((uint64_t)data[31])<<24);
              
    
    sizeOfAttributeContent = (((uint64_t)data[32]))+
                             (((uint64_t)data[33])<<8)+
                             (((uint64_t)data[34])<<16)+
                             (((uint64_t)data[35])<<24)+
                             (((uint64_t)data[36])<<32)+
                             (((uint64_t)data[37])<<40)+
                             (((uint64_t)data[38])<<48)+
                             (((uint64_t)data[39])<<56);
                            
    sizeOnDiskOfAttributeContent = (((uint64_t)data[40]))+
                                   (((uint64_t)data[41])<<8)+
                                   (((uint64_t)data[42])<<16)+
                                   (((uint64_t)data[43])<<24)+
                                   (((uint64_t)data[44])<<32)+
                                   (((uint64_t)data[45])<<40)+
                                   (((uint64_t)data[46])<<48)+
                                   (((uint64_t)data[47])<<56);
    
    initializedSizeOfAttributeContent = (((uint64_t)data[48]))+
                                        (((uint64_t)data[49])<<8)+
                                        (((uint64_t)data[50])<<16)+
                                        (((uint64_t)data[51])<<24)+
                                        (((uint64_t)data[52])<<32)+
                                        (((uint64_t)data[53])<<40)+
                                        (((uint64_t)data[54])<<48)+
                                        (((uint64_t)data[55])<<56);
                                
    int pos = 56;
//      cout << "Runlistoffset is " << hex << runListOffset << endl;
    pos = runListOffset;
    cout << dec;
    
    ifstream is;
    ofstream os;
    
    is.open(ddFilename);
    
    std::ostringstream filenameStream;    
    filenameStream << "data/nonResident/_" << dataAttributePosition << "_" << dataFilename;
    
    os.open(filenameStream.str());
    
    uint8_t* fileData;
    uint64_t clusterSize = 4096; // HardCoded !!!
 
    // TODO: Check if we overrun MFT Entry Size
    uint64_t bytesParsed = 0;
    
    // cout << endl << "Saving file @" << dec << dataAttributePosition << endl;

    cout << "Flags: 0x1 means Record is in use, 0x02 means directory " << hex << flags << endl;    
    uint64_t prevClusterNr = 0;
    bool firstTime = true;
    
    
    
    while ((data[pos]!=0)) { 

        cout << "First Byte is "<<hex << (uint32_t)data[pos] << endl;
        
        uint8_t firstByteOfRunlist = data[pos];
        uint8_t nrBytesWhichDescribeClusterNr = firstByteOfRunlist >> 4;
        uint8_t nrBytesConsecutiveClustersInRunEntry = firstByteOfRunlist & 0xF;
        
        uint64_t posClusterNr = pos+nrBytesConsecutiveClustersInRunEntry+1;
            
        uint64_t clusterNr = 0;
        for (int i=0; i<nrBytesWhichDescribeClusterNr; i++) {
            clusterNr = clusterNr << 8;
            clusterNr +=  data[posClusterNr+nrBytesWhichDescribeClusterNr-i-1];                                    
        }
        
        
        cout << "Nr of bytes which describe Cluster Nr " << dec << (uint16_t)nrBytesWhichDescribeClusterNr << endl;
        
        uint64_t tmp = 1;
        for (int i=0; i<nrBytesWhichDescribeClusterNr;i++) {
            tmp*=256;
        } 
        tmp = tmp >> 1;       
        
        int64_t signedClusterNr = clusterNr & tmp; // tmpClusterNr is now just the positive values of clusterNr
        
        // Now add highest bit as negative value
        uint64_t highestBitValue = 1 << (nrBytesWhichDescribeClusterNr*8-1);
        
        signedClusterNr = clusterNr - (clusterNr & highestBitValue);
        
        cout << endl << "Cluster Nr is " << hex << clusterNr << endl;
        cout << endl << "Cluster Nr signed is " << hex << signedClusterNr << endl;
                             
        uint64_t posNrCluster = pos;
        uint64_t nrOfConsecutiveClusters = 0; 
        for (int i=0; i<nrBytesConsecutiveClustersInRunEntry; i++) {
            nrOfConsecutiveClusters = nrOfConsecutiveClusters << 8;
            nrOfConsecutiveClusters +=  data[posNrCluster+nrBytesConsecutiveClustersInRunEntry-i];                                    
        }
        cout << "Nr of Clusters are " << hex << nrOfConsecutiveClusters<< endl;
            
        uint64_t lengthOfChainBlock = nrOfConsecutiveClusters*clusterSize;
        if (lengthOfChainBlock>(uint64_t)8*(uint64_t)1024*(uint64_t)1024*(uint64_t)1024) {
            // Something is wrong here, way too much memory allocation
            // TODO: Find out why this needed to be checked here...
            cout << "Overrun in memory allocation" << endl;
            break;
        }
        
        fileData = (uint8_t*) malloc(lengthOfChainBlock);
                
        if (firstTime)
            prevClusterNr = clusterNr; 
        else         
            prevClusterNr += signedClusterNr;
                                       
        uint64_t posClusterToDecrypt = prevClusterNr*clusterSize+partitionStartPosition;
        
        is.seekg(posClusterToDecrypt, ios::beg);
        is.read((char*)fileData, lengthOfChainBlock);
        os.write((char*)fileData, lengthOfChainBlock);
        
        free(fileData);
            
        pos+=nrBytesWhichDescribeClusterNr+nrBytesConsecutiveClustersInRunEntry+1;
        
        bytesParsed+=nrBytesWhichDescribeClusterNr+nrBytesConsecutiveClustersInRunEntry;
        
        if (bytesParsed>1024) { // Definitely over the MFT Size! break!!!!
            break;
        }
    }

    is.close();
    os.close();
    
}


void extractDataResident(uint8_t* data, string filename, uint64_t dataAttributePosition) {
    uint32_t typeId;
    uint32_t length;
    uint8_t formCode;
    uint8_t nameLength;
    uint16_t nameOffset;
    uint16_t flags;  // 0x00FF = compressed, 0x8000 Sparse, 0x4000 Encrypted
    uint16_t attributeId;
    uint32_t contentLength;
    uint16_t contentOffset;
    uint16_t unused;
    
    
    typeId = (data[0])+
             (data[1]<<8)+
             (data[2]<<16)+
             (data[3]<<24);

    length = (data[4])+
             (data[5]<<8)+
             (data[6]<<16)+
             (data[7]<<24);
    
    formCode = data[8];
    
    nameLength = data[9];

    nameOffset = (data[10])+
                 (data[11]<<8);
                 
    flags = (data[12])+
            (data[13]<<8);
            
    
    attributeId = (data[14])+
                  (data[15]<<8);
                  
    contentLength = (data[16])+
                    (data[17]<<8)+
                    (data[18]<<16)+
                    (data[19]<<24);
                    
    contentOffset = (data[20])+
                    (data[21]<<8);
                    
                    
    uint8_t* fileData = (uint8_t*) malloc(contentLength);

    memcpy(fileData, data+ contentOffset, contentLength);
    
    ofstream os;
    
    std::ostringstream filenameStream;
    
    filenameStream << "data/resident/_" << dataAttributePosition << "_" << filename;
    
    os.open(filenameStream.str());
    os.write((char*)fileData, contentLength);
    os.close();
         
    free(fileData);
}



bool decryptMFTBlockCalculated(unsigned char* fileBlock, uint8_t nonce[8], uint8_t key[16], uint64_t currentDataPosition,  bool* decryptionSuccessful) {


    // Check if we need to decrypt...
    if (!(fileBlock[0x00] == (unsigned char)0x46) || // F
       !(fileBlock[0x01] == (unsigned char)0x49) ||     // I
       !(fileBlock[0x02] == (unsigned char)0x4c) ||     // L
       !(fileBlock[0x03] == (unsigned char)0x45))       // E
    {
        uint8_t buf[1024];
        memcpy((char*)buf, (char*)fileBlock, 1024);
                
        s20_crypt_sector(key, nonce, buf, 1024, currentDataPosition/512);
        
        
        if (!(buf[0x00] == (unsigned char)0x46) || // F
            !(buf[0x01] == (unsigned char)0x49) || // I
            !(buf[0x02] == (unsigned char)0x4c) || // L
            !(buf[0x03] == (unsigned char)0x45))    // E
        {

            return false; 
        }
        
        memcpy((char*)fileBlock, (char*)buf, 1024);
          
        
    } 
    return true;    
}


bool decryptMFTBlock(unsigned char* fileBlock, uint8_t *chiffreStream, uint64_t chiffreStreamSize, bool* decryptionSuccessful, uint64_t* lastFoundChiffrePosition=NULL) {
	// 46 49 4C 45 30 FILE0
	if (!(fileBlock[0x00] == (unsigned char)0x46) || // F
		!(fileBlock[0x01] == (unsigned char)0x49) || // I
		!(fileBlock[0x02] == (unsigned char)0x4c) || // L
		!(fileBlock[0x03] == (unsigned char)0x45)    // E
		/*!(fileBlock[0x04] == (unsigned char)0x30) // 0 */

		) {
		
		/*
		cout << "No valid File Block found" << endl;

		cout << "Bytes are ";

		cout << hex << (unsigned int)(fileBlock[0x00]);
		cout << hex << (unsigned int)(fileBlock[0x01]);
		cout << hex << (unsigned int)(fileBlock[0x02]);
		cout << hex << (unsigned int)(fileBlock[0x03]);
		cout << hex << (unsigned int)(fileBlock[0x04]) << endl;
        */

		// Searching for Chiffre Stream to decode data....

		// cout << "Data is decrypred, looking for chiffre stream...." << endl;

		uint8_t chiffreToLookFor[39];

		chiffreToLookFor[0] = fileBlock[0x00] ^ (unsigned char)0x46;  // F
		chiffreToLookFor[1] = fileBlock[0x01] ^ (unsigned char)0x49;  // I
		chiffreToLookFor[2] = fileBlock[0x02] ^ (unsigned char)0x4c;  // L
		chiffreToLookFor[3] = fileBlock[0x03] ^ (unsigned char)0x45;  // E
		//chiffreToLookFor[4] = fileBlock[0x04] ^ (unsigned char)0x30;

		// 10's at File Block Type...
		chiffreToLookFor[5] = fileBlock[0x38] ^ (unsigned char)0x10; // Standard Information Attribute Marker...       
		chiffreToLookFor[6] = fileBlock[0x39] ^ (unsigned char)0x00;
		chiffreToLookFor[7] = fileBlock[0x3A] ^ (unsigned char)0x00;
		chiffreToLookFor[8] = fileBlock[0x3B] ^ (unsigned char)0x00;



		// End of MFT...
		chiffreToLookFor[9] = fileBlock[0x38] ^ (unsigned char)0xFF; // End of MFT Marker...       
		chiffreToLookFor[10] = fileBlock[0x39] ^ (unsigned char)0xFF;
		chiffreToLookFor[11] = fileBlock[0x3A] ^ (unsigned char)0xFF;
		chiffreToLookFor[12] = fileBlock[0x3B] ^ (unsigned char)0xFF;

		// 20's at File Block Type...
		chiffreToLookFor[13] = fileBlock[0x38] ^ (unsigned char)0x20; // $Attribute List...       

		// 30's at File Block Type...
		chiffreToLookFor[14] = fileBlock[0x38] ^ (unsigned char)0x30; // $Filename...       

		// 40's at File Block Type...
		chiffreToLookFor[15] = fileBlock[0x38] ^ (unsigned char)0x40; // $Object ID...       

		// 50's at File Block Type...
		chiffreToLookFor[16] = fileBlock[0x38] ^ (unsigned char)0x50; // $Security Descriptor...       

		// 60's at File Block Type...
		chiffreToLookFor[17] = fileBlock[0x38] ^ (unsigned char)0x60; // $Volume Name...       

		// 70's at File Block Type...
		chiffreToLookFor[18] = fileBlock[0x38] ^ (unsigned char)0x70; // $Volume Information...       

		// 80's at File Block Type...
		chiffreToLookFor[19] = fileBlock[0x38] ^ (unsigned char)0x80; // $Data...       

		// 90's at File Block Type...
		chiffreToLookFor[20] = fileBlock[0x38] ^ (unsigned char)0x90; // $Index_Root...       

		// A0's at File Block Type...
		chiffreToLookFor[21] = fileBlock[0x38] ^ (unsigned char)0xA0; // $Index Allocation...       

		// B0's at File Block Type...
		chiffreToLookFor[22] = fileBlock[0x38] ^ (unsigned char)0xB0; // $Bitmap...       

		// C0's at File Block Type...
		chiffreToLookFor[23] = fileBlock[0x38] ^ (unsigned char)0xC0; // $Reparse_Point...       

		// D0's at File Block Type...
		chiffreToLookFor[24] = fileBlock[0x38] ^ (unsigned char)0xD0; // $Ea_Information...       

		// E0's at File Block Type...
		chiffreToLookFor[25] = fileBlock[0x38] ^ (unsigned char)0xE0; // $EA...       

		// F0's at File Block Type...
		chiffreToLookFor[26] = fileBlock[0x38] ^ (unsigned char)0xF0; // $Property_Set...       

		// 00 01 00 00's at File Block Type...
		chiffreToLookFor[27] = fileBlock[0x38] ^ (unsigned char)0x00; // $Logged_Utility_Stream...       
		chiffreToLookFor[28] = fileBlock[0x39] ^ (unsigned char)0x01;
		chiffreToLookFor[29] = fileBlock[0x3A] ^ (unsigned char)0x00;
		chiffreToLookFor[30] = fileBlock[0x3B] ^ (unsigned char)0x00;

		// 00 10 00 00 's at File Block Type...
		chiffreToLookFor[31] = fileBlock[0x38] ^ (unsigned char)0x00; // First User Defined Attribute...       
		chiffreToLookFor[32] = fileBlock[0x39] ^ (unsigned char)0x10;
		chiffreToLookFor[33] = fileBlock[0x3A] ^ (unsigned char)0x00;
		chiffreToLookFor[34] = fileBlock[0x3B] ^ (unsigned char)0x00;


        // TODO: Implement this one too! Check if the trojan implements it
		chiffreToLookFor[35] = fileBlock[0x00] ^ (unsigned char)0x42;  // B
		chiffreToLookFor[36] = fileBlock[0x01] ^ (unsigned char)0x41;  // A
		chiffreToLookFor[37] = fileBlock[0x02] ^ (unsigned char)0x41;  // A
		chiffreToLookFor[38] = fileBlock[0x03] ^ (unsigned char)0x44;  // D
		
        /*
		cout << "Looking for ";
		hexdump((char*)chiffreToLookFor, 5);
		cout << endl;
		cout << "File Block Data is ";
		hexdump((char*)fileBlock, 5);
		cout << endl;
        */
        
		bool found;
		
		uint64_t chiffrePos = 0;

		uint64_t nrNotFound = 0;
		do {

			found = false;

			if (chiffreStream[(chiffrePos+0)%chiffreStreamSize] == chiffreToLookFor[0] &&
				chiffreStream[(chiffrePos+1)%chiffreStreamSize] == chiffreToLookFor[1] &&
				chiffreStream[(chiffrePos+2)%chiffreStreamSize] == chiffreToLookFor[2] &&
				chiffreStream[(chiffrePos+3)%chiffreStreamSize] == chiffreToLookFor[3] /*&&
				chiffreStream[(chiffrePos+4)%chiffreStreamSize] == chiffreToLookFor[4]*/)
			{

				if ((((
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[5] ||    // Standard Information Attribute Marker...   
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[13] ||   // $Attribute List...   
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[14] ||   // $Filename...
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[15] ||   // $Object ID...
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[16] ||   // $Security Descriptor...  
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[17] ||   // $Volume Name...      
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[18] ||   // $Volume Information...   
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[19] ||   // $Data...     
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[20] ||   // $Index_Root...   
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[21] ||   // $Index Allocation...   
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[22] ||   // $Bitmap...
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[23] ||   // $Reparse_Point...
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[24] ||   // $Ea_Information...       
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[25] ||   // $EA...
					  chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[26]	  // $Property_Set...  				
					)  &&   // Standard Information Marker
					chiffreStream[(chiffrePos+0x39)%chiffreStreamSize] == chiffreToLookFor[6] &&
					chiffreStream[(chiffrePos+0x3A)%chiffreStreamSize] == chiffreToLookFor[7] &&
					chiffreStream[(chiffrePos+0x3B)%chiffreStreamSize] == chiffreToLookFor[8])
					) || (
					chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[27] &&    // $Logged_Utility_Stream...   
					chiffreStream[(chiffrePos+0x39)%chiffreStreamSize] == chiffreToLookFor[28] &&
					chiffreStream[(chiffrePos+0x3A)%chiffreStreamSize] == chiffreToLookFor[29] &&
					chiffreStream[(chiffrePos+0x3B)%chiffreStreamSize] == chiffreToLookFor[30])
					|| (
					chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[31] &&    // First User Defined Attribute...
					chiffreStream[(chiffrePos+0x39)%chiffreStreamSize] == chiffreToLookFor[32] &&
					chiffreStream[(chiffrePos+0x3A)%chiffreStreamSize] == chiffreToLookFor[33] &&
					chiffreStream[(chiffrePos+0x3B)%chiffreStreamSize] == chiffreToLookFor[34])

					) {
					found = true;
				}
/*				else if (chiffreStream[(chiffrePos+0x38)%chiffreStreamSize] == chiffreToLookFor[9] &&   // End of MFT Marker
					     chiffreStream[(chiffrePos+0x39)%chiffreStreamSize] == chiffreToLookFor[10] &&
					     chiffreStream[(chiffrePos+0x3A)%chiffreStreamSize] == chiffreToLookFor[11] &&
					     chiffreStream[(chiffrePos+0x3B)%chiffreStreamSize] == chiffreToLookFor[12])
				{
				    cout << "MFT end found " << endl;
					found = false;
					return false;
				}*/
			}

            if (lastFoundChiffrePosition!=NULL) *lastFoundChiffrePosition=chiffrePos;
			if (found == true) break;

			chiffrePos+=2; // usually 1 
			nrNotFound++;
			if (nrNotFound > chiffreStreamSize) break;

		} while (!found);

		*decryptionSuccessful = found;
		if (found){

			for (uint64_t i = 0; i < 1024; i++) {
				fileBlock[i] ^= chiffreStream[(chiffrePos+i)%chiffreStreamSize];
			}

			return true;
		}
		else return false;


	}
	else return true;

//	return -1;
}


bool decryptMFTBlockCalculatedAndTrialAndError(unsigned char* fileBlock, uint8_t nonce[8], uint8_t key[16], uint64_t currentDataPosition,  bool* decryptionSuccessful,
          uint8_t *chiffreStream, uint64_t chiffreStreamSize, uint64_t* lastFoundChiffrePosition=NULL)
{
/*
    if (!decryptMFTBlock(fileBlock, chiffreStream, chiffreStreamSize, decryptionSuccessful, lastFoundChiffrePosition)) {
        return decryptMFTBlockCalculated(fileBlock, nonce, key, currentDataPosition,  decryptionSuccessful);
    } else {
    *decryptionSuccessful = true;
    return true;
    }
*/
    if (!decryptMFTBlockCalculated(fileBlock, nonce, key, currentDataPosition,  decryptionSuccessful)) {
        return decryptMFTBlock(fileBlock, chiffreStream, chiffreStreamSize, decryptionSuccessful, lastFoundChiffrePosition);
    } else {
    *decryptionSuccessful = true;
    return true;
    }

}


void printHexCodes(unsigned char* position, uint64_t len) {
	cout << hex;



	for (uint64_t i = 0; i<len; i++) {
		cout << setfill('0') << setw(2) << hex << (unsigned int)position[i] << " ";
	}


	cout << dec << endl;

}

void debugFunction() {
    cout << "Debugging only" << endl;
}


unordered_map<uint64_t, bool> decryptedSectors;
unordered_map<uint64_t, bool> decryptedDataBlocks;




uint64_t decryptMFT(uint64_t firstFilePosition, ifstream *is, fstream *dstFs, ofstream *os, ofstream *info,  uint8_t* patternStream, uint64_t chiffreStreamSize, uint64_t partitionStartPosition, uint8_t key[16], uint8_t nonce[8], bool recurse, string destinationDDFilename);


bool parseMFTEntries(uint8_t mftEntry[1024], ifstream* is, fstream* dstFs, ofstream *os, ofstream* info, uint64_t partitionStartPosition, uint8_t key[16], uint8_t nonce[8], uint8_t *patternStream, uint64_t chiffreStreamSize, string destinationDDFilename, uint64_t* nrOfBlocksDecrypted, uint64_t debugPosition) {
    uint16_t firstAttributeOffset = mftEntry[0x14] + mftEntry[0x15] * 256;
    uint64_t offsetToCurrentAttribute = firstAttributeOffset;

    string currentFilename;
    

    bool mftEntryEndFound = false;
     
     
   
    uint64_t attributeSize = 0;
    uint16_t usedMFTEntrySize = mftEntry[25]*256+mftEntry[24];       // @922D
     
    uint64_t nrOfFilesFoundInMFTEntry = 0;        

    do {
                        
        uint64_t attributeType = (((uint64_t)mftEntry[offsetToCurrentAttribute + 0])) +
                                 (((uint64_t)mftEntry[offsetToCurrentAttribute + 1]) << 8) +
                                 (((uint64_t)mftEntry[offsetToCurrentAttribute + 2]) << 16) +
                                 (((uint64_t)mftEntry[offsetToCurrentAttribute + 3]) << 24);
        

        attributeSize = (((uint64_t)mftEntry[offsetToCurrentAttribute + 4])) +
            (((uint64_t)mftEntry[offsetToCurrentAttribute + 5]) << 8) +
            (((uint64_t)mftEntry[offsetToCurrentAttribute + 6]) << 16) +
            (((uint64_t)mftEntry[offsetToCurrentAttribute + 7]) << 24);


            
        switch (attributeType) {
			case 0x90:  // Root Index Found...
						*info << "Index Root found @" << debugPosition << endl;

						break;
            case 0x30: // Filename Block
            {    
                uint64_t filenameLengthInCharacters = mftEntry[offsetToCurrentAttribute + 0x58]; // Should be 0x40 and 0x42, Find out why not!
                unsigned char *filename = (unsigned char*)malloc(filenameLengthInCharacters * 2);


                 uint32_t flags = (((uint64_t)mftEntry[offsetToCurrentAttribute + 0x50])) +
                                  (((uint64_t)mftEntry[offsetToCurrentAttribute + 0x51]) << 8) +
                                  (((uint64_t)mftEntry[offsetToCurrentAttribute + 0x52]) << 16) +
                                  (((uint64_t)mftEntry[offsetToCurrentAttribute + 0x53]) << 24);
                                  
                memcpy(filename, mftEntry + offsetToCurrentAttribute + 0x5A, filenameLengthInCharacters * 2); // UTF16

                nrOfFilesFoundInMFTEntry++;
                
//                cout << " @" << currentPosition<<"\t\t";
//                cout << "Found ";
                std::ostringstream filenameStream;
                    
                for (int i = 0; i<filenameLengthInCharacters * 2; i += 2) {
                    if (filename[i]>32 && filename[i]<123) {
                        // cout << (unsigned char) filename[i];
                        filenameStream << (unsigned char) filename[i];
                    }
                }
                    
                
                
//                *info << " @" << currentPosition;
//                *info << "\t" << filenameStream.str();

                if (flags == 0x10000000) {
//                    cout << " is a Directory ";
//                    *info << " is a Directory ";
                }
                
                currentFilename = string(filenameStream.str());
                cout << "\r" << currentFilename; 
                for (int i=0; i<80; i++) cout << " ";
               
				*info << "@" << debugPosition << " Found " << currentFilename << endl;
				info->flush();
//                info->flush();
/*                
                for (int i=0; i<80; i++) cout << " ";
                cout << "\r";
                cout.flush();                       
*/ 
                free(filename);
                break;
                
            }
            
            case 0x80:  // Data Block
                // Resident or Non Resident file?
            {
//                cout << "Data Block... at (" << (offsetToCurrentAttribute+0+currentPosition) << ")" << endl;                        
//                *info << "\tFound Data Block @" << (currentPosition+offsetToCurrentAttribute);

                if (currentFilename.c_str()[0]=='$') {
//                    *info << "\t Not decrpyting fileblock as it starts with $ ";
                    break; // Do not decrypt File Data of files which start with a Dollar sign, just the MFT Entry
                }

                
                unsigned char isNotResident = mftEntry[offsetToCurrentAttribute + 8]; // 0 is resident 1 is not resident
                if (isNotResident) {
                                                                             
                    uint16_t flags = (mftEntry[offsetToCurrentAttribute +0x16])+
                                     (mftEntry[offsetToCurrentAttribute +0x17]<<8);
                    
                    uint16_t offsetToRunlists = (mftEntry[offsetToCurrentAttribute +0x20])+
                                                (mftEntry[offsetToCurrentAttribute +0x21]<<8);

                    if ((offsetToCurrentAttribute+attributeSize-4)<usedMFTEntrySize) {
                        uint8_t firstByteOfRunlist = mftEntry[offsetToCurrentAttribute + offsetToRunlists];
                        uint8_t nrBytesWhichDescribeClusterNr = firstByteOfRunlist >> 4;
                        uint8_t nrBytesConsecutiveClustersInRunEntry = firstByteOfRunlist & 0xF;
                        
                         if (nrBytesWhichDescribeClusterNr<4) {
                            uint64_t posClusterNr = offsetToCurrentAttribute + offsetToRunlists+nrBytesConsecutiveClustersInRunEntry+1;
                            
                            uint64_t clusterNr = 0;
                            for (int i=0; i<nrBytesWhichDescribeClusterNr; i++) {
                                clusterNr = clusterNr << 8;
                                clusterNr +=  mftEntry[posClusterNr+nrBytesWhichDescribeClusterNr-i-1];                                    
                            }
                            *info << "\tCluster Nr is " << hex << clusterNr;
                                                 
                            uint64_t posNrCluster = offsetToCurrentAttribute + offsetToRunlists +1;
                            uint64_t nrOfConsecutiveClusters = 0; 
                            for (int i=0; i<nrBytesConsecutiveClustersInRunEntry; i++) {
                                nrOfConsecutiveClusters = nrOfConsecutiveClusters << 8;
                                nrOfConsecutiveClusters +=  mftEntry[posNrCluster-i];                                    
                            }
                            *info << "\tNr of Clusters are " << hex << nrOfConsecutiveClusters;
                            *info << dec;
                            
                            uint64_t posClusterToDecrypt = clusterNr*4096+partitionStartPosition;
                           
                            uint64_t currentFilePositionSave = is->tellg();
                            uint64_t currentFilePositionSaveDstFsRead = dstFs->tellg();
                            uint64_t currentFilePositionSaveDstFsWrite = dstFs->tellp();
                            
                            uint8_t fileChunk[1024];
                            is->seekg(posClusterToDecrypt, ios::beg);
                            dstFs->seekg(posClusterToDecrypt, ios::beg);
                            dstFs->seekp(posClusterToDecrypt, ios::beg);

                            is->read((char*)fileChunk, 1024);

                            // Try to decrypt this entry as well as an MFT Entry
                            
                            bool dataBlockDecryptionSuccessful = false;
                            uint64_t lastFoundChiffrePositionDataBlock = 0;
                            
                            // if (nrOfConsecutiveClusters>1) // Only encrypt if number of consecutive clusters is bigger than 1
                            //{
                                uint64_t nrBlocksFound = 0;
                                if (decryptedSectors.find(posClusterToDecrypt) == decryptedSectors.end()) {
                                    nrBlocksFound = decryptMFT(posClusterToDecrypt, is, dstFs, os, info,  patternStream, chiffreStreamSize, partitionStartPosition, key, nonce, false, destinationDDFilename);
                                    nrOfBlocksDecrypted+=nrBlocksFound;
                                }
                                
                                //decryptMFTBlock(fileChunk, patternStream, chiffreStreamSize, &dataBlockDecryptionSuccessful, &lastFoundChiffrePositionDataBlock);
                                
                                uint32_t sectorNr = posClusterToDecrypt/512;
    
    
                                if (/*nrBlocksFound==0*/!dataBlockDecryptionSuccessful && decryptedDataBlocks.find(sectorNr) == decryptedDataBlocks.end()) {
                                    
                                    /*
                                    ifstream test;
                                    test.open("../Windows7FreshDropperStage1\ Kopie.dd", ios::binary);
                                    test.seekg(posClusterToDecrypt, ios::beg);
                                    uint8_t testVector[1024];
                                    test.read((char*)testVector, 1024);
                                    test.close();
                                    */
                                        
                                    decryptedDataBlocks[sectorNr] = true;
    
                                    
                                    // Nope,no data block...
                                    // Now we need to rely that Petya is only decrypting the first 1024 bytes of each File,
                                    // Fingers crossed!
                                    
                                    uint8_t buf[1024];
                                    memset(buf,0,1024);
                                    
                                    // cout << endl << "Decrypting DATA Block with chiffre stream @" << posClusterToDecrypt << endl;
                                     *info << "\tDecrypting DATA Block with chiffre stream @" << posClusterToDecrypt;
                                     
                                    s20_crypt_sector(key, nonce, buf, 1024, sectorNr);
                                    
                                    // hexdump((char*)buf, 10);
                                    
                                    bool problemFound = false;
                                    
                                    for (uint64_t i = 0; i < 1024; i++) {
                                        fileChunk[i] ^= buf[i];
                                        /*
                                        if (fileChunk[i]!=testVector[i]) {
                                            problemFound = true;
                                        } 
                                        */
                                    }
                                    
                                    if (problemFound) cout << "Problem found @" << posClusterToDecrypt << "     FName:" << currentFilename << " @" << debugPosition << endl;
                                    // Abort operation...
                                    //return 0;
                                } else {
                                    decryptedSectors[posClusterToDecrypt] = true;
                                }
    
                                    
    
                                 
                                dstFs->write((char*)fileChunk, 1024);
                                                                
                                dstFs->flush();
                                
                                // Now Save Data to Harddisk
                                
                               // if (flags>0) { // Record is in use..., don't know if Petya checks this
								// extractDataNonResident(partitionStartPosition, offsetToCurrentAttribute + debugPosition, mftEntry + offsetToCurrentAttribute, destinationDDFilename, currentFilename);
                               // }
                           // }
                             
                            
                            is->seekg(currentFilePositionSave, ios::beg);
                            dstFs->seekg(currentFilePositionSaveDstFsRead, ios::beg);
                            dstFs->seekp(currentFilePositionSaveDstFsWrite, ios::beg);
                            // dstFs->close();
                            
                            
                            //return 0;                     
                         }
                    }
                    
//                        return 0;
                    
                    
                }
                else {
                    //cout << "Data is resident" << endl;

                    uint64_t dataLength = (mftEntry[offsetToCurrentAttribute + 0x10]) +
                        (mftEntry[offsetToCurrentAttribute + 0x11] << 8) +
                        (mftEntry[offsetToCurrentAttribute + 0x12] << 16) +
                        (mftEntry[offsetToCurrentAttribute + 0x13] << 24);

                    uint64_t dataStartPosition = (mftEntry[offsetToCurrentAttribute + 0x14]) +
                        (mftEntry[offsetToCurrentAttribute + 0x15] << 8) +
                        (mftEntry[offsetToCurrentAttribute + 0x16] << 16) +
                        (mftEntry[offsetToCurrentAttribute + 0x17] << 24);
                        
                        uint64_t dataAttributePosition = offsetToCurrentAttribute;
                        
                        
                        // extractDataResident(&mftEntry[dataAttributePosition], currentFilename, dataAttributePosition);
                }

                break;
            }
            /*
            case 0x90: // Index Root
                cout << "Index Root found" << endl;
                break;
            */
             case 0xFFFFFFFF: // End of MFT Entry found...
                // cout << "MFT Entry End found" << endl;
                mftEntryEndFound = true;
                return true;
                break;
            default:
                //cout << "Unknown File Block Type detected "<<hex << attributeType << dec  << endl;
                break;
            }

        offsetToCurrentAttribute += attributeSize;
    } while (!mftEntryEndFound &&  offsetToCurrentAttribute<1024-4 && attributeSize>0);
    
    if (offsetToCurrentAttribute>1024-4 || attributeSize==0) return false;
    
    
    return mftEntryEndFound;

}


uint64_t debugFlagNr = 0;

//       decryptMFT(firstFilePosition, is, dstFs, os, info,  patternStream, chiffreStreamSize, &nrOfFilesDecryptedInTotal);



uint64_t decryptMFT(uint64_t firstFilePosition, ifstream *is, fstream *dstFs, ofstream *os, ofstream *info,  uint8_t* patternStream, uint64_t chiffreStreamSize, uint64_t partitionStartPosition, uint8_t key[16], uint8_t nonce[8], bool recurse, string destinationDDFilename) {



    
    unsigned char mftEntry[1024];
    
    is->seekg(firstFilePosition, ios::beg);

    bool mftEndFound;
    
    bool decryptionSuccessful = false;
    uint64_t nrOfFiles = 0;
    uint64_t nrOfBlocksDecrypted = 0;

    uint64_t retryCounter = 0;
    
    uint64_t retries = 0;
    
    if (recurse) retries = 25; else retries=0;
    
    uint64_t currentPosition = is->tellg();
     
    do {
        decryptionSuccessful = false;
        mftEndFound = false;
        cout << "\r@" << currentPosition << " ";
        cout.flush(); 
        is->seekg(currentPosition, ios::beg);
        dstFs->seekg(currentPosition, ios::beg);
        
        //currentPosition = is->tellg();
        
        while (decryptedSectors.find(currentPosition) != decryptedSectors.end()){
             is->read((char*)mftEntry, 1024);
             currentPosition = is->tellg();
			 if (currentPosition == -1) break;
             dstFs->seekg(is->tellg(), ios::beg);
        }
        
		if (currentPosition == -1) break;
    
        decryptedSectors[currentPosition] = true;
    

        is->read((char*)mftEntry, 1024);

        
        uint64_t lastFoundChiffrePosition;
        
        // if (decryptMFTBlock(mftEntry, patternStream, chiffreStreamSize, &decryptionSuccessful, &lastFoundChiffrePosition)) {
         if (decryptMFTBlockCalculatedAndTrialAndError(mftEntry, nonce, key, currentPosition,   &decryptionSuccessful,
          patternStream, chiffreStreamSize, &lastFoundChiffrePosition)) {

            mftEndFound = false;

            // debugOs << dec << currentPosition << endl;
            
            if (!decryptionSuccessful) {
                //info << "Problem found @" << currentPosition << endl;
                // info.flush();
                break; // To break on first problem...
            } else {
            
              // Just for debug purposes to compare if we decoded everything...
              /*
              char debug[1024];
              memset(debug, 0xAA, 1024);
              dstFs->write((char*)debug, 1024);
              */
                                
              dstFs->write((char*)mftEntry, 1024);
              dstFs->flush();
            }

            if (os!=NULL) os->write((char*)mftEntry, 1024);
            if (os!=NULL) os->flush();
            nrOfBlocksDecrypted++;
            
            
            // Walk files...
            // 14+15
             
           // hexdump((char*)mftEntry,1024);
            
            string currentFilename = "";
            

            bool wasParsed = parseMFTEntries(mftEntry, is, dstFs, os, info, partitionStartPosition, key, nonce, patternStream, chiffreStreamSize, destinationDDFilename, &nrOfBlocksDecrypted, currentPosition);
            
            
            *info << endl;

        } else {
            // cout << endl << "Failed to decrypt MFT Block @" << currentPosition << " maybe it's the end of the MFT?" << endl;
            retryCounter++;
        }
        
        if (mftEndFound) {
            retryCounter=0;
        }
        
        // Go to next MFT Entry...
        currentPosition+=1024;
        
    } while (!mftEndFound  && retryCounter<retries);
    //  } while (recurse && (mftEndFound || retryCounter<retries));
    
    return nrOfBlocksDecrypted;
}


uint64_t searchMFTBegin(int partitionNr, uint8_t *bootsector, ifstream *is) {
    uint64_t partitionStart = getPartitionStartPosition(partitionNr, (unsigned char*)bootsector);
    uint64_t mftBegin = 0;
    
    uint64_t currentPosition = is->tellg();
    bool mftEntryFound = false;
    uint8_t buffer[512];
    do {
        is->read((char*)buffer, 512);
        if (buffer[0]=='F' &&
            buffer[1]=='I' &&
            buffer[2]=='L' &&
            buffer[3]=='E') 
        {
            mftEntryFound = true;
            mftBegin = is->tellg();
            mftBegin-=512;
            break;
        }
        
    } while (!is->eof() || !mftEntryFound);
    
    
    
    is->clear();

    is->seekg(currentPosition, ios::beg);
    
    return mftBegin; // If 0 nothing was found as this would be the bootsector...
}



void repairPartitionBootsectors(char* inFilename, char* outFilename)
{
        ifstream is;
        fstream fs;
        
        is.open(inFilename, ios::binary);
        fs.open(outFilename);
        
       	uint8_t bootsector[512];
       		
        is.read((char*)bootsector, 512);
        
        if (bootsector[0]==0xFA && // Bootsector is a Petya Bootsector, we need to fetch the bootsector from a different location...
            bootsector[1]==0x66 &&
            bootsector[2]==0x31)
        {
            is.seekg(28672, ios::beg);  // Read xor 0x07 crypted bootsector
            is.read((char*)bootsector, 512);
            for (int i = 0; i<512; i++) {
                bootsector[i] ^= 07;
            }
        }
        
        
        // Now determine Partition Boot Sector locations...
        for (int i = 0; i<4; i++) {
        
            if (getPartitionType(i, (unsigned char*)bootsector) == ntfs) {

                cout << "Repairing Partition Bootsector from Partition " <<(i+1) << endl;
                uint8_t partitionBootsector[512];
                uint64_t startPos = getPartitionStartPosition(i, (unsigned char*)bootsector);
                uint64_t bootsectorBackupLocation = getPartitionBootsectorBackupPosition(i, (unsigned char*)bootsector);
                
                is.seekg(bootsectorBackupLocation);
                is.read((char*)partitionBootsector, 512);
                fs.seekg(startPos);
                fs.write((char*)partitionBootsector, 512);                                                
            }
        }

       
       is.close();       
       fs.close();

}

void restorePartitionBootsectorBackups(char* inputFilenameStr, string outputFilenameStr, unsigned char* bootsector, uint8_t* patternStream, uint64_t chiffreStreamSize) {
        cout << "Restoring Partititon Bootsector Backups...";
        
        ifstream is;
        fstream fs;
        
        is.open(inputFilenameStr, ios::binary);
        fs.open(outputFilenameStr, ios::binary);
       		
        is.read((char*)bootsector, 512);
        
        if (bootsector[0]==0xFA && // Bootsector is a Petya Bootsector, we need to fetch the bootsector from a different location...
            bootsector[1]==0x66 &&
            bootsector[2]==0x31)
        {
            is.seekg(28672, ios::beg);  // Read xor 0x07 crypted bootsector
            is.read((char*)bootsector, 512);
            for (int i = 0; i<512; i++) {
                bootsector[i] ^= 07;
            }
        }
        
        
        // Now determine Partition Boot Sector locations...
        for (int i = 0; i<4; i++) {
        
            if (getPartitionType(i, (unsigned char*)bootsector) == ntfs) {
                
                uint64_t bootsectorBackupLocation = getPartitionBootsectorBackupPosition(i, (unsigned char*)bootsector);
                is.seekg(bootsectorBackupLocation, ios::beg);
                
                cout << "Partitition "<<i<< "Bootsector Location is "<< bootsectorBackupLocation << endl;
                unsigned char sector[1024]; // 1024 because Petya encrypts always 1024 bytes never just 512
    
                fs.seekg(is.tellg(), ios::beg);
                is.read((char*)sector, 512);
                            
                is.clear();
                
                uint64_t startPos = getPartitionStartPosition(i, (unsigned char*)bootsector);
                is.seekg(startPos, ios::beg);
    
                if (checkNTFSSector(sector, patternStream, chiffreStreamSize,512)) {
                    fs.write((char*)sector, 512);
                }
                                            
            }
        }
       
        is.close();       
        fs.close();   
        cout << "done"<<endl;
                
	}


void printHelp() {
    cout << "Please specifiy the following parameters:" << endl;
    cout << "1.Dump of your encrypted harddrive" << endl;
    cout << "2.Your key" << endl;
    cout << endl;
    cout << "or" << endl;
    cout << "1.Dump of your encrypted harddrive" << endl;
    cout << "2.--repairParitionBootsectors" << endl;            
    cout << "or" << endl;
    cout << "1.Dump of your encrypted harddrive (ending with _decrypted.dd)" << endl;
    cout << "2.Your key" << endl;
    cout << "3.--decryptPosition" << endl;    
    cout << "4.position in bytes which shall be decrypted" << endl;                        
    cout << "Notice here we write to the provided file" << endl;                        
}

void restoreMBR(char* inputFileNameStr, string outputFileNameStr) {
    // Restore MBR...
    
    ifstream inputFile;
    inputFile.open(inputFileNameStr, ios::binary);
        
    fstream dstFs;
    dstFs.open(outputFileNameStr, ios::in | ios::out);    
    
    inputFile.seekg(28672, ios::beg);  // Read xor 0x07 crypted bootsector
	char cryptedBootsector[512];
	
	inputFile.read((char*)cryptedBootsector, 512);
	for (int i = 0; i<512; i++) {
		cryptedBootsector[i] ^= 07;
	}
    
    dstFs.seekg(0, ios::beg);
    dstFs.write(cryptedBootsector, 512);
    
    dstFs.close();
    inputFile.close();
}


void readNonce(char* inputFilenameStr, uint8_t *nonce) {
    ifstream is;
    is.open(inputFilenameStr, ios::binary);
    
	// Read Nonce
	is.seekg(27681, ios::beg);	
	is.read((char*)nonce, 8);
    is.close();
}

uint64_t determineChiffreStreamSize(uint8_t* nonce, uint8_t* key) {
   
    
        // Generate Chiffre Stream...
    uint8_t pattern[64];    
    uint8_t* patternStream;
    
    initChiffre(nonce, key);
    
    for (int i=0; i<64; i++) {
        pattern[i] = (uint8_t) getNextChiffreByte();
    }    
        
    // Check when chiffre repeats...
    patternStream = (uint8_t*) malloc(64);
    
    memcpy(patternStream, pattern,64);

    bool found=false;
    uint64_t chiffreStreamSize=0;
    
    uint64_t prevMegaBytes = 0;
    uint64_t megabytes = 0;
    
    do {
            
        for (int i=0; i<63;i++) {
             patternStream[i] = patternStream[i+1];
        }
        
        patternStream[63] = (uint8_t)getNextChiffreByte(); 
        
        chiffreStreamSize++;
                        
        found = true;
        for (int i=0; i<64;i++) {
            if (patternStream[i]!=pattern[i]) {
                found=false; 
                break;
            }
        }
                                
    } while (!found);
    chiffreStreamSize-=64;
    
    free(patternStream);
    //cout << "Chiffre Stream size is " << chiffreStreamSize << endl;
    return chiffreStreamSize;
}

uint8_t* generateChiffreStream(uint8_t* nonce, uint8_t* key, uint64_t prevCalculatedChiffreStreamSize=0) {
    
    uint64_t chiffreStreamSize = 0;
    
    if (prevCalculatedChiffreStreamSize>0) {
        chiffreStreamSize = prevCalculatedChiffreStreamSize;
    } else {
        chiffreStreamSize = determineChiffreStreamSize(nonce, key);
    }
    
    uint8_t* patternStream;
        
    patternStream = (uint8_t *) malloc(chiffreStreamSize);
    initChiffre(nonce, key);
    
    for (uint64_t i=0; i< chiffreStreamSize; i++) {
        patternStream[i] = getNextChiffreByte();
    }
    
    return patternStream;
}


void restorePartitionBootsectors(char* inputFilenameStr, string outputFilenameStr, unsigned char* bootsector, uint8_t* patternStream, uint64_t chiffreStreamSize) {
    ifstream is;
    is.open(inputFilenameStr, ios::binary);
    fstream dstFs;
    dstFs.open(outputFilenameStr);
    
    
    uint64_t startPos[4];

	for (int i = 0; i<4; i++) {
		startPos[i] = getPartitionStartPosition(i, bootsector);
	}


	for (int i = 0; i<4; i++) {
		if (getPartitionType(i, bootsector) == ntfs) {
			is.seekg(startPos[i], ios::beg);

			unsigned char sector[1024]; // 1024 because Petya encrypts always 1024 bytes never just 512

			dstFs.seekg(is.tellg(), ios::beg);
			is.read((char*)sector, 1024);
						
			is.clear();
			is.seekg(startPos[i], ios::beg);

			if (checkNTFSSector(sector, patternStream, chiffreStreamSize)) {
                dstFs.write((char*)sector, 1024);
			}
		}
	}
	
	is.close();
	dstFs.close();
}


#define BUF_SIZE 20*1024*1024 
uint64_t bufferSize = BUF_SIZE;
uint8_t copyBuffer[BUF_SIZE];

int main(int argc, char**argv) {
    
	unsigned char nonce[8];
	unsigned char key[17];
	
	
	if (argc<3) {
	    printHelp();
        return -1;
	}
    
    
    // Check if inputfile could be opened...
	ifstream is;
	is.open(argv[1], ios::binary);

    if (!is || !is.is_open()) {
        cout << "Error opening file" << endl;
        return -1; 
    }
    
    std::ostringstream outputFileStream;
    outputFileStream << argv[1] << "_decrypted.dd" ;
    string outputFileNameStr = outputFileStream.str();

    string fileStr(argv[1]);
    
    // If file doesn't contain "_decrypted.dd" ...
    if (fileStr.find("_decrypted.dd")==string::npos) {
    
        // Backup Image 
        
        
        std::ifstream dstIs(outputFileNameStr);
        
        uint64_t infileLength = 0;
        
        if (dstIs.good()) {
               dstIs.seekg(0, ios::end);
               infileLength = dstIs.tellg();
               dstIs.seekg(0, ios::beg);
               dstIs.close();
        }
        
        ifstream inputFile;
        inputFile.open(argv[1], ios::binary);
        
        inputFile.seekg(0, ios::end);
        uint64_t inputFileLength = inputFile.tellg();
        inputFile.seekg(0, ios::beg);
        
        if (inputFileLength!=infileLength) {
        
            cout << "Creating a new backup (depending on the filesize this may take a while)" << endl;

            ofstream dstOs;
            dstOs.open(outputFileNameStr);
            dstOs << inputFile.rdbuf();
            dstOs.close();
        }
        


    } else {
        if (argc<4) {
            printHelp();
            return -1;
        }
        
        // Rest is done below...
        is.close(); // Close file as we are reading and writing to the same file below...
        
    }
    
	restoreMBR(argv[1], outputFileNameStr);

    if (argc==3 && string("--repairParitionBootsectors").compare(string(argv[2]))==0) {
       cout << "Repairing Partition Bootsectors" << endl;
       
       char *outFilename = (char*) malloc(outputFileNameStr.length()*sizeof(uint8_t));
       
       memcpy(outFilename,  outputFileNameStr.c_str(), outputFileNameStr.length()*sizeof(uint8_t));
       repairPartitionBootsectors(argv[1], outFilename);
       
       free(outFilename);
       
       return 0;
    }
    
	
    // Copy Key from argv 2
	memcpy(key, argv[2], 16);
	key[16] = 0;

    
	// Read Nonce
	readNonce(argv[1], nonce);
	
	cout << "Your key is:" << key << endl;
	cout << "Nonce is:";
	hexdump((char*)nonce, 8);
	cout << endl;

    

    
    uint64_t chiffreStreamSize = determineChiffreStreamSize(nonce, key);
    
    uint8_t* patternStream;
    patternStream = generateChiffreStream(nonce, key, chiffreStreamSize);    

    if (argc>4) {
        string arg3Str = string(argv[3]);
        if (arg3Str.compare("--decryptPosition")!=string::npos) {
    
            uint64_t bytePosition = 0;
            std::istringstream ss(argv[4]);
                
            if (!(ss >> bytePosition))
            std::cout << "No byte position as 4th parameter provided!" << std::endl;
        
                
            ifstream is;  
            fstream dstFs;
            
            is.open(argv[1], ios::binary);
            dstFs.open(argv[4], ios::binary);
                
                
            uint8_t readSector[1024];    
            uint8_t buf[1024];
            memset(buf,0,1024);
                         
            s20_crypt_sector(key, nonce, buf, 1024, bytePosition/512);
            
            // hexdump((char*)buf, 10);
            
            is.seekg(bytePosition, ios::beg);
            is.read((char*)readSector, 1024);
            
            for (uint64_t i = 0; i < 1024; i++) {
                readSector[i] ^= buf[i]; 
            }
            cout << "Decrypted data is " << endl;
            hexdump((char*)readSector, 1024);
            
            // dstFs.write((char*)readSector,1024);                                        
            // uint64_t nrOfEntriesDecrypted = decryptMFT(bytePosition, &is, &dstFs, NULL, NULL,  patternStream, chiffreStreamSize, 0, key, nonce, true);
            
            //cout << "Nr of entries decrypted:" << nrOfEntriesDecrypted << endl;
            is.close();
            dstFs.close();
            return -1;
        }
    }



	is.seekg(0, ios::end);
	uint64_t length = is.tellg();
	is.seekg(0, ios::beg);
	
	// Read Nr of encrypted Sectors...
    is.seekg(29184, ios::beg);
    uint64_t nrOfEncryptedSectors;
    uint8_t bytes[8];  // Don't know if 8 is correct?!?
    is.read((char*)bytes, 8);
    


	is.seekg(28672, ios::beg);  // Read xor 0x07 crypted bootsector
	char cryptedBootsector[512]; 
	
	is.read((char*)cryptedBootsector, 512);
	for (int i = 0; i<512; i++) {
		cryptedBootsector[i] ^= 07;
	}
	memcpy(bootsector, cryptedBootsector, 512);
	checkBootsector(bootsector);




	cout << "Partition Information according to Bootsector" << endl;

	for (int i = 0; i<4; i++) {
		if (getPartitionType(i, bootsector) != empty) {
			printPartition(bootsector, i);
		}
	}


    restorePartitionBootsectors(argv[1], outputFileNameStr, bootsector,  patternStream, chiffreStreamSize);
   // storePartitionBootsectorBackups(argv[1], outputFileNameStr, bootsector,  patternStream, chiffreStreamSize);

	uint64_t startPos[4];

	for (int i = 0; i<4; i++) {
		startPos[i] = getPartitionStartPosition(i, bootsector);
	}
	cout << endl;

    /*
	cout << "Paritition Information according to Partition Begin" << endl;
	for (int i = 0; i<4; i++) {
		if (getPartitionType(i, bootsector) == ntfs) {
			cout << "Partition " << (i + 1) << ":" << endl;
			is.seekg(startPos[i], ios::beg);

			unsigned char sector[1024]; // 1024 because Petya encrypts always 1024 bytes never just 512

			is.read((char*)sector, 1024);
						
			is.clear();
			is.seekg(startPos[i], ios::beg);

			if (checkNTFSSector(sector, patternStream, chiffreStreamSize)) {
				printNTFSInformation(sector, getPartitionStartPosition(i, bootsector), i);
			}
			cout << endl;
		}
	}*/

    std::ostringstream stream;
	stream << argv[1] << "_Info.txt" ;
	string infoFilenameStr = stream.str();
		
	ofstream info;
	info.open(infoFilenameStr);
	
	fstream dstFs;
	
	dstFs.clear();
	cout << "Opening " << outputFileNameStr << endl;
    dstFs.open(outputFileNameStr); // , ios::binary <-- This seems to make trouble, file fails to open when option is given

    /*
    if (dstFs.good()) {
        cout << "OK" << endl;
        dstFs.clear();
        char debug[1024];
        memset(debug,0xAB,1024);
        dstFs.seekp(0, ios::beg);
        dstFs.seekg(0, ios::beg);
        
    //    dstFs.seekp(21192704, ios::beg);
        dstFs.write(debug, 1024);
        dstFs.close();
    } else {
        cout << "Failed to open file " <<outputFileNameStr<<endl;
    }
    
    return 0;*/
    
    // ofstream debugOs;
    // debugOs.open("decryptedMFTBlocks.txt");

    uint64_t nrOfFilesDecryptedInTotal = 0;

	for (int i = 0; i<4; i++) {



        std::ostringstream streamMFT;
	    streamMFT << argv[1] << "_MFT_" << i << ".data" ;
	    string outFilename = streamMFT.str();

		info << "Partition Number " << (i + 1) << endl;

		ofstream os;
		os.open(outFilename, ios::binary);
		

//        cout << "DEBUG: Partition End Position is " << getPartitionEndPosition(i, (uint8_t *) bootsector);
        
		if (getPartitionType(i, bootsector) == ntfs) {
		
		    //uint64_t manualMFTSearchPosition = searchMFTBegin(i,(uint8_t *) bootsector, &is);
            //cout << "Manual MFT Search Position is " << manualMFTSearchPosition << endl;


			cout << "Partition " << (i + 1) << ":" << endl;
			is.seekg(startPos[i], ios::beg);
			unsigned char sector[1024]; // Always 2 sectors are encrypted

			is.read((char*)sector, 1024);
			if (checkNTFSSector(sector, patternStream, chiffreStreamSize)) {

				uint64_t nrOfBlocksDecrypted = 0;

				uint64_t mftStartPosition = getMFTStartPosition(sector, getPartitionStartPosition(i, bootsector));

				// 46494C4530 FILE0

				// if (i==1) mftStartPosition = 1441792000;
				//                    if (i==1) mftStartPosition -= 232278;
				//if (i>0) mftStartPosition *= 8;

				uint64_t firstFilePosition = mftStartPosition + 0 * 1024; 

                cout << "MFT Size in Sectors is " << getMFTLength(sector, getPartitionStartPosition(i, bootsector));
                cout << "Start of MFT is " << mftStartPosition << endl;
                info << "Start of MFT is " << mftStartPosition << endl;
                
				cout << "First file position is " << firstFilePosition << endl;
				info << "First file position is " << firstFilePosition << endl;

                cout << "Start of Partition is " << startPos[i] << endl;
                nrOfBlocksDecrypted = decryptMFT(firstFilePosition, &is, &dstFs, &os, &info,  patternStream, chiffreStreamSize, startPos[i], key, nonce, true, outputFileNameStr);
				
				nrOfFilesDecryptedInTotal+=nrOfBlocksDecrypted;
				cout << endl << "Total MFT Blocks in partition: " << dec << nrOfBlocksDecrypted << endl;
				info << "Total MFT Blocks in partition: " << dec << nrOfBlocksDecrypted << endl << endl;
								
				// Decrypt MFT Copy
                uint64_t mftCopyStartPosition = getMFTCopyStartPosition(sector, getPartitionStartPosition(i, bootsector));
				uint64_t firstFilePositionMFTCopy = mftCopyStartPosition /*+ 35 * 1024*/; 
                cout << endl << endl <<"MFT Copy Starts at position " << mftCopyStartPosition << endl;
                cout << "First File at MFT Copy starts  at " << firstFilePositionMFTCopy << endl;
                
                nrOfBlocksDecrypted = decryptMFT(firstFilePositionMFTCopy, &is, &dstFs, &os, &info,  patternStream, chiffreStreamSize, startPos[i], key, nonce, true, outputFileNameStr);
				nrOfFilesDecryptedInTotal+=nrOfBlocksDecrypted;
                
                cout << endl << "Total MFT Blocks in MFT Copy of partition: " << dec << nrOfBlocksDecrypted << endl;
				info << "Total MFT Blocks in MFT Copy of partition:  " << dec << nrOfBlocksDecrypted << endl;

                cout << endl << endl;
                info << endl << endl;
                
			}
			cout << endl;
		}
		os.close();
	}
	info.close();

	is.close();
	
	dstFs.close();
    // debugOs.close();
	
	cout << "Nr of MFT Blocks decrypted in Total:" << nrOfFilesDecryptedInTotal << endl;
	// Tidy Up
	// Todo: Make sure this is called upon Ctrl+C
	free(patternStream);

}


/*
static long filetimeToMillis(long filetime) {

        filetime -= 116444736000000000L;

        if (filetime < 0) {
            filetime = -1 - ((-filetime - 1) / 10000);
        } else  {
            filetime = filetime / 10000;
        }
        return filetime;
    }
   static Date filetimeToDate(long filetime) {
        return new Date(filetimeToMillis(filetime));
    }
    
    
    import java.util.*;
class timeStampConverter
{
   public static long filetimeToMillis(long filetime)
   {
     filetime -= 116444736000000000L;
     if (filetime < 0)
     {
       filetime = -1 - ((-filetime - 1) / 10000);
     }
     else
     {
       filetime = filetime / 10000;
     }
     return filetime;
   }

   public static Date filetimeToDate(long filetime)
   {
     return new Date(filetimeToMillis(filetime));
   }	

   public static void main(String args[])
   {
     long time = 127541957167518016L; // Specify your long here!!
     Date timeDate = filetimeToDate(time);			
     System.out.println(timeDate.toString());
   }
}
}*/
