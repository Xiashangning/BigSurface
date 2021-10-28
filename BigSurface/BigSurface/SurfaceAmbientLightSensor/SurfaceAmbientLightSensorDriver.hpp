//
//  SurfaceAmbientLightSensorDriver.hpp
//  BigSurface
//
//  Created by Xia on 2021/10/27.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef SurfaceAmbientLightSensorDriver_hpp
#define SurfaceAmbientLightSensorDriver_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "../../../Dependencies/VoodooGPIO/VoodooGPIO/VoodooGPIO.hpp"
#include "../../../Dependencies/helpers.hpp"
#include "../../../Dependencies/VoodooI2CACPIResourcesParser/VoodooI2CACPIResourcesParser.hpp"
#include "../VoodooI2C/VoodooI2CDevice/VoodooI2CDeviceNub.hpp"

#include "APDS9960Constants.h"

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

#define POLLING_INTERVAL 500

class EXPORT SurfaceAmbientLightSensorDriver : public IOService {
    OSDeclareDefaultStructors(SurfaceAmbientLightSensorDriver);
    
public:
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;
    
private:
    IOACPIPlatformDevice* acpi_device {nullptr};
    VoodooI2CDeviceNub* api {nullptr};
    IOWorkLoop* work_loop {nullptr};
    IOCommandGate* command_gate {nullptr};
    IOTimerEventSource* poller {nullptr};
    bool is_interrupt_started = false;
    
    bool awake {true};
    
    void pollALI(OSObject* owner, IOTimerEventSource *timer);
    
    IOReturn initDevice();
    
    IOReturn configDevice(UInt8 func, bool enable);
    
    void releaseResources();
    
    IOReturn readRegister(UInt8 reg, UInt8* values, size_t len);
    
    IOReturn writeRegister(UInt8 reg, UInt8 cmd);
};

#endif /* SurfaceAmbientLightSensorDriver_hpp */
