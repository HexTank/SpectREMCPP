//
//  DebugViewController.m
//  SpectREM
//
//  Created by Mike on 02/01/2018.
//  Copyright © 2018 71Squared Ltd. All rights reserved.
//

#import "DebugViewController.h"
#include "ZXSpectrum.hpp"
#include "Z80Core.h"

@interface DebugViewController ()

@property (assign) unsigned short disassembleAddress;
@property (strong) NSMutableArray *disassemblyArray;
@property (strong) NSMutableArray *stackArray;
@property (strong) NSMutableArray *breakpointsArray;
@property (assign) unsigned int byteWidth;
@property (assign) int memoryTableSearchAddress;
@property (strong) NSDictionary *z80ByteRegisters;
@property (strong) NSDictionary *z80WordRegisters;
@property (assign) NSInteger colWidth1;
@property (assign) NSInteger colWidth2;
@property (strong) NSTimer *updateTimer;
@end

@implementation DebugViewController

- (instancetype)initWithCoder:(NSCoder *)coder
{
    if ([super initWithCoder:coder])
    {
        NSLog(@"DEBUG_VIEW_CONTROLLER INIT");
        self.byteWidth = 12;
        self.disassembleAddress = 0;
        self.memoryTableSearchAddress = -1;
        self.breakpointsArray = [NSMutableArray new];
        
        self.z80ByteRegisters = @{@"A" : @(CZ80Core::eREG_A),
                                  @"F" : @(CZ80Core::eREG_F),
                                  @"B" : @(CZ80Core::eREG_B),
                                  @"C" : @(CZ80Core::eREG_C),
                                  @"D" : @(CZ80Core::eREG_D),
                                  @"E" : @(CZ80Core::eREG_E),
                                  @"H" : @(CZ80Core::eREG_H),
                                  @"L" : @(CZ80Core::eREG_L),
                                  @"A'" : @(CZ80Core::eREG_ALT_A),
                                  @"F'" : @(CZ80Core::eREG_ALT_F),
                                  @"B'" : @(CZ80Core::eREG_ALT_B),
                                  @"C'" : @(CZ80Core::eREG_ALT_C),
                                  @"D'" : @(CZ80Core::eREG_ALT_D),
                                  @"E'" : @(CZ80Core::eREG_ALT_E),
                                  @"H'" : @(CZ80Core::eREG_ALT_H),
                                  @"L'" : @(CZ80Core::eREG_ALT_L),
                                  @"I" : @(CZ80Core::eREG_I),
                                  @"R" : @(CZ80Core::eREG_R)
                                  };
        
        self.z80WordRegisters = @{
                                  @"AF" : @(CZ80Core::eREG_AF),
                                  @"BC" : @(CZ80Core::eREG_BC),
                                  @"DE" : @(CZ80Core::eREG_DE),
                                  @"HL" : @(CZ80Core::eREG_HL),
                                  @"AF'" : @(CZ80Core::eREG_ALT_AF),
                                  @"BC'" : @(CZ80Core::eREG_ALT_BC),
                                  @"DE'" : @(CZ80Core::eREG_ALT_DE),
                                  @"HL'" : @(CZ80Core::eREG_ALT_HL),
                                  @"PC" : @(CZ80Core::eREG_PC),
                                  @"SP" : @(CZ80Core::eREG_SP),
                                  };
        
        return self;
    }
    
    return nil;
}

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do view setup here.
    
    self.decimalFormat = NO;
    self.disassemblyTableview.enclosingScrollView.wantsLayer = YES;
    self.disassemblyTableview.enclosingScrollView.layer.cornerRadius = 5;
    self.stackTable.enclosingScrollView.wantsLayer = YES;
    self.stackTable.enclosingScrollView.layer.cornerRadius = 6;
    self.breakpointTableView.enclosingScrollView.wantsLayer = YES;
    self.breakpointTableView.enclosingScrollView.layer.cornerRadius = 6;
    self.memoryTableView.enclosingScrollView.wantsLayer = YES;
    self.memoryTableView.enclosingScrollView.layer.cornerRadius = 6;
    
    self.effectView.material = NSVisualEffectMaterialUltraDark;
    
}

