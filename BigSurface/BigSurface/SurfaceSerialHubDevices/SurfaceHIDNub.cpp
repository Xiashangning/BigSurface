//
//  SurfaceHIDNub.cpp
//  SurfaceSerialHubDevices
//
//  Created by Xavier on 2022/5/20.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include "SurfaceHIDNub.hpp"

#define super SurfaceSerialHubClient
OSDefineMetaClassAndStructors(SurfaceHIDNub, SurfaceSerialHubClient)

bool SurfaceHIDNub::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;

    ssh = OSDynamicCast(SurfaceSerialHubDriver, provider);
    if (!ssh)
        return false;

    return true;
}

void SurfaceHIDNub::detach(IOService* provider) {
    ssh = nullptr;
    super::detach(provider);
}

bool SurfaceHIDNub::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    
    SurfaceHIDDescriptor desc;
    if (getHIDDescriptor(SurfaceLegacyKeyboardDevice, &desc) != kIOReturnSuccess) {
        legacy = false;
        if (getHIDDescriptor(SurfaceKeyboardDevice, &desc) != kIOReturnSuccess)
            return false;
    }
    LOG("HID version %d", !legacy+1);
    setProperty(SURFACE_LEGACY_HID_STRING, legacy);
    
    PMinit();
    ssh->joinPMtree(this);
    registerPowerDriver(this, myIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    return true;
}

void SurfaceHIDNub::stop(IOService *provider) {
    unregisterHIDEvent(target);
    super::stop(provider);
}

IOReturn SurfaceHIDNub::setPowerState(unsigned long whichState, IOService *device) {
    if (device != this)
        return kIOReturnInvalid;
    return kIOPMAckImplied;
}

IOReturn SurfaceHIDNub::registerHIDEvent(OSObject* owner, EventHandler _handler) {
    if (!owner || !_handler)
        return kIOReturnError;
    if (target) {
        LOG("HID event already registered for a handler!");
        return kIOReturnNoResources;
    }
    
    IOReturn ret;
    if (legacy)
        ret = ssh->registerEvent(this, SurfaceSerialEventHostManagedV1, SSH_TC_KBD, SurfaceLegacyKeyboardDevice);
    else {
        ret = ssh->registerEvent(this, SurfaceSerialEventHostManagedV2, SSH_TC_HID, SurfaceKeyboardDevice);
        if (ret == kIOReturnSuccess)
            ret = ssh->registerEvent(this, SurfaceSerialEventHostManagedV2, SSH_TC_HID, SurfaceTouchpadDevice);
    }
    if (ret != kIOReturnSuccess)
        return ret;
    
    target = owner;
    handler = _handler;
    return kIOReturnSuccess;
}

void SurfaceHIDNub::unregisterHIDEvent(OSObject* owner) {
    if (!target)
        return;
    if (target == owner) {
        if (legacy)
            ssh->unregisterEvent(this, SurfaceSerialEventHostManagedV1, SSH_TC_KBD, SurfaceLegacyKeyboardDevice);
        else {
            ssh->unregisterEvent(this, SurfaceSerialEventHostManagedV2, SSH_TC_HID, SurfaceKeyboardDevice);
            ssh->unregisterEvent(this, SurfaceSerialEventHostManagedV2, SSH_TC_HID, SurfaceTouchpadDevice);
        }
        target = nullptr;
        handler = nullptr;
    } else
        LOG("HID event not registered for this handler!");
}

void SurfaceHIDNub::eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) {
    SurfaceHIDDeviceType device;
    if (legacy) {
        if ((cid != SSH_EVENT_CID_KBD_INPUT_GENERIC && cid != SSH_EVENT_CID_KBD_INPUT_HOTKEYS)
            || tid != SSH_TID_SECONDARY
            || iid != SurfaceLegacyKeyboardDevice)
            goto error;
        
        device = SurfaceLegacyKeyboardDevice;
    } else {
        if (cid != SSH_EVENT_CID_HID_INPUT || tid != SSH_TID_SECONDARY)
            goto error;
        
        switch (iid) {
            case SurfaceKeyboardDevice:
                device = SurfaceKeyboardDevice;
                break;
            case SurfaceTouchpadDevice:
                device = SurfaceTouchpadDevice;
                break;
            default:
                goto error;
        }
    }
    
    if (handler)
        handler(target, this, device, data_buffer, length);
    return;
    
error:
    DBG_LOG("Unknown HID event with tid: %d, iid: %d, cid: %d, data_len: %d", tid, iid, cid, length);
}

