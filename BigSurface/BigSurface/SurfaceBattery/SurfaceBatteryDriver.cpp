//
//  SurfaceBatteryDriver.cpp
//  SurfaceBattery
//
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "SurfaceBatteryDriver.hpp"
#include "KeyImplementations.hpp"
#include <IOKit/battery/AppleSmartBatteryCommands.h>

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#define super IOService
OSDefineMetaClassAndStructors(SurfaceBatteryDriver, IOService)

void SurfaceBatteryDriver::eventReceived(SurfaceBatteryNub *sender, SurfaceBatteryEventType type) {
    switch (type) {
        case SurfaceBatteryInformationChanged:
            LOG("Got BIX update event");
            update_bix->interruptOccurred(nullptr, this, 0);
            break;
        case SurfaceBatteryStatusChanged:
        case SurfaceAdaptorStatusChanged:
            LOG("Got BST/PSR update event");
            AbsoluteTime cur_time;
            UInt64 nsecs;
            clock_get_uptime(&cur_time);
            SUB_ABSOLUTETIME(&cur_time, &last_update);
            absolutetime_to_nanoseconds(cur_time, &nsecs);
            if (nsecs > 1000000000) {   // PSR often comes with BST changes, so avoid update it twice in a short period(1s).
                quick_cnt = BST_UPDATE_QUICK_CNT;
                clock_get_uptime(&last_update);
                update_bst->interruptOccurred(nullptr, this, 0);
            }
            break;
        default:
            LOG("WTF? Unknown event type");
            break;
    }
}

void SurfaceBatteryDriver::updateBatteryInformation(IOInterruptEventSource *sender, int count) {
    if (!awake)
        return;
    
    bix_fail = false;
    bool connected = false;
    if (nub->getBatteryConnection(1, &connected) != kIOReturnSuccess) {
        LOG("Failed to get battery connection status from SSH!");
        bix_fail = true;
        return;
    }
    if (!connected)
        BatteryManager::getShared()->updateBatteryInfoExtended(0, nullptr); // reset battery
    else if (BatteryManager::getShared()->needUpdateBIX(0)) {
        OSArray *bix;
        if (nub->getBatteryInformation(1, &bix) != kIOReturnSuccess) {
            LOG("Failed to get BIX from SSH!");
            bix_fail = true;
        } else {
            BatteryManager::getShared()->updateBatteryInfoExtended(0, bix);
            bix->flushCollection();
            OSSafeReleaseNULL(bix);
        }
    }
}

void SurfaceBatteryDriver::updateBatteryStatus(IOInterruptEventSource *sender, int count) {
    if (!awake)
        return;
    
    timer->cancelTimeout();
    if (bix_fail)
        updateBatteryInformation(nullptr, 0);
    
    UInt32 psr;
    bool connected = false;
    if (nub->getAdaptorStatus(&psr) != kIOReturnSuccess) {
        LOG("Failed to get power source status from SSH!");
        goto fail;
    } else
        power_connected = BatteryManager::getShared()->updateAdapterStatus(0, psr);
    BatteryManager::getShared()->externalPowerNotify(power_connected);
    
    if (nub->getBatteryConnection(1, &connected) != kIOReturnSuccess) {
        LOG("Failed to get battery connection status from SSH!");
        return;
    }
    if (connected) {
        UInt16 temp = 0;
        UInt32 bst[4];
        if (nub->getBatteryStatus(1, bst, &temp) != kIOReturnSuccess) {
            LOG("Failed to get BST from SSH!");
            goto fail;
        } else
            BatteryManager::getShared()->updateBatteryStatus(0, bst);
        if (temp)
            BatteryManager::getShared()->updateBatteryTemperature(0, temp);
        BatteryManager::getShared()->informStatusChanged();
    }

    if (quick_cnt) {
        if (--quick_cnt == 0)
            sync = true;    // after finishing quick update, sync with normal update
        timer->setTimeoutMS(BST_UPDATE_QUICK);
    } else {    // sync normal update interval
        if (sync) {
            AbsoluteTime cur_time;
            UInt64 nsecs;
            sync = false;
            clock_get_uptime(&cur_time);
            IOSimpleLockLock(BatteryManager::getShared()->stateLock);
            SUB_ABSOLUTETIME(&cur_time, &BatteryManager::getShared()->lastAccess);
            IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
            absolutetime_to_nanoseconds(cur_time, &nsecs);
            UInt8 timerDelta = nsecs / (1000000 * BST_UPDATE_QUICK);
            if (timerDelta < BST_UPDATE_NORMAL/BST_UPDATE_QUICK - 5) {
                timer->setTimeoutMS(BST_UPDATE_NORMAL - (2 + timerDelta) * BST_UPDATE_QUICK);
                return;
            }
        }
        timer->setTimeoutMS(BST_UPDATE_NORMAL);
    }
    return;
fail:
    timer->setTimeoutMS(BST_UPDATE_QUICK/2);
}

