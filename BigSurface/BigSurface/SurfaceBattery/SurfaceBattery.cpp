//
//  ACPIBattery.cpp
//  SMCBatteryManager
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
		if (auto data = OSDynamicCast(OSData, object)) {
			src = reinterpret_cast<const char *>(data->getBytesNoCopy());
			dstSize = min(dstSize - 1, data->getLength());
		} else if (auto string = OSDynamicCast(OSString, object)) {
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

void SurfaceBattery::resetBattery() {
    IOSimpleLockLock(batteryInfoLock);
    *batteryInfo = BatteryInfo{};
    IOSimpleLockUnlock(batteryInfoLock);
}

void SurfaceBattery::updateBatteryInfoExtended(OSArray *bix) {
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
    getStringFromArray(bix, BIXBatteryType, bi.batteryType, BatteryInfo::MaxStringLen);
    getStringFromArray(bix, BIXOEMInformation, bi.manufacturer, BatteryInfo::MaxStringLen);

	bi.validateData(id);
    
    IOSimpleLockLock(batteryInfoLock);
    *batteryInfo = bi;
    IOSimpleLockUnlock(batteryInfoLock);
}

bool SurfaceBattery::updateBatteryStatus(UInt32 *bst) {
	IOSimpleLockLock(batteryInfoLock);
	auto st = batteryInfo->state;
	IOSimpleLockUnlock(batteryInfoLock);

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

	// Average rate calculation
    if (!st.presentRate || (st.presentRate == BatteryInfo::ValueUnknown)) {
        auto delta = (st.remainingCapacity > st.lastRemainingCapacity ?
                      st.remainingCapacity - st.lastRemainingCapacity :
                      st.lastRemainingCapacity - st.remainingCapacity);
        AbsoluteTime current_time, last_time;
        UInt64 nsecs;
        clock_get_uptime(&current_time);
        last_time = st.lastUpdateTime;
        st.lastUpdateTime = current_time;
        if (last_time) {
            SUB_ABSOLUTETIME(&current_time, &last_time);
            absolutetime_to_nanoseconds(current_time, &nsecs);
            UInt32 interval = 3600 / (nsecs / 1000000000);
            st.presentRate = delta * interval; // per hour
        }
    }

    if (st.presentRate && st.presentRate != BatteryInfo::ValueUnknown) {
        if (!st.averageRate) {
            st.averageRate = st.presentRate;
        } else {
            st.averageRate += st.presentRate;
            st.averageRate >>= 1;
        }
    }

    UInt32 highAverageBound = st.presentRate * (100 + AverageBoundPercent) / 100;
    UInt32 lowAverageBound  = st.presentRate * (100 - AverageBoundPercent) / 100;
    if (st.averageRate > highAverageBound)
        st.averageRate = highAverageBound;
    if (st.averageRate < lowAverageBound)
        st.averageRate = lowAverageBound;

	// Remaining capacity
	if (!st.remainingCapacity || st.remainingCapacity == BatteryInfo::ValueUnknown) {
		IOLog("ACPIBattery::battery %d has no remaining capacity reported (%u)", id, st.remainingCapacity);
		// Invalid or zero capacity is not allowed and will lead to boot failure, hack 1 here.
		st.remainingCapacity = st.averageTimeToEmpty = st.runTimeToEmpty = 1;
	} else {
		st.averageTimeToEmpty = st.averageRate ? 60 * st.remainingCapacity / st.averageRate : 60 * st.remainingCapacity;
		st.runTimeToEmpty = st.presentRate ? 60 * st.remainingCapacity / st.presentRate : 60 * st.remainingCapacity;
	}

	// Voltage
	if (!st.presentVoltage || st.presentVoltage == BatteryInfo::ValueUnknown)
		st.presentVoltage = st.designVoltage;

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
			int diff = st.remainingCapacity < st.lastFullChargeCapacity ? st.lastFullChargeCapacity - st.remainingCapacity : 0;
            st.timeToFull = st.averageRate ? 60 * diff / st.averageRate : 60 * diff;
			st.signedPresentRate = st.presentRate;
			st.signedAverageRate = st.averageRate;
			break;
		}
		default: {
            IOLog("ACPIBattery::Bogus status data from battery %d (%x)", id, st.state);
			st.batteryIsFull = false;
			bogus = true;
			break;
		}
	}

	st.lastRemainingCapacity = st.remainingCapacity;

	// bool warning  = st.remainingCapacity <= st.designCapacityWarning || st.runTimeToEmpty < 10;
	bool critical = st.remainingCapacity <= st.designCapacityLow || st.runTimeToEmpty < 5;
	if (st.state & BSTCritical) {
        IOLog("ACPIBattery::Battery %d is in critical state", id);
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
