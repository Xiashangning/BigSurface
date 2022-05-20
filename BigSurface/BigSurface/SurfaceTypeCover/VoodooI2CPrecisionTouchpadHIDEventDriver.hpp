//
//  VoodooI2CPrecisionTouchpadHIDEventDriver.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 21/09/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CPrecisionTouchpadHIDEventDriver_hpp
#define VoodooI2CPrecisionTouchpadHIDEventDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#include "VoodooI2CMultitouchHIDEventDriver.hpp"

#define INPUT_MODE_MOUSE 0x00
#define INPUT_MODE_TOUCHPAD 0x03

/* Implements an HID Event Driver for Precision Touchpad devices as specified by Microsoft's protocol in the following document: https://docs.microsoft.com/en-us/windows-hardware/design/component-guidelines/precision-touchpad-devices
 *
 * The members of this class are responsible for instructing a Precision Touchpad device to enter Touchpad mode.
 */

class EXPORT VoodooI2CPrecisionTouchpadHIDEventDriver : public VoodooI2CMultitouchHIDEventDriver {
    OSDeclareDefaultStructors(VoodooI2CPrecisionTouchpadHIDEventDriver);

 public:
    /* @inherit */
    
    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) override;

    /* @inherit */

    bool handleStart(IOService* provider) override;

    /* @inherit */
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;

 protected:
    bool ready = false;
    
 private:
    /* Sends a report to the device to instruct it to enter Touchpad mode */
    void enterPrecisionTouchpadMode();
};


#endif /* VoodooI2CPrecisionTouchpadHIDEventDriver_hpp */
