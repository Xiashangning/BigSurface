//
//  SurfaceLaptopHIDDevice.cpp
//  SurfaceLaptopHID
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceLaptop3HIDDevice.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#define super IOHIDDevice
OSDefineMetaClassAndAbstractStructors(SurfaceLaptop3HIDDevice, IOHIDDevice);

bool SurfaceLaptop3HIDDevice::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;
    
    api = OSDynamicCast(SurfaceLaptop3HIDDriver, provider);
    if (!api)
        return false;

    return true;
}

void SurfaceLaptop3HIDDevice::detach(IOService* provider) {
    api = nullptr;
    super::detach(provider);
}

bool SurfaceLaptop3HIDDevice::handleStart(IOService *provider) {
    if (!super::handleStart(provider))
        return false;
    
    if (descriptor.desc_len != sizeof(SurfaceHIDDescriptor)) {
        LOG("Unexpected HID descriptor length: got %u, expected %lu", descriptor.desc_len, sizeof(SurfaceHIDDescriptor));
        return false;
    }
    if (descriptor.desc_type != kIOUSBDecriptorTypeHID) {
        LOG("Unexpected HID descriptor type: got %#04x, expected %#04x", descriptor.desc_type, kIOUSBDecriptorTypeHID);
        return false;
    }
    if (descriptor.num_descriptors != 1) {
        LOG("Unexpected number of descriptors: got %u, expected 1", descriptor.num_descriptors);
        return false;
    }
    if (descriptor.report_desc_type != kIOUSBDecriptorTypeReport) {
        LOG("Unexpected report descriptor type: got %#04x, expected %#04x", descriptor.report_desc_type, kIOUSBDecriptorTypeReport);
        return false;
    }
    
    if (attributes.length != sizeof(SurfaceHIDAttributes)) {
        LOG("Unexpected attribute length: got %u, expected %lu\n", attributes.length, sizeof(SurfaceHIDAttributes));
        return false;
    }
    
    setProperty("AppleVendorSupported", kOSBooleanTrue);
    setProperty("Built-In", kOSBooleanTrue);
    setProperty("HIDDefaultBehavior", kOSBooleanTrue);

    return true;
}

OSString* SurfaceLaptop3HIDDevice::newTransportString() const {
    return OSString::withCString("Surface Serial Hub");
}

OSString *SurfaceLaptop3HIDDevice::newManufacturerString() const {
    return OSString::withCString("Microsoft Inc.");
}

OSNumber *SurfaceLaptop3HIDDevice::newVendorIDNumber() const {
    return OSNumber::withNumber(attributes.vendor, 32);
}

OSNumber *SurfaceLaptop3HIDDevice::newProductIDNumber() const {
    return OSNumber::withNumber(attributes.product, 32);
}

OSNumber *SurfaceLaptop3HIDDevice::newLocationIDNumber() const {
    return OSNumber::withNumber(0x1000000, 32);
}

OSNumber *SurfaceLaptop3HIDDevice::newCountryCodeNumber() const {
    return OSNumber::withNumber(descriptor.country_code, 32);
}

OSNumber *SurfaceLaptop3HIDDevice::newVersionNumber() const {
    return OSNumber::withNumber(attributes.version, 32);
}
