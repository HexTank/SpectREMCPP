//
//  Snapshot.cpp
//  SpectREM
//
//  Created by Mike Daley on 26/08/2017.
//  Copyright © 2017 71Squared Ltd. All rights reserved.
//

#include <stdio.h>
#include "ZXSpectrum.hpp"

#pragma mark - Constants

const int               cSNA_HEADER_SIZE = 27;
const unsigned short    cZ80_V3_HEADER_SIZE = 86;
const unsigned short    cZ80_V3_ADD_HEADER_SIZE = 54;
const unsigned char     cZ80_V3_PAGE_HEADER_SIZE = 3;

const int               cZ80_V2_MACHINE_TYPE_48 = 0;
const int               cZ80_V3_MACHINE_TYPE_48 = 0;
const int               cZ80_V2_MACHINE_TYPE_48_IF1 = 1;
const int               cZ80_V3_MACHINE_TYPE_48_IF1 = 1;
//const int               cZ80_V2_MACHINE_TYPE_SAMRAM = 2;
//const int               cZ80_V3_MACHINE_TYPE_SAMRAM = 2;
const int               cZ80_V2_MACHINE_TYPE_128 = 3;
const int               cZ80_V3_MACHINE_TYPE_48_MGT = 3;
const int               cZ80_V2_MACHINE_TYPE_128_IF1 = 4;
const int               cZ80_V3_MACHINE_TYPE_128 = 4;
const int               cZ80_V3_MACHINE_TYPE_128_IF1 = 5;
const int               cz80_V3_MACHINE_TYPE_128_MGT = 6;
const int               cZ80_V3_MACHINE_TYPE_128_2 = 12;


#pragma mark - SNA

ZXSpectrum::Snap ZXSpectrum::snapshotCreateSNA()
{
    // We don't want the core running when we take a snapshot
    pause();
    
    Snap snap;
    snap.length = (48 * 1024) + cSNA_HEADER_SIZE;
    snap.data = new unsigned char[snap.length];
    
    snap.data[0] = z80Core.GetRegister(CZ80Core::eREG_I);
    snap.data[1] = z80Core.GetRegister(CZ80Core::eREG_ALT_HL) & 0xff;
    snap.data[2] = z80Core.GetRegister(CZ80Core::eREG_ALT_HL) >> 8;
    
    snap.data[3] = z80Core.GetRegister(CZ80Core::eREG_ALT_DE) & 0xff;
    snap.data[4] = z80Core.GetRegister(CZ80Core::eREG_ALT_DE) >> 8;
    
    snap.data[5] = z80Core.GetRegister(CZ80Core::eREG_ALT_BC) & 0xff;
    snap.data[6] = z80Core.GetRegister(CZ80Core::eREG_ALT_BC) >> 8;
    
    snap.data[7] = z80Core.GetRegister(CZ80Core::eREG_ALT_AF) & 0xff;
    snap.data[8] = z80Core.GetRegister(CZ80Core::eREG_ALT_AF) >> 8;
    
    snap.data[9] = z80Core.GetRegister(CZ80Core::eREG_HL) & 0xff;
    snap.data[10] = z80Core.GetRegister(CZ80Core::eREG_HL) >> 8;
    
    snap.data[11] = z80Core.GetRegister(CZ80Core::eREG_DE) & 0xff;
    snap.data[12] = z80Core.GetRegister(CZ80Core::eREG_DE) >> 8;
    
    snap.data[13] = z80Core.GetRegister(CZ80Core::eREG_BC) & 0xff;
    snap.data[14] = z80Core.GetRegister(CZ80Core::eREG_BC) >> 8;
    
    snap.data[15] = z80Core.GetRegister(CZ80Core::eREG_IY) & 0xff;
    snap.data[16] = z80Core.GetRegister(CZ80Core::eREG_IY) >> 8;
    
    snap.data[17] = z80Core.GetRegister(CZ80Core::eREG_IX) & 0xff;
    snap.data[18] = z80Core.GetRegister(CZ80Core::eREG_IX) >> 8;
    
    snap.data[19] = (z80Core.GetIFF1() & 1) << 2;
    snap.data[20] = z80Core.GetRegister(CZ80Core::eREG_R);
    
    snap.data[21] = z80Core.GetRegister(CZ80Core::eREG_AF) & 0xff;
    snap.data[22] = z80Core.GetRegister(CZ80Core::eREG_AF) >> 8;
    
    unsigned short pc = z80Core.GetRegister(CZ80Core::eREG_PC);
    unsigned short sp = z80Core.GetRegister(CZ80Core::eREG_SP) - 2;
    
    snap.data[23] = sp & 0xff;
    snap.data[24] = sp >> 8;
    
    snap.data[25] = z80Core.GetIMMode();
    snap.data[26] = displayBorderColor & 0x07;
    
    int dataIndex = cSNA_HEADER_SIZE;
    for (unsigned int addr = 16384; addr < 16384 + (48 * 1024); addr++)
    {
        snap.data[dataIndex++] = z80Core.Z80CoreDebugMemRead(addr, NULL);
    }
    
    // Update the SP location in the snapshot buffer with the new SP, as PC has been added to the stack
    // as part of creating the snapshot
    snap.data[sp - 16384 + cSNA_HEADER_SIZE] = pc & 0xff;
    snap.data[sp - 16384 + cSNA_HEADER_SIZE + 1] = pc >> 8;
    
    resume();
    
    return snap;
}

