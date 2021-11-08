//
//  VirtualAppleKeyboard.cpp
//  VirtualAppleKeyboard
//
//  Copyright © 2018-2020 Le Bao Hiep. All rights reserved.
//

#include "SurfaceButtonHIDDevice.hpp"

#define super IOHIDDevice
OSDefineMetaClassAndStructors(SurfaceButtonHIDDevice, super);

const UInt8 virt_reportDescriptor[] = {
    0x05, 0x0c,       // Usage Page (Consumer)
    0x09, 0x01,       // Usage 1 (kHIDUsage_Csmr_ConsumerControl)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x02,       //   Report Id (2)
    0x05, 0x0c,       //   Usage Page (Consumer)
    0x95, 0x20,       //   Report Count............ (32)
    0x75, 0x08,       //   Report Size............. (8)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xff, 0x00, //   Logical Maximum......... (255)
    0x19, 0x00,       //   Usage Minimum........... (0)
    0x29, 0xff,       //   Usage Maximum........... (255)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
    0xc0,             // End Collection

    0x06, 0x00, 0xff, // Usage Page (kHIDPage_AppleVendor)
    0x09, 0x01,       // Usage 1 (kHIDUsage_AppleVendor_TopCase)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0x03,       //   Report Id (3)
    0x05, 0xff,       //   Usage Page (kHIDPage_AppleVendorTopCase)
    0x95, 0x20,       //   Report Count............ (32)
    0x75, 0x08,       //   Report Size............. (8)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xff, 0x00, //   Logical Maximum......... (255)
    0x19, 0x00,       //   Usage Minimum........... (0)
    0x29, 0xff,       //   Usage Maximum........... (255)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
    0xc0,             // End Collection

    0x06, 0x00, 0xff, // Usage Page (kHIDPage_AppleVendor)
    0x09, 0x0f,       // Usage 15 (kHIDUsage_AppleVendor_KeyboardBacklight)
    0xa1, 0x01,       // Collection (Application)
    0x85, 0xbf,       //   Report ID (191)
    0x06, 0x00, 0xff, //   Usage Page (kHIDUsage_AppleVendor_KeyboardBacklight)
    0x95, 0x20,       //   Report Count............ (32)
    0x75, 0x08,       //   Report Size............. (8)
    0x15, 0x00,       //   Logical Minimum......... (0)
    0x26, 0xff, 0x00, //   Logical Maximum......... (255)
    0x19, 0x00,       //   Usage Minimum........... (0)
    0x29, 0xff,       //   Usage Maximum........... (255)
    0x81, 0x00,       //   Input...................(Data, Array, Absolute)
    0xc0,             // End Collection
};

IOReturn SurfaceButtonHIDDevice::simulateKeyboardEvent(UInt32 usagePage, UInt32 usage, bool status) {
    IOReturn result = kIOReturnError;
    
    if (usagePage == kHIDPage_Consumer) {
        if (status)
            csmrreport.keys.insert(usage);
        else
            csmrreport.keys.erase(usage);
        if (auto buffer = IOBufferMemoryDescriptor::withBytes(&csmrreport, sizeof(csmrreport), kIODirectionNone)) {
            result = handleReport(buffer, kIOHIDReportTypeInput, kIOHIDOptionsTypeNone);
            buffer->release();
        }
    } else {
        result = kIOReturnBadArgument;
    }
    
    return result;
}

bool SurfaceButtonHIDDevice::handleStart(IOService *provider) {
    if (!super::handleStart(provider)) {
        return false;
    }

    setProperty("AppleVendorSupported", kOSBooleanTrue);
    setProperty("Built-In", kOSBooleanTrue);
    setProperty("HIDDefaultBehavior", kOSBooleanTrue);

    return true;
}

IOReturn SurfaceButtonHIDDevice::newReportDescriptor(IOMemoryDescriptor **descriptor) const {
    *descriptor = IOBufferMemoryDescriptor::withBytes(virt_reportDescriptor, sizeof(virt_reportDescriptor), kIODirectionNone);
    return kIOReturnSuccess;
}

IOReturn SurfaceButtonHIDDevice::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    UInt8 report_id = options & 0xff;
    OSData *get_buffer = OSData::withCapacity(1);

    if (report_id == 0xbf) {
        UInt8 buffer[] = {0x00, 0x01, 0x00, 0x00, 0x00, 0x20, 0x02, 0x00};
        get_buffer->appendBytes(buffer, sizeof(buffer));
    }

    report->writeBytes(0, get_buffer->getBytesNoCopy(), get_buffer->getLength());
    get_buffer->release();

    return kIOReturnSuccess;
}

OSString *SurfaceButtonHIDDevice::newManufacturerString() const {
    return OSString::withCString("Microsoft Inc.");
}

OSString *SurfaceButtonHIDDevice::newProductString() const {
    return OSString::withCString("Surface Button HID Device");
}

OSNumber *SurfaceButtonHIDDevice::newVendorIDNumber() const {
    return OSNumber::withNumber(0x5ac, 32);
}

OSNumber *SurfaceButtonHIDDevice::newProductIDNumber() const {
    return OSNumber::withNumber(0x276, 32);
}

OSNumber *SurfaceButtonHIDDevice::newLocationIDNumber() const {
    return OSNumber::withNumber(0x14400000, 32);
}

OSNumber *SurfaceButtonHIDDevice::newCountryCodeNumber() const {
    return OSNumber::withNumber(0x21, 32);
}

OSNumber *SurfaceButtonHIDDevice::newVersionNumber() const {
    return OSNumber::withNumber(0x895, 32);
}

OSNumber *SurfaceButtonHIDDevice::newPrimaryUsagePageNumber() const {
    return OSNumber::withNumber(kHIDPage_Consumer, 32);
}

OSNumber *SurfaceButtonHIDDevice::newPrimaryUsageNumber() const {
    return OSNumber::withNumber(kHIDUsage_Csmr_ConsumerControl, 32);
}