- (void)viewWillDisappear
{
    [[NSNotificationCenter defaultCenter] removeObserver:NSViewFrameDidChangeNotification];
    [[NSNotificationCenter defaultCenter] removeObserver:cCPU_PAUSED_NOTIFICATION];
    [[NSNotificationCenter defaultCenter] removeObserver:cCPU_RESUMED_NOTIFICATION];
    [[NSNotificationCenter defaultCenter] removeObserver:@"READ_BREAKPOINT"];
    [self.updateTimer invalidate];
}

- (void)viewWillAppear
{
    [[NSNotificationCenter defaultCenter] addObserverForName:NSViewFrameDidChangeNotification object:self.memoryTableView.enclosingScrollView queue:NULL usingBlock:^(NSNotification * _Nonnull note) {
        [self updateMemoryTableSize];
    }];

    [self.emulationViewController pauseMachine];

    [self disassemmbleFromAddress:0 length:65535];
    [self updateDisassemblyTable];
    [self updateCPUDetails];
    [self updateStackTable];
    [self updateMemoryTable];
}

#pragma mark - Table View Methods

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    if ([tableView.identifier isEqualToString:@"DisassembleTableView"])
    {
        return self.disassemblyArray.count;
    }
    
    else if ([tableView.identifier isEqualToString:@"MemoryTableView"])
    {
        return (65535 / self.byteWidth) + 1;
    }
    
    else if ([tableView.identifier isEqualToString:@"StackTable"])
    {
        return self.stackArray.count;
    }

    else if ([tableView.identifier isEqualToString:@"BreakpointTableView"])
    {
        return self.breakpointsArray.count;
    }

    return 0;
}

