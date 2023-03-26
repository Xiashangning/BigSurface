//
//  SurfaceAmbientLightSensorDriver.cpp
//  SurfaceAmbientLightSensor
//
//  Created by Xia on 2021/10/27.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#include <Headers/kern_util.hpp>

#include "SurfaceAmbientLightSensorDriver.hpp"

#define super IOService
OSDefineMetaClassAndStructors(SurfaceAmbientLightSensorDriver, IOService);

IOService* SurfaceAmbientLightSensorDriver::probe(IOService *provider, SInt32 *score) {
    if (!super::probe(provider, score))
        return nullptr;
    
    api = OSDynamicCast(VoodooI2CDeviceNub, provider);
    if (!api)
        return nullptr;
      
    OSDictionary *dict = nameMatching("ACPI0008");
    IOService *matched = waitForMatchingService(dict, 1000000000);
    alsd_device = OSDynamicCast(IOACPIPlatformDevice, matched);
    OSSafeReleaseNULL(dict);
    OSSafeReleaseNULL(matched);
    if (!alsd_device) {
        LOG("Ambient Light Sensor ACPI device not found");
        return nullptr;
    }
    
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
    
    LOG("Surface Ambient Light Sensor device found!");
    return this;
}

bool SurfaceAmbientLightSensorDriver::start(IOService *provider) {
    if (!super::start(provider))
        return false;

    work_loop = getWorkLoop();
    if (!work_loop) {
        LOG("Could not get a IOWorkLoop instance");
        return false;
    }
    work_loop->retain();
    poller = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceAmbientLightSensorDriver::pollALI));
    if (!poller) {
        LOG("Could not create timer event source");
        goto exit;
    }
    work_loop->addEventSource(poller);
    
    if (!api->open(this)) {
        LOG("Could not open API");
        goto exit;
    }

    if (initDevice() != kIOReturnSuccess) {
        LOG("Failed to init device");
        goto exit;
    }
    
    PMinit();
    api->joinPMtree(this);
    registerPowerDriver(this, myIOPMPowerStates, kIOPMNumberPowerStates);
    
    vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
    return vsmcNotifier != nullptr;
exit:
    releaseResources();
    return false;
}

void SurfaceAmbientLightSensorDriver::stop(IOService* provider) {
//    writeRegister(APDS9960_ENABLE, 0x00);
//    PMstop();
//    releaseResources();
//    super::stop(provider);
    PANIC("SurfaceAmbientLightSensorDriver", "called stop!!!");
}

bool SurfaceAmbientLightSensorDriver::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
    if (sensors && vsmc) {
        auto self = static_cast<SurfaceAmbientLightSensorDriver *>(sensors);
        auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &self->vsmcPlugin, nullptr, nullptr);
        if (ret == kIOReturnSuccess) {
            IOLog("%s::Plugin submitted\n", self->getName());
            self->poller->setTimeoutMS(POLLING_INTERVAL);
            return true;
        } else
            IOLog("%s::Plugin submission failure %X\n", self->getName(), ret);
    }
    return false;
}

#define ENSURE(exp) ret=exp;\
                    if(ret != kIOReturnSuccess) {\
                        LOG(""#exp" failed!");\
                        return ret;\
                    }

IOReturn SurfaceAmbientLightSensorDriver::initDevice() {
    IOReturn ret;
    UInt8 id;
    ENSURE(readRegister(APDS9960_ID, &id, 1))
    LOG("Device id is %x", id);
    
    ENSURE(writeRegister(APDS9960_ENABLE, 0x00))
    // set short wait time to 700ms
    ENSURE(writeRegister(APDS9960_WTIME, TIME_TO_VALUE(700)))
    ENSURE(configDevice(ENABLE_WAIT, true))
    // set ADC integration time to 300 ms
    ENSURE(writeRegister(APDS9960_ATIME, TIME_TO_VALUE(300)))
    ENSURE(writeRegister(APDS9960_CONFIG1, CONFIG1_DEFAULT))
    ENSURE(writeRegister(APDS9960_CONTROL, ALS_GAIN_4X))
    // these values come from Windows and is NECESSARY
    ENSURE(writeRegister(APDS9960_CONFIG2, 0x03))
    ENSURE(writeRegister(APDS9960_CONFIG3, 0x14))
    ENSURE(configDevice(ENABLE_POWER, true))
    ENSURE(configDevice(ENABLE_ALS, true));

    return kIOReturnSuccess;
}

inline IOReturn SurfaceAmbientLightSensorDriver::readRegister(UInt8 reg, UInt8* values, size_t len) {
    return api->writeReadI2C(&reg, 1, values, len);
}

inline IOReturn SurfaceAmbientLightSensorDriver::writeRegister(UInt8 reg, UInt8 cmd) {
    UInt8 buffer[] {
        reg,
        cmd
    };
    return api->writeI2C(buffer, sizeof(buffer));
}

IOReturn SurfaceAmbientLightSensorDriver::configDevice(UInt8 func, bool enable) {
    UInt8 reg;
    if (readRegister(APDS9960_ENABLE, &reg, 1) != kIOReturnSuccess) {
        LOG("Read enable register failed!");
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

void SurfaceAmbientLightSensorDriver::pollALI(IOTimerEventSource *timer) {
    if (!awake)
        return;
    
    UInt16 color[4];
    if (readRegister(APDS9960_CDATAL, reinterpret_cast<UInt8 *>(color), sizeof(color)) != kIOReturnSuccess) {
        LOG("Read from ALS failed!");
        poller->setTimeoutMS(POLLING_INTERVAL);
        return;
    }
    atomic_store_explicit(&current_lux, color[0]*1.5, memory_order_release);
    
    VirtualSMCAPI::postInterrupt(SmcEventALSChange);
    
    OSObject *result;
    OSObject *params[] = {
        OSNumber::withNumber(current_lux, 32),
    };
    alsd_device->evaluateObject("XALI", &result, params, 1);
    OSSafeReleaseNULL(result);
    params[0]->release();
    
//    LOG("Value: ALI=%04d", current_lux);
    poller->setTimeoutMS(POLLING_INTERVAL);
}

IOReturn SurfaceAmbientLightSensorDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            poller->cancelTimeout();
            poller->disable();
            awake = false;
            writeRegister(APDS9960_ENABLE, 0x00);
            DBG_LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            initDevice();
            poller->enable();
            poller->setTimeoutMS(POLLING_INTERVAL);
            DBG_LOG("Woke up");
        }
    }
    return kIOPMAckImplied;
}

void SurfaceAmbientLightSensorDriver::releaseResources() {
    if (poller) {
        poller->cancelTimeout();
        poller->disable();
        work_loop->removeEventSource(poller);
        OSSafeReleaseNULL(poller);
    }
    OSSafeReleaseNULL(work_loop);

    if (api) {
        if (api->isOpen(this)) {
            api->close(this);
        }
        api = nullptr;
    }
}
