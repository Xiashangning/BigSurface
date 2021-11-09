//
//  SurfaceBattery.cpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "SurfaceBattery.hpp"
#include <IOKit/battery/AppleSmartBatteryCommands.h>
#include <Headers/kern_util.hpp>
#include <Headers/kern_compat.hpp>

UInt32 SurfaceBattery::getNumberFromArray(OSArray *array, UInt32 index) {
	auto number = OSDynamicCast(OSNumber, array->getObject(index));
	if (number)
		return number->unsigned32BitValue();
	return BatteryInfo::ValueUnknown;
}

void SurfaceBattery::getStringFromArray(OSArray *array, UInt32 index, char *dst, UInt32 dstSize) {
	if (dstSize == 0)
		return;

	const char *src = nullptr;
	auto object = array->getObject(index);
	if (object) {
		if (auto string = OSDynamicCast(OSString, object)) {
			src = string->getCStringNoCopy();
			dstSize = min(dstSize - 1, string->getLength());
		}
	}

	if (src) {
		strncpy(dst, src, dstSize);
		dst[dstSize] = '\0';
	} else {
		dst[0] = '\0';
	}
}

void SurfaceBattery::reset() {
    IOSimpleLockLock(batteryInfoLock);
    *batteryInfo = BatteryInfo{};
    IOSimpleLockUnlock(batteryInfoLock);
}

void SurfaceBattery::updateInfoExtended(OSArray *bix) {
    BatteryInfo bi;
    bi.connected = true;

    if (getNumberFromArray(bix, BIXPowerUnit) == 0)
        bi.state.powerUnitIsWatt = true;
    bi.designCapacity               = getNumberFromArray(bix, BIXDesignCapacity);
    bi.state.lastFullChargeCapacity = getNumberFromArray(bix, BIXLastFullChargeCapacity);
    bi.technology                   = getNumberFromArray(bix, BIXBatteryTechnology);
    bi.state.designVoltage          = getNumberFromArray(bix, BIXDesignVoltage);
    bi.state.designCapacityWarning  = getNumberFromArray(bix, BIXDesignCapacityOfWarning);
    bi.state.designCapacityLow      = getNumberFromArray(bix, BIXDesignCapacityOfLow);
    bi.cycle                        = getNumberFromArray(bix, BIXCycleCount);
    getStringFromArray(bix, BIXModelNumber, bi.deviceName, BatteryInfo::MaxStringLen);
    getStringFromArray(bix, BIXSerialNumber, bi.serial, BatteryInfo::MaxStringLen);
//    getStringFromArray(bix, BIXBatteryType, bi.batteryType, BatteryInfo::MaxStringLen);
//    getStringFromArray(bix, BIXOEMInformation, bi.manufacturer, BatteryInfo::MaxStringLen);
    strncpy(bi.batteryType, "SurfaceBattery", BatteryInfo::MaxStringLen);
    strncpy(bi.manufacturer, "XavierXia", BatteryInfo::MaxStringLen);

	bi.validateData(id);
    
    IOSimpleLockLock(batteryInfoLock);
    *batteryInfo = bi;
    IOSimpleLockUnlock(batteryInfoLock);
}

