//
//  SurfaceAmbientLightSensorDriver.hpp
//  SurfaceAmbientLightSensor
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
#include "../../../Dependencies/VoodooSerial/VoodooSerial/utils/helpers.hpp"
#include "../../../Dependencies/VoodooSerial/VoodooSerial/utils/VoodooACPIResourcesParser/VoodooACPIResourcesParser.hpp"
#include "./../../Dependencies/VoodooSerial/VoodooSerial/VoodooI2C/VoodooI2CDevice/VoodooI2CDeviceNub.hpp"
#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <Headers/kern_util.hpp>

#include "AmbientLightValue.hpp"
#include "APDS9960Constants.h"

#define POLLING_INTERVAL 1000

class EXPORT SurfaceAmbientLightSensorDriver : public IOService {
    OSDeclareDefaultStructors(SurfaceAmbientLightSensorDriver);
    
public:
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;
    
    static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
    
private:
    IOACPIPlatformDevice*   alsd_device {nullptr};
    VoodooI2CDeviceNub*     api {nullptr};
    IOWorkLoop*             work_loop {nullptr};
    IOTimerEventSource*     poller {nullptr};
    
    bool awake {true};
    
    _Atomic(UInt32) current_lux;
    IONotifier *vsmcNotifier {nullptr};
    static constexpr SMC_KEY KeyAL   = SMC_MAKE_IDENTIFIER('A','L','!',' ');
    static constexpr SMC_KEY KeyALI0 = SMC_MAKE_IDENTIFIER('A','L','I','0');
    static constexpr SMC_KEY KeyALI1 = SMC_MAKE_IDENTIFIER('A','L','I','1');
    static constexpr SMC_KEY KeyALRV = SMC_MAKE_IDENTIFIER('A','L','R','V');
    static constexpr SMC_KEY KeyALV0 = SMC_MAKE_IDENTIFIER('A','L','V','0');
    static constexpr SMC_KEY KeyALV1 = SMC_MAKE_IDENTIFIER('A','L','V','1');
    static constexpr SMC_KEY KeyLKSB = SMC_MAKE_IDENTIFIER('L','K','S','B');
    static constexpr SMC_KEY KeyLKSS = SMC_MAKE_IDENTIFIER('L','K','S','S');
    static constexpr SMC_KEY KeyMSLD = SMC_MAKE_IDENTIFIER('M','S','L','D');
    VirtualSMCAPI::Plugin vsmcPlugin {
        "SurfaceAmbientLightSensorDriver",
        parseModuleVersion(xStringify(MODULE_VERSION)),
        VirtualSMCAPI::Version,
    };
    struct ALSSensor {
        enum Type : UInt8 {
            NoSensor  = 0,
            BS520     = 1,
            TSL2561CS = 2,
            LX1973A   = 3,
            ISL29003  = 4,
            Unknown7  = 7
        };
        Type sensorType {Type::NoSensor};
        bool validWhenLidClosed {false};
        UInt8 unknown {0};
        bool controlSIL {false};
    };
    ALSForceBits forceBits;
    
    void pollALI(IOTimerEventSource *timer);
    
    IOReturn initDevice();
    
    IOReturn configDevice(UInt8 func, bool enable);
    
    void releaseResources();
    
    inline IOReturn readRegister(UInt8 reg, UInt8* values, size_t len);
    
    inline IOReturn writeRegister(UInt8 reg, UInt8 cmd);
};

#endif /* SurfaceAmbientLightSensorDriver_hpp */
