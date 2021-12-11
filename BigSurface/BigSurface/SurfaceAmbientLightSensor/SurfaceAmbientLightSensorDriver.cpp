//
//  SurfaceAmbientLightSensorDriver.cpp
//  BigSurface
//
//  Created by Xia on 2021/10/27.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#include "SurfaceAmbientLightSensorDriver.hpp"
#include <Headers/kern_version.hpp>

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
    if (!api)
        return nullptr;
    
    ALSSensor sensor {ALSSensor::Type::Unknown7, true, 6, false};
    ALSSensor noSensor {ALSSensor::Type::NoSensor, false, 0, false};
    SMCAmbientLightValue::Value emptyValue;

    VirtualSMCAPI::addKey(KeyAL, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, &forceBits, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyALI0, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
        reinterpret_cast<const SMC_DATA *>(&sensor), sizeof(sensor), SmcKeyTypeAli, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
    VirtualSMCAPI::addKey(KeyALI1, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
        reinterpret_cast<const SMC_DATA *>(&noSensor), sizeof(noSensor), SmcKeyTypeAli, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
    VirtualSMCAPI::addKey(KeyALRV, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(1, nullptr, SMC_KEY_ATTRIBUTE_CONST | SMC_KEY_ATTRIBUTE_READ));
    VirtualSMCAPI::addKey(KeyALV0, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
        reinterpret_cast<const SMC_DATA *>(&emptyValue), sizeof(emptyValue), SmcKeyTypeAlv, new SMCAmbientLightValue(&current_lux, &forceBits),
        SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyALV1, vsmcPlugin.data, VirtualSMCAPI::valueWithData(
        reinterpret_cast<const SMC_DATA *>(&emptyValue), sizeof(emptyValue), SmcKeyTypeAlv, nullptr,
        SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyMSLD, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0));
    
    IOLog("%s::Microsoft Ambient Light Sensor Device found!\n", getName());
    return this;
}

bool SurfaceAmbientLightSensorDriver::start(IOService *provider) {
    OSNumber *base;
    if (!super::start(provider))
        return false;

    work_loop = getWorkLoop();
    if (!work_loop) {
        IOLog("%s::Could not get a IOWorkLoop instance\n", getName());
        return false;
    }
    work_loop->retain();
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        IOLog("%s::Could not open command gate\n", getName());
        goto exit;
    }
    if (!api->open(this)) {
        IOLog("%s::Could not open API\n", getName());
        goto exit;
    }

    if (initDevice() != kIOReturnSuccess) {
        IOLog("%s::Failed to init device\n", getName());
        goto exit;
    }
    base = OSDynamicCast(OSNumber, getProperty("BaseALIValue"));
    if (!base) {
        IOLog("%s::Failed to obtain base ALI setting from property! Using default 100 instead!\n", getName());
    } else {
        base_ali = base->unsigned16BitValue();
    }
    
    PMinit();
    api->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    
    vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
    return vsmcNotifier != nullptr;
exit:
    releaseResources();
    return false;
}

void SurfaceAmbientLightSensorDriver::stop(IOService* provider) {
    writeRegister(APDS9960_ENABLE, 0x00);
    PMstop();
    releaseResources();
//    PANIC("SurfaceAmbientLightSensorDriver", "called stop!!!");
    super::stop(provider);
}

bool SurfaceAmbientLightSensorDriver::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
    if (sensors && vsmc) {
        auto self = static_cast<SurfaceAmbientLightSensorDriver *>(sensors);
        auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &self->vsmcPlugin, nullptr, nullptr);
        if (ret == kIOReturnSuccess) {
            IOLog("%s::Plugin submitted\n", self->getName());
            
            self->poller = IOTimerEventSource::timerEventSource(self, OSMemberFunctionCast(IOTimerEventSource::Action, self, &SurfaceAmbientLightSensorDriver::pollALI));
            if (!self->poller) {
                IOLog("%s::Could not get timer event source\n", self->getName());
                return false;
            }
            self->work_loop->addEventSource(self->poller);
            self->poller->setTimeoutMS(POLLING_INTERVAL);
            return true;
        } else {
            IOLog("%s::Plugin submission failure %X\n", self->getName(), ret);
        }
    }
    return false;
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
    // set short wait time to 712ms maximum
    ENSURE(writeRegister(APDS9960_WTIME, TIME_TO_VALUE(712)))
    ENSURE(configDevice(ENABLE_WAIT, true))
    // set ADC integration time to 100 ms
    ENSURE(writeRegister(APDS9960_ATIME, 0xFF))
    ENSURE(writeRegister(APDS9960_CONFIG1, CONFIG1_DEFAULT))
    ENSURE(writeRegister(APDS9960_CONTROL, ALS_GAIN_1X))
    // these values come from Windows and is NECESSARY
    ENSURE(writeRegister(APDS9960_CONFIG2, 0x03))
    ENSURE(writeRegister(APDS9960_CONFIG3, 0x14))
    ENSURE(configDevice(ENABLE_POWER, true))
    ENSURE(configDevice(ENABLE_ALS, true));

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
        IOLog("%s::Read enable register failed!\n", getName());
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
    if (readRegister(APDS9960_CDATAL, reinterpret_cast<UInt8 *>(color), sizeof(color)) != kIOReturnSuccess) {
        IOLog("%s::Read from ALS failed!\n", getName());
        goto exit;
    }
    atomic_store_explicit(&current_lux, color[0]+base_ali, memory_order_release);
    VirtualSMCAPI::postInterrupt(SmcEventALSChange);
    IOLog("%s::Value: ALI=%04d, r=0x%04x, g=0x%04x, b=0x%04x\n", getName(), color[0]+base_ali, color[1], color[2], color[3]);
exit:
    poller->setTimeoutMS(POLLING_INTERVAL);
}

IOReturn SurfaceAmbientLightSensorDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            poller->disable();
            writeRegister(APDS9960_ENABLE, 0x00);
            IOLog("%s::Going to sleep\n", getName());
            awake = false;
        }
    } else {
        if (!awake) {
            initDevice();
            poller->setTimeoutMS(POLLING_INTERVAL);
            poller->enable();
            IOLog("%s::Woke up\n", getName());
            awake = true;
        }
    }
    return kIOPMAckImplied;
}

void SurfaceAmbientLightSensorDriver::releaseResources() {
    if (poller) {
        poller->disable();
        work_loop->removeEventSource(poller);
        OSSafeReleaseNULL(poller);
    }
    if (command_gate) {
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    
    OSSafeReleaseNULL(work_loop);

    if (api) {
        if (api->isOpen(this)) {
            api->close(this);
        }
        api = nullptr;
    }
}
