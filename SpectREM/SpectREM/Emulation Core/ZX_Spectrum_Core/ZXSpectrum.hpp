//
//  ZXSpectrum.hpp
//  z80test
//
//  Created by Michael Daley on 30/07/2017.
//  Copyright © 2017 Mike Daley. All rights reserved.
//

#ifndef ZXSpectrum_hpp
#define ZXSpectrum_hpp

#define QT = true

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>

#ifdef QT_SPECTRUM
#include <QFile>
#endif

#include "../Z80_Core/Z80Core.h"
#include "MachineInfo.h"
#include "../Tape/Tape.hpp"

using namespace std;

// - Base ZXSpectrum class

typedef enum {
    eULAPlusPaletteGroup = 0,
    eULAPlusModeGroup
} ULAPlusMode;

class ZXSpectrum
{

public:
    static const uint16_t    cBITMAP_ADDRESS    = 16384;
    static const uint16_t    cBITMAP_SIZE       = 6144;
    static const uint16_t    cATTR_SIZE         = 768;
    static const uint16_t    cMEMORY_PAGE_SIZE  = 16384;
    
    enum
    {
        eAYREGISTER_A_FINE = 0,
        eAYREGISTER_A_COARSE,
        eAYREGISTER_B_FINE,
        eAYREGISTER_B_COARSE,
        eAYREGISTER_C_FINE,
        eAYREGISTER_C_COARSE,
        eAYREGISTER_NOISEPER,
        eAYREGISTER_ENABLE,
        eAYREGISTER_A_VOL,
        eAYREGISTER_B_VOL,
        eAYREGISTER_C_VOL,
        eAYREGISTER_E_FINE,
        eAYREGISTER_E_COARSE,
        eAYREGISTER_E_SHAPE,
        eAYREGISTER_PORT_A,
        eAYREGISTER_PORT_B,
        
        // Used to emulate the odd floating behaviour of setting an AY register > 15. The value
        // written to registers > 15 decays over time and this is the value returned when reading
        // a register > 15
        eAYREGISTER_FLOATING,
        
        eAY_MAX_REGISTERS
    };
    
    // ULAPlus mode values
    enum
    {
        eULAplusPaletteGroup,
        eULAplusModeGroup
    };
    
    // Debug operation type
    enum
    {
        eDebugReadOp = 0x01,
        eDebugWriteOp = 0x02,
        eDebugExecuteOp = 0x04
    };
    
    enum class ZXSpectrumKey
    {
        __NoKey,
        Key_0,
        Key_1,
        Key_2,
        Key_3,
        Key_4,
        Key_5,
        Key_6,
        Key_7,
        Key_8,
        Key_9,
        Key_A,
        Key_B,
        Key_C,
        Key_D,
        Key_E,
        Key_F,
        Key_G,
        Key_H,
        Key_I,
        Key_J,
        Key_K,
        Key_L,
        Key_M,
        Key_N,
        Key_O,
        Key_P,
        Key_Q,
        Key_R,
        Key_S,
        Key_T,
        Key_U,
        Key_V,
        Key_W,
        Key_X,
        Key_Y,
        Key_Z,
        Key_Shift,
        Key_Enter,
        Key_Space,
        Key_SymbolShift,
        Key_InvVideo,
        Key_TrueVideo,
        Key_Quote,
        Key_SemiColon,
        Key_Comma,
        Key_Minus,
        Key_Plus,
        Key_Period,
        Key_Edit,
        Key_Graph,
        Key_Break,
        Key_Backspace,
        Key_ArrowUp,
        Key_ArrowDown,
        Key_ArrowLeft,
        Key_ArrowRight,
        Key_ExtendMode,
        Key_CapsLock,
    };


private:
    // Holds details of the host platforms key codes and how they map to the spectrum keyboard matrix
    typedef struct
    {
        ZXSpectrumKey   key;
        int             mapEntry1;
        int             mapBit1;
        int             mapEntry2;
        int             mapBit2;
    } KEYBOARD_ENTRY;
    
public:
    // Holds the data returned when creating a Snapshot or Z80 snapshot
    struct Snap {
        int32_t     length = 0;
        uint8_t     *data = nullptr;
    };
    
    // Breakpoint information
    struct Breakpoint {
        uint16_t    address = 0;
        bool        breakPoint = 0;
    };
    