IOReturn SurfaceHIDNub::getHIDDescriptor(SurfaceHIDDeviceType device, SurfaceHIDDescriptor *desc) {
    return getDescriptorData(device, SurfaceHIDDescriptorEntry, reinterpret_cast<UInt8 *>(desc), sizeof(SurfaceHIDDescriptor));
}

IOReturn SurfaceHIDNub::getHIDAttributes(SurfaceHIDDeviceType device, SurfaceHIDAttributes *attr) {
    return getDescriptorData(device, SurfaceHIDAttributesEntry, reinterpret_cast<UInt8 *>(attr), sizeof(SurfaceHIDAttributes));
}

IOReturn SurfaceHIDNub::getReportDescriptor(SurfaceHIDDeviceType device, UInt8 *buffer, UInt16 len) {
    return getDescriptorData(device, SurfaceReportDescriptorEntry, buffer, len);
}

IOReturn SurfaceHIDNub::getDescriptorData(SurfaceHIDDeviceType device, SurfaceHIDDescriptorEntryType entry, UInt8 *buffer, UInt16 buffer_len) {
    if (legacy)
        return getLegacyData(device, entry, buffer, buffer_len);
    else
        return getData(device, entry, buffer, buffer_len);
}

IOReturn SurfaceHIDNub::getLegacyData(SurfaceHIDDeviceType device, SurfaceHIDDescriptorEntryType entry, UInt8 *buffer, UInt16 buffer_len) {
    return ssh->getResponse(SSH_TC_KBD, SSH_TID_SECONDARY, device, SSH_CID_KBD_GET_DESCRIPTOR, reinterpret_cast<UInt8 *>(&entry), 1, true, buffer, buffer_len);
}

IOReturn SurfaceHIDNub::getData(SurfaceHIDDeviceType device, SurfaceHIDDescriptorEntryType entry, UInt8 *buffer, UInt16 buffer_len) {
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

IOReturn SurfaceHIDNub::getHIDRawReport(SurfaceHIDDeviceType device, UInt8 report_id, UInt8 *buffer, UInt16 len) {
    if (legacy) {
        UInt8 payload = 0;
        UInt8 report[SURFACE_LEGACY_FEAT_REPORT_SIZE];
        
        if (len < SURFACE_LEGACY_FEAT_REPORT_SIZE)
            return kIOReturnNoSpace;
        
        IOReturn ret = ssh->getResponse(SSH_TC_KBD, SSH_TID_SECONDARY, device, SSH_CID_KBD_GET_FEAT_REPORT, &payload, 1, true, report, SURFACE_LEGACY_FEAT_REPORT_SIZE);
        if (ret != kIOReturnSuccess)
            return ret;
        if (report[0] != report_id)
            return kIOReturnUnsupported;
        
        memcpy(buffer, report, SURFACE_LEGACY_FEAT_REPORT_SIZE);
        return kIOReturnSuccess;
    } else
        return ssh->getResponse(SSH_TC_HID, SSH_TID_SECONDARY, device, SSH_CID_HID_GET_FEAT_REPORT, &report_id, 1, true, buffer, len);
}

void SurfaceHIDNub::setHIDRawReport(SurfaceHIDDeviceType device, UInt8 report_id, bool feature, UInt8 *buffer, UInt16 len) {
    if (!legacy) {
        UInt8 cid = feature ? SSH_CID_HID_SET_FEAT_REPORT : SSH_CID_HID_OUT_REPORT;
        buffer[0] = report_id;
        ssh->sendCommand(SSH_TC_HID, SSH_TID_SECONDARY, device, cid, buffer, len, true);
    }
}
