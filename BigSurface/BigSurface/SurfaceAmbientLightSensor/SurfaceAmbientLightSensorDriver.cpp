//
//  SurfaceAmbientLightSensorDriver.cpp
//  BigSurface
//
//  Created by Xia on 2021/10/27.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#include "SurfaceAmbientLightSensorDriver.hpp"

#define super IOService
OSDefineMetaClassAndStructors(SurfaceAmbientLightSensorDriver, IOService);

IOService* SurfaceAmbientLightSensorDriver::probe(IOService *provider, SInt32 *score) {
    if (!super::probe(provider, score))
        return nullptr;
    
    acpi_device = OSDynamicCast(IOACPIPlatformDevice, provider->getProperty("acpi-device"));
    if (!acpi_device) {
        IOLog("%s::Could not get ACPI device\n", getName());
        return nullptr;
    }
    api = OSDynamicCast(VoodooI2CDeviceNub, provider);
    if (!api) {
        IOLog("%s::Could not get VoodooI2C API instance\n", getName());
        return nullptr;
    }
    IOLog("%s::Microsoft Ambient Light Sensor device found!\n", getName());
    return this;
}

bool SurfaceAmbientLightSensorDriver::start(IOService *provider) {
    if (!super::start(provider))
        return false;

    work_loop = this->getWorkLoop();
    if (!work_loop) {
        IOLog("%s::Could not get a IOWorkLoop instance\n", getName());
        return false;
    }
    work_loop->retain();
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        IOLog("%s::Could not open command gate\n", getName());
        goto start_exit;
    }
    acpi_device->retain();
    if (!api->open(this)) {
        IOLog("%s::Could not open API\n", getName());
        goto start_exit;
    }

    poller = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceAmbientLightSensorDriver::pollALI));
    if (!poller) {
        IOLog("%s::Could not get timer event source\n", getName());
        goto start_exit;
    }
    if (initDevice() != kIOReturnSuccess) {
        IOLog("%s::Failed to init device\n", getName());
        goto start_exit;
    }
    work_loop->addEventSource(poller);
    poller->setTimeoutMS(POLLING_INTERVAL);
    
    PMinit();
    api->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    return true;
start_exit:
    releaseResources();
    return false;
}

void SurfaceAmbientLightSensorDriver::stop(IOService* provider) {
    writeRegister(APDS9960_ENABLE, 0x00);
    releaseResources();
    PMstop();
    IOLog("%s::stopped\n", getName());
    super::stop(provider);
}

#define ENSURE(exp) ret=exp;\
                    if(ret != kIOReturnSuccess) {\
                        IOLog("%s::"#exp" failed!\n", getName());\
                        return ret;\
                    }

IOReturn SurfaceAmbientLightSensorDriver::initDevice() {
    IOReturn ret;
    UInt8 id;
    ENSURE(readRegister(APDS9960_ID, &id, 1))
    IOLog("%s::Device id is %x\n", getName(), id);
    
    ENSURE(writeRegister(APDS9960_ENABLE, 0x00))
    ENSURE(writeRegister(APDS9960_WTIME, 0xFF))

    ENSURE(configDevice(ENABLE_POWER, true))
    ENSURE(configDevice(ENABLE_WAIT, true))
    // set ADC integration time to 100 ms
    ENSURE(writeRegister(APDS9960_ATIME, TIME_TO_VALUE(100)))
//    ENSURE(writeRegister(APDS9960_WTIME, TIME_TO_VALUE(100)))
    ENSURE(writeRegister(APDS9960_CONTROL, ALS_GAIN_4X))
    IODelay(10);
    ENSURE(configDevice(ENABLE_POWER, true))
    
    configDevice(ENABLE_ALS, true);
    readRegister(APDS9960_CICLEAR, &id, 1);

    return kIOReturnSuccess;
}

IOReturn SurfaceAmbientLightSensorDriver::readRegister(UInt8 reg, UInt8* values, size_t len) {
    IOReturn ret = kIOReturnSuccess;
    ret = api->writeReadI2C(&reg, 1, values, len);
    return ret;
}

IOReturn SurfaceAmbientLightSensorDriver::writeRegister(UInt8 reg, UInt8 cmd) {
    UInt8 buffer[] {
        reg,
        cmd
    };
    IOReturn ret = kIOReturnSuccess;
    ret = api->writeI2C(buffer, sizeof(buffer));
    return ret;
}

IOReturn SurfaceAmbientLightSensorDriver::configDevice(UInt8 func, bool enable) {
    UInt8 reg;
    if (readRegister(APDS9960_ENABLE, &reg, 1) != kIOReturnSuccess) {
        IOLog("%s::read enable register failed!\n", getName());
        return kIOReturnError;
    }
    if (!(reg & func)!=enable)
        return kIOReturnSuccess;
    if (enable)
        reg |= func;
    else
        reg &= ~func;
    return writeRegister(APDS9960_ENABLE, reg);
}

void SurfaceAmbientLightSensorDriver::pollALI(OSObject* owner, IOTimerEventSource *timer) {
    UInt16 color[4];
    IOReturn ret1 = readRegister(APDS9960_CDATAL, reinterpret_cast<UInt8*>(color), sizeof(color));
    if (ret1 != kIOReturnSuccess) {
        IOLog("%s::Read from ALS failed!\n", getName());
        goto exit;
    }
    IOLog("%s::Value: ALI=%04x, r=%04x, g=%04x, b=%04x\n", getName(), color[0], color[1], color[2], color[3]);
exit:
    poller->setTimeoutMS(POLLING_INTERVAL);
}

IOReturn SurfaceAmbientLightSensorDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {

            IOLog("%s::Going to sleep\n", getName());
            awake = false;
        }
    } else {
        if (!awake) {

            IOLog("%s::Woke up\n", getName());
            awake = true;
        }
    }
    return kIOPMAckImplied;
}

void SurfaceAmbientLightSensorDriver::releaseResources() {
    if (command_gate) {
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }

    if (poller) {
        poller->disable();
        work_loop->removeEventSource(poller);
        OSSafeReleaseNULL(poller);
    }

    OSSafeReleaseNULL(work_loop);
    OSSafeReleaseNULL(acpi_device);

    if (api) {
        if (api->isOpen(this)) {
            api->close(this);
        }
        api = nullptr;
    }
}
