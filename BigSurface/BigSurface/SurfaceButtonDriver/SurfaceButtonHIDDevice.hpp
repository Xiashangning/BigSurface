//
//  VirtualAppleKeyboard.hpp
//  VirtualAppleKeyboard
//
//  Copyright © 2018-2020 Le Bao Hiep. All rights reserved.
//

#include <IOKit/hid/IOHIDDevice.h>

#include "HIDReport.hpp"

class SurfaceButtonHIDDevice final : public IOHIDDevice {
    OSDeclareDefaultStructors(SurfaceButtonHIDDevice);
private:
    consumer_input csmrreport;

public:
    IOReturn simulateKeyboardEvent(UInt32 usagePage, UInt32 usage, bool status);
    
    bool handleStart(IOService *provider) override;

    IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;

    IOReturn getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) override;

    OSString *newManufacturerString() const override;
    OSString *newProductString() const override;
    OSNumber *newVendorIDNumber() const override;
    OSNumber *newProductIDNumber() const override;
    OSNumber *newLocationIDNumber() const override;
    OSNumber *newCountryCodeNumber() const override;
    OSNumber *newVersionNumber() const override;
    OSNumber *newPrimaryUsagePageNumber() const override;
    OSNumber *newPrimaryUsageNumber() const override;
};