-(NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row
{
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    if (!machine) { return nil; }
    CZ80Core core = machine->z80Core;
    
    // DISASSEMBLY TABLE
    if ([tableView.identifier isEqualToString:@"DisassembleTableView"])
    {
        NSTableCellView *view;
        view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
        if (view)
        {
            if ([tableColumn.identifier isEqualToString:@"AddressColID"])
            {
                int address = [(DisassembledOpcode *)[self.disassemblyArray objectAtIndex:row] address];
                
                if (self.decimalFormat)
                {
                    view.textField.stringValue = [NSString stringWithFormat:@"%05i", address];
                }
                else
                {
                    view.textField.stringValue = [NSString stringWithFormat:@"$%04X", address];
                }
            }
            else if ([tableColumn.identifier isEqualToString:@"BytesColID"])
            {
                view.textField.stringValue = [(DisassembledOpcode *)[self.disassemblyArray objectAtIndex:row] bytes];
            }
            else if ([tableColumn.identifier isEqualToString:@"DisassemblyColID"])
            {
                view.textField.stringValue = [(DisassembledOpcode *)[self.disassemblyArray objectAtIndex:row] instruction];
            }
        }
        
        return view;
    }
    
    // MEMORY TABLE
    if ([tableView.identifier isEqualToString:@"MemoryTableView"])
    {
        NSTableCellView *view;
        view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
        if (view)
        {
            if ([tableColumn.identifier isEqualToString:@"MemoryAddressColID"])
            {
                unsigned short address = row * self.byteWidth;
                
                if (self.decimalFormat)
                {
                    view.textField.stringValue = [NSString stringWithFormat:@"%05i", address];
                }
                else
                {
                    view.textField.stringValue = [NSString stringWithFormat:@"$%04X", address];
                }
            }
            else if ([tableColumn.identifier isEqualToString:@"MemoryBytesColID"])
            {
                NSMutableAttributedString *content = [NSMutableAttributedString new];
                for (unsigned int i = 0; i < self.byteWidth; i++)
                {
                    unsigned int address = ((int)row * self.byteWidth) + i;
                    NSMutableAttributedString *attrString = [[NSMutableAttributedString alloc] initWithString:[NSString stringWithFormat:@"%02X ", (unsigned short)core.Z80CoreDebugMemRead(address, NULL)]];

                    if (self.memoryTableSearchAddress == address)
                    {
                        [attrString addAttribute:NSBackgroundColorAttributeName value:[NSColor colorWithRed:0 green:0.5 blue:0 alpha:1.0] range:NSMakeRange(0, 2)];
                    }
                    else if (address > 0xffff)
                    {
                        [content appendAttributedString:[[NSAttributedString alloc] initWithString:@"   "]];
                    }
                    else if (machine->breakpoints[ address ] & (0x01 | 0x02))
                    {
                        [attrString addAttribute:NSBackgroundColorAttributeName value:[NSColor colorWithRed:0.75 green:0 blue:0 alpha:1.0] range:NSMakeRange(0, 2)];
                    }
                    else if (machine->breakpoints[ address ] & 0x04)
                    {
                        [attrString addAttribute:NSBackgroundColorAttributeName value:[NSColor colorWithRed:0.75 green:0 blue:0.75 alpha:1.0] range:NSMakeRange(0, 2)];
                    }

                    [content appendAttributedString:attrString];
                }
                
                view.textField.attributedStringValue = content;
            }
            else if ([tableColumn.identifier isEqualToString:@"MemoryASCIIColID"])
            {
                NSMutableString *content = [NSMutableString new];
                for (int i = 0; i < self.byteWidth; i++)
                {
                    unsigned char c = core.Z80CoreDebugMemRead((unsigned short)((row * self.byteWidth) + i), NULL);
                    if ((c >= 0 && c < 32) || c > 126)
                    {
                        [content appendString:@"."];
                    }
                    else
                    {
                        NSString *character = [NSString stringWithFormat:@"%c", c];
                        [content appendString:character];
                    }
                }
                view.textField.stringValue = content;
            }
        }
        
        return view;
        
    }
    
    // STACK TABLE
    if ([tableView.identifier isEqualToString:@"StackTable"])
    {
        NSTableCellView *view;
        view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
        if (view)
        {
            view.textField.stringValue = [NSString stringWithFormat:@"%04X", [[self.stackArray objectAtIndex:row] unsignedShortValue]];
        }
        
        return view;
        
    }
    
    // BREAKPOINT TABLE
    if ([tableView.identifier isEqualToString:@"BreakpointTableView"])
    {
        NSTableCellView *view;
        view = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
        if (view)
        {
            if ([tableColumn.identifier isEqualToString:@"BreakpointAddressColID"])
            {
                Breakpoint *bp = [self.breakpointsArray objectAtIndex:row];
                
                if (self.decimalFormat)
                {
                    view.textField.stringValue = [NSString stringWithFormat:@"%05i", bp.address];
                }
                else
                {
                    view.textField.stringValue = [NSString stringWithFormat:@"$%04X", bp.address];
                }
            }
            else if ([tableColumn.identifier isEqualToString:@"BreakpointConditionColID"])
            {
                Breakpoint *bp = [self.breakpointsArray objectAtIndex:row];
                view.textField.stringValue = bp.condition;
            }
        }
        
        return view;
    }
    
    return nil;
}

- (void)tableView:(NSTableView *)tableView didAddRowView:(NSTableRowView *)rowView forRow:(NSInteger)row
{
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    if (!machine) { return; }
    CZ80Core core = machine->z80Core;

    if ([tableView.identifier isEqualToString:@"DisassembleTableView"])
    {
        NSColor *rowColor = [NSColor clearColor];
        
        uint16_t address = [(DisassembledOpcode *)[self.disassemblyArray objectAtIndex:row] address];

        if (machine->breakpoints[ address ])
        {
            rowColor =  [NSColor colorWithRed:0.6 green:0 blue:0 alpha:1.0];
        }

        if (core.GetRegister(CZ80Core::eREG_PC) == address)
        {
            rowColor = [NSColor colorWithRed:0 green:0.6 blue:0 alpha:1.0];
        }
        
        rowView.backgroundColor = rowColor;
    }
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification
{
    
}

- (IBAction)disassembleTableDoubleClick:(id)sender {
    
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    NSAssert(machine, @"****> No Machine Instance Found!");
    
    uint16_t address = [(DisassembledOpcode *)[self.disassemblyArray objectAtIndex:self.disassemblyTableview.clickedRow] address];
    
    if (machine->breakpoints[ address ])
    {
        machine->breakpoints[ address ] = 0;
        [self removeBreakpointAtAddress:address];
    }
    else
    {
        machine->breakpoints[ address ] = 0x04;
        [self addBreakpointAtAddress:address operation:0x04];
    }
    
    [self reloadDisassemblyData];
    [self.breakpointTableView reloadData];
    [self.memoryTableView reloadData];
}

- (void)addBreakpointAtAddress:(uint16_t)address operation:(uint8_t)operation
{
    Breakpoint *bp = [Breakpoint new];
    bp.address = address;
    bp.condition = @"";

    switch (operation) {
        case 0x01:
            bp.condition = [bp.condition stringByAppendingString:@"READ "];
            break;
        case 0x02:
            bp.condition = [bp.condition stringByAppendingString:@"WRITE "];
            break;
        case 0x04:
            bp.condition = [bp.condition stringByAppendingString:@"EXECUTE"];
            break;
            
        default:
            break;
    }
    
    [self.breakpointsArray addObject:bp];
}

- (void)removeBreakpointAtAddress:(uint16_t)address
{
    for (Breakpoint *bp in self.breakpointsArray) {
        if(bp.address == address)
        {
            [self.breakpointsArray removeObject:bp];
            break;
        }
    }
}

#pragma mark - Disassemble

- (void)disassemmbleFromAddress:(int)address length:(int)size
{
    dispatch_async(dispatch_get_main_queue(), ^{
        ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
        NSAssert(machine, @"****> No Machine Instance Found!");
        CZ80Core core = machine->z80Core;
        
        self.disassemblyArray = [NSMutableArray new];

        int pc = address;
        while (pc < address + size)
        {
            char opcode[128];
            int length = core.Debug_Disassemble(opcode, 128, pc, !self.decimalFormat, NULL);
            
            if ( length == 0 )
            {
                // Invalid opcode, so because we don't know what it is just show it as a DB statement
                DisassembledOpcode *instruction = [DisassembledOpcode new];
                instruction.address = pc;
                NSMutableString *bytes = [NSMutableString new];
                [bytes appendFormat:@"%02X ", core.Z80CoreDebugMemRead(pc, NULL)];
                instruction.bytes = bytes;
                instruction.instruction = [NSString stringWithFormat:@"DB $%@", bytes];
                [self.disassemblyArray addObject:instruction];
                pc++;
            }
            else
            {
                DisassembledOpcode *instruction = [DisassembledOpcode new];
                instruction.address = pc;
                
                NSMutableString *bytes = [NSMutableString new];
                for (int i = 0; i <= length - 1; i++)
                {
                    [bytes appendFormat:@"%02X ", core.Z80CoreDebugMemRead(pc + i, NULL)];
                }
                
                instruction.bytes = bytes;
                instruction.instruction = [NSString stringWithCString:opcode encoding:NSUTF8StringEncoding];
                [self.disassemblyArray addObject:instruction];
                pc += length;
            }
        }
    });
}

- (void)updateMemoryTableSize
{
    NSInteger col1Width = self.memoryTableView.enclosingScrollView.frame.size.width * 0.614;
    NSInteger col2Width = self.memoryTableView.enclosingScrollView.frame.size.width * 0.260;
    [self.memoryTableView.tableColumns[1] setWidth:col1Width];
    [self.memoryTableView.tableColumns[2] setWidth:col2Width];
    self.byteWidth = fabs(self.memoryTableView.tableColumns[1].width / 21.66);

    [self.memoryTableView reloadDataForRowIndexes:
     [NSIndexSet indexSetWithIndexesInRange:
      [self.memoryTableView rowsInRect:self.memoryTableView.visibleRect]
      ] columnIndexes:[self.memoryTableView columnIndexesInRect:self.memoryTableView.visibleRect]
     ];
}

#pragma mark - Debug Controls

- (IBAction)step:(id)sender {
    NSAssert(self.emulationViewController, @"****> No EmulationViewController Instance Found!");
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    NSAssert(machine, @"****> No Machine Instance Found!");
    CZ80Core core = machine->z80Core;

    machine->step();

    [self updateDisassemblyTable];
    [self updateCPUDetails];
    [self updateStackTable];
}

- (IBAction)startMachine:(id)sender
{
    [self.emulationViewController startMachine];
}

- (IBAction)pauseMachine:(id)sender
{
    [self.emulationViewController pauseMachine];
    [self updateViewDetails];
    [self updateDisassemblyTable];
}

- (void)controlTextDidEndEditing:(NSNotification *)obj
{
    NSAssert(self.emulationViewController, @"****> No EmulationViewController Instance Found!");
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    NSAssert(machine, @"****> No Machine Instance Found!");
    CZ80Core core = machine->z80Core;

    // Explode the entered command on space
    NSArray *commandList = [[(NSTextField *)obj.object stringValue] componentsSeparatedByString:@" "];
    
    NSString *command = [(NSString *)commandList[0] uppercaseString];
    
    // Check to see if the command is recognised
    if ([command isEqualToString:@"D"])
    {
        NSScanner *scanner = [NSScanner scannerWithString:commandList[1]];
        unsigned int address;
        if ([scanner scanHexInt:&address])
        {
            self.disassembleAddress = address;
            [self disassemmbleFromAddress:self.disassembleAddress length:65536 - self.disassembleAddress];
            [self reloadDisassemblyData];
        }
        else if ([[commandList[1] uppercaseString]isEqualToString:@"PC"])
        {
            self.disassembleAddress = core.GetRegister(CZ80Core::eREG_PC);
            [self disassemmbleFromAddress:self.disassembleAddress length:65536 - self.disassembleAddress];
            [self reloadDisassemblyData];
        }
        else if ([[commandList[1] uppercaseString]isEqualToString:@"SP"])
        {
            if (self.stackArray.count > 0)
            {
                self.disassembleAddress = [self.stackArray[0] unsignedShortValue];
                [self disassemmbleFromAddress:self.disassembleAddress length:65536 - self.disassembleAddress];
                [self reloadDisassemblyData];
            }
        }
    }
    else if ([command isEqualToString:@"R"])
    {
        [self.emulationViewController startMachine];
    }
    else if ([command isEqualToString:@"P"])
    {
        [self.emulationViewController pauseMachine];
        [self updateDisassemblyTable];
        [self updateViewDetails];
    }
    else if ([command isEqualToString:@"S"])
    {
        [self.emulationViewController pauseMachine];
        [self step:nil];
        [self updateDisassemblyTable];
        [self updateViewDetails];
        [[NSNotificationCenter defaultCenter] postNotificationName:cDISPLAY_UPDATE_NOTIFICATION object:NULL];

    }
    else if ([command isEqualToString:@"M"])
    {
        NSScanner *scanner = [NSScanner scannerWithString:commandList[1]];
        unsigned int address;
        if ([scanner scanHexInt:&address])
        {
            self.memoryTableSearchAddress = address;
            NSUInteger row = (address / self.byteWidth);
            [self updateMemoryTable];
            [self.memoryTableView scrollRowToVisible:row];
        }
    }
    else if ([command isEqualToString:@"WM"])
    {
        NSScanner *scanner = [NSScanner scannerWithString:commandList[1]];
        unsigned int address;
        if ([scanner scanHexInt:&address])
        {
            scanner = [NSScanner scannerWithString:commandList[2]];
            unsigned int value;
            if ([scanner scanHexInt:&value])
            {
                self.memoryTableSearchAddress = address;
                core.Z80CoreDebugMemWrite(address, value, NULL);
                NSUInteger row = (address / self.byteWidth);
                [self updateViewDetails];
                [self.memoryTableView scrollRowToVisible:row];
                [[NSNotificationCenter defaultCenter] postNotificationName:cDISPLAY_UPDATE_NOTIFICATION object:NULL];
            }
        }
    }
    else if ([command isEqualToString:@"SR"])
    {
        NSString *reg = [commandList[1] uppercaseString];
        for (NSString *key in self.z80ByteRegisters)
        {
            if ([key isEqualToString:reg])
            {
                NSScanner *scanner = [NSScanner scannerWithString:commandList[2]];
                unsigned int value;
                if ([scanner scanHexInt:&value])
                {
                    core.SetRegister((CZ80Core::eZ80BYTEREGISTERS)[self.z80ByteRegisters[key] integerValue], value);
                }
                [self updateCPUDetails];
                return;
            }
        }
        
        for (NSString *key in self.z80WordRegisters)
        {
            if ([key isEqualToString:reg])
            {
                NSScanner *scanner = [NSScanner scannerWithString:commandList[2]];
                unsigned int value;
                if ([scanner scanHexInt:&value])
                {
                    core.SetRegister((CZ80Core::eZ80WORDREGISTERS)[self.z80WordRegisters[key] integerValue], value);
                    NSLog(@"%u", (CZ80Core::eZ80WORDREGISTERS)[self.z80WordRegisters[key] integerValue]);
                }
                [self updateCPUDetails];
                return;
            }
        }
        
    }
    else if ([command isEqualToString:@"RF"])
    {
        [self.emulationViewController pauseMachine];
        machine->generateFrame();
        [self updateDisassemblyTable];
        [self updateViewDetails];
        [[NSNotificationCenter defaultCenter] postNotificationName:cDISPLAY_UPDATE_NOTIFICATION object:NULL];
    }
    else if ([command isEqualToString:@"BP"])
    {
        if ([[commandList[1] uppercaseString] isEqualToString:@"X"])
        {
            NSScanner *scanner = [NSScanner scannerWithString:commandList[2]];
            unsigned int value;
            if ([scanner scanHexInt:&value])
            {
                machine->breakpoints[ value ] = machine->breakpoints[ value ] | 0x04;
                [self addBreakpointAtAddress:value operation:0x04];
                [self.breakpointTableView reloadData];
                [self.memoryTableView reloadData];
            }
        }
        else if ([[commandList[1] uppercaseString] isEqualToString:@"R"])
        {
            NSScanner *scanner = [NSScanner scannerWithString:commandList[2]];
            unsigned int value;
            if ([scanner scanHexInt:&value])
            {
                machine->breakpoints[ value ] = machine->breakpoints[ value ] | 0x01;
                [self addBreakpointAtAddress:value operation:0x01];
                [self.breakpointTableView reloadData];
                [self.memoryTableView reloadData];
            }
        }
        else if ([[commandList[1] uppercaseString] isEqualToString:@"W"])
        {
            NSScanner *scanner = [NSScanner scannerWithString:commandList[2]];
            unsigned int value;
            if ([scanner scanHexInt:&value])
            {
                machine->breakpoints[ value ] = machine->breakpoints[ value ] | 0x02;
                [self addBreakpointAtAddress:value operation:0x02];
                [self.breakpointTableView reloadData];
                [self.memoryTableView reloadData];
            }
        }
    }
}

#pragma mark - View Updates

- (void)breakpointHitAddress:(unsigned short)address operation:(uint8_t)operation
{
    NSLog(@"%i - %i", address, operation);
    
    self.disassembleAddress = address;
    [self disassemmbleFromAddress:self.disassembleAddress length:65536 - self.disassembleAddress];
    [self updateDisassemblyTable];
    [self reloadDisassemblyData];
    [self updateViewDetails];
    [self.emulationViewController updateDisplay];
}

- (void)reloadDisassemblyData
{
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.disassemblyTableview reloadData];
    });
}

