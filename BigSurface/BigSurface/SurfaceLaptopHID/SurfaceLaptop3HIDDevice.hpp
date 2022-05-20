//
//  SurfaceLaptopHIDDevice.hpp
//  SurfaceLaptopHID
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceLaptopHIDDevice_hpp
#define SurfaceLaptopHIDDevice_hpp

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/usb/AppleUSBDefinitions.h>

#include "SurfaceLaptop3HIDDriver.hpp"

class EXPORT SurfaceLaptop3HIDDevice : public IOHIDDevice {
    OSDeclareAbstractStructors(SurfaceLaptop3HIDDevice);

public:
    bool attach(IOService *provider) override;
    
    void detach(IOService* provider) override;
    
    bool handleStart(IOService *provider) override;

    OSString *newTransportString() const override;
    OSString *newManufacturerString() const override;
    OSNumber *newVendorIDNumber() const override;
    OSNumber *newProductIDNumber() const override;
    OSNumber *newLocationIDNumber() const override;
    OSNumber *newCountryCodeNumber() const override;
    OSNumber *newVersionNumber() const override;
    
protected:
    SurfaceLaptop3HIDDriver*        api {nullptr};
    SurfaceHIDDescriptor            descriptor;
    SurfaceHIDAttributes            attributes;
};

#endif /* SurfaceLaptopHIDDevice_hpp */
