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
    OSSafeReleaseNULL(hid_desc);
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
//    UInt16 desc_len = 0;
//    if (hid_desc)
//        desc_len = hid_desc->getLength();
//    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withCapacity(sizeof(virt_reportDescriptor) + desc_len, kIODirectionNone);
//    buffer->writeBytes(0, virt_reportDescriptor, sizeof(virt_reportDescriptor));
//    if (hid_desc)
//        buffer->writeBytes(sizeof(virt_reportDescriptor), hid_desc->getBytesNoCopy(), desc_len);
    IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::withBytes(virt_reportDescriptor, sizeof(virt_reportDescriptor), kIODirectionNone);
    *reportDescriptor = buffer;
    return kIOReturnSuccess;
}

IOReturn SurfaceTouchScreenDevice::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeFeature) {
        UInt8 report_id = options & 0xff;
        OSData *get_buffer = OSData::withCapacity(1);
        LOG("Get feature report for id %d", report_id);
        if (report_id == 0x40) {
            UInt8 buffer[] = {0x40, max_contacts};
            get_buffer->appendBytes(buffer, sizeof(buffer));
        }
        report->writeBytes(0, get_buffer->getBytesNoCopy(), get_buffer->getLength());
        get_buffer->release();
    }
    return kIOReturnSuccess;
}

IOReturn SurfaceTouchScreenDevice::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeFeature || reportType == kIOHIDReportTypeOutput) {
        UInt8 report_id = options & 0xff;
        LOG("Set feature/output report for id %d", report_id);
    }
    return kIOReturnSuccess;
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
