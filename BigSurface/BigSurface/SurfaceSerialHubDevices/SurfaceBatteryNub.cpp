//
//  SurfaceBatteryNub.cpp
//  SurfaceSerialHubDevices
//
//  Created by Xavier on 2022/5/19.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceBatteryNub.hpp"

#define super SurfaceSerialHubClient
OSDefineMetaClassAndStructors(SurfaceBatteryNub, SurfaceSerialHubClient)

bool SurfaceBatteryNub::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;

    ssh = OSDynamicCast(SurfaceSerialHubDriver, provider);
    if (!ssh)
        return false;

    return true;
}

void SurfaceBatteryNub::detach(IOService* provider) {
    ssh = nullptr;
    super::detach(provider);
}

bool SurfaceBatteryNub::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    
    PMinit();
    ssh->joinPMtree(this);
    registerPowerDriver(this, myIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    return true;
}

void SurfaceBatteryNub::stop(IOService *provider) {
    unregisterBatteryEvent(target);
    super::stop(provider);
}

IOReturn SurfaceBatteryNub::setPowerState(unsigned long whichState, IOService *device) {
    if (device != this)
        return kIOReturnInvalid;
    return kIOPMAckImplied;
}

IOReturn SurfaceBatteryNub::registerBatteryEvent(OSObject* owner, EventHandler _handler) {
    if (!owner || !_handler)
        return kIOReturnError;
    if (target) {
        LOG("Battery event already registered for a handler!");
        return kIOReturnNoResources;
    }
    
    IOReturn ret = ssh->registerEvent(this, SurfaceSerialEventHostManagedV1, SSH_TC_BAT, 0x00);
    if (ret != kIOReturnSuccess)
        return ret;
    
    target = owner;
    handler = _handler;
    return kIOReturnSuccess;
}

void SurfaceBatteryNub::unregisterBatteryEvent(OSObject* owner) {
    if (target) {
        if (target == owner) {
            ssh->unregisterEvent(this, SurfaceSerialEventHostManagedV1, SSH_TC_BAT, 0x00);
            target = nullptr;
            handler = nullptr;
        } else
            LOG("Battery event not registered for this handler!");
    }
}

void SurfaceBatteryNub::eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) {
    // iid corresponds to battery/adaptor index, starts with 1
    SurfaceBatteryEventType type;
    switch (cid) {
        case SSH_EVENT_CID_BAT_BIX:
            type = SurfaceBatteryInformationChanged;
            break;
        case SSH_EVENT_CID_BAT_BST:
            type = SurfaceBatteryStatusChanged;
            break;
        case SSH_EVENT_CID_BAT_PSR:
            type = SurfaceAdaptorStatusChanged;
            break;
        default:
            DBG_LOG("Unknown battery event! tid: %d, iid: %d, cid: %x, data_len: %d", tid, iid, cid, length);
            return;
    }
    if (handler)
        handler(target, this, type);
}

IOReturn SurfaceBatteryNub::getBatteryConnection(UInt8 index, bool *connected) {
    UInt32 sta = 0x00;
    if (ssh->getResponse(SSH_TC_BAT, index, 0x01, SSH_CID_BAT_STA, nullptr, 0, true, reinterpret_cast<UInt8 *>(&sta), 4) != kIOReturnSuccess)
        return kIOReturnError;

    *connected = (sta & 0x10) ? true : false;
    
    return kIOReturnSuccess;
}