- (void)updateViewDetails
{
    [self updateCPUDetails];
    [self updateMemoryTable];
    [self updateStackTable];
}

- (void)updateDisassemblyTable
{
    NSAssert(self.emulationViewController, @"****> No EmulationViewController Instance Found!");
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    NSAssert(machine, @"****> No Machine Instance Found!");
    CZ80Core core = machine->z80Core;

    dispatch_async(dispatch_get_main_queue(), ^{

        NSRect visibleRect = self.disassemblyTableview.visibleRect;
        NSRange range = [self.disassemblyTableview rowsInRect:visibleRect];
        
        if (range.length != 0)
        {
            for (NSUInteger i = range.location; i < range.location + range.length - 1; i++)
            {
                DisassembledOpcode *instruction = [self.disassemblyArray objectAtIndex:i];
                if (instruction.address == core.GetRegister(CZ80Core::eREG_PC))
                {
                    [self reloadDisassemblyData];
                    [self.disassemblyTableview deselectAll:NULL];
                    [self.disassemblyTableview scrollRowToVisible:i];
                    return;
                }
            }
        }
        self.disassembleAddress = core.GetRegister(CZ80Core::eREG_PC);
        [self disassemmbleFromAddress:self.disassembleAddress length:65536 - self.disassembleAddress];
        [self reloadDisassemblyData];
        [self.disassemblyTableview scrollRowToVisible:0];
    });

}

