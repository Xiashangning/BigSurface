//
//  SurfaceBatteryDriver.cpp
//  SurfaceBatteryDriver
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include <VirtualSMCSDK/kern_vsmcapi.hpp>
#include <stdatomic.h>

#include "SurfaceBatteryDriver.hpp"
#include "KeyImplementations.hpp"
#include <IOKit/battery/AppleSmartBatteryCommands.h>
#include <Headers/kern_version.hpp>

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#define super SurfaceSerialHubClient
OSDefineMetaClassAndStructors(SurfaceBatteryDriver, SurfaceSerialHubClient)

_Atomic(bool) SURFACE_BAT_DRIVER_STARTED = false;

void SurfaceBatteryDriver::eventReceived(UInt8 tc, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) {
    LOG("cid: %x, iid: %x, data_len: %d", cid, iid, length);
    if (cid == 0x15)
        update_bix_idx = iid;
    else if (cid == 0x16)
        update_bst_idx = iid;
    else if (cid == 0x17)
        update_adp_idx = iid;
    interrupt_source->interruptOccurred(nullptr, this, 0);
}

void SurfaceBatteryDriver::updateStatus(OSObject *target, void *refCon, IOService *nubDevice, int source) {
    bool battery_connection[BatteryManagerState::MaxBatteriesSupported] = {false};
    if (update_adp_idx != -1) {
        UInt32 psr;
        if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, 0x01, SSH_CID_BAT_PSR, nullptr, 0, true, reinterpret_cast<UInt8*>(&psr), 4) != kIOReturnSuccess) {
            LOG("Failed to get PSR!");
        } else {
            BatteryManager::getShared()->updateAdapterStatus(0, psr);
        }
        update_adp_idx = -1;
    }
    for (UInt8 i=1; i<=BatteryManager::getShared()->batteriesCount; i++) {
        UInt32 sta;
        if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, i, SSH_CID_BAT_STA, nullptr, 0, true, reinterpret_cast<UInt8*>(&sta), 4) != kIOReturnSuccess) {
            LOG("Failed to get STA for index %d!", i);
            continue;
        }
        battery_connection[i-1] = (sta & 0x10) ? true : false;
        if (!update_bix_idx || i == update_bix_idx) {
            if (battery_connection[i-1]) {
                UInt8 bix[119];
                if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, i, SSH_CID_BAT_BIX, nullptr, 0, true, bix, 119) != kIOReturnSuccess) {
                    LOG("Failed to get BIX for index %d!", i);
                } else {
                    BatteryManager::getShared()->updateBatteryInfoExtended(i-1, bix);
                    LOG("Finished BIX update for index %d!", i);
                }
            } else {
                BatteryManager::getShared()->updateBatteryInfoExtended(i-1, nullptr); // reset battery
            }
        }
        if (battery_connection[i-1] && (!update_bst_idx || i == update_bst_idx)) {
            UInt8 bst[16];
            if (ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, i, SSH_CID_BAT_BST, nullptr, 0, true, bst, 16) != kIOReturnSuccess) {
                LOG("Failed to get BST for index %d!", i);
            } else {
                BatteryManager::getShared()->updateBatteryStatus(i-1, bst);
                LOG("Finished BST update for index %d!", i);
            }
        }
    }
    update_bix_idx = update_bst_idx = -1;
}

IOService *SurfaceBatteryDriver::probe(IOService *provider, SInt32 *score) {
	if (!super::probe(provider, score))
        return nullptr;
    
    ssh = OSDynamicCast(SurfaceSerialHubDriver, provider);
    if (!ssh)
        return nullptr;
    
    BatteryManager::createShared(1, 1);

	//TODO: implement the keys below as well
	// IB0R: sp4s or sp5s
	// IBAC: sp7s
	// PB0R = IB0R * VP0R

	return this;
}

