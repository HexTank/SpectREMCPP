//
//  ZXSpectrum48.cpp
//  SpectREM
//
//  Created by Michael Daley on 23/08/2017.
//  Copyright © 2017 71Squared Ltd. All rights reserved.
//

#include "ZXSpectrum48.hpp"

#include <iostream>
#include <fstream>

#pragma mark - Constants

static const int cROM_SIZE = 16384;
static const char *cDEFAULT_ROM = "48.ROM";
static const char *cSMART_ROM = "snapload.v31";

// SmartCard ROM and sundries
static const uint8_t cFAFB_ROM_SWITCHOUT = 0x40;
static const uint8_t cFAF3_SRAM_ENABLE = 0x80;

unsigned char smartCardPortFAF3 = 0;
unsigned char smartCardPortFAFB = 0;
unsigned char smartCardSRAM[8 * 64 * 1024];		// 8 * 8k banks, mapped @ $2000-$3FFF

#pragma mark - Constructor/Destructor

ZXSpectrum48::ZXSpectrum48(Tape *t) : ZXSpectrum()
{
    cout << "ZXSpectrum48::Constructor" << endl;
    if (t)
    {
        tape = t;
    }
    else
    {
        tape = NULL;
    }
}

ZXSpectrum48::~ZXSpectrum48()
{
    cout << "ZXSpectrum48::Destructor" << endl;
    release();
}

#pragma mark - Initialise

void ZXSpectrum48::initialise(string romPath)
{
    cout << "ZXSpectrum48::initialise(char *rom)" << endl;
    
    machineInfo = machines[ eZXSpectrum48 ];
    ZXSpectrum::initialise( romPath );

    // Register an opcode callback function with the Z80 core so that opcodes can be intercepted
    // when handling things like ROM saving and loading
    z80Core.RegisterOpcodeCallback( ZXSpectrum48::opcodeCallback );
    
    loadROM( cDEFAULT_ROM, 0 );
}

#pragma mark - ULA

unsigned char ZXSpectrum48::coreIORead(unsigned short address)
{
    bool contended = false;
    int memoryPage = address / cMEMORY_PAGE_SIZE;
    if (memoryPage == 1)
    {
        contended = true;
    }
    
    ZXSpectrum::ULAApplyIOContention(address, contended);
        
    // ULA Un-owned ports
    if (address & 0x01)
    {
        // Add Kemptston joystick support. Until then return 0. Byte returned by a Kempston joystick is in the
        // format: 000FDULR. F = Fire, D = Down, U = Up, L = Left, R = Right
        // Joystick is read first as it takes priority if you read from a port that activates the keyboard as well on a
        // real machine.
        if ((address & 0xff) == 0x1f)
        {
            return 0x0;
        }
        
        // AY-3-8912 ports
        else if ((address & 0xc002) == 0xc000 && (machineInfo.hasAY || emuUseAYSound) )
        {
            return audioAYReadData();
        }

        // SPI - HexTank
        if (address == spiPort)
        {
            return ZXSpectrum48::spi_read();
        }
		// Retroleum Smart Card - HexTank
		else if ((address & 0xfff1) == 0xfaf1)
		{
			if(address == 0xfaf3)
			{
				return smartCardPortFAF3;
			}
			else if(address == 0xfafb)
			{
				return smartCardPortFAFB & 0x7f;
			}
		}
		
        // Getting here means that nothing has handled that port read so based on a real Spectrum
        // return the floating bus value
        return ULAFloatingBus();
    }

    // Check to see if the keyboard is being read and if so return any keys currently pressed
    unsigned char result = 0xff;
    if (address & 0xfe)
    {
        for (int i = 0; i < 8; i++)
        {
            if (!(address & (0x100 << i)))
            {
                result &= keyboardMap[i];
            }
        }
    }
    
    result = (result & 191) | (audioEarBit << 6) | (tape->inputBit << 6);
    
    return result;
}

void ZXSpectrum48::coreIOWrite(unsigned short address, unsigned char data)
{
    bool contended = false;
    int memoryPage = address / cMEMORY_PAGE_SIZE;
    if (memoryPage == 1)
    {
        contended = true;
    }
    
    ZXSpectrum::ULAApplyIOContention(address, contended);

    // Port: 0xFE
    //   7   6   5   4   3   2   1   0
    // +---+---+---+---+---+-----------+
    // |   |   |   | E | M |  BORDER   |
    // +---+---+---+---+---+-----------+
    if (!(address & 0x01))
    {
        displayUpdateWithTs((z80Core.GetTStates() - emuCurrentDisplayTs) + machineInfo.borderDrawingOffset);
        audioEarBit = (data & 0x10) >> 4;
        audioMicBit = (data & 0x08) >> 3;
        displayBorderColor = data & 0x07;
    }
    
    // AY-3-8912 ports
    if(address == 0xfffd && (machineInfo.hasAY || emuUseAYSound))
    {
        ULAPortnnFDValue = data;
        audioAYSetRegister(data);
    }
    
    if ((address & 0xc002) == 0x8000 && (machineInfo.hasAY || emuUseAYSound))
    {
        audioAYWriteData(data);
    }

    // SPI - HexTank
    if (address == spiPort)
    {
        return ZXSpectrum48::spi_write(data);
    }
	// Retroleum Smart Card - HexTank
	else if ((address & 0xfff1) == 0xfaf1)
	{
		if(address == 0xfaf3)
		{
			smartCardPortFAF3 = data;
		}
		else if(address == 0xfafb)
		{
			smartCardPortFAFB = data;
		}
	}
}

