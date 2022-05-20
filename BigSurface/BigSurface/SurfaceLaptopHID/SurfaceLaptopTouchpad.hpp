//
//  SurfaceLaptopTouchpad.hpp
//  SurfaceLaptopHID
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceLaptopTouchpad_hpp
#define SurfaceLaptopTouchpad_hpp

#include "SurfaceLaptop3HIDDevice.hpp"

class EXPORT SurfaceLaptopTouchpad final : public SurfaceLaptop3HIDDevice {
    OSDeclareDefaultStructors(SurfaceLaptopTouchpad);

public:
    bool attach(IOService *provider) override;

    IOReturn newReportDescriptor(IOMemoryDescriptor **descriptor) const override;

    IOReturn getReport(IOMemoryDescriptor *report, IOHIDReportType reportType, IOOptionBits options) override;
    
    IOReturn setReport(IOMemoryDescriptor * report, IOHIDReportType reportType, IOOptionBits options=0) override;
    
    OSString *newProductString() const override;
};

#endif /* SurfaceLaptopTouchpad_hpp */