bool ZXSpectrum::snapshotSNALoadWithPath(const char *path)
{
    FILE *fileHandle;
    
    fileHandle = fopen(path, "rb");
    
    if (!fileHandle)
    {
        cout << "ERROR LOADING SNAPSHOT: " << path << endl;
        return false;
    }
    
    cout << "Loading SNA snapshot: " << path << endl;
    
    pause();
    displayFrameReset();
    displayClear();
    audioReset();
    
    fseek(fileHandle, 0, SEEK_END);
    long size = ftell(fileHandle);
    fseek(fileHandle, 0, SEEK_SET);
    
    unsigned char fileBytes[size];
    
    fread(&fileBytes, 1, size, fileHandle);
    
    // Decode the header
    z80Core.SetRegister(CZ80Core::eREG_I, fileBytes[0]);
    z80Core.SetRegister(CZ80Core::eREG_R, fileBytes[20]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_HL, ((unsigned short *)&fileBytes[1])[0]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_DE, ((unsigned short *)&fileBytes[1])[1]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_BC, ((unsigned short *)&fileBytes[1])[2]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_AF, ((unsigned short *)&fileBytes[1])[3]);
    z80Core.SetRegister(CZ80Core::eREG_HL, ((unsigned short *)&fileBytes[1])[4]);
    z80Core.SetRegister(CZ80Core::eREG_DE, ((unsigned short *)&fileBytes[1])[5]);
    z80Core.SetRegister(CZ80Core::eREG_BC, ((unsigned short *)&fileBytes[1])[6]);
    z80Core.SetRegister(CZ80Core::eREG_IY, ((unsigned short *)&fileBytes[1])[7]);
    z80Core.SetRegister(CZ80Core::eREG_IX, ((unsigned short *)&fileBytes[1])[8]);
    
    z80Core.SetRegister(CZ80Core::eREG_AF, ((unsigned short *)&fileBytes[21])[0]);
    
    z80Core.SetRegister(CZ80Core::eREG_SP, ((unsigned short *)&fileBytes[21])[1]);
    
    // Border colour
    displayBorderColor = fileBytes[26] & 0x07;
    
    // Set the IM
    z80Core.SetIMMode(fileBytes[25]);
    
    // Do both on bit 2 as a RETN copies IFF2 to IFF1
    z80Core.SetIFF1((fileBytes[19] >> 2) & 1);
    z80Core.SetIFF2((fileBytes[19] >> 2) & 1);
    
    if (size == (48 * 1024) + cSNA_HEADER_SIZE)
    {
        int snaAddr = cSNA_HEADER_SIZE;
        for (int i = 16384; i < (64 * 1024); i++)
        {
            memoryRam[i] = fileBytes[snaAddr++];
        }
        
        // Set the PC
        unsigned char pc_lsb = memoryRam[z80Core.GetRegister(CZ80Core::eREG_SP)];
        unsigned char pc_msb = memoryRam[(z80Core.GetRegister(CZ80Core::eREG_SP) + 1)];
        z80Core.SetRegister(CZ80Core::eREG_PC, (pc_msb << 8) | pc_lsb);
        z80Core.SetRegister(CZ80Core::eREG_SP, z80Core.GetRegister(CZ80Core::eREG_SP) + 2);
    }
    
    resume();
    
    return true;
    
}

