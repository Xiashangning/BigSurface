//
//  SurfaceBatteryDriver.cpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <stdatomic.h>

#include "SurfaceBatteryDriver.hpp"
#include "KeyImplementations.hpp"
#include <IOKit/battery/AppleSmartBatteryCommands.h>

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#define super SurfaceSerialHubClient
OSDefineMetaClassAndStructors(SurfaceBatteryDriver, SurfaceSerialHubClient)

void SurfaceBatteryDriver::eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) {
    if (cid == SSH_EVENT_CID_BAT_BIX)
        update_bix->interruptOccurred(nullptr, this, 0);
    else if (cid == SSH_EVENT_CID_BAT_BST) {
        if (!quick_cnt)
            quick_cnt = BST_UPDATE_QUICK_CNT + 1;
        update_bst->interruptOccurred(nullptr, this, 0);
    }
    // we don't need to deal with EVENT_PSR here because PSR always comes with BST changes
}

void SurfaceBatteryDriver::updateBatteryInformation(OSObject *target, void *refCon, IOService *nubDevice, int count) {
    if (!awake)
        return;
    
    bool battery_connected = false;
    bix_fail = false;
    
    UInt32 sta = 0x00;
    if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, 0x01, SSH_CID_BAT_STA, nullptr, 0, true, reinterpret_cast<UInt8*>(&sta), 4) != kIOReturnSuccess) {
        LOG("Failed to get STA!");
        bix_fail = true;
    }
    battery_connected = (sta & 0x10) ? true : false;
    if (!battery_connected)
        BatteryManager::getShared()->updateBatteryInfoExtended(0, nullptr); // reset battery
    else if (BatteryManager::getShared()->needUpdateBIX(0)) {
        UInt8 bix[BIX_LENGTH];
        if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, 0x01, SSH_CID_BAT_BIX, nullptr, 0, true, bix, BIX_LENGTH) != kIOReturnSuccess) {
            LOG("Failed to get BIX!");
            bix_fail = true;
        } else
            BatteryManager::getShared()->updateBatteryInfoExtended(0, bix);
    }
}

void SurfaceBatteryDriver::updateBatteryStatus(OSObject *target, void *refCon, IOService *nubDevice, int count) {
    if (!awake)
        return;
    
    timer->cancelTimeout();
    
    if (bix_fail)
        updateBatteryInformation(this, nullptr, this, 0);
    
    UInt32 psr;
    UInt16 temp;
    UInt8 bst[BST_LENGTH];
    
    if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, 0x01, SSH_CID_BAT_PSR, nullptr, 0, true, reinterpret_cast<UInt8*>(&psr), 4) != kIOReturnSuccess) {
        LOG("Failed to get PSR!");
        goto fail;
    } else
        power_connected = BatteryManager::getShared()->updateAdapterStatus(0, psr);
    BatteryManager::getShared()->externalPowerNotify(power_connected);
    
    if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, 0x01, SSH_CID_BAT_BST, nullptr, 0, true, bst, BST_LENGTH) != kIOReturnSuccess) {
        LOG("Failed to get BST!");
        goto fail;
    } else
        BatteryManager::getShared()->updateBatteryStatus(0, bst);
    
    if (ssh->getResponse(SSH_TC_TMP, SSH_TID_PRIMARY, TEMP_SENSOR_BAT, SSH_CID_TMP_SENSOR, nullptr, 0, true, reinterpret_cast<UInt8*>(&temp), 2) != kIOReturnSuccess)
        LOG("Failed to get battery temperature!");
    else
        BatteryManager::getShared()->updateBatteryTemperature(0, temp);
    BatteryManager::getShared()->informStatusChanged();

    if (--quick_cnt) {
        timer->setTimeoutMS(BST_UPDATE_QUICK);
    } else {    // sync normal update interval
        AbsoluteTime cur_time;
        UInt64 nsecs;
        clock_get_uptime(&cur_time);
        SUB_ABSOLUTETIME(&cur_time, &BatteryManager::getShared()->lastAccess);
        absolutetime_to_nanoseconds(cur_time, &nsecs);
        UInt8 timerDelta = nsecs / (1000000 * BST_UPDATE_QUICK);
        if (timerDelta < BST_UPDATE_NORMAL/BST_UPDATE_QUICK - 5)
            timer->setTimeoutMS(BST_UPDATE_NORMAL - (2 + timerDelta) * BST_UPDATE_QUICK);
        else
            timer->setTimeoutMS(BST_UPDATE_NORMAL);
    }
    return;
fail:
    timer->setTimeoutMS(BST_UPDATE_QUICK/2);
}

void SurfaceBatteryDriver::pollBatteryStatus(OSObject *target, IOTimerEventSource *sender) {
    updateBatteryStatus(this, nullptr, this, 0);
}

