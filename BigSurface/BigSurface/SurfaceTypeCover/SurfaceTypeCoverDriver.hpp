//
//  SurfaceTypeCoverDriver.hpp
//  SurfaceTypeCover
//
//  Created by Xavier on 21/04/2021.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef SurfaceTypeCoverDriver_hpp
#define SurfaceTypeCoverDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#include "VoodooI2CPrecisionTouchpadHIDEventDriver.hpp"

/*
 * Merged code for Surface Type Cover
 * Support keyboard & touchpad at the same time
 */

class EXPORT SurfaceTypeCoverDriver : public VoodooI2CPrecisionTouchpadHIDEventDriver {
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
};


#endif /* SurfaceTypeCoverDriver_hpp */