bool SurfaceBatteryDriver::start(IOService *provider) {
	if (!super::start(provider))
		return false;

    LOG("waiting");
	// AppleSMC presence is a requirement, wait for it.
	auto dict = IOService::nameMatching("AppleSMC");
	if (!dict) {
		LOG("Failed to create applesmc matching dictionary");
		return false;
	}
	auto applesmc = IOService::waitForMatchingService(dict);
	dict->release();
	if (!applesmc) {
		LOG("Timeout in waiting for AppleSMC");
		return false;
	}
	applesmc->release();
    LOG("starting");
    
    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Could not get work loop!");
        return false;
    }
    interrupt_source = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceBatteryDriver::updateStatus));
    if (!interrupt_source) {
        LOG("Could not register interrupt!");
        OSSafeReleaseNULL(work_loop);
        return false;
    }
    work_loop->addEventSource(interrupt_source);
    interrupt_source->enable();
    
    // register battery event
    if (ssh->registerEvent(SSH_TC_BAT, 0x00, this) != kIOReturnSuccess) {
        LOG("Battery event registration failed!");
        return false;
    }
    // Update info on startup
    update_bix_idx = update_bst_idx = update_adp_idx = 0;
    interrupt_source->interruptOccurred(nullptr, this, 0);

	//WARNING: watch out, key addition is sorted here!
	//FIXME: This needs to be implemented along with AC-W!
	// VirtualSMCAPI::addKey(KeyAC_N, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new AC_N(batteryManager)));

	const auto adaptCount = BatteryManager::getShared()->adapterCount;
	if (adaptCount > 0) {
		VirtualSMCAPI::addKey(KeyACEN, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0, new ACIN));
		VirtualSMCAPI::addKey(KeyACFP, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
		VirtualSMCAPI::addKey(KeyACID, vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 8, SmcKeyTypeCh8s, new ACID));
		VirtualSMCAPI::addKey(KeyACIN, vsmcPlugin.data, VirtualSMCAPI::valueWithFlag(false, new ACIN));
	}

	const auto batCount = min(BatteryManager::getShared()->batteriesCount, MaxIndexCount);
	for (size_t i = 0; i < batCount; i++) {
		VirtualSMCAPI::addKey(KeyB0AC(i), vsmcPlugin.data, VirtualSMCAPI::valueWithSint16(400, new B0AC(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0AV(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(13000, new B0AV(i)));
		VirtualSMCAPI::addKey(KeyB0BI(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(1, new B0BI(i)));
		VirtualSMCAPI::addKey(KeyB0CT(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(1, new B0CT(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0FC(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(4000, new B0FC(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0PS(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 2, SmcKeyTypeHex, new B0PS(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0RM(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(2000, new B0RM(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0St(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(nullptr, 2, SmcKeyTypeHex, new B0St(i), SMC_KEY_ATTRIBUTE_PRIVATE_WRITE|SMC_KEY_ATTRIBUTE_WRITE|SMC_KEY_ATTRIBUTE_READ));
		VirtualSMCAPI::addKey(KeyB0TF(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0, new B0TF(i)));
		if (BatteryManager::getShared()->state.btInfo[i].state.publishTemperatureKey) {
			VirtualSMCAPI::addKey(KeyTB0T(i+1), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TB0T(i)));
			if (i == 0)
				VirtualSMCAPI::addKey(KeyTB0T(0), vsmcPlugin.data, VirtualSMCAPI::valueWithSp(0, SmcKeyTypeSp78, new TB0T(0)));
		}
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

	if (getKernelVersion() >= KernelVersion::BigSur) {
		for (size_t i = 0; i < batCount; i++) {
			VirtualSMCAPI::addKey(KeyBC1V(i+1), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(defaultBatteryCellVoltage));
#if 0
			VirtualSMCAPI::addKey(KeyD0IR(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(5000));
			VirtualSMCAPI::addKey(KeyD0VM(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(5000));
			VirtualSMCAPI::addKey(KeyD0VR(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(5000));
			VirtualSMCAPI::addKey(KeyD0VX(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(5000));
#endif
		}

#if 0
		for (size_t i = 0; i < adaptCount; i++) {
			VirtualSMCAPI::addKey(KeyD0PT(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(i));
			VirtualSMCAPI::addKey(KeyD0BD(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint32(static_cast<UInt32>(i+1)));
			VirtualSMCAPI::addKey(KeyD0ER(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0));
			VirtualSMCAPI::addKey(KeyD0DE(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA*>("trop"), 4, SmcKeyTypeCh8s));
			VirtualSMCAPI::addKey(KeyD0is(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA*>("43210"), 5, SmcKeyTypeCh8s));
			VirtualSMCAPI::addKey(KeyD0if(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA*>("0.1"), 3, SmcKeyTypeCh8s));
			VirtualSMCAPI::addKey(KeyD0ih(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA*>("0.1"), 3, SmcKeyTypeCh8s));
			VirtualSMCAPI::addKey(KeyD0ii(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA*>("0.0.1"), 5, SmcKeyTypeCh8s));
			VirtualSMCAPI::addKey(KeyD0im(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA*>("resu"), 4, SmcKeyTypeCh8s));
			VirtualSMCAPI::addKey(KeyD0in(i), vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA*>("LPAA"), 4, SmcKeyTypeCh8s));
			VirtualSMCAPI::addKey(KeyD0PI(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(0x87));
			VirtualSMCAPI::addKey(KeyD0FC(i), vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0));
		}

		VirtualSMCAPI::addKey(KeyBNCB, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(batCount));
		VirtualSMCAPI::addKey(KeyAC_N, vsmcPlugin.data, VirtualSMCAPI::valueWithUint8(adaptCount-1));
		VirtualSMCAPI::addKey(KeyAC_W, vsmcPlugin.data, VirtualSMCAPI::valueWithSint8(1, nullptr, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_FUNCTION));
		VirtualSMCAPI::addKey(KeyCHII, vsmcPlugin.data, VirtualSMCAPI::valueWithUint16(0xB58));
#endif
	}

	qsort(const_cast<VirtualSMCKeyValue *>(vsmcPlugin.data.data()), vsmcPlugin.data.size(), sizeof(VirtualSMCKeyValue), VirtualSMCKeyValue::compare);

	vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
	SURFACE_BAT_DRIVER_STARTED = (vsmcNotifier != nullptr);
	return vsmcNotifier != nullptr;
}

bool SurfaceBatteryDriver::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
	if (sensors && vsmc) {
		auto &plugin = static_cast<SurfaceBatteryDriver *>(sensors)->vsmcPlugin;
		auto ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &plugin, nullptr, nullptr);
		if (ret == kIOReturnSuccess) {
			IOLog("SMCBatteryManager::Plugin submitted\n");
			return true;
		} else {
            IOLog("SMCBatteryManager::Plugin submission failure %X\n", ret);
		}
	}
	return false;
}

void SurfaceBatteryDriver::stop(IOService *provider) {
	PANIC("SMCBatteryManager", "called stop!!!");
}

EXPORT extern "C" kern_return_t ADDPR(kern_stop)(kmod_info_t *, void *) {
	// It is not safe to unload VirtualSMC plugins!
	return KERN_FAILURE;
}
