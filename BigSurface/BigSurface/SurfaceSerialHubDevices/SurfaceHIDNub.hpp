//
//  SurfaceHIDNub.hpp
//  SurfaceSerialHubDevices
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceHIDNub_hpp
#define SurfaceHIDNub_hpp

#include "../SurfaceSerialHub/SurfaceSerialHubDriver.hpp"

enum SurfaceHIDDescriptorEntryType : UInt8 {
    SurfaceHIDDescriptorEntry       = 0,
    SurfaceHIDAttributesEntry       = 2,
    SurfaceReportDescriptorEntry    = 1,
};

enum SurfaceHIDDeviceType {
    SurfaceLegacyKeyboardDevice = 0x00,
    SurfaceKeyboardDevice       = 0x01,
    SurfaceTouchpadDevice       = 0x03,
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

#define SURFACE_LEGACY_HID_STRING       "SurfaceLegacyHID"
#define SURFACE_LEGACY_FEAT_REPORT_SIZE 7

#define SURFACE_HID_DESC_HEADER_SIZE sizeof(SurfaceHIDDescriptorBufferHeader)

class EXPORT SurfaceHIDNub : public SurfaceSerialHubClient {
    OSDeclareDefaultStructors(SurfaceHIDNub)

public:
    typedef void (*EventHandler)(OSObject *owner, SurfaceHIDNub *sender, SurfaceHIDDeviceType device, UInt8 *buffer, UInt16 len);
    
    bool attach(IOService* provider) override;
    
    void detach(IOService* provider) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn registerHIDEvent(OSObject* owner, EventHandler _handler);
    
    void unregisterHIDEvent(OSObject* owner);
    
    void eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) override;
    
    IOReturn getHIDDescriptor(SurfaceHIDDeviceType device, SurfaceHIDDescriptor *desc);
    
    IOReturn getHIDAttributes(SurfaceHIDDeviceType device, SurfaceHIDAttributes *attr);
    
    IOReturn getReportDescriptor(SurfaceHIDDeviceType device, UInt8 *buffer, UInt16 len);
    
    IOReturn getHIDRawReport(SurfaceHIDDeviceType device, UInt8 report_id, UInt8 *buffer, UInt16 len);
    
    void setHIDRawReport(SurfaceHIDDeviceType device, UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len);
    
private:
    SurfaceSerialHubDriver* ssh {nullptr};
    OSObject*               target {nullptr};
    EventHandler            handler {nullptr};

    bool    legacy {true};

    IOReturn getDescriptorData(SurfaceHIDDeviceType device, SurfaceHIDDescriptorEntryType entry, UInt8 *buffer, UInt16 buffer_len);
    
    IOReturn getLegacyData(SurfaceHIDDeviceType device, SurfaceHIDDescriptorEntryType entry, UInt8 *buffer, UInt16 buffer_len);
    IOReturn getData(SurfaceHIDDeviceType device, SurfaceHIDDescriptorEntryType entry, UInt8 *buffer, UInt16 buffer_len);
};

#endif /* SurfaceHIDNub_hpp */