    typedef struct
    {
        float r;
        float g;
        float b;
        float a;
    } Color;

    
public:
    ZXSpectrum();
    virtual ~ZXSpectrum();

public:
    virtual void            initialise(string romPath);
    virtual void            resetMachine(bool hard = true);
    virtual void            resetToSnapLoad() = 0;
    void                    pause();
    void                    resume();
    virtual void            release();

    // Main function that when called generates an entire frame, which includes processing interrupts, beeper sound and AY Sound.
    // On completion the displayBuffer member variable will contain RGBA formatted image data that can then be used to build a display image
    void                    generateFrame();
    
    void                    keyboardKeyDown(ZXSpectrumKey key);
    void                    keyboardKeyUp(ZXSpectrumKey key);
    void                    keyboardFlagsChanged(uint64_t flags, ZXSpectrumKey key);
    
    bool                    snapshotZ80LoadWithPath(const char *path);
    bool                    snapshotSNALoadWithPath(const char *path);
    int                     snapshotMachineInSnapshotWithPath(const char *path);
    Snap                    snapshotCreateSNA();
    Snap                    snapshotCreateZ80();
    
    void                    step();
    
    void                    registerDebugOpCallback(std::function<bool(uint16_t, uint8_t)> debugOpCallbackBlock);
    std::function<bool(uint16_t, uint8_t)>    debugOpCallbackBlock = nullptr;
    
    void                   *getScreenBuffer();
    uint32_t               getLastAudioBufferIndex() { return audioLastIndex; }

protected:
    void                    emuReset();
    void                    loadROM(const char *rom, uint32_t page);
    
    void                    displayFrameReset();
    void                    displayUpdateWithTs(int32_t tStates);

    void                    ULAApplyIOContention(uint16_t address, bool contended);
    void                    ULABuildFloatingBusTable();
    uint8_t                 ULAFloatingBus();

    void                    audioAYSetRegister(uint8_t reg);
    void                    audioAYWriteData(uint8_t data);
    uint8_t                 audioAYReadData();
    void                    audioAYUpdate();
    void                    audioReset();
    void                    audioUpdateWithTs(uint32_t tStates);
    void                    audioDecayAYFloatingRegister();
    
private:
    void                    displayBuildTsTable();
    void                    displayBuildLineAddressTable();
    void                    displayBuildCLUT();
    void                    ULABuildContentionTable();
    void                    audioBuildAYVolumesTable();
    void                    keyboardCheckCapsLockStatus();
    void                    keyboardMapReset();
    string                  snapshotHardwareTypeForVersion(uint32_t version, uint32_t hardwareType);
    void                    snapshotExtractMemoryBlock(uint8_t *fileBytes, uint32_t memAddr, uint32_t fileOffset, bool isCompressed, uint32_t unpackedLength);
    void                    displaySetup();
    void                    displayClear();
    void                    audioSetup(double sampleRate, double fps);
    
    // Core memory/IO functions
    static uint8_t          zxSpectrumMemoryRead(uint16_t address, void *param);
    static void             zxSpectrumMemoryWrite(uint16_t address, uint8_t data, void *param);
    static void             zxSpectrumMemoryContention(uint16_t address, uint32_t tStates, void *param);
    static uint8_t          zxSpectrumDebugRead(uint16_t address, void *param, void *m);
    static void             zxSpectrumDebugWrite(uint16_t address, uint8_t byte, void *param, void *data);
    static uint8_t          zxSpectrumIORead(uint16_t address, void *param);
    static void             zxSpectrumIOWrite(uint16_t address, uint8_t data, void *param);

public:
    virtual uint8_t         coreMemoryRead(uint16_t address) = 0;
    virtual void            coreMemoryWrite(uint16_t address, uint8_t data) = 0;
    virtual void            coreMemoryContention(uint16_t address, uint32_t tStates) = 0;
    virtual uint8_t         coreIORead(uint16_t address) = 0;
    virtual void            coreIOWrite(uint16_t address, uint8_t data) = 0;

    virtual uint8_t         coreDebugRead(uint16_t address, void *data) = 0;
    virtual void            coreDebugWrite(uint16_t address, uint8_t byte, void *data) = 0;
        
    // Machine hardware
    CZ80Core                z80Core;
    
    vector<char>            memoryRom;
    vector<char>            memoryRam;
    
    uint8_t                 keyboardMap[8]{0};
    static KEYBOARD_ENTRY   keyboardLookup[];
    uint32_t                keyboardCapsLockFrames = 0;
    
