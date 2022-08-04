//
//  SurfaceHIDDevice.hpp
//  SurfaceHID
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceHIDDevice_hpp
#define SurfaceHIDDevice_hpp

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/usb/AppleUSBDefinitions.h>

#include "SurfaceHIDDriver.hpp"

class EXPORT SurfaceHIDDevice : public IOHIDDevice {
    OSDeclareDefaultStructors(SurfaceHIDDevice);
    
    friend class SurfaceHIDDriver;

public:
    bool attach(IOService *provider) override;
    
    void detach(IOService* provider) override;
    
    bool handleStart(IOService *provider) override;
    
    IOReturn getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) override;
    
    IOReturn setReport(IOMemoryDescriptor * report, IOHIDReportType reportType, IOOptionBits options=0) override;

    IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;
    
    OSString *newTransportString() const override;
    OSString *newManufacturerString() const override;
    OSNumber *newVendorIDNumber() const override;
    OSNumber *newProductIDNumber() const override;
    OSNumber *newLocationIDNumber() const override;
    OSNumber *newCountryCodeNumber() const override;
    OSNumber *newVersionNumber() const override;
    OSString *newProductString() const override;
    
private:
    SurfaceHIDDriver*       api {nullptr};
    SurfaceHIDDescriptor    descriptor;
    SurfaceHIDAttributes    attributes;
    SurfaceHIDDeviceType    device;
    const char*             device_name;
};

#endif /* SurfaceHIDDevice_hpp */
