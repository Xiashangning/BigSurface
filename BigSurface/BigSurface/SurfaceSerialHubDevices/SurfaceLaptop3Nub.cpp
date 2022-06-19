//
//  SurfaceLaptop3Nub.cpp
//  SurfaceSerialHubDevices
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceLaptop3Nub.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", "SurfaceLaptop3Nub", ##__VA_ARGS__)

#define super SurfaceSerialHubClient
OSDefineMetaClassAndStructors(SurfaceLaptop3Nub, SurfaceSerialHubClient)

bool SurfaceLaptop3Nub::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;

    ssh = OSDynamicCast(SurfaceSerialHubDriver, provider);
    if (!ssh)
        return false;

    return true;
}

void SurfaceLaptop3Nub::detach(IOService* provider) {
    ssh = nullptr;
    super::detach(provider);
}

bool SurfaceLaptop3Nub::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    
    SurfaceHIDDescriptor desc;
    if (getHIDDescriptor(SurfaceLaptop3Keyboard, &desc) != kIOReturnSuccess)
        return false;

    PMinit();
    ssh->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    return true;
}

void SurfaceLaptop3Nub::stop(IOService *provider) {
    unregisterHIDEvent(target);
    PMstop();
    super::stop(provider);
}

IOReturn SurfaceLaptop3Nub::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            awake = false;
            if (target && registered) {
                ssh->unregisterEvent(this, SSH_TC_HID, 0x00);
                registered = false;
            }
            LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            if (target && !registered) {
                IOSleep(50); // let UART and SSH be prepared
                if (ssh->registerEvent(this, SSH_TC_HID, 0x00) != kIOReturnSuccess)
                    LOG("HID event registration failed!");
                registered = true;
            }
            LOG("Woke up");
        }
    }
    return kIOPMAckImplied;
}

IOReturn SurfaceLaptop3Nub::registerHIDEvent(OSObject* owner, EventHandler _handler) {
    if (!owner || !_handler)
        return kIOReturnError;
    if (target) {
        LOG("HID event already registered for a handler!");
        return kIOReturnNoResources;
    }
    
    IOReturn ret = ssh->registerEvent(this, SSH_TC_HID, 0x00);
    if (ret != kIOReturnSuccess)
        return ret;
    
    target = owner;
    handler = _handler;
    registered = true;
    return kIOReturnSuccess;
}

void SurfaceLaptop3Nub::unregisterHIDEvent(OSObject* owner) {
    if (target) {
        if (target == owner) {
            ssh->unregisterEvent(this, SSH_TC_HID, 0x00);
            target = nullptr;
            handler = nullptr;
            registered = false;
        } else
            LOG("HID event not registered for this handler!");
    }
}

void SurfaceLaptop3Nub::eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) {
    if (!awake)
        return;
    
    if (cid != SSH_EVENT_CID_HID_INPUT || tid != SSH_TID_SECONDARY) {
        LOG("Unknown HID event with tid: %d, iid: %d, cid: %d, data_len: %d", tid, iid, cid, length);
        return;
    }
    // iid=0x01 kb len 12, 0x03 touchpad len 6 for sl3
    SurfaceLaptop3HIDDeviceType device;
    switch (iid) {
        case SurfaceLaptop3Keyboard:
            device = SurfaceLaptop3Keyboard;
            break;
        case SurfaceLaptop3Touchpad:
            device = SurfaceLaptop3Touchpad;
            break;
        default:
            LOG("Unknown HID event with tid: %d, iid: %d, cid: %d, data_len: %d", tid, iid, cid, length);
            return;
    }
    if (handler)
        handler(target, this, device, data_buffer, length);
}

IOReturn SurfaceLaptop3Nub::getHIDDescriptor(SurfaceLaptop3HIDDeviceType device, SurfaceHIDDescriptor *desc) {
    return getHIDData(device, SurfaceDescriptorHIDEntry, reinterpret_cast<UInt8 *>(desc), sizeof(SurfaceHIDDescriptor));
}

IOReturn SurfaceLaptop3Nub::getHIDAttributes(SurfaceLaptop3HIDDeviceType device, SurfaceHIDAttributes *attr) {
    return getHIDData(device, SurfaceDescriptorAttributesEntry, reinterpret_cast<UInt8 *>(attr), sizeof(SurfaceHIDAttributes));
}

IOReturn SurfaceLaptop3Nub::getHIDReportDescriptor(SurfaceLaptop3HIDDeviceType device, UInt8 *buffer, UInt16 len) {
    return getHIDData(device, SurfaceDescriptorReportEntry, buffer, len);
}

IOReturn SurfaceLaptop3Nub::getHIDData(SurfaceLaptop3HIDDeviceType device, SurfaceHIDDescriptorEntry entry, UInt8 *buffer, UInt16 buffer_len) {
    UInt8 cache[SURFACE_HID_DESC_HEADER_SIZE + 0x76];    // 0x76 is used by windows driver
    SurfaceHIDDescriptorBufferHeader *cache_as_buf = reinterpret_cast<SurfaceHIDDescriptorBufferHeader *>(cache);
    
    UInt16 rx_data_len = sizeof(cache) - SURFACE_HID_DESC_HEADER_SIZE;
    UInt16 offset = 0;
    UInt16 length = rx_data_len;
    UInt16 remain_len = buffer_len;
    UInt16 response_len;
    
    cache_as_buf->entry = entry;
    cache_as_buf->finished = false;
    
    while (!cache_as_buf->finished && offset < buffer_len) {
        response_len = rx_data_len > remain_len ? SURFACE_HID_DESC_HEADER_SIZE + remain_len : sizeof(cache);
        
        cache_as_buf->offset = offset;
        cache_as_buf->length = length;

        if (ssh->getResponse(SSH_TC_HID, SSH_TID_SECONDARY, device, SSH_CID_HID_GET_DESCRIPTOR, cache, SURFACE_HID_DESC_HEADER_SIZE, true, cache, response_len) != kIOReturnSuccess) {
            LOG("Failed to get data from SSH!");
            return kIOReturnError;
        }

        offset = cache_as_buf->offset;
        length = cache_as_buf->length;

        // Don't mess stuff up in case we receive garbage.
        if (length > rx_data_len || offset > buffer_len) {
            LOG("Received bogus data!");
            return kIOReturnError;
        }

        if (offset + length > buffer_len) {
            LOG("Not enough space for buffer, need at least %d", offset+length);
            length = buffer_len - offset;
        }

        memcpy(buffer + offset, &cache_as_buf->data[0], length);

        offset += length;
        remain_len -= length;
        length = rx_data_len;
    }

    if (offset != buffer_len) {
        LOG("Unexpected descriptor length: got %u, expected %u", offset, buffer_len);
        return kIOReturnError;
    }
    return kIOReturnSuccess;
}

IOReturn SurfaceLaptop3Nub::getHIDRawReport(SurfaceLaptop3HIDDeviceType device, UInt8 report_id, UInt8 *buffer, UInt16 len) {
    return ssh->getResponse(SSH_TC_HID, SSH_TID_SECONDARY, device, SSH_CID_HID_GET_FEAT_REPORT, &report_id, 1, true, buffer, len);
}

void SurfaceLaptop3Nub::setHIDRawReport(SurfaceLaptop3HIDDeviceType device, UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len) {
    UInt8 cid = feature ? SSH_CID_HID_SET_FEAT_REPORT : SSH_CID_HID_OUT_REPORT;
    buffer[0] = report_id;
    ssh->sendCommand(SSH_TC_HID, SSH_TID_SECONDARY, device, cid, buffer, len, true);
}
