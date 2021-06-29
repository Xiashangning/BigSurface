//
//  SurfaceTypeCoverDriver.hpp
//  BigSurface
//
//  Created by Xia on 21/04/2021.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef SurfaceTypeCoverDriver_hpp
#define SurfaceTypeCoverDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <kern/clock.h>

#include "VoodooI2CMultitouchHIDEventDriver.hpp"

#define kHIDUsage_Dig_Surface_Switch 0x57
#define kHIDUsage_Dig_Button_Switch 0x58

#define INPUT_MODE_MOUSE 0x00
#define INPUT_MODE_TOUCHPAD 0x03

typedef struct __attribute__((__packed__)) {
    UInt8 value;
    UInt8 reserved;
} SurfaceTypeCoverDriverFeatureReport;

/* Merged code for Surface Type Cover
 * Thanks VoodooI2C/VoodooI2CHID for most part of the code
 */

class EXPORT SurfaceTypeCoverDriver : public VoodooI2CMultitouchHIDEventDriver {
  OSDeclareDefaultStructors(SurfaceTypeCoverDriver);

 public:
    
    struct {
            OSArray *           elements;
            UInt8               bootMouseData[4];
            bool                appleVendorSupported;
    } keyboard;
    
    /* @inherit */
    
    void handleInterruptReport(AbsoluteTime timestamp, IOMemoryDescriptor *report, IOHIDReportType report_type, UInt32 report_id) override;

    /* @inherit */

    bool handleStart(IOService* provider) override;
    
    /* @inherit */
    
    IOReturn parseElements() override;

    /* @inherit */
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;
    
    /* Called during the interrupt routine to interate over keyboard events
     * @timestamp The timestamp of the interrupt report
     * @report_id The report ID of the interrupt report
     */

    void handleKeyboardReport(AbsoluteTime timeStamp, UInt32 reportID);

    /* Parses a keyboard usage page element
     * @element The element to parse
     *
     * This function is reponsible for examining the child elements of a digitser elements to determine the
     * capabilities of the keyboard.
     *
     * @return *true* on successful parse, *false* otherwise
     */

    bool parseKeyboardElement(IOHIDElement* element);
    
    void setKeyboardProperties();

 protected:
 private:
    bool ready = false;

    /* Sends a report to the device to instruct it to enter Touchpad mode */
    void enterPrecisionTouchpadMode();
};


#endif /* SurfaceTypeCoverDriver_hpp */
