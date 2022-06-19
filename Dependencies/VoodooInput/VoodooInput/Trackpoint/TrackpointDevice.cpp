/*
 * TrackpointDevice.cpp
 * VoodooTrackpoint
 *
 * Copyright (c) 2019 Leonard Kleinhans <leo-labs>
 *
 */

#include "TrackpointDevice.hpp"

OSDefineMetaClassAndStructors(TrackpointDevice, IOHIPointing);

UInt32 TrackpointDevice::deviceType() {
    return NX_EVS_DEVICE_TYPE_MOUSE;
}

UInt32 TrackpointDevice::interfaceID() {
    return NX_EVS_DEVICE_INTERFACE_BUS_ACE;
}

IOItemCount TrackpointDevice::buttonCount() {
    return 3;
};

IOFixed TrackpointDevice::resolution() {
    return (150) << 16;
};

bool TrackpointDevice::start(IOService* provider) {
    if (!super::start(provider)) {
        return false;
    }

    setProperty(kIOHIDScrollAccelerationTypeKey, kIOHIDTrackpadScrollAccelerationKey);
    setProperty(kIOHIDScrollResolutionKey, 800 << 16, 32);
    setProperty("HIDScrollResolutionX", 800 << 16, 32);
    setProperty("HIDScrollResolutionY", 800 << 16, 32);

    registerService();
    return true;
}

void TrackpointDevice::stop(IOService* provider) {
    super::stop(provider);
}


void TrackpointDevice::updateRelativePointer(int dx, int dy, int buttons, uint64_t timestamp) {
    dispatchRelativePointerEvent(dx, dy, buttons, timestamp);
};

void TrackpointDevice::updateScrollwheel(short deltaAxis1, short deltaAxis2, short deltaAxis3, uint64_t timestamp) {
    dispatchScrollWheelEvent(deltaAxis1, deltaAxis2, deltaAxis3, timestamp);
}
