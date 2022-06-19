//
//  VoodooInputActuatorDevice.hpp
//  VoodooInput
//
//  Copyright © 2019 Alexandre Daoud and Kishor Prins. All rights reserved.
//

#ifndef VOODOO_ACTUATOR_DEVICE_HPP
#define VOODOO_ACTUATOR_DEVICE_HPP

#include <IOKit/IOService.h>
#include <IOKit/hid/IOHIDDevice.h>

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

class EXPORT VoodooInputActuatorDevice : public IOHIDDevice {
    OSDeclareDefaultStructors(VoodooInputActuatorDevice);
    
public:
    IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;
    
    IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
    OSNumber* newVendorIDNumber() const override;
    
    
    OSNumber* newProductIDNumber() const override;
    
    
    OSNumber* newVersionNumber() const override;
    
    
    OSString* newTransportString() const override;
    
    
    OSString* newManufacturerString() const override;
    
    OSNumber* newPrimaryUsageNumber() const override;
    
    OSNumber* newPrimaryUsagePageNumber() const override;
    
    OSString* newProductString() const override;
    
    OSString* newSerialNumberString() const override;
    
    OSNumber* newLocationIDNumber() const override;
};


#endif
