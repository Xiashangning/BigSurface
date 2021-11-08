//
//  SMCBatteryManager.hpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//
//  Portions copyright © 2009 Superhai.
//  VoodooBattery. Contact http://www.superhai.com/
//

#ifndef BatteryManagerBase_hpp
#define BatteryManagerBase_hpp

#include "SurfaceBattery.hpp"
#include "SurfaceACAdapter.hpp"
#include "BatteryManagerState.hpp"
#include <IOKit/IOService.h>
#include <IOKit/IOReportTypes.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <Headers/kern_util.hpp>
#include <stdatomic.h>

class EXPORT BatteryManager : public OSObject {
	OSDeclareDefaultStructors(BatteryManager)
    friend class SurfaceBatteryDriver;
	
public:
	UInt8 batteriesCount {0};
	UInt8 adapterCount {0};

	/**
	 *  State lock, every state access must be guarded by this lock
	 */
	IOSimpleLock *stateLock {nullptr};

	/**
	 *  Main refreshed battery state containing battery information
	 */
	BatteryManagerState state {};

	using PowerSourceInterestHandler = IOReturn (*)(void *target);

	/**
	 *  Subscribe to power source interest changes
	 *  Only one handler could be subscribed to atm.
	 *
	 *  @param h  handler to be called
	 *  @param t  handler target (context)
	 */
	void subscribe(PowerSourceInterestHandler h, void *t);
    
    void informStatus();
	
	bool batteriesConnected();
	
	bool adaptersConnected();
	
	bool batteriesAreFull();

	bool externalPowerConnected();

	static void createShared(UInt8 bat_cnt, UInt8 adp_cnt);

	static BatteryManager *getShared() {
		return instance;
	}
	
	/**
	 *  Calculate battery status in AlarmWarning format
	 *
	 *  @param  index battery index
	 *
	 *  @return calculated value
	 */
	UInt16 calculateBatteryStatus(UInt8 index);

private:
	/**
	 *  The only allowed battery manager instance
	 */
	static BatteryManager *instance;

	/**
	 *  Not publicly exported notifications
	 */
	enum {
		kIOPMSetValue				= 1 << 16,
		kIOPMSetDesktopMode			= 1 << 17,
		kIOPMSetACAdaptorConnected	= 1 << 18
	};

	SurfaceBattery batteries[BatteryManagerState::MaxBatteriesSupported] {};

	SurfaceACAdapter adapters[BatteryManagerState::MaxAcAdaptersSupported] {};

	/**
	 *  A lock to permit concurrent access
	 */
	IOLock *mainLock {nullptr};

	/**
	 *  Stored subscribed power source change handler target
	 */
	_Atomic(void *) handlerTarget;

	/**
	 *  Stored subscribed power source change handler
	 */
	_Atomic(PowerSourceInterestHandler) handler;
    
    /**
     *  Post external power plug-in/plug-out update
     *
     *  @param status  true on AC adapter connect
     */
    void externalPowerNotify(bool status);
    
    /**
     *  @return whether battery is full
     */
    bool updateBatteryStatus(UInt8 index, UInt8 *buffer);
    
    void updateBatteryInfoExtended(UInt8 index, UInt8 *buffer);
    
    void updateAdapterStatus(UInt8 index, UInt32 psr);
};

#endif /* BatteryManagerBase_hpp */
