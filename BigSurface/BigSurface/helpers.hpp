//
//  helpers.hpp
//  BigSurface
//
//  Created by Xavier on 2023/2/7.
//  Copyright © 2023 Xia Shangning. All rights reserved.
//

#ifndef helpers_hpp
#define helpers_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

#define kIOPMPowerOff                       0
#define kIOPMNumberPowerStates              2

#define BIT(nr) (1UL << (nr))

#ifndef GENMASK
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (64 - 1 - (h))))
#endif

static IOPMPowerState myIOPMPowerStates[kIOPMNumberPowerStates] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#ifdef DEBUG
#define DBG_LOG(str, ...)   LOG("dbg-" str, ##__VA_ARGS__)
#else
#define DBG_LOG(str, ...)   ;
#endif

#endif /* helpers_hpp */
