//
//  AmbientLightValue.cpp
//  SMCLightSensor
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include "AmbientLightValue.hpp"

SMC_RESULT SMCAmbientLightValue::readAccess() {
	auto value = reinterpret_cast<Value *>(data);
	UInt32 lux = atomic_load_explicit(currentLux, memory_order_acquire);
	UInt8 bits = forceBits->bits();

	if (lux == 0xFFFFFFFF) {
		value->valid = false;
	} else {
		value->valid = true;
		if (!(bits & ALSForceBits::kALSForceHighGain))
			value->highGain = true;
		if (!(bits & ALSForceBits::kALSForceChan))
			value->chan0 = OSSwapHostToBigInt16(lux);
		if (!(bits & ALSForceBits::kALSForceLux))
			value->roomLux = OSSwapHostToBigInt32(lux << 14);
	}

	return SmcSuccess;
}