#pragma mark - Z80

ZXSpectrum::Snap ZXSpectrum::snapshotCreateZ80()
{
    pause();
    
    int snapshotSize = 0;
    if (machineInfo.machineType == eZXSpectrum48)
    {
        snapshotSize = (48 * 1024) + cZ80_V3_HEADER_SIZE + (cZ80_V3_PAGE_HEADER_SIZE * 3);
    }
    else if (machineInfo.machineType == eZXSpectrum128 || machineInfo.machineType == eZXSpectrum128_2)
    {
        snapshotSize = (128 * 1024) + cZ80_V3_HEADER_SIZE + (cZ80_V3_PAGE_HEADER_SIZE * 8);
    }
    else
    {
        cout << "Unknown machine type" << endl;
        exit(1);
    }
    
    // Structure to be returned containing the length and size of the snapshot
    Snap snapData;
    snapData.length = snapshotSize;
    snapData.data = new unsigned char[ snapshotSize ];
    
    // Header
    snapData.data[0] = z80Core.GetRegister(CZ80Core::eREG_A);
    snapData.data[1] = z80Core.GetRegister(CZ80Core::eREG_F);
    snapData.data[2] = z80Core.GetRegister(CZ80Core::eREG_BC) & 0xff;
    snapData.data[3] = z80Core.GetRegister(CZ80Core::eREG_BC) >> 8;
    snapData.data[4] = z80Core.GetRegister(CZ80Core::eREG_HL) & 0xff;
    snapData.data[5] = z80Core.GetRegister(CZ80Core::eREG_HL) >> 8;
    snapData.data[6] = 0x0; // PC
    snapData.data[7] = 0x0;
    snapData.data[8] = z80Core.GetRegister(CZ80Core::eREG_SP) & 0xff;
    snapData.data[9] = z80Core.GetRegister(CZ80Core::eREG_SP) >> 8;
    snapData.data[10] = z80Core.GetRegister(CZ80Core::eREG_I);
    snapData.data[11] = z80Core.GetRegister(CZ80Core::eREG_R) & 0x7f;
    
    unsigned char byte12 = z80Core.GetRegister(CZ80Core::eREG_R) >> 7;
    byte12 |= (displayBorderColor & 0x07) << 1;
    byte12 &= ~(1 << 5);
    snapData.data[12] = byte12;
    
    snapData.data[13] = z80Core.GetRegister(CZ80Core::eREG_E);            // E
    snapData.data[14] = z80Core.GetRegister(CZ80Core::eREG_D);            // D
    snapData.data[15] = z80Core.GetRegister(CZ80Core::eREG_ALT_C);        // C'
    snapData.data[16] = z80Core.GetRegister(CZ80Core::eREG_ALT_B);        // B'
    snapData.data[17] = z80Core.GetRegister(CZ80Core::eREG_ALT_E);        // E'
    snapData.data[18] = z80Core.GetRegister(CZ80Core::eREG_ALT_D);        // D'
    snapData.data[19] = z80Core.GetRegister(CZ80Core::eREG_ALT_L);        // L'
    snapData.data[20] = z80Core.GetRegister(CZ80Core::eREG_ALT_H);        // H'
    snapData.data[21] = z80Core.GetRegister(CZ80Core::eREG_ALT_A);        // A'
    snapData.data[22] = z80Core.GetRegister(CZ80Core::eREG_ALT_F);        // F'
    snapData.data[23] = z80Core.GetRegister(CZ80Core::eREG_IY) & 0xff;    // IY
    snapData.data[24] = z80Core.GetRegister(CZ80Core::eREG_IY) >> 8;      //
    snapData.data[25] = z80Core.GetRegister(CZ80Core::eREG_IX) & 0xff;    // IX
    snapData.data[26] = z80Core.GetRegister(CZ80Core::eREG_IX) >> 8;      //
    snapData.data[27] = (z80Core.GetIFF1()) ? 0xff : 0x0;
    snapData.data[28] = (z80Core.GetIFF2()) ? 0xff : 0x0;
    snapData.data[29] = z80Core.GetIMMode() & 0x03;                       // IM Mode
    
    // Version 3 Additional Header
    snapData.data[30] = (cZ80_V3_ADD_HEADER_SIZE) & 0xff;               // Addition Header Length
    snapData.data[31] = (cZ80_V3_ADD_HEADER_SIZE) >> 8;
    snapData.data[32] = z80Core.GetRegister(CZ80Core::eREG_PC) & 0xff;    // PC
    snapData.data[33] = z80Core.GetRegister(CZ80Core::eREG_PC) >> 8;
    
    if (machineInfo.machineType == eZXSpectrum48)
    {
        snapData.data[34] = cZ80_V2_MACHINE_TYPE_48;
    }
    else if (machineInfo.machineType == eZXSpectrum128)
    {
        snapData.data[34] = cZ80_V3_MACHINE_TYPE_128;
    }
    else if (machineInfo.machineType == eZXSpectrum128_2)
    {
        snapData.data[34] = cZ80_V3_MACHINE_TYPE_128_2;
    }
    
    if (machineInfo.machineType == eZXSpectrum128 || machineInfo.machineType == eZXSpectrum128_2)
    {
        snapData.data[35] = ULAPortFFFDValue; // last 128k 0x7ffd port value
    }
    else
    {
        snapData.data[35] = 0;
    }
    
    snapData.data[36] = 0; // Interface 1 ROM
    snapData.data[37] = 4; // AY Sound
    snapData.data[38] = ULAPortFFFDValue; // Last OUT fffd
    
    // Save the AY register values
    int dataIndex = 39;
    for (int i = 0; i < 16; i++)
    {
        audioAYSetRegister(i);
        snapData.data[dataIndex++] = audioAYReadData();
    }
    
    int quarterStates = machineInfo.tsPerFrame / 4;
    int lowTStates = quarterStates - (z80Core.GetTStates() % quarterStates) - 1;
    snapData.data[55] = lowTStates & 0xff;
    snapData.data[56] = lowTStates >> 8;
    
    snapData.data[57] = ((z80Core.GetTStates() / quarterStates) + 3) % 4;
    snapData.data[58] = 0; // QL Emu
    snapData.data[59] = 0; // MGT Paged ROM
    snapData.data[60] = 0; // Multiface ROM paged
    snapData.data[61] = 0; // 0 - 8192 ROM
    snapData.data[63] = 0; // 8192 - 16384 ROM
    snapData.data[83] = 0; // MGT Type
    snapData.data[84] = 0; // Disciple inhibit button
    snapData.data[85] = 0; // Disciple inhibit flag
    
    int snapPtr = cZ80_V3_HEADER_SIZE;
    
    if (machineInfo.machineType == eZXSpectrum48)
    {
        snapData.data[snapPtr++] = 0xff;
        snapData.data[snapPtr++] = 0xff;
        snapData.data[snapPtr++] = 4;
        
        for (int memAddr = 0x8000; memAddr <= 0xbfff; memAddr++)
        {
            snapData.data[snapPtr++] = z80Core.Z80CoreDebugMemRead(memAddr, NULL);
        }
        
        snapData.data[snapPtr++] = 0xff;
        snapData.data[snapPtr++] = 0xff;
        snapData.data[snapPtr++] = 5;
        
        for (int memAddr = 0xc000; memAddr <= 0xffff; memAddr++)
        {
            snapData.data[snapPtr++] = z80Core.Z80CoreDebugMemRead(memAddr, NULL);
        }
        
        snapData.data[snapPtr++] = 0xff;
        snapData.data[snapPtr++] = 0xff;
        snapData.data[snapPtr++] = 8;
        
        for (int memAddr = 0x4000; memAddr <= 0x7fff; memAddr++)
        {
            snapData.data[snapPtr++] = z80Core.Z80CoreDebugMemRead(memAddr, NULL);
        }
    }
    else if (machineInfo.machineType == eZXSpectrum128)
    {
        // 128k/Next
        for (int page = 0; page < 8; page++)
        {
            snapData.data[snapPtr++] = 0xff;
            snapData.data[snapPtr++] = 0xff;
            snapData.data[snapPtr++] = page + 3;
            
            for (int memAddr = page * 0x4000; memAddr < (page * 0x4000) + 0x4000; memAddr++)
            {
                snapData.data[snapPtr++] = memoryRam[memAddr];
            }
        }
    }
    
    resume();
    
    return snapData;
}

