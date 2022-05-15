//
//  BatteryManager.cpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "BatteryManager.hpp"

#define LOG(str, ...)    IOLog("%s::" str, getName(), ##__VA_ARGS__)

BatteryManager *BatteryManager::instance = nullptr;

OSDefineMetaClassAndStructors(BatteryManager, OSObject)

void BatteryInfo::validateData(SInt32 id) {
	if (!state.designVoltage)
		state.designVoltage = DummyVoltage;
	if (state.powerUnitIsWatt) {
		auto mV = state.designVoltage;
		IOLog("BatteryInfo::battery %d design voltage %d,%03d\n", id, mV / 1000, mV % 1000);
		if (designCapacity * 1000 / mV < 900) {
            IOLog("BatteryInfo::battery %d reports mWh but uses mAh (%u)\n", id, designCapacity);
			state.powerUnitIsWatt = false;
		} else {
			designCapacity = designCapacity * 1000 / mV;
			state.lastFullChargeCapacity = state.lastFullChargeCapacity * 1000 / mV;
			state.designCapacityWarning = state.designCapacityWarning * 1000 / mV;
			state.designCapacityLow = state.designCapacityLow * 1000 / mV;
		}
	}

	if (designCapacity < state.lastFullChargeCapacity) {
        IOLog("BatteryInfo::battery %d reports lower design capacity than maximum charged (%u/%u)\n", id,
			   designCapacity, state.lastFullChargeCapacity);
		if (state.lastFullChargeCapacity < ValueMax) {
			auto temp = designCapacity;
			designCapacity = state.lastFullChargeCapacity;
			state.lastFullChargeCapacity = temp;
		}
	}

    IOLog("BatteryInfo::battery %d cycle count %u remaining capacity %u\n", id, cycle, state.lastFullChargeCapacity);
}

bool BatteryManager::needUpdateBIX(UInt8 index) {
    bool ret;
    
    IOSimpleLockLock(stateLock);
    ret = !state.btInfo[index].connected;
    if (!ret) {
        AbsoluteTime cur_time;
        UInt64 nsecs;
        clock_get_uptime(&cur_time);
        SUB_ABSOLUTETIME(&cur_time, &state.btInfo[index].lastBIXUpdateTime);
        absolutetime_to_nanoseconds(cur_time, &nsecs);
        if (nsecs > 60000000000) // > 60s
            ret = true;
    }
    IOSimpleLockUnlock(stateLock);
    
    return ret;
}

bool BatteryManager::updateBatteryStatus(UInt8 index, UInt8 *buffer) {
    if (index >= batteriesCount)
        return false;
    
    UInt32 *bst = reinterpret_cast<UInt32*>(buffer);
    bool is_full;
    
    IOLockLock(mainLock);
    is_full = batteries[index].updateStatus(bst);
    IOLockUnlock(mainLock);
    
    return is_full;
}