- (void)updateCPUDetails
{
    NSAssert(self.emulationViewController, @"****> No EmulationViewController Instance Found!");
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    NSAssert(machine, @"****> No Machine Instance Found!");
    CZ80Core core = machine->z80Core;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!self.decimalFormat)
        {
            self.pc = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_PC)];
            self.sp = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_SP)];
            
            self.af = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_AF)];
            self.bc = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_BC)];
            self.de = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_DE)];
            self.hl = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_HL)];
            
            self.a_af = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_ALT_AF)];
            self.a_bc = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_ALT_BC)];
            self.a_de = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_ALT_DE)];
            self.a_hl = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_ALT_HL)];
            
            self.i = [NSString stringWithFormat:@"$%02X", core.GetRegister(CZ80Core::eREG_I)];
            self.r = [NSString stringWithFormat:@"$%02X", core.GetRegister(CZ80Core::eREG_R)];
            
            self.ix = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_IX)];
            self.iy = [NSString stringWithFormat:@"$%04X", core.GetRegister(CZ80Core::eREG_IY)];
        }
        else
        {
            self.pc = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_PC)];
            self.sp = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_SP)];
            
            self.af = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_AF)];
            self.bc = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_BC)];
            self.de = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_DE)];
            self.hl = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_HL)];
            
            self.a_af = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_ALT_AF)];
            self.a_bc = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_ALT_BC)];
            self.a_de = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_ALT_DE)];
            self.a_hl = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_ALT_HL)];
            
            self.i = [NSString stringWithFormat:@"$%02i", core.GetRegister(CZ80Core::eREG_I)];
            self.r = [NSString stringWithFormat:@"$%02i", core.GetRegister(CZ80Core::eREG_R)];
            
            self.ix = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_IX)];
            self.iy = [NSString stringWithFormat:@"$%04i", core.GetRegister(CZ80Core::eREG_IY)];
        }
        
        self.currentRom = [NSString stringWithFormat:@"%02i", machine->emuROMPage];
        self.displayPage = [NSString stringWithFormat:@"%02i", machine->emuDisplayPage];
        self.ramPage = [NSString stringWithFormat:@"%02i", machine->emuRAMPage];
        self.iff1 = [NSString stringWithFormat:@"%02i", core.GetIFF1()];
        self.im = [NSString stringWithFormat:@"%02i", core.GetIMMode()];
        
        self.tStates = [NSString stringWithFormat:@"%04i", core.GetTStates()];
        
        self.fs = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_S) ? @"1" : @"0";
        self.fz = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_Z) ? @"1" : @"0";
        self.f5 = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_5) ? @"1" : @"0";
        self.fh = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_H) ? @"1" : @"0";
        self.f3 = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_3) ? @"1" : @"0";
        self.fpv = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_P) ? @"1" : @"0";
        self.fn = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_N) ? @"1" : @"0";
        self.fc = (core.GetRegister(CZ80Core::eREG_F) & core.FLAG_C) ? @"1" : @"0";
    });
    

}