IOService *SurfaceBatteryDriver::probe(IOService *provider, SInt32 *score) {
	if (!super::probe(provider, score))
        return nullptr;
    
    ssh = getSurfaceSerialHub();
    if (!ssh) {
        LOG("Could not find Surface Serial Hub, exiting");
        return nullptr;
    }
    // Give the Surface Serial Hub some time to load
    IOSleep(100);
    
    //TODO: SurfaceBook series have two batteries
//    OSNumber *bat_cnt_prop = OSDynamicCast(OSNumber, getProperty("BatteryCount"));
    UInt32 bat_cnt = 1;
//    if (bat_cnt_prop) {
//        bat_cnt = bat_cnt_prop->unsigned32BitValue();
//    } else {
//        LOG("Fall back to default: battery count = 1");
//    }
    BatteryManager::createShared(bat_cnt, 1);

	//TODO: implement the keys below as well
	// IB0R: sp4s or sp5s
	// IBAC: sp7s
	// PB0R = IB0R * VP0R

	return this;
}

bool SurfaceBatteryDriver::start(IOService *provider) {
	if (!super::start(provider))
		return false;

	// AppleSMC presence is a requirement, wait for it.
	auto dict = nameMatching("AppleSMC");
	auto applesmc = waitForMatchingService(dict, 1000000000);
	if (!applesmc) {
		LOG("Timeout in waiting for AppleSMC");
		return false;
	}
    OSSafeReleaseNULL(dict);
    OSSafeReleaseNULL(applesmc);

    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Could not get work loop!");
        return false;
    }
    
    timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceBatteryDriver::pollBatteryStatus));
    if (!timer || (work_loop->addEventSource(timer) != kIOReturnSuccess)) {
        LOG("Could not create timer!");
        releaseResources();
        return false;
    }
    
    update_bix = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceBatteryDriver::updateBatteryInformation));
    update_bst = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceBatteryDriver::updateBatteryStatus));
    if (!update_bix || !update_bst ||
        work_loop->addEventSource(update_bix) != kIOReturnSuccess ||
        work_loop->addEventSource(update_bst) != kIOReturnSuccess) {
        LOG("Could not register interrupt!");
        releaseResources();
        return false;
    }
    
	//WARNING: watch out, key addition is sorted here!
	const auto adaptCount = BatteryManager::getShared()->adapterCount;
	if (adaptCount > 0) {
		VirtualSMCAPI::addKey(KeyACEN, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new ACIN));
		VirtualSMCAPI::addKey(KeyACFP, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
		VirtualSMCAPI::addKey(KeyACID, vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 8, SmcKeyTypeCh8s, new ACID));
		VirtualSMCAPI::addKey(KeyACIN, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
	}

	const auto batCount = min(BatteryManager::getShared()->batteriesCount, MaxIndexCount);
	for (UInt8 i = 0; i < batCount; i++) {
		VirtualSMCAPI::addKey(KeyB0AC(i), vsmcPlugin.data, VirtualSMCAPI::valueWithSint16(400, new B0AC(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0AV(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(13000, new B0AV(i)));
		VirtualSMCAPI::addKey(KeyB0BI(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(1, new B0BI(i)));
		VirtualSMCAPI::addKey(KeyB0CT(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(1, new B0CT(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0FC(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(4000, new B0FC(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0PS(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 2, SmcKeyTypeHex, new B0PS(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0RM(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(2000, new B0RM(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0St(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 2, SmcKeyTypeHex, new B0St(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0TF(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new B0TF(i)));
		VirtualSMCAPI::addKey(KeyTB0T(i+1), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TB0T(i)));
        if (i == 0)
            VirtualSMCAPI::addKey(KeyTB0T(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TB0T(0)));
	}

	VirtualSMCAPI::addKey(KeyBATP, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(true, new BATP));
	VirtualSMCAPI::addKey(KeyBBAD, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new BBAD));
	VirtualSMCAPI::addKey(KeyBBIN, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(true, new BBIN));
	VirtualSMCAPI::addKey(KeyBFCL, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(100, new BFCL, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
	VirtualSMCAPI::addKey(KeyBNum, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(1, new BNum));
	VirtualSMCAPI::addKey(KeyBRSC, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(40, new BRSC, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE | SMC_KEY_ATTRIBUTE_PRIVATE_WRITE));
	VirtualSMCAPI::addKey(KeyBSIn, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new BSIn));

	VirtualSMCAPI::addKey(KeyCHBI, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new CHBI));
	VirtualSMCAPI::addKey(KeyCHBV, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(8000, new CHBV));
	VirtualSMCAPI::addKey(KeyCHLC, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(1, new CHLC));

    for (UInt8 i = 0; i < batCount; i++) {
        VirtualSMCAPI::addKey(KeyBC1V(i+1), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(defaultBatteryCellVoltage, new BC1V(i)));
    }

	qsort(const_cast<VirtualSMCKeyValue *>(vsmcPlugin.data.data()), vsmcPlugin.data.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);
    
    PMinit();
    ssh->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);

	vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
    if (!vsmcNotifier) {
        PMstop();
        releaseResources();
        return false;
    }
    
    registerService();
	return true;
}

bool SurfaceBatteryDriver::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
	if (sensors && vsmc) {
		auto &plugin = static_cast<SurfaceBatteryDriver *>(sensors)->vsmcPlugin;
		auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &plugin, nullptr, nullptr);
		if (ret == kIOReturnSuccess) {
			IOLog("SurfaceBatteryDriver::Plugin submitted\n");
			return true;
		} else {
            IOLog("SurfaceBatteryDriver::Plugin submission failure %X\n", ret);
		}
	}
	return false;
}

void SurfaceBatteryDriver::releaseResources() {
    ssh->unregisterEvent(SSH_TC_BAT, 0x00, this);
    if (timer) {
        timer->cancelTimeout();
        timer->disable();
        work_loop->removeEventSource(timer);
        OSSafeReleaseNULL(timer);
    }
    if (update_bix) {
        update_bix->disable();
        work_loop->removeEventSource(update_bix);
        OSSafeReleaseNULL(update_bix);
    }
    if (update_bst) {
        update_bst->disable();
        work_loop->removeEventSource(update_bst);
        OSSafeReleaseNULL(update_bst);
    }
    OSSafeReleaseNULL(work_loop);
}

void SurfaceBatteryDriver::stop(IOService *provider) {
    PMstop();
    releaseResources();
    super::stop(provider);
}

SurfaceSerialHubDriver* SurfaceBatteryDriver::getSurfaceSerialHub() {
    SurfaceSerialHubDriver* ssh_driver = nullptr;

    // Wait for Surface Serial Hub, up to 2 second
    OSDictionary* name_match = serviceMatching("SurfaceSerialHubDriver");
    IOService* matched = waitForMatchingService(name_match, 2000000000);
    ssh_driver = OSDynamicCast(SurfaceSerialHubDriver, matched);

    if (ssh_driver)
        LOG("Got Surface Serial Hub! %s", ssh_driver->getName());
    OSSafeReleaseNULL(name_match);
    OSSafeReleaseNULL(matched);
    return ssh_driver;
}

IOReturn SurfaceBatteryDriver::setProperties(OSObject *props) {
    OSDictionary* dict = OSDynamicCast(OSDictionary, props);
    if (!dict)
        return kIOReturnError;
    
    OSCollectionIterator* i = OSCollectionIterator::withCollection(dict);
    if (i) {
        while (OSString* key = OSDynamicCast(OSString, i->getNextObject())) {
            if (key->isEqualTo("PerformanceMode")) {
                OSNumber *mode = OSDynamicCast(OSNumber, dict->getObject(key));
                if (mode) {
                    UInt32 m = mode->unsigned32BitValue();
                    LOG("Set performance mode to %d", m);
                    if (!ssh->sendCommand(SSH_TC_TMP, SSH_TID_PRIMARY, 0, SSH_CID_TMP_SET_PERF, reinterpret_cast<UInt8*>(&m), 4, true))
                        LOG("Set performance failed!");
                }
            }
        }
        i->release();
    }
    return kIOReturnSuccess;
}

IOReturn SurfaceBatteryDriver::setPowerState(unsigned long whichState, IOService *device) {
    if (device != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            awake = false;
            timer->cancelTimeout();
            timer->disable();
            ssh->unregisterEvent(SSH_TC_BAT, 0x00, this);
            update_bix->disable();
            update_bst->disable();
            IOLog("%s::Going to sleep\n", getName());
        }
    } else {
        if (!awake) {
            awake = true;
            timer->enable();
            timer->setTimeoutMS(5000);
            update_bix->enable();
            update_bst->enable();
            // register battery event
            if (ssh->registerEvent(SSH_TC_BAT, 0x00, this) != kIOReturnSuccess)
                LOG("Battery event registration failed!");
            bix_fail = true;
            updateBatteryStatus(this, nullptr, this, 0);
            
            // set performance mode
            OSNumber *mode = OSDynamicCast(OSNumber, getProperty("PerformanceMode"));
            if (mode) {
                UInt32 m = mode->unsigned32BitValue();
                LOG("Set performance mode to %d", m);
                if (!ssh->sendCommand(SSH_TC_TMP, SSH_TID_PRIMARY, 0, SSH_CID_TMP_SET_PERF, reinterpret_cast<UInt8*>(&m), 4, true))
                    LOG("Set performance failed!");
            }
            IOLog("%s::Woke up\n", getName());
        }
    }
    return kIOPMAckImplied;
}
