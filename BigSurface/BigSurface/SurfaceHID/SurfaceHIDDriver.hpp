//
//  SurfaceHIDDriver.hpp
//  SurfaceHID
//
//  Created by Xavier on 2022/5/16.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceHIDDriver_hpp
#define SurfaceHIDDriver_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOInterruptEventSource.h>

#include "../SurfaceSerialHubDevices/SurfaceHIDNub.hpp"

class SurfaceHIDDevice;

class EXPORT SurfaceHIDDriver : public IOService {
	OSDeclareDefaultStructors(SurfaceHIDDriver)

public:
    bool init(OSDictionary *properties) override;
    
	IOService *probe(IOService *provider, SInt32 *score) override;
	
	bool start(IOService *provider) override;
	
	void stop(IOService *provider) override;
    
    void free() override;
    
    IOReturn getHIDDescriptor(SurfaceHIDDeviceType device, SurfaceHIDDescriptor *desc);
    
    IOReturn getHIDAttributes(SurfaceHIDDeviceType device, SurfaceHIDAttributes *attr);
    
    IOReturn getReportDescriptor(SurfaceHIDDeviceType device, UInt8 *buffer, UInt16 len);
    
    IOReturn getRawReport(SurfaceHIDDeviceType device, UInt8 report_id, UInt8 *buffer, UInt16 len);
    
    void setRawReport(SurfaceHIDDeviceType device, UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len);
    
private:
    SurfaceHIDNub*          nub {nullptr};
    IOWorkLoop*             work_loop {nullptr};
    IOInterruptEventSource* kbd_interrupt {nullptr};
    IOInterruptEventSource* tpd_interrupt {nullptr};
    SurfaceHIDDevice*       keyboard {nullptr};
    SurfaceHIDDevice*       touchpad {nullptr};
    
    bool    awake {true};
    bool    legacy {false};
    bool    ready {false};
    UInt8   kbd_report[32];
    UInt16  kbd_report_len {0};
    UInt8   tpd_report[32];
    UInt16  tpd_report_len {0};
    
    void eventReceived(SurfaceHIDNub *sender, SurfaceHIDDeviceType device, UInt8 *buffer, UInt16 len);
    
    void keyboardInputReceived(IOInterruptEventSource *sender, int count);
    
    void touchpadInputReceived(IOInterruptEventSource *sender, int count);
    
    void releaseResources();
};

#endif /* SurfaceHIDDriver_hpp */