    int16_t                 *audioBuffer = nullptr;
  
public:
    
    // Emulation
    MachineInfo             machineInfo;
    uint32_t                emuCurrentDisplayTs = 0;
    uint32_t                emuFrameCounter = 0;
    bool                    emuPaused = 0;
    uint32_t                emuRAMPage = 0;
    uint32_t                emuROMPage = 0;
    uint32_t                emuDisplayPage = 0;
    bool                    emuDisablePaging = true;
    string                  emuROMPath;
    bool                    emuTapeInstantLoad = 0;
    bool                    emuUseAYSound = 0;
    bool                    emuLoadTrapTriggered = 0;
    bool                    emuSaveTrapTriggered = 0;
    bool                    emuUseSpecDRUM = 0;

    // Display
    uint8_t                 *displayBuffer;
    uint32_t                displayBufferIndex = 0;
    uint32_t                screenWidth = 48 + 256 + 48;
    uint32_t                screenHeight = 48 + 192 + 48;
    uint32_t                screenBufferSize = 0;
    uint32_t                displayTstateTable[312][228]{{0}};
    uint16_t                displayLineAddrTable[192]{0};
    uint64_t                *displayCLUT = nullptr;
    uint8_t                 *displayALUT = nullptr;
    uint32_t                displayBorderColor = 0;
    bool                    displayReady = false;
    Color                   clutBuffer[64];
    
    // ULAPlus
    uint8_t                 ulaPlusMode = 0;
    uint8_t                 ulaPlusPaletteOn = 0;
    uint8_t                 ulaPlusCurrentReg = 0;

    // Audio
    int8_t                  audioEarBit = 0;
    int8_t                  audioMicBit = 0;
    uint32_t                audioBufferSize = 0;
    uint32_t                audioBufferIndex = 0;
    float                   audioTsCounter = 0;
    float                   audioTsStepCounter = 0;
    uint32_t                audioLastIndex = 0;

    double                  audioBeeperTsStep = 0;
    double                  audioOutputLevelLeft = 0;
    double                  audioOutputLevelRight = 0;
	float                   audioAYLevelLeft = 0;
	float                   audioAYLevelRight = 0;
    
    float                   audioAYChannelOutput[3]{0};
    uint32_t                audioAYChannelCount[3]{0};
    uint16_t                audioAYVolumes[16]{0};
    uint32_t                audioAYrandom = 0;
    uint32_t                audioAYOutput = 0;
    uint32_t                audioAYNoiseCount = 0;
    uint32_t                audioATaudioAYEnvelopeCount = 0;
    int32_t                 audioAYaudioAYEnvelopeStep = 0;
    uint8_t                 audioAYRegisters[ eAY_MAX_REGISTERS ]{0};
    uint8_t                 audioAYCurrentRegister = 0;
    uint8_t                 audioAYFloatingRegister = 0;
    bool                    audioAYaudioAYaudioAYEnvelopeHolding = 0;
    bool                    audioAYaudioAYEnvelopeHold = 0;
    bool                    audioAYaudioAYEnvelopeAlt = 0;
    bool                    audioAYEnvelope = 0;
    uint32_t                audioAYAttackEndVol = 0;
    float                   audioAYTsStep = 0;
    float                   audioAYTs = 0;
    
    //Specdrum Peripheral
    int                     specdrumDACValue = 0;
    
    // Keyboard
    bool                    keyboardCapsLockPressed = false;
    
    // ULA
    uint32_t                ULAMemoryContentionTable[80000]{0};
    uint32_t                ULAIOContentionTable[80000]{0};
    uint32_t                ULAFloatingBusTable[80000]{0};
    const static uint32_t   ULAConentionValues[];
    uint8_t                 ULAPortnnFDValue = 0;
    bool                    ULAApplySnow = false;
    uint8_t                 ULAPlusMode = eULAplusModeGroup;
    uint8_t                 ULAPlusCurrentReg = 0;
    uint8_t                 ULAPlusPaletteOn = 0;

    // Floating bus
    const static uint32_t   ULAFloatingBusValues[];
    
    // Tape object
    Tape                    *tape = nullptr;
    
    // SPI port
    uint16_t          spiPort = 0xfaf7;

    bool                    breakpointHit = false;

};

#endif /* ZXSpectrum_hpp */