IOReturn SurfaceBatteryNub::getBatteryInformation(UInt8 index, OSArray **bix) {
    UInt8 buffer[BIX_LENGTH];
    if (ssh->getResponse(SSH_TC_BAT, index, 0x01, SSH_CID_BAT_BIX, nullptr, 0, true, buffer, BIX_LENGTH) != kIOReturnSuccess)
        return kIOReturnError;
    
    UInt32 *temp = reinterpret_cast<UInt32 *>(buffer+1);
    const char *str = reinterpret_cast<const char *>(buffer);
    
    OSNumber *revision = OSNumber::withNumber(*buffer, 8);
    OSNumber *power_unit = OSNumber::withNumber(temp[0], 32);
    OSNumber *design_cap = OSNumber::withNumber(temp[1], 32);
    OSNumber *last_full_cap = OSNumber::withNumber(temp[2], 32);
    OSNumber *bat_tech = OSNumber::withNumber(temp[3], 32);
    OSNumber *design_volt = OSNumber::withNumber(temp[4], 32);
    OSNumber *design_cap_warn = OSNumber::withNumber(temp[5], 32);
    OSNumber *design_cap_low = OSNumber::withNumber(temp[6], 32);
    OSNumber *cycle_cnt = OSNumber::withNumber(temp[7], 32);
    OSNumber *mesure_acc = OSNumber::withNumber(temp[8], 32);
    OSNumber *max_sample_t = OSNumber::withNumber(temp[9], 32);
    OSNumber *min_sample_t = OSNumber::withNumber(temp[10], 32);
    OSNumber *max_avg_interval = OSNumber::withNumber(temp[11], 32);
    OSNumber *min_avg_interval = OSNumber::withNumber(temp[12], 32);
    OSNumber *bat_cap_gra_1 = OSNumber::withNumber(temp[13], 32);
    OSNumber *bat_cap_gra_2 = OSNumber::withNumber(temp[14], 32);
    OSString *model_number = OSString::withCString(str+0x3D);
    OSString *serial_number = OSString::withCString(str+0x52);
    OSString *bat_type = OSString::withCString(str+0x5D);
    OSString *oem_info = OSString::withCString(str+0x62);
    
    const OSObject *arr[] = {revision, power_unit, design_cap, last_full_cap, bat_tech,
                        design_volt, design_cap_warn, design_cap_low, cycle_cnt,
                        mesure_acc, max_sample_t, min_sample_t, max_avg_interval,
                        min_avg_interval, bat_cap_gra_1, bat_cap_gra_2, model_number,
                        serial_number, bat_type, oem_info};
    *bix = OSArray::withObjects(arr, 20);
    if (!(*bix)) {
        LOG("Could not create a 20 sized array!");
        for (int i=0; i<20; i++)
            OSSafeReleaseNULL(arr[i]);
        return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}


IOReturn SurfaceBatteryNub::getBatteryStatus(UInt8 index, UInt32 *bst, UInt16 *temp) {
    if (ssh->getResponse(SSH_TC_BAT, index, 0x01, SSH_CID_BAT_BST, nullptr, 0, true, reinterpret_cast<UInt8 *>(bst), BST_LENGTH) != kIOReturnSuccess)
        return kIOReturnError;
    
    
    if (ssh->getResponse(SSH_TC_TMP, SSH_TID_PRIMARY, SSH_TEMP_SENSOR_BAT, SSH_CID_TMP_SENSOR, nullptr, 0, true, reinterpret_cast<UInt8 *>(temp), 2) != kIOReturnSuccess)
        LOG("Failed to get battery temperature!");
    
    return kIOReturnSuccess;
}

IOReturn SurfaceBatteryNub::getAdaptorStatus(UInt32 *psr) {
    return ssh->getResponse(SSH_TC_BAT, SSH_TID_PRIMARY, 0x01, SSH_CID_BAT_PSR, nullptr, 0, true, reinterpret_cast<UInt8 *>(psr), 4);
}

IOReturn SurfaceBatteryNub::setPerformanceMode(UInt32 mode) {
    if (!ssh->sendCommand(SSH_TC_TMP, SSH_TID_PRIMARY, 0, SSH_CID_TMP_SET_PERF, reinterpret_cast<UInt8 *>(&mode), 4, true))
        return kIOReturnError;
    else
        return kIOReturnSuccess;
}