bool SurfaceBattery::updateStatus(UInt32 *bst) {
	IOSimpleLockLock(batteryInfoLock);
	auto st = batteryInfo->state;
	IOSimpleLockUnlock(batteryInfoLock);
    
    UInt32 defaultRate = (5000*1000/st.designVoltage);
	st.state = bst[BSTState];
	st.presentRate = bst[BSTPresentRate];
	st.remainingCapacity = bst[BSTRemainingCapacity];
	st.presentVoltage = bst[BSTPresentVoltage];
	if (st.powerUnitIsWatt) {
		st.presentRate = st.presentRate * 1000 / st.designVoltage;
		st.remainingCapacity = st.remainingCapacity * 1000 / st.designVoltage;
	}

	// Sometimes this value can be either reported incorrectly or miscalculated
	// and exceed the actual capacity. Simply workaround it by capping the value.
	// REF: https://github.com/acidanthera/bugtracker/issues/565
	if (st.remainingCapacity > st.lastFullChargeCapacity)
		st.remainingCapacity = st.lastFullChargeCapacity;

    if (!st.averageRate) {
        st.averageRate = st.presentRate;
    } else { // We take a 5 minutes window
        AbsoluteTime cur_time;
        UInt64 nsecs;
        clock_get_uptime(&cur_time);
        SUB_ABSOLUTETIME(&cur_time, &st.lastUpdateTime);
        absolutetime_to_nanoseconds(cur_time, &nsecs);
        UInt32 secs = (UInt32)(nsecs/1000000000);
        st.averageRate = (5*60*st.averageRate + secs*st.presentRate)/(5*60+secs);
    }
    clock_get_uptime(&st.lastUpdateTime);

    UInt32 highAverageBound = st.presentRate * (100 + AverageBoundPercent) / 100;
    UInt32 lowAverageBound  = st.presentRate * (100 - AverageBoundPercent) / 100;
    if (st.averageRate > highAverageBound)
        st.averageRate = highAverageBound;
    if (st.averageRate < lowAverageBound)
        st.averageRate = lowAverageBound;

	// Remaining capacity
	st.averageTimeToEmpty = st.averageRate ? 60 * st.remainingCapacity / st.averageRate : 60 * st.remainingCapacity / defaultRate;
	st.runTimeToEmpty = st.presentRate ? 60 * st.remainingCapacity / st.presentRate : 60 * st.remainingCapacity / defaultRate;

	// Check battery state
	bool bogus = false;
	switch (st.state & BSTStateMask) {
		case BSTNotCharging: {
			st.calculatedACAdapterConnected = true;
			st.batteryIsFull = true;
			st.chargingCurrent = 0;
			st.timeToFull = 0;
			st.signedPresentRate = st.presentRate;
			st.signedAverageRate = st.averageRate;
			break;
		}
		case BSTDischarging: {
			st.calculatedACAdapterConnected = false;
			st.batteryIsFull = false;
			st.chargingCurrent = 0;
			st.timeToFull = 0;
			st.signedPresentRate = -st.presentRate;
			st.signedAverageRate = -st.averageRate;
			break;
		}
		case BSTCharging: {
			st.calculatedACAdapterConnected = true;
			st.batteryIsFull = false;
			int diff = st.lastFullChargeCapacity - st.remainingCapacity;
            st.timeToFull = st.averageRate ? 60 * diff / st.averageRate : 60 * diff / defaultRate;
			st.signedPresentRate = st.presentRate;
			st.signedAverageRate = st.averageRate;
			break;
		}
		default: {
            IOLog("SurfaceBattery::Bogus status data from battery %d (%x)", id, st.state);
			st.batteryIsFull = false;
			bogus = true;
			break;
		}
	}

	st.lastRemainingCapacity = st.remainingCapacity;

	// bool warning  = st.remainingCapacity <= st.designCapacityWarning || st.runTimeToEmpty < 10;
	bool critical = st.remainingCapacity <= st.designCapacityLow || st.runTimeToEmpty < 5;
	if (st.state & BSTCritical) {
        IOLog("SurfaceBattery::Battery %d is in critical state", id);
		critical = true;
	}

	// When we report battery failure, AppleSmartBatteryManager sets isCharging=false.
	// So we don't report battery failure when it's charging.
	st.bad = (st.state & BSTStateMask) != BSTCharging && st.lastFullChargeCapacity < 2 * st.designCapacityWarning;
	st.bogus = bogus;
	st.critical = critical;

	IOSimpleLockLock(batteryInfoLock);
	batteryInfo->state = st;
	IOSimpleLockUnlock(batteryInfoLock);
    
    return st.batteryIsFull;
}

UInt16 SurfaceBattery::calculateBatteryStatus() {
	UInt16 value = 0;
	if (batteryInfo->connected) {
		value = kBInitializedStatusBit;
		auto st = batteryInfo->state.state;
		if (st & BSTDischarging)
			value |= kBDischargingStatusBit;
		if (st & BSTCritical)
			value |= kBFullyDischargedStatusBit;
		if ((st & BSTStateMask) == BSTNotCharging) {
			if (batteryInfo->state.lastFullChargeCapacity > 0 &&
				batteryInfo->state.lastFullChargeCapacity != BatteryInfo::ValueUnknown &&
				batteryInfo->state.lastFullChargeCapacity <= BatteryInfo::ValueMax &&
				batteryInfo->state.remainingCapacity > 0 &&
				batteryInfo->state.remainingCapacity != BatteryInfo::ValueUnknown &&
				batteryInfo->state.remainingCapacity <= BatteryInfo::ValueMax &&
				batteryInfo->state.remainingCapacity >= batteryInfo->state.lastFullChargeCapacity)
				value |= kBFullyChargedStatusBit;
			value |= kBTerminateChargeAlarmBit;
		}

		if (batteryInfo->state.bad) {
			value |= kBTerminateChargeAlarmBit;
			value |= kBTerminateDischargeAlarmBit;
		}
	}

	return value;
}