void SurfaceBatteryDriver::pollBatteryStatus(IOTimerEventSource *sender) {
    updateBatteryStatus(nullptr, 0);
}

IOService *SurfaceBatteryDriver::probe(IOService *provider, SInt32 *score) {
	if (!super::probe(provider, score))
        return nullptr;
    
    nub = OSDynamicCast(SurfaceBatteryNub, provider);
    if (!nub)
        return nullptr;
    
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
	auto applesmc = waitForMatchingService(dict);
	if (!applesmc) {
		LOG("Timeout in waiting for AppleSMC");
		return false;
	}
    OSSafeReleaseNULL(dict);
    OSSafeReleaseNULL(applesmc);
    
    const UInt8 adaptCount = BatteryManager::getShared()->adapterCount;
    const UInt8 batCount = min(BatteryManager::getShared()->batteriesCount, MaxIndexCount);

    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Could not get work loop!");
        return false;
    }
    
    timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceBatteryDriver::pollBatteryStatus));
    if (!timer) {
        LOG("Could not create timer!");
        goto exit;
    }
    work_loop->addEventSource(timer);
    
    update_bix = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceBatteryDriver::updateBatteryInformation));
    update_bst = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceBatteryDriver::updateBatteryStatus));
    if (!update_bix || !update_bst) {
        LOG("Could not create interrupt event!");
        goto exit;
    }
    work_loop->addEventSource(update_bix);
    work_loop->addEventSource(update_bst);
    
    if (nub->registerBatteryEvent(this, OSMemberFunctionCast(SurfaceBatteryNub::EventHandler, this, &SurfaceBatteryDriver::eventReceived)) != kIOReturnSuccess) {
        LOG("Battery event registration failed!");
        goto exit;
    }
    
	//WARNING: watch out, key addition is sorted here!
	if (adaptCount > 0) {
		VirtualSMCAPI::addKey(KeyACEN, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new ACIN));
		VirtualSMCAPI::addKey(KeyACFP, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
		VirtualSMCAPI::addKey(KeyACID, vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 8, SmcKeyTypeCh8s, new ACID));
		VirtualSMCAPI::addKey(KeyACIN, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
	}

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
    nub->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);

	vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
    if (!vsmcNotifier) {
        PMstop();
        goto exit;
    }
    
    registerService();
	return true;
exit:
    releaseResources();
    return false;
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
    nub->unregisterBatteryEvent(this);
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
                    if (nub->setPerformanceMode(m) != kIOReturnSuccess)
                        LOG("Set performance mode failed!");
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
            update_bix->disable();
            update_bst->disable();
            LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            timer->enable();
            update_bix->enable();
            update_bst->enable();
            bix_fail = true;
            quick_cnt = BST_UPDATE_QUICK_CNT;
            clock_get_uptime(&last_update);
            update_bst->interruptOccurred(nullptr, this, 0);
            
            // set performance mode
            OSNumber *mode = OSDynamicCast(OSNumber, getProperty("PerformanceMode"));
            if (mode) {
                UInt32 m = mode->unsigned32BitValue();
                LOG("Set performance mode to %d", m);
                if (nub->setPerformanceMode(m) != kIOReturnSuccess)
                    LOG("Set performance mode failed!");
            }
            LOG("Woke up");
        }
    }
    return kIOPMAckImplied;
}
