//
//  SurfaceHIDDevice.cpp
//  SurfaceHID
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceHIDDevice.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#define super IOHIDDevice
OSDefineMetaClassAndStructors(SurfaceHIDDevice, IOHIDDevice);

bool SurfaceHIDDevice::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;
    
    api = OSDynamicCast(SurfaceHIDDriver, provider);
    if (!api)
        return false;
    
    if (api->getHIDDescriptor(device, &descriptor) != kIOReturnSuccess) {
        LOG("Failed to get HID descriptor!");
        return false;
    }
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
    
    if (api->getHIDAttributes(device, &attributes) != kIOReturnSuccess) {
        LOG("Failed to get attributes!");
        return false;
    }
    if (attributes.length != sizeof(SurfaceHIDAttributes)) {
        LOG("Unexpected attribute length: got %u, expected %lu\n", attributes.length, sizeof(SurfaceHIDAttributes));
        return false;
    }

    return true;
}

void SurfaceHIDDevice::detach(IOService* provider) {
    api = nullptr;
    super::detach(provider);
}

bool SurfaceHIDDevice::handleStart(IOService *provider) {
    if (!super::handleStart(provider))
        return false;
    
    setProperty("AppleVendorSupported", kOSBooleanTrue);
    setProperty("Built-In", kOSBooleanTrue);
    setProperty("HIDDefaultBehavior", kOSBooleanTrue);

    return true;
}

IOReturn SurfaceHIDDevice::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeFeature) {
        UInt8 report_id = options & 0xff;
        UInt16 buffer_len = report->getLength();
        UInt8 *buffer = new UInt8[buffer_len];
        if (api->getRawReport(device, report_id, buffer, buffer_len) != kIOReturnSuccess) {
            LOG("Get feature report failed");
            delete[] buffer;
            return kIOReturnError;
        }
        report->writeBytes(0, buffer, buffer_len);
        delete[] buffer;
    }
    return kIOReturnSuccess;
}

IOReturn SurfaceHIDDevice::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeFeature || reportType == kIOHIDReportTypeOutput) {
        UInt8 report_id = options & 0xff;
        UInt16 buffer_len = report->getLength();
        UInt8 *buffer = new UInt8[buffer_len];
        report->readBytes(0, buffer, buffer_len);
        api->setRawReport(device, report_id, reportType == kIOHIDReportTypeFeature, buffer, buffer_len);
        delete[] buffer;
    }
    return kIOReturnSuccess;
}

IOReturn SurfaceHIDDevice::newReportDescriptor(IOMemoryDescriptor **reportDescriptor) const {
    UInt8 *desc = new UInt8[descriptor.report_desc_len];

    if (api->getReportDescriptor(device, desc, descriptor.report_desc_len) != kIOReturnSuccess) {
        LOG("Failed to get report descriptor!");
        delete[] desc;
        return kIOReturnError;
    }
    *reportDescriptor = IOBufferMemoryDescriptor::withBytes(desc, descriptor.report_desc_len, kIODirectionNone);
    delete[] desc;
    return kIOReturnSuccess;
}

OSString* SurfaceHIDDevice::newTransportString() const {
    return OSString::withCString("Surface Serial Hub");
}

OSString *SurfaceHIDDevice::newManufacturerString() const {
    return OSString::withCString("Microsoft Inc.");
}

OSNumber *SurfaceHIDDevice::newVendorIDNumber() const {
    return OSNumber::withNumber(attributes.vendor, 32);
}

OSNumber *SurfaceHIDDevice::newProductIDNumber() const {
    return OSNumber::withNumber(attributes.product, 32);
}

OSNumber *SurfaceHIDDevice::newLocationIDNumber() const {
    return OSNumber::withNumber(0x1000000, 32);
}

OSNumber *SurfaceHIDDevice::newCountryCodeNumber() const {
    return OSNumber::withNumber(descriptor.country_code, 32);
}

OSNumber *SurfaceHIDDevice::newVersionNumber() const {
    return OSNumber::withNumber(attributes.version, 32);
}

OSString *SurfaceHIDDevice::newProductString() const {
    return device_name;
}
