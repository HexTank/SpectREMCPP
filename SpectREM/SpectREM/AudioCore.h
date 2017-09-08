//
//  AudioCore.h
//  ZXRetroEmulator
//
//  Created by Mike Daley on 03/09/2016.
//  Copyright © 2016 71Squared Ltd. All rights reserved.
//

#include "ZXSpectrum.hpp"

@class EmulationViewController;

@interface AudioCore : NSObject

@property (assign) double lowPassFilter;
@property (assign) double highPassFilter;
@property (assign) double soundVolume;

#pragma mark - Methods

/*! @method initWithSampleRate:fps
	@abstract
 Initialize the audio engine
	@param sampleRate to be used for audio
	@param fps being rendered which is used to calculate the frame capacity for each audio buffer
 */
- (instancetype)initWithSampleRate:(int)sampleRate framesPerSecond:(float)fps callback:(EmulationViewController *)callback;

- (void)stop;
- (void)start;
- (BOOL)isRunning;

@end
