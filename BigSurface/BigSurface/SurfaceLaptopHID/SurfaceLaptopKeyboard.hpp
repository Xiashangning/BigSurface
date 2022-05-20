//
//  SurfaceLaptopKeyboard.hpp
//  SurfaceLaptopHID
//
//  Created by Xavier on 2022/5/16.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceLaptopKeyboard_hpp
#define SurfaceLaptopKeyboard_hpp

#include "SurfaceLaptop3HIDDevice.hpp"

class EXPORT SurfaceLaptopKeyboard final : public SurfaceLaptop3HIDDevice {
    OSDeclareDefaultStructors(SurfaceLaptopKeyboard);

public:
    bool attach(IOService *provider) override;

    IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;

    IOReturn getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) override;
    
    IOReturn setReport(IOMemoryDescriptor * report, IOHIDReportType reportType, IOOptionBits options=0) override;
    
    OSString *newProductString() const override;
};

#endif /* SurfaceLaptopKeyboard_hpp */
