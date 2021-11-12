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
    
    if (device) {
        OSObject *result;
        OSObject *params[] = {
            OSNumber::withNumber(connected, 32),
        };
        device->evaluateObject("XPSR", &result, params, 1);
        OSSafeReleaseNULL(result);
        params[0]->release();
    }
    
    return connected;
}
