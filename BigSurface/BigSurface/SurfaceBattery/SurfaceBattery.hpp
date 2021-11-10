//
//  SurfaceBattery.hpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef SurfaceBattery_hpp
#define SurfaceBattery_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOReportTypes.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include "BatteryManagerState.hpp"

class SurfaceBattery {
    friend class BatteryManager;
public:
	/**
	 *  Peak accuracy for average in percents
	 */
	static constexpr UInt8 AverageBoundPercent = 25;

	/**
	 *  Battery status obtained from ACPI _BST
	 */
	enum {
		/**
		 Means that the battery is not charging and not discharging.
		 It may happen when the battery is fully charged,
		 or when AC adapter is connected but charging has not started yet,
		 or when charging is limited to some percent in BIOS settings.
		 */
		BSTNotCharging  = 0,
		BSTDischarging  = 1 << 0,
		BSTCharging     = 1 << 1,
		BSTCritical     = 1 << 2,
		BSTStateMask    = BSTNotCharging | BSTDischarging | BSTCharging,
	};

	/**
	 *  Reference to shared lock for refreshing battery info
	 */
	IOSimpleLock *batteryInfoLock {nullptr};

	/**
	 *  Reference to shared battery info
	 */
	BatteryInfo *batteryInfo {nullptr};

	/**
	 *  Dummy constructor
	 */
	SurfaceBattery() {}

	/**
	 *  Actual constructor representing a real device with its own index and shared info struct
	 */
	SurfaceBattery(SInt32 id, IOSimpleLock *lock, BatteryInfo *info) :
		id(id), batteryInfoLock(lock), batteryInfo(info) {}
	
	/**
	 * Calculate value for B0St SMC key and corresponding SMBus command
	 *
	 *  @return value
	 */
	UInt16 calculateBatteryStatus();

	/**
	 *  QuickPoll will be disable when average rate is available from EC
	 */
	bool averageRateAvailable {false};

private:
    bool hasBIX {false};
    
	UInt32 getNumberFromArray(OSArray *array, UInt32 index);

	void getStringFromArray(OSArray *array, UInt32 index, char *dst, UInt32 dstSize);
    
    /**
     * Reset BatteryInfo
     */
    void reset();

	/**
	 *  Obtain aggregated battery information from SAM
	 *
	 *  @param bix   _bix returned from SAM
	 */
	void updateInfoExtended(OSArray *bix);
    
    /**
     *  Obtain aggregated battery status from SAM
     *
     *  @param bst   _bst returned from SSH
     *
     *  @return whether battery is full
     */
    bool updateStatus(UInt32 *bst);
    
    void updateTemperature(UInt16 temp);

	/**
	 *  Current battery id
	 */
	SInt32 id {-1};

	/**
	 *  Extended Battery Static Information pack layout
	 */
	enum {
		BIXRevision,                    //Integer
		BIXPowerUnit,
		BIXDesignCapacity,
		BIXLastFullChargeCapacity,      //Integer (DWORD)
		BIXBatteryTechnology,
		BIXDesignVoltage,
		BIXDesignCapacityOfWarning,
		BIXDesignCapacityOfLow,
		BIXCycleCount,                  //Integer (DWORD)
		BIXMeasurementAccuracy,         //Integer (DWORD)
		BIXMaxSamplingTime,             //Integer (DWORD)
		BIXMinSamplingTime,             //Integer (DWORD)
		BIXMaxAveragingInterval,        //Integer (DWORD)
		BIXMinAveragingInterval,        //Integer (DWORD)
		BIXBatteryCapacityGranularity1,
		BIXBatteryCapacityGranularity2,
		BIXModelNumber,
		BIXSerialNumber,
		BIXBatteryType,
		BIXOEMInformation,
	};

	/**
	 *  Battery Real-time Information pack layout
	 */
	enum {
		BSTState,
		BSTPresentRate,
		BSTRemainingCapacity,
		BSTPresentVoltage
	};
};

#endif /* SurfaceBattery_hpp */
