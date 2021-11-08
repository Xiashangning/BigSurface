//
//  SurfaceACAdapter.hpp
//  SMCBatteryManager
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef SurfaceACAdapter_hpp
#define SurfaceACAdapter_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOReportTypes.h>
#include "BatteryManagerState.hpp"

class SurfaceACAdapter {
public:
	SurfaceACAdapter() {}
	/**
	 *  Actual constructor representing a real device with its own index and shared info struct
	 */
	SurfaceACAdapter(SInt32 id, IOSimpleLock *lock, ACAdapterInfo *info) :
		id(id), adapterInfoLock(lock), adapterInfo(info) {}

	bool updateStatus(bool connected);
    
private:
	/**
	 *  Current adapter id
	 */
	SInt32 id {-1};

	/**
	 *  Reference to shared lock for refreshing adapter info
	 */
	IOSimpleLock *adapterInfoLock {nullptr};

	/**
	 *  Reference to shared adapter info
	 */
	ACAdapterInfo *adapterInfo {nullptr};
};

#endif /* SurfaceACAdapter_hpp */
