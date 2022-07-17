//
//  SurfaceTouchScreenDevice.cpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/8.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceTouchScreenDevice.hpp"
#include "SurfaceTouchScreenReportDescriptor.h"

#define LOG(str, ...)    IOLog("%s::" str "\n", "SurfaceTouchScreenDevice", ##__VA_ARGS__)

#define super IOHIDDevice
OSDefineMetaClassAndStructors(SurfaceTouchScreenDevice, IOHIDDevice);

bool SurfaceTouchScreenDevice::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;
    
    api = OSDynamicCast(IntelPreciseTouchStylusDriver, provider);
    if (!api)
        return false;

    return true;
}

void SurfaceTouchScreenDevice::detach(IOService* provider) {
    api = nullptr;
    super::detach(provider);
}

bool SurfaceTouchScreenDevice::handleStart(IOService *provider) {
    if (!super::handleStart(provider))
        return false;
    
    setProperty("MaxContactCount", max_contacts, 32);
    
    setProperty("Built-In", kOSBooleanTrue);
    setProperty("HIDDefaultBehavior", kOSBooleanTrue);

    return true;
}

IOReturn SurfaceTouchScreenDevice::newReportDescriptor(IOMemoryDescriptor **reportDescriptor) const {
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withBytes(virt_reportDescriptor, sizeof(virt_reportDescriptor), kIODirectionNone);
    *reportDescriptor = buffer;
    return kIOReturnSuccess;
}

IOReturn SurfaceTouchScreenDevice::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeFeature) {
        UInt8 report_id = options & 0xff;
        if (report_id == 0x40) {
            UInt8 buffer[] = {0x40, max_contacts};
            report->writeBytes(0, buffer, sizeof(buffer));
            return kIOReturnSuccess;
        }
    }
    return kIOReturnUnsupported;
}

IOReturn SurfaceTouchScreenDevice::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    return kIOReturnUnsupported;
}

OSString* SurfaceTouchScreenDevice::newTransportString() const {
    return OSString::withCString("MEI");
}

OSString *SurfaceTouchScreenDevice::newManufacturerString() const {
    return OSString::withCString("Microsoft Inc.");
}

OSString *SurfaceTouchScreenDevice::newProductString() const {
    return OSString::withCString("Surface Touch Screen");
}

OSNumber *SurfaceTouchScreenDevice::newVendorIDNumber() const {
    return OSNumber::withNumber(vendor_id, 32);
}

OSNumber *SurfaceTouchScreenDevice::newProductIDNumber() const {
    return OSNumber::withNumber(device_id, 32);
}

OSNumber *SurfaceTouchScreenDevice::newVersionNumber() const {
    return OSNumber::withNumber(version, 32);
}
