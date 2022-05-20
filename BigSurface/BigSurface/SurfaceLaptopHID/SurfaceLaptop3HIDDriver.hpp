//
//  SurfaceLaptop3HIDDriver.hpp
//  SurfaceLaptopHID
//
//  Created by Xavier on 2022/5/16.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceLaptopHIDDriver_hpp
#define SurfaceLaptopHIDDriver_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOInterruptEventSource.h>

#include "../SurfaceSerialHubDevices/SurfaceLaptop3Nub.hpp"

class SurfaceLaptopKeyboard;
class SurfaceLaptopTouchpad;

class EXPORT SurfaceLaptop3HIDDriver : public IOService {
	OSDeclareDefaultStructors(SurfaceLaptop3HIDDriver)

public:
    bool init(OSDictionary *properties) override;
    
	IOService *probe(IOService *provider, SInt32 *score) override;
	
	bool start(IOService *provider) override;
	
	void stop(IOService *provider) override;
    
    void free() override;
    
    IOReturn setPowerState(unsigned long whichState, IOService * device) override;
    
    IOReturn getKeyboardDescriptor(SurfaceHIDDescriptor *desc);
    
    IOReturn getKeyboardAttributes(SurfaceHIDAttributes *attr);
    
    IOReturn getKeyboardReportDescriptor(UInt8 *buffer, UInt16 len);
    
    IOReturn getKeyboardRawReport(UInt8 report_id, UInt8 *buffer, UInt16 len);
    
    void setKeyboardRawReport(UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len);
    
    IOReturn getTouchpadDescriptor(SurfaceHIDDescriptor *desc);
    
    IOReturn getTouchpadAttributes(SurfaceHIDAttributes *attr);
    
    IOReturn getTouchpadReportDescriptor(UInt8 *buffer, UInt16 len);
    
    IOReturn getTouchpadRawReport(UInt8 report_id, UInt8 *buffer, UInt16 len);
    
    void setTouchpadRawReport(UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len);
    
private:
    IOWorkLoop*                     work_loop {nullptr};
    IOInterruptEventSource*         kbd_interrupt {nullptr};
    IOInterruptEventSource*         tpd_interrupt {nullptr};
    SurfaceLaptop3Nub*              nub {nullptr};
    SurfaceLaptopKeyboard*          keyboard {nullptr};
    SurfaceLaptopTouchpad*          touchpad {nullptr};
    
    bool    awake {false};
    UInt8   kbd_report[32];
    UInt16  kbd_report_len {0};
    UInt8   kbd_country_code {0x21}; // default US
    UInt8   tpd_report[32];
    UInt16  tpd_report_len {0};
    
    void eventReceived(SurfaceLaptop3Nub *sender, SurfaceLaptop3HIDDeviceType device, UInt8 *buffer, UInt16 len);
    
    void keyboardInputReceived(OSObject *owner, IOInterruptEventSource *sender, int count);
    
    void touchpadInputReceived(OSObject *owner, IOInterruptEventSource *sender, int count);
    
    void releaseResources();
};

#endif /* SurfaceLaptopHIDDriver_hpp */