void BatteryManager::updateBatteryInfoExtended(UInt8 index, UInt8 *buffer) {
    if (index >= batteriesCount)
        return;
    
    if (!buffer) {
        IOLockLock(mainLock);
        batteries[index].reset();
        IOLockUnlock(mainLock);
    } else {
        UInt32 *temp = reinterpret_cast<UInt32*>(buffer+1);
        const char *str = reinterpret_cast<const char*>(buffer);
        
        OSNumber *revision = OSNumber::withNumber(*buffer, 8);
        OSNumber *power_unit = OSNumber::withNumber(temp[0], 32);
        OSNumber *design_cap = OSNumber::withNumber(temp[1], 32);
        OSNumber *last_full_cap = OSNumber::withNumber(temp[2], 32);
        OSNumber *bat_tech = OSNumber::withNumber(temp[3], 32);
        OSNumber *design_volt = OSNumber::withNumber(temp[4], 32);
        OSNumber *design_cap_warn = OSNumber::withNumber(temp[5], 32);
        OSNumber *design_cap_low = OSNumber::withNumber(temp[6], 32);
        OSNumber *cycle_cnt = OSNumber::withNumber(temp[7], 32);
        OSNumber *mesure_acc = OSNumber::withNumber(temp[8], 32);
        OSNumber *max_sample_t = OSNumber::withNumber(temp[9], 32);
        OSNumber *min_sample_t = OSNumber::withNumber(temp[10], 32);
        OSNumber *max_avg_interval = OSNumber::withNumber(temp[11], 32);
        OSNumber *min_avg_interval = OSNumber::withNumber(temp[12], 32);
        OSNumber *bat_cap_gra_1 = OSNumber::withNumber(temp[13], 32);
        OSNumber *bat_cap_gra_2 = OSNumber::withNumber(temp[14], 32);
        OSString *model_number = OSString::withCString(str+0x3D);
        OSString *serial_number = OSString::withCString(str+0x52);
        OSString *bat_type = OSString::withCString(str+0x5D);
        OSString *oem_info = OSString::withCString(str+0x62);
        const OSObject *arr[] = {revision, power_unit, design_cap, last_full_cap, bat_tech,
                            design_volt, design_cap_warn, design_cap_low, cycle_cnt,
                            mesure_acc, max_sample_t, min_sample_t, max_avg_interval,
                            min_avg_interval, bat_cap_gra_1, bat_cap_gra_2, model_number,
                            serial_number, bat_type, oem_info};
        OSArray *bix = OSArray::withObjects(arr, 20);
        if (!bix) {
            IOLog("BatteryManager::Could not create a 20 sized array!\n");
            return;
        }
        
        IOLockLock(mainLock);
        batteries[index].updateInfoExtended(bix);
        IOLockUnlock(mainLock);
        
        bix->release();
        for (int i=0; i<20; i++)
            arr[i]->release();
    }
}

void BatteryManager::updateBatteryTemperature(UInt8 index, UInt16 temp) {
    IOLockLock(mainLock);
    batteries[index].updateTemperature(temp);
    IOLockUnlock(mainLock);
}

bool BatteryManager::updateAdapterStatus(UInt8 index, UInt32 psr) {
    if (index >= adapterCount)
        return false;
    
    bool power_connected;
    
    IOLockLock(mainLock);
    power_connected = adapters[index].updateStatus(psr!=0);
    IOLockUnlock(mainLock);
    
    return power_connected;
}

void BatteryManager::externalPowerNotify(bool status) {
	IOPMrootDomain *rd = IOACPIPlatformDevice::getPMRootDomain();
	rd->receivePowerNotification(kIOPMSetACAdaptorConnected | (kIOPMSetValue * status));
}

void BatteryManager::subscribe(PowerSourceInterestHandler h, void *t) {
	atomic_store_explicit(&handlerTarget, t, memory_order_release);
	atomic_store_explicit(&handler, h, memory_order_release);
}

void BatteryManager::informStatusChanged() {
	auto h = atomic_load_explicit(&handler, memory_order_acquire);
	if (h) h(atomic_load_explicit(&handlerTarget, memory_order_acquire));
}

bool BatteryManager::batteriesConnected() {
	for (UInt8 i = 0; i < batteriesCount; i++)
		if (state.btInfo[i].connected)
			return true;
	return false;
}

bool BatteryManager::adaptersConnected() {
	if (adapterCount) {
		for (UInt8 i = 0; i < adapterCount; i++)
			if (state.acInfo[i].connected)
				return true;
	}
	else {
		for (UInt8 i = 0; i < batteriesCount; i++)
			if (state.btInfo[i].state.calculatedACAdapterConnected)
				return true;
	}
	return false;
}

bool BatteryManager::batteriesAreFull() {
	// I am not fully convinced we should assume that batteries are full when there are none, but so be it.
	for (UInt8 i = 0; i < batteriesCount; i++)
		if (state.btInfo[i].connected && (state.btInfo[i].state.state & SurfaceBattery::BSTStateMask) != SurfaceBattery::BSTNotCharging)
			return false;
	return true;
}