bool ZXSpectrum::snapshotZ80LoadWithPath(const char *path)
{
    FILE *fileHandle;

    fileHandle = fopen(path, "rb");
    
    if (!fileHandle)
    {
        cout << "ERROR LOADING Z80 SNAPSHOT: " << path << endl;
        fclose(fileHandle);
        return false;
    }
    
    pause();
    displayFrameReset();
    displayClear();
    audioReset();
    
    fseek(fileHandle, 0, SEEK_END);
    long size = ftell(fileHandle);
    fseek(fileHandle, 0, SEEK_SET);

    unsigned char fileBytes[ size ];
    
    fread(&fileBytes, 1, size, fileHandle);
    
    // Decode the header
    unsigned short headerLength = ((unsigned short *)&fileBytes[30])[0];
    int version;
    unsigned char hardwareType;
    unsigned short pc;
    
    switch (headerLength) {
        case 23:
            version = 2;
            pc = ((unsigned short *)&fileBytes[32])[0];
            break;
        case 54:
        case 55:
            version = 3;
            pc = ((unsigned short *)&fileBytes[32])[0];
            break;
        default:
            version = 1;
            pc = ((unsigned short *)&fileBytes[6])[0];
            break;
    }
    
    if (pc == 0)
    {
        version = 2;
        pc = ((unsigned short *)&fileBytes[32])[0];
    }
    
    cout << "Loading Z80 snapshot v" << version << ": " << path << endl;
    
    z80Core.SetRegister(CZ80Core::eREG_A, (unsigned char)fileBytes[0]);
    z80Core.SetRegister(CZ80Core::eREG_F, (unsigned char)fileBytes[1]);
    z80Core.SetRegister(CZ80Core::eREG_BC, ((unsigned short *)&fileBytes[2])[0]);
    z80Core.SetRegister(CZ80Core::eREG_HL, ((unsigned short *)&fileBytes[2])[1]);
    z80Core.SetRegister(CZ80Core::eREG_PC, pc);
    z80Core.SetRegister(CZ80Core::eREG_SP, ((unsigned short *)&fileBytes[8])[0]);
    z80Core.SetRegister(CZ80Core::eREG_I, (unsigned char)fileBytes[10]);
    z80Core.SetRegister(CZ80Core::eREG_R, (fileBytes[11] & 127) | ((fileBytes[12] & 1) << 7));
    
    // Decode byte 12
    //    Bit 0  : Bit 7 of the R-register
    //    Bit 1-3: Border colour
    //    Bit 4  : 1=Basic SamRom switched in
    //    Bit 5  : 1=Block of data is compressed
    //    Bit 6-7: No meaning
    unsigned char byte12 = fileBytes[12];
    
    // For campatibility reasons if byte 12 = 255 then it should be assumed to = 1
    byte12 = (byte12 == 255) ? 1 : byte12;
    
    displayBorderColor = (byte12 & 14) >> 1;
    bool compressed = byte12 & 32;
    
    z80Core.SetRegister(CZ80Core::eREG_DE, ((unsigned short *)&fileBytes[13])[0]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_BC, ((unsigned short *)&fileBytes[13])[1]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_DE, ((unsigned short *)&fileBytes[13])[2]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_HL, ((unsigned short *)&fileBytes[13])[3]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_A, (unsigned char)fileBytes[21]);
    z80Core.SetRegister(CZ80Core::eREG_ALT_F, (unsigned char)fileBytes[22]);
    z80Core.SetRegister(CZ80Core::eREG_IY, ((unsigned short *)&fileBytes[23])[0]);
    z80Core.SetRegister(CZ80Core::eREG_IX, ((unsigned short *)&fileBytes[23])[1]);
    z80Core.SetIFF1((unsigned char)fileBytes[27] & 1);
    z80Core.SetIFF2((unsigned char)fileBytes[28] & 1);
    z80Core.SetIMMode((unsigned char)fileBytes[29] & 3);
    
    // Load AY register values
//    int fileBytesIndex = 39;
//    for (int i = 0; i < 16; i++)
//    {
//        audioAYSetRegister(i);
//        audioAYWriteData(fileBytes[ fileBytesIndex++ ]);
//    }

//    audioAYSetRegister(fileBytes[38]);

    // Based on the version number of the snapshot, decode the memory contents
    switch (version) {
        case 1:
            snapshotExtractMemoryBlock(fileBytes, 0x4000, 30, compressed, 0xc000);
            break;
            
        case 2:
        case 3:
            hardwareType = ((unsigned char *)&fileBytes[34])[0];
            short additionHeaderBlockLength = 0;
            additionHeaderBlockLength = ((unsigned short *)&fileBytes[30])[0];
            int offset = 32 + additionHeaderBlockLength;
            
            if ( (version == 2 && (hardwareType == cZ80_V2_MACHINE_TYPE_128 || hardwareType == cZ80_V2_MACHINE_TYPE_128_IF1)) ||
                (version == 3 && (hardwareType == cZ80_V3_MACHINE_TYPE_128 || hardwareType == cZ80_V3_MACHINE_TYPE_128_IF1 || hardwareType == cz80_V3_MACHINE_TYPE_128_MGT ||
                                  hardwareType == cZ80_V3_MACHINE_TYPE_128_2)) )
            {
                // Decode byte 35 so that port 0x7ffd can be set on the 128k
                unsigned char data = ((unsigned char *)&fileBytes[35])[0];
                emuDisablePaging = ((data & 0x20) == 0x20) ? true : false;
                emuROMPage = ((data & 0x10) == 0x10) ? 1 : 0;
                emuDisplayPage = ((data & 0x08) == 0x08) ? 7 : 5;
                emuRAMPage = (data & 0x07);
            }
            else
            {
                emuDisablePaging = true;
                emuROMPage = 0;
                emuRAMPage = 0;
                emuDisplayPage = 1;
            }
            
            while (offset < size)
            {
                int compressedLength = ((unsigned short *)&fileBytes[offset])[0];
                bool isCompressed = true;
                if (compressedLength == 0xffff)
                {
                    compressedLength = 0x4000;
                    isCompressed = false;
                }
                
                int pageId = fileBytes[offset + 2];
                
                if (version == 1 || ((version == 2 || version == 3) && (hardwareType == cZ80_V2_MACHINE_TYPE_48 || hardwareType == cZ80_V3_MACHINE_TYPE_48 ||
                                                                        hardwareType == cZ80_V2_MACHINE_TYPE_48_IF1 || hardwareType == cZ80_V3_MACHINE_TYPE_48_IF1)
                                     ))
                {
                    // 48k
                    switch (pageId) {
                        case 4:
                            snapshotExtractMemoryBlock(fileBytes, 0x8000, offset + 3, isCompressed, 0x4000);
                            break;
                        case 5:
                            snapshotExtractMemoryBlock(fileBytes, 0xc000, offset + 3, isCompressed, 0x4000);
                            break;
                        case 8:
                            snapshotExtractMemoryBlock(fileBytes, 0x4000, offset + 3, isCompressed, 0x4000);
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    // 128k
                    snapshotExtractMemoryBlock(fileBytes, (pageId - 3) * 0x4000, offset + 3, isCompressed, 0x4000);
                }
                
                offset += compressedLength + 3;
            }
            break;
    }
    
    resume();

    return true;
}

void ZXSpectrum::snapshotExtractMemoryBlock(unsigned char *fileBytes, int memAddr, int fileOffset, bool isCompressed, int unpackedLength)
{
    int filePtr = fileOffset;
    int memoryPtr = memAddr;
    
    if (!isCompressed)
    {
        while (memoryPtr < unpackedLength + memAddr)
        {
            memoryRam[memoryPtr++] = fileBytes[filePtr++];
        }
    }
    else
    {
        while (memoryPtr < unpackedLength + memAddr)
        {
            unsigned char byte1 = fileBytes[filePtr];
            unsigned char byte2 = fileBytes[filePtr + 1];
            
            if ((unpackedLength + memAddr) - memoryPtr >= 2 && byte1 == 0xed && byte2 == 0xed)
            {
                unsigned char count = fileBytes[filePtr + 2];
                unsigned char value = fileBytes[filePtr + 3];
                for (int i = 0; i < count; i++)
                {
                    memoryRam[memoryPtr++] = value;
                }
                filePtr += 4;
            }
            else
            {
                memoryRam[memoryPtr++] = fileBytes[filePtr++];
            }
        }
    }
}


string ZXSpectrum::snapshotHardwareTypeForVersion(int version, int hardwareType)
{
    string hardware = "Unknown";
    if (version == 2)
    {
        switch (hardwareType) {
            case cZ80_V2_MACHINE_TYPE_48:
                hardware = "48k";
                break;
            case cZ80_V2_MACHINE_TYPE_48_IF1:
                hardware = "48k + Interface 1";
                break;
            case cZ80_V2_MACHINE_TYPE_128:
                hardware = "128k";
                break;
            case cZ80_V2_MACHINE_TYPE_128_IF1:
                hardware = "128k + Interface 1";
                break;
            case 5:
            case 6:
                hardware = "UNKNOWN";
                break;
                
            default:
                break;
        }
    }
    else
    {
        switch (hardwareType) {
            case cZ80_V3_MACHINE_TYPE_48:
                hardware = "48k";
                break;
            case cZ80_V3_MACHINE_TYPE_48_IF1:
                hardware = "48k + Interface 1";
                break;
            case cZ80_V3_MACHINE_TYPE_48_MGT:
                hardware = "48k + M.G.T";
                break;
            case cZ80_V3_MACHINE_TYPE_128:
                hardware = "128k";
                break;
            case cZ80_V3_MACHINE_TYPE_128_IF1:
                hardware = "128k + Interface 1";
                break;
            case cz80_V3_MACHINE_TYPE_128_MGT:
                hardware = "128k + M.G.T";
                break;
                
            default:
                break;
        }
    }
    return hardware;
}

int ZXSpectrum::snapshotMachineInSnapshotWithPath(const char *path)
{
    FILE *fileHandle;
    
    fileHandle = fopen(path, "rb");
    
    if (!fileHandle)
    {
        cout << "ERROR LOADING SNAPSHOT: " << path << endl;
        fclose(fileHandle);
        return -1;
    }
    
    fseek(fileHandle, 0, SEEK_END);
    long size = ftell(fileHandle);
    fseek(fileHandle, 0, SEEK_SET);
    
    unsigned char fileBytes[size];
    
    fread(&fileBytes, 1, size, fileHandle);
    
    // Decode the header
    unsigned short headerLength = ((unsigned short *)&fileBytes[30])[0];
    int version;
    unsigned char hardwareType;
    unsigned short pc;
    
    switch (headerLength) {
        case 23:
            version = 2;
            pc = ((unsigned short *)&fileBytes[32])[0];
            break;
        case 54:
        case 55:
            version = 3;
            pc = ((unsigned short *)&fileBytes[32])[0];
            break;
        default:
            version = 1;
            pc = ((unsigned short *)&fileBytes[6])[0];
            break;
    }
    
    if (pc == 0)
    {
        version = 2;
    }
    
    if (version == 1)
    {
        return eZXSpectrum48;
    }
    
    if (version == 2)
    {
        hardwareType = ((unsigned char *)&fileBytes[34])[0];
        if (hardwareType == 0 || hardwareType == 1)
        {
            return eZXSpectrum48;
        }
        else if (hardwareType == 3 || hardwareType == 4)
        {
            return eZXSpectrum128;
        }
        else
        {
            return eZXSpectrum48;
        }
    }
    
    if (version == 3)
    {
        hardwareType = ((unsigned char *)&fileBytes[34])[0];
        if (hardwareType == cZ80_V3_MACHINE_TYPE_48 || hardwareType == cZ80_V3_MACHINE_TYPE_48_IF1 || hardwareType == cZ80_V3_MACHINE_TYPE_48_MGT)
        {
            return eZXSpectrum48;
        }
        else if (hardwareType == cZ80_V3_MACHINE_TYPE_128 || hardwareType == cZ80_V3_MACHINE_TYPE_128_IF1 || hardwareType == cz80_V3_MACHINE_TYPE_128_MGT)
        {
            return eZXSpectrum128;
        }
        else if (hardwareType == cZ80_V3_MACHINE_TYPE_128_2)
        {
            return eZXSpectrum128_2;
        }
    }
    
    return -1;
}
