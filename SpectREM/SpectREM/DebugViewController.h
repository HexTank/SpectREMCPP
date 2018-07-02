//
//  DebugViewController.h
//  SpectREM
//
//  Created by Mike on 02/01/2018.
//  Copyright © 2018 71Squared Ltd. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "EmulationViewController.h"

@interface DebugViewController : NSViewController <NSTableViewDataSource, NSTabViewDelegate, NSTextFieldDelegate, NSSplitViewDelegate>

@property (assign) BOOL decimalFormat;

// Registers
@property (strong, nonatomic) NSString *pc;
@property (strong) NSString *sp;

@property (strong) NSString *af;
@property (strong) NSString *bc;
@property (strong) NSString *de;
@property (strong) NSString *hl;

@property (strong) NSString *a_af;
@property (strong) NSString *a_bc;
@property (strong) NSString *a_de;
@property (strong) NSString *a_hl;

@property (strong) NSString *ix;
@property (strong) NSString *iy;

@property (strong) NSString *i;
@property (strong) NSString *r;

@property (strong) NSString *im;

// Flags
@property (strong) NSString *fs;
@property (strong) NSString *fz;
@property (strong) NSString *f5;
@property (strong) NSString *fh;
@property (strong) NSString *f3;
@property (strong) NSString *fpv;
@property (strong) NSString *fn;
@property (strong) NSString *fc;

// Machine specifics
@property (strong) NSString *currentRom;
@property (strong) NSString *displayPage;
@property (strong) NSString *ramPage;
@property (strong) NSString *iff1;
@property (strong) NSString *tStates;

@property (assign) EmulationViewController *emulationViewController;
@property (weak) IBOutlet NSTableView *disassemblyTableview;
@property (weak) IBOutlet NSTableView *memoryTableView;
@property (weak) IBOutlet NSTableView *stackTable;
@property (strong) NSData *imageData;
@property (strong) NSImage *displayImage;
@property (weak) IBOutlet NSTableView *breakpointTableView;
@property (strong) IBOutlet NSVisualEffectView *effectView;
@property (weak) IBOutlet NSView *displayView;

@property (assign) void *machine;

#pragma mark - Methods

- (void)updateMemoryTableSize;

@end

#pragma mark - Disassembled Instruction Class

@interface DisassembledOpcode : NSObject

@property (assign) int address;
@property (strong) NSString *bytes;
@property (strong) NSString *instruction;

@end

#pragma mark - Breakpoint Class

@interface Breakpoint : NSObject

@property (assign) uint16_t address;
@property (strong) NSString *condition;

@end