#pragma mark - Memory Read/Write

void ZXSpectrum48::coreMemoryWrite(unsigned short address, unsigned char data)
{
    if (address < cROM_SIZE)
    {
		if ((smartCardPortFAF3 & 0x80) && address >= 8192 && address < 16384)
		{
			smartCardSRAM[ (address - 8192) + ((smartCardPortFAF3 & 0x07) * 8192) ] = data;
		}
		
        return;
    }
    
    if (address >= cROM_SIZE && address < cBITMAP_ADDRESS + cBITMAP_SIZE + cATTR_SIZE){
        displayUpdateWithTs((z80Core.GetTStates() - emuCurrentDisplayTs) + machineInfo.paperDrawingOffset);
    }

    if (debugOpCallbackBlock != NULL)
    {
        if (breakpoints[ address ] & eDebugWriteOp)
        {
            debugOpCallbackBlock( address, eDebugWriteOp );
        }
    }

    memoryRam[ address ] = data;
}

unsigned char ZXSpectrum48::coreMemoryRead(unsigned short address)
{
    if (address < cROM_SIZE)
    {
		if ((smartCardPortFAF3 & 0x80) && address >= 8192 && address < 16384)
		{
			return smartCardSRAM[  (address - 8192) + ((smartCardPortFAF3 & 0x07) * 8192) ];
		}
		if((address & 0xff) == 0x72)
		{
			if (smartCardPortFAFB & 0x40)
			{
                smartCardPortFAFB &= ~cFAFB_ROM_SWITCHOUT;
                smartCardPortFAF3 &= ~cFAF3_SRAM_ENABLE;
				unsigned char retOpCode = memoryRom[ address ];
                loadROM( cDEFAULT_ROM, 0 );
				return retOpCode;
			}
		}
		
        if (debugOpCallbackBlock != NULL)
        {
            if (breakpoints[ address ] & eDebugReadOp)
            {
                debugOpCallbackBlock( address, eDebugReadOp );
            }
        }

        return memoryRom[address];
    }

    if (debugOpCallbackBlock != NULL)
    {
        if (breakpoints[ address ] & eDebugReadOp)
        {
            debugOpCallbackBlock( address, eDebugReadOp );
        }
    }

    return memoryRam[ address ];
}

#pragma mark - Debug Memory Read/Write

unsigned char ZXSpectrum48::coreDebugRead(unsigned int address, void *data)
{
    if (address < cROM_SIZE)
    {
        return memoryRom[address];
    }
    
    return memoryRam[address];
}

void ZXSpectrum48::coreDebugWrite(unsigned int address, unsigned char byte, void *data)
{
    if (address < cROM_SIZE)
    {
        memoryRom[address] = byte;
    }
    else
    {
        memoryRam[address] = byte;
    }
}

#pragma mark - Memory Contention

void ZXSpectrum48::coreMemoryContention(unsigned short address, unsigned int tStates)
{
    if (address >= 16384 && address <= 32767)
    {
        z80Core.AddContentionTStates( ULAMemoryContentionTable[z80Core.GetTStates() % machineInfo.tsPerFrame] );
    }
}

#pragma mark - Release/Reset

void ZXSpectrum48::release()
{
    ZXSpectrum::release();
}

void ZXSpectrum48::resetMachine(bool hard)
{
    if (hard)
    {
        // If a hard reset is requested, reload the default ROM and make sure that the smart card
        // ROM switch is disabled along with the smart card SRAM
        loadROM( cDEFAULT_ROM, 0 );
        smartCardPortFAFB &= ~cFAFB_ROM_SWITCHOUT;
        smartCardPortFAF3 &= ~cFAF3_SRAM_ENABLE;
    }

    emuDisplayPage = 1;
    ZXSpectrum::resetMachine(hard);
}

void ZXSpectrum48::resetToSnapLoad()
{
    loadROM( cSMART_ROM, 0 );
    resetMachine(false);
}

#pragma mark - Opcode Callback Function

bool ZXSpectrum48::opcodeCallback(unsigned char opcode, unsigned short address, void *param)
{
    ZXSpectrum48 *machine = static_cast<ZXSpectrum48*>(param);
    CZ80Core core = machine->z80Core;
    
    if (machine->emuTapeInstantLoad)
    {
        // Trap ROM tap LOADING
        if (address == 0x056b || address == 0x0111)
        {
            if (opcode == 0xc0)
            {
                machine->emuLoadTrapTriggered = true;
                machine->tape->updateStatus();
                return true;
            }
        }
    }
    
    // Trap ROM tape SAVING
    if (opcode == 0x08 && address == 0x04d0)
    {
        if (opcode == 0x08)
        {
            machine->emuSaveTrapTriggered = true;
            machine->tape->updateStatus();
            return true;
        }
    }

    machine->emuSaveTrapTriggered = false;
    machine->emuLoadTrapTriggered = false;

    return false;
}



