//
//  SurfaceLaptop3Nub.hpp
//  SurfaceSerialHubDevices
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceLaptop3Nub_hpp
#define SurfaceLaptop3Nub_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOInterruptEventSource.h>

#include "../SurfaceSerialHub/SurfaceSerialHubDriver.hpp"

enum SurfaceHIDDescriptorEntry {
    SurfaceDescriptorHIDEntry = 0,
    SurfaceDescriptorReportEntry = 1,
    SurfaceDescriptorAttributesEntry = 2,
};

enum SurfaceLaptop3HIDDeviceType {
    SurfaceLaptop3Keyboard = 0x01,
    SurfaceLaptop3Touchpad = 0x03,
};

struct PACKED SurfaceHIDDescriptorBufferHeader {
    UInt8  entry;
    UInt32 offset;
    UInt32 length;
    UInt8  finished;
    UInt8  data[];
};

struct PACKED SurfaceHIDDescriptor {
    UInt8  desc_len;            /* = 9 */
    UInt8  desc_type;           /* = HID_DT_HID */
    UInt16 hid_version;
    UInt8  country_code;
    UInt8  num_descriptors;     /* = 1 */
    UInt8  report_desc_type;    /* = HID_DT_REPORT */
    UInt16 report_desc_len;
};

struct PACKED SurfaceHIDAttributes {
    UInt32 length;
    UInt16 vendor;
    UInt16 product;
    UInt16 version;
    UInt8  _unknown[22];
};

#define SURFACE_HID_DESC_HEADER_SIZE sizeof(SurfaceHIDDescriptorBufferHeader)

class EXPORT SurfaceLaptop3Nub : public SurfaceSerialHubClient {
    OSDeclareDefaultStructors(SurfaceLaptop3Nub)

public:
    typedef void (*EventHandler)(OSObject *owner, SurfaceLaptop3Nub *sender, SurfaceLaptop3HIDDeviceType device, UInt8 *buffer, UInt16 len);
    
    bool attach(IOService* provider) override;
    
    void detach(IOService* provider) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;
    
    IOReturn registerHIDEvent(OSObject* owner, EventHandler _handler);
    
    void unregisterHIDEvent(OSObject* owner);
    
    void eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) override;
    
    IOReturn getHIDDescriptor(SurfaceLaptop3HIDDeviceType device, SurfaceHIDDescriptor *desc);
    
    IOReturn getHIDAttributes(SurfaceLaptop3HIDDeviceType device, SurfaceHIDAttributes *attr);
    
    IOReturn getHIDReportDescriptor(SurfaceLaptop3HIDDeviceType device, UInt8 *buffer, UInt16 len);
    
    IOReturn getHIDRawReport(SurfaceLaptop3HIDDeviceType device, UInt8 report_id, UInt8 *buffer, UInt16 len);
    
    void setHIDRawReport(SurfaceLaptop3HIDDeviceType device, UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len);
    
private:
    SurfaceSerialHubDriver* ssh {nullptr};
    OSObject*               target {nullptr};
    EventHandler            handler {nullptr};
    
    bool    awake {false};
    bool    registered {false};

    IOReturn getHIDData(SurfaceLaptop3HIDDeviceType device, SurfaceHIDDescriptorEntry entry, UInt8 *buffer, UInt16 buffer_len);
};

#endif /* SurfaceLaptop3Nub_hpp */
