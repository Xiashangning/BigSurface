//
//  SurfaceManagementEngineClient.cpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/1.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceManagementEngineClient.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", "SurfaceManagementEngineClient", ##__VA_ARGS__)

#define super IOService
OSDefineMetaClassAndStructors(SurfaceManagementEngineClient, IOService)

bool SurfaceManagementEngineClient::attach(IOService* provider) {
    if (!super::attach(provider))
        return false;

    api = OSDynamicCast(SurfaceManagementEngineDriver, provider);
    if (!api)
        return false;
    
    queue_head_init(rx_queue);

    return true;
}

void SurfaceManagementEngineClient::detach(IOService* provider) {
    api = nullptr;
    super::detach(provider);
}

bool SurfaceManagementEngineClient::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    
    queue_lock = IOLockAlloc();
    if (!queue_lock) {
        LOG("Failed to create lock");
        goto exit;
    }
    
    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Failed to create work loop");
        goto exit;
    }
    interrupt_source = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceManagementEngineClient::notifyMessage));
    if (!interrupt_source) {
        LOG("Failed to create interrupt source");
        goto exit;
    }
    work_loop->addEventSource(interrupt_source);
    
    uuid_t swapped_uuid;
    uuid_string_t uuid_str;
    uuid_copy(swapped_uuid, properties.uuid);
    *(reinterpret_cast<UInt32 *>(swapped_uuid)) = OSSwapInt32(*(reinterpret_cast<UInt32 *>(swapped_uuid)));
    *(reinterpret_cast<UInt16 *>(swapped_uuid) + 2) = OSSwapInt16(*(reinterpret_cast<UInt16 *>(swapped_uuid) + 2));
    *(reinterpret_cast<UInt16 *>(swapped_uuid) + 3) = OSSwapInt16(*(reinterpret_cast<UInt16 *>(swapped_uuid) + 3));
    uuid_unparse_lower(swapped_uuid, uuid_str);
    setProperty("MEIClientUUID", uuid_str);
    setProperty("MEIClientAddress", properties.fixed_address, 32);
    setProperty("MEIClientMaxMessageLength", properties.max_msg_length, 32);
    
    rx_cache = new UInt8[properties.max_msg_length];
    
    initial = false;
    
    registerService();
    return true;
exit:
    releaseResources();
    return false;
}

void SurfaceManagementEngineClient::stop(IOService *provider) {
    releaseResources();
    super::stop(provider);
}

void SurfaceManagementEngineClient::releaseResources() {
    MEIClientMessage *msg;
    qe_foreach_element_safe(msg, &rx_queue, entry) {
        remqueue(&msg->entry);
        delete[] msg->msg;
        delete msg;
    }
    if (rx_cache)
        delete[] rx_cache;
    
    if (interrupt_source) {
        interrupt_source->disable();
        work_loop->removeEventSource(interrupt_source);
        OSSafeReleaseNULL(interrupt_source);
    }
    OSSafeReleaseNULL(work_loop);
    if (queue_lock)
        IOLockFree(queue_lock);
}

IOReturn SurfaceManagementEngineClient::registerMessageHandler(OSObject *owner, MessageHandler _handler) {
    if (!owner || !_handler)
        return kIOReturnError;
    if (target) {
        LOG("Already has a handler!");
        return kIOReturnNoResources;
    }

    target = owner;
    handler = _handler;
    return kIOReturnSuccess;
}

void SurfaceManagementEngineClient::unregisterMessageHandler(OSObject *owner) {
    if (target && target == owner) {
        target = nullptr;
        handler = nullptr;
    }
}

IOReturn SurfaceManagementEngineClient::sendMessage(UInt8 *data, UInt16 data_len, bool blocking) {
    if (!active)
        return kIOReturnNoDevice;
    
    return api->sendClientMessage(this, data, data_len, blocking);
}

void SurfaceManagementEngineClient::resetProperties(MEIClientProperty *client_props, UInt8 me_addr) {
    active = true;
    properties = *client_props;
    addr = me_addr;
}

void SurfaceManagementEngineClient::hostRequestDisconnect() {
    rx_cache_pos = 0;
    
    IOLockLock(queue_lock);
    while (dequeue(&rx_queue));
    IOLockUnlock(queue_lock);
}

void SurfaceManagementEngineClient::hostRequestReconnect() {
    
}

void SurfaceManagementEngineClient::messageComplete() {
    if (!rx_cache_pos)
        return;
    
    MEIClientMessage *client_msg = new MEIClientMessage;
    client_msg->msg = new UInt8[rx_cache_pos];
    client_msg->len = rx_cache_pos;
    memcpy(client_msg->msg, rx_cache, rx_cache_pos);
    rx_cache_pos = 0;
    
    IOLockLock(queue_lock);
    enqueue(&rx_queue, &client_msg->entry);
    IOLockUnlock(queue_lock);
    interrupt_source->interruptOccurred(nullptr, this, 0);
}

void SurfaceManagementEngineClient::notifyMessage(IOInterruptEventSource *sender, int count) {
    MEIClientMessage *client_msg;
    queue_entry *item;
    
    IOLockLock(queue_lock);
    while ((item = dequeue(&rx_queue)) != nullptr) {
        IOLockUnlock(queue_lock);
        client_msg = qe_element(item, MEIClientMessage, entry);
        
        if (handler)
            handler(target, this, client_msg->msg, client_msg->len);
        
        delete[] client_msg->msg;
        delete client_msg;
        IOLockLock(queue_lock);
    }
    IOLockUnlock(queue_lock);
}