bool BatteryManager::externalPowerConnected() {
	// Firstly try real adapters
	for (UInt8 i = 0; i < adapterCount; i++)
		if (state.acInfo[i].connected)
			return true;

	// Then try calculated adapters
	bool hasBateries = false;
	for (UInt8 i = 0; i < batteriesCount; i++) {
		if (state.btInfo[i].connected) {
			// Ignore calculatedACAdapterConnected when real adapters exist!
			if (adapterCount == 0 && state.btInfo[i].state.calculatedACAdapterConnected)
				return true;
			hasBateries = true;
		}
	}

	// Safe to assume without batteries you need AC
	return hasBateries == false;
}

UInt16 BatteryManager::calculateBatteryStatus(UInt8 index) {
	return batteries[index].calculateBatteryStatus();
}

void BatteryManager::createShared(UInt8 bat_cnt, UInt8 adp_cnt) {
	if (instance)
		PANIC("BatteryManager", "attempted to allocate battery manager again");
	instance = new BatteryManager;
	if (!instance)
		PANIC("BatteryManager", "failed to allocate battery manager");
	instance->mainLock = IOLockAlloc();
	if (!instance->mainLock)
		PANIC("BatteryManager", "failed to allocate main battery manager lock");
	instance->stateLock = IOSimpleLockAlloc();
	if (!instance->stateLock)
		PANIC("BatteryManager", "failed to allocate state battery manager lock");
    
	atomic_init(&instance->handlerTarget, nullptr);
	atomic_init(&instance->handler, nullptr);
    
    if (bat_cnt >= BatteryManagerState::MaxBatteriesSupported) {
        IOLog("BatteryManager::battery number more than %d!\n", BatteryManagerState::MaxBatteriesSupported);
        bat_cnt = BatteryManagerState::MaxBatteriesSupported;
    }
    if (adp_cnt >= BatteryManagerState::MaxAcAdaptersSupported) {
        IOLog("BatteryManager::adapter number more than %d!\n", BatteryManagerState::MaxAcAdaptersSupported);
        adp_cnt = BatteryManagerState::MaxAcAdaptersSupported;
    }
    
    auto dict = IOService::nameMatching("PNP0C0A");
    OSIterator *deviceIterator = nullptr;
    if (dict) {
        deviceIterator = IOService::getMatchingServices(dict);
        dict->release();
    }
    if (deviceIterator) {
        for (UInt8 i=0; i<bat_cnt; i++)
            instance->batteries[i] = SurfaceBattery(OSDynamicCast(IOACPIPlatformDevice, deviceIterator->getNextObject()), i, instance->stateLock, &instance->state.btInfo[i]);
        deviceIterator->release();
    } else {
        for (UInt8 i=0; i<bat_cnt; i++)
            instance->batteries[i] = SurfaceBattery(nullptr, i, instance->stateLock, &instance->state.btInfo[i]);
    }
    instance->batteriesCount = bat_cnt;
    
    dict = IOService::nameMatching("ACPI0003");
    deviceIterator = nullptr;
    if (dict) {
        deviceIterator = IOService::getMatchingServices(dict);
        dict->release();
    }
    if (deviceIterator) {
        for (UInt8 i=0; i<adp_cnt; i++)
            instance->adapters[i] = SurfaceACAdapter(OSDynamicCast(IOACPIPlatformDevice, deviceIterator->getNextObject()), i, instance->stateLock, &instance->state.acInfo[i]);
        deviceIterator->release();
    } else {
        for (UInt8 i=0; i<adp_cnt; i++)
            instance->adapters[i] = SurfaceACAdapter(nullptr, i, instance->stateLock, &instance->state.acInfo[i]);
    }
    instance->adapterCount = adp_cnt;
}

