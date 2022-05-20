//
//  SurfaceLaptopHIDDriver.cpp
//  SurfaceLaptopHID
//
//  Created by Xavier on 2022/5/16.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceLaptopKeyboard.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

#define super SurfaceLaptop3HIDDevice
OSDefineMetaClassAndStructors(SurfaceLaptopKeyboard, SurfaceLaptop3HIDDevice);

bool SurfaceLaptopKeyboard::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;
    
    if (api->getKeyboardDescriptor(&descriptor) != kIOReturnSuccess) {
        LOG("Failed to get HID descriptor!");
        return false;
    }
    if (api->getKeyboardAttributes(&attributes) != kIOReturnSuccess) {
        LOG("Failed to get attributes!");
        return false;
    }

    return true;
}

IOReturn SurfaceLaptopKeyboard::newReportDescriptor(IOMemoryDescriptor **reportDescriptor) const {
    UInt8 *desc = new UInt8[descriptor.report_desc_len];

    if (api->getKeyboardReportDescriptor(desc, descriptor.report_desc_len) != kIOReturnSuccess) {
        LOG("Failed to get report descriptor!");
        delete[] desc;
        return kIOReturnError;
    }
    *reportDescriptor = IOBufferMemoryDescriptor::withBytes(desc, descriptor.report_desc_len, kIODirectionNone);
    delete[] desc;
    return kIOReturnSuccess;
}

IOReturn SurfaceLaptopKeyboard::getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeFeature) {
        UInt8 report_id = options & 0xff;
        UInt16 buffer_len = report->getLength();
        UInt8 *buffer = new UInt8[buffer_len];
        if (api->getKeyboardRawReport(report_id, buffer, buffer_len) != kIOReturnSuccess) {
            LOG("Get feature report failed");
            delete[] buffer;
            return kIOReturnError;
        }
        report->writeBytes(0, buffer, buffer_len);
        delete[] buffer;
    }
    return kIOReturnSuccess;
}

IOReturn SurfaceLaptopKeyboard::setReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) {
    if (reportType == kIOHIDReportTypeFeature || reportType == kIOHIDReportTypeOutput) {
        UInt8 report_id = options & 0xff;
        UInt16 buffer_len = report->getLength();
        UInt8 *buffer = new UInt8[buffer_len];
        report->readBytes(0, buffer, buffer_len);
        api->setKeyboardRawReport(report_id, reportType == kIOHIDReportTypeFeature, buffer, buffer_len);
        delete[] buffer;
    }
    return kIOReturnSuccess;
}

OSString *SurfaceLaptopKeyboard::newProductString() const {
    return OSString::withCString("Surface Laptop3 Keyboard");
}
