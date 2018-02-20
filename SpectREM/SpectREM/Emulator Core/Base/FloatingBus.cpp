//
//  ULAFloatingBus.cpp
//  SpectREM
//
//  Created by Michael Daley on 25/08/2017.
//  Copyright © 2017 71Squared Ltd. All rights reserved.
//

#include "ZXSpectrum.hpp"

const uint32_t ZXSpectrum::ULAFloatingBusValues[] = { 0, 0, 1, 2, 1, 2, 0, 0 };

enum
{
    eFloatBusTypeValuePixel = 1,
    eFloatBusTypeValueAttribute = 2
};

/**
 When the Z80 reads from an unattached port, such as 0xFF, it actually reads the data currently on the
 Spectrums ULA data bus. This may happen to be a byte being transferred from screen memory. If the ULA
 is building the border then the bus is idle and the return value is 0xFF, otherwise its possible to
 predict if the ULA is reading a pixel or attribute byte based on the current t-state.
 This routine works out what would be on the ULA bus for a given tstate and returns the result
 **/
uint8_t ZXSpectrum::ULAFloatingBus()
{
    int32_t cpuTs = z80Core.GetTStates() - 1;
    int32_t currentDisplayLine = cpuTs / machineInfo.tsPerLine;
    int32_t currentTs = cpuTs % machineInfo.tsPerLine;
    
    // If the line and tState are within the paper area of the screen then grab the
    // pixel or attribute value which is determined by looking at the current tState
    if (currentDisplayLine >= (machineInfo.pxVertBorder + machineInfo.pxVerticalBlank)
        && currentDisplayLine < (machineInfo.pxVertBorder + machineInfo.pxVerticalBlank + machineInfo.pxVerticalDisplay)
        && currentTs < machineInfo.tsHorizontalDisplay)
    {
        uint8_t ulaValueType = ULAFloatingBusValues[ currentTs & 0x07 ];
        
        int32_t y = currentDisplayLine - (machineInfo.pxVertBorder + machineInfo.pxVerticalBlank);
        int32_t x = currentTs >> 2;
        
        if (ulaValueType == eFloatBusTypeValuePixel)
        {
            return memoryRam[ (cBITMAP_ADDRESS) + displayLineAddrTable[y] + x ];
        }
        
        if (ulaValueType == eFloatBusTypeValueAttribute)
        {
            return memoryRam[ (cBITMAP_ADDRESS) + cBITMAP_SIZE + ((y >> 3) << 5) + x ];
        }
    }
    
    return 0xff;
}
