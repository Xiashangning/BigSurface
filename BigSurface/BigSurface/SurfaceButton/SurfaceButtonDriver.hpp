//
//  SurfaceButtonDriver.hpp
//  SurfaceButton
//
//  Created by Xavier on 22/03/2021.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef SurfaceButtonDriver_hpp
#define SurfaceButtonDriver_hpp

#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "../../../Dependencies/VoodooGPIO/VoodooGPIO/VoodooGPIO.hpp"
#include "../../../Dependencies/VoodooSerial/VoodooSerial/ACPIParser/VoodooACPIResourcesParser.hpp"
#include "../helpers.hpp"
#include "SurfaceButtonDevice.hpp"

#define BTN_CNT 3

#define POWER_BUTTON_IDX        0
#define VOLUME_UP_BUTTON_IDX    1
#define VOLUME_DOWN_BUTTON_IDX  2

const char *BTN_DESCRIPTION[BTN_CNT] = {"Power Button", "Volume Up Button", "Volume Down Button"};
const UInt32 BTN_CMD[BTN_CNT] = {kHIDUsage_Csmr_Power, kHIDUsage_Csmr_VolumeIncrement, kHIDUsage_Csmr_VolumeDecrement};
const UInt32 BTN_CMD_PAGE[BTN_CNT] = {kHIDPage_Consumer, kHIDPage_Consumer, kHIDPage_Consumer};

class EXPORT SurfaceButtonDriver : public IOService {
    OSDeclareDefaultStructors(SurfaceButtonDriver);
    
private:
    IOWorkLoop*             work_loop {nullptr};
    VoodooGPIO*             gpio_controller {nullptr};
    IOInterruptEventSource* interrupt_source[BTN_CNT] = {nullptr, nullptr, nullptr};
    IOACPIPlatformDevice*   acpi_device {nullptr};
    SurfaceButtonDevice*    button_device {nullptr};
    
    bool    is_interrupt_started[BTN_CNT] = {false, false, false};
    bool    btn_status[BTN_CNT] = {false, false, false};
    int     gpio_irq[BTN_CNT] = {0,0,0};
    UInt16  gpio_pin[BTN_CNT] = {0,0,0};
    bool    awake {false};    
    
    void startInterrupt(int source);
    
    void stopInterrupt(int source);
    
    void releaseResources();
    
    IOReturn getDeviceResources();
    
    IOReturn parseButtonResources(VoodooACPIResourcesParser* parser1, VoodooACPIResourcesParser* parser2, VoodooACPIResourcesParser* parser3);
    
    VoodooGPIO* getGPIOController();
    
    void powerInterruptOccured(IOInterruptEventSource* src, int intCount);
    
    void volumeUpInterruptOccured(IOInterruptEventSource* src, int intCount);
    
    void volumeDownInterruptOccured(IOInterruptEventSource* src, int intCount);
    
    void response(int btn_idx, bool status);
    
public:
    IOReturn enableInterrupt(int source) override;
    
    IOReturn disableInterrupt(int source) override;

    IOReturn getInterruptType(int source, int *interruptType) override;
    
    IOReturn registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refcon) override;
    
    IOReturn unregisterInterrupt(int source) override;
    
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;
};

#endif