- (void)updateStackTable
{
    
    NSAssert(self.emulationViewController, @"****> No EmulationViewController Instance Found!");
    ZXSpectrum *machine = (ZXSpectrum *)[self.emulationViewController getCurrentMachine];
    NSAssert(machine, @"****> No Machine Instance Found!");
    CZ80Core core = machine->z80Core;

    self.stackArray = [NSMutableArray new];
    
    unsigned short sp = core.GetRegister(CZ80Core::eREG_SP);
    
    for (unsigned int i = sp; i <= 0xfffe; i += 2)
    {
        unsigned short address = core.Z80CoreDebugMemRead(i + 1, NULL) << 8;
        address |= core.Z80CoreDebugMemRead(i, NULL);
        [self.stackArray addObject:@(address)];
    }
        
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.stackTable reloadData];
    });
}

- (void)updateMemoryTable
{
    dispatch_async(dispatch_get_main_queue(), ^{
        NSRect visibleRect = self.memoryTableView.visibleRect;
        NSRange visibleRows = [self.memoryTableView rowsInRect:visibleRect];
        NSIndexSet *visibleCols = [self.memoryTableView columnIndexesInRect:visibleRect];
        [self.memoryTableView reloadDataForRowIndexes:[NSIndexSet indexSetWithIndexesInRange:visibleRows] columnIndexes:visibleCols];
    });
}

@end

#pragma mark - Disassembled Instruction Implementation

@implementation DisassembledOpcode



@end

#pragma mark - Breakpoint Implementation

@implementation Breakpoint


@end

