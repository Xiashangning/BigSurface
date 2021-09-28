//
//  SurfaceButtons.hpp
//  SurfaceButtons
//
//  Created by Xia on 22/03/2021.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef SurfaceButtonDriver_hpp
#define SurfaceButtonDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/hid/IOHIDEventService.h>

#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "../../../Dependencies/VoodooGPIO/VoodooGPIO/VoodooGPIO.hpp"
#include "../../../Dependencies/VoodooI2CACPIResourcesParser/VoodooI2CACPIResourcesParser.hpp"
#include "SurfaceButtonHIDDevice.hpp"

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

#define BTN_CNT 3

#define PWBT_IDX 0
#define VUBT_IDX 1
#define VDBT_IDX 2

const char *BTN_DESCRIPTION[BTN_CNT] = {"Power Button", "Volume Up Button", "Volume Down Button"};
const UInt32 BTN_CMD[BTN_CNT] = {kHIDUsage_Csmr_Power, kHIDUsage_Csmr_VolumeIncrement, kHIDUsage_Csmr_VolumeDecrement};
const UInt32 BTN_CMD_PAGE[BTN_CNT] = {kHIDPage_Consumer, kHIDPage_Consumer, kHIDPage_Consumer};

class EXPORT SurfaceButtonDriver : public IOHIDEventService {
    OSDeclareDefaultStructors(SurfaceButtonDriver);
    
private:
    IOCommandGate* command_gate;
    VoodooGPIO* gpio_controller;
    bool btn_status[BTN_CNT] = {false, false, false};
    int gpio_irq[BTN_CNT] = {0,0,0};
    UInt16 gpio_pin[BTN_CNT] = {0,0,0};
    IOWorkLoop* work_loop {nullptr};
    IOInterruptEventSource* interrupt_source[BTN_CNT] = {nullptr, nullptr, nullptr};
    bool is_interrupt_started[BTN_CNT] = {false, false, false};
    
    IOACPIPlatformDevice* acpi_device {nullptr};
    SurfaceButtonHIDDevice* button_device {nullptr};
    
    void startInterrupt(int source);
    
    void stopInterrupt(int source);
    
    void releaseResources();
    
    IOReturn getDeviceResources();
    
    IOReturn parseButtonResources(VoodooI2CACPIResourcesParser* parser1, VoodooI2CACPIResourcesParser* parser2, VoodooI2CACPIResourcesParser* parser3);
    
    VoodooGPIO* getGPIOController();
    
    void powerInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount);
    
    void volumeUpInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount);
    
    void volumeDownInterruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount);
    
    IOReturn response(void* btn);
    
public:
    
    IOReturn disableInterrupt(int source) override;

    IOReturn enableInterrupt(int source) override;

    IOReturn getInterruptType(int source, int *interruptType) override;
    
    IOReturn registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refcon) override;
    
    IOReturn unregisterInterrupt(int source) override;
    
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
};

#endif
