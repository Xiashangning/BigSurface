//
//  SurfaceACAdapter.cpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "SurfaceACAdapter.hpp"

bool SurfaceACAdapter::updateStatus(bool connected) {
	IOSimpleLockLock(adapterInfoLock);
	adapterInfo->connected = connected;
	IOSimpleLockUnlock(adapterInfoLock);
    
    return connected;
}
