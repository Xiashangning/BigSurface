//
//  SurfaceACAdapter.hpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef SurfaceACAdapter_hpp
#define SurfaceACAdapter_hpp

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include "BatteryManagerState.hpp"

class SurfaceACAdapter {
public:
	SurfaceACAdapter() {}
	/**
	 *  Actual constructor representing a real device with its own index and shared info struct
	 */
	SurfaceACAdapter(IOACPIPlatformDevice *device, SInt32 id, IOSimpleLock *lock, ACAdapterInfo *info) :
        device(device), id(id), adapterInfoLock(lock), adapterInfo(info) {}

	bool updateStatus(bool connected);
    
private:
    /**
     *  Current adapter device
     */
    IOACPIPlatformDevice *device {nullptr};
    
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
