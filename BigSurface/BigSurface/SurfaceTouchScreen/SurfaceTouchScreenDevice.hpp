//
//  SurfaceTouchScreenDevice.hpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/8.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceTouchScreenDevice_hpp
#define SurfaceTouchScreenDevice_hpp

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/usb/AppleUSBDefinitions.h>

#include "IntelPreciseTouchStylusDriver.hpp"

class EXPORT SurfaceTouchScreenDevice : public IOHIDDevice {
    OSDeclareDefaultStructors(SurfaceTouchScreenDevice);
    
    friend class IntelPreciseTouchStylusDriver;

public:
    bool attach(IOService *provider) override;
    
    void detach(IOService* provider) override;
    
    bool handleStart(IOService *provider) override;
    
    IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;

    IOReturn getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) override;
    
    IOReturn setReport(IOMemoryDescriptor * report, IOHIDReportType reportType, IOOptionBits options=0) override;
    
    OSString *newTransportString() const override;
    OSString *newManufacturerString() const override;
    OSString *newProductString() const override;
    OSNumber *newVendorIDNumber() const override;
    OSNumber *newProductIDNumber() const override;
    OSNumber *newVersionNumber() const override;
    
private:
    IntelPreciseTouchStylusDriver*  api {nullptr};
    IOBufferMemoryDescriptor *hid_desc {nullptr};
    
    UInt16 vendor_id {0};
    UInt16 device_id {0};
    UInt32 version {0};
    UInt8 max_contacts {1};
};

#endif /* SurfaceTouchScreenDevice_hpp */
