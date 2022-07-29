//
//  IntelPreciseTouchStylusUserClient.cpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/11.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "IntelPreciseTouchStylusUserClient.hpp"

#define super IOUserClient
OSDefineMetaClassAndStructors(IntelPreciseTouchStylusUserClient, IOUserClient);

const IOExternalMethodDispatch IntelPreciseTouchStylusUserClient::methods[kNumberOfMethods] = {
    [kMethodGetDeviceInfo] = {
        .function = (IOExternalMethodAction)&IntelPreciseTouchStylusUserClient::sMethodGetDeviceInfo,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = 0,
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = sizeof(IPTSDeviceInfo),
    },
    [kMethodReceiveInput] = {
        .function = (IOExternalMethodAction)&IntelPreciseTouchStylusUserClient::sMethodReceiveInput,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = 0,
        .checkScalarOutputCount = 1,    // rx buffer index
        .checkStructureOutputSize = 0,
    },
    [kMethodSendHIDReport] = {
        .function = (IOExternalMethodAction)&IntelPreciseTouchStylusUserClient::sMethodSendHIDReport,
        .checkScalarInputCount = 0,
        .checkStructureInputSize = sizeof(IPTSHIDReport),
        .checkScalarOutputCount = 0,
        .checkStructureOutputSize = 0,
    },
};

IOReturn IntelPreciseTouchStylusUserClient::externalMethod(uint32_t selector, IOExternalMethodArguments *arguments, IOExternalMethodDispatch *dispatch, OSObject *target, void *reference) {
    if (selector < kNumberOfMethods) {
        dispatch = const_cast<IOExternalMethodDispatch *>(&methods[selector]);
        if (!target)
            target = this;
    }
    return super::externalMethod(selector, arguments, dispatch, target, reference);
}

IOReturn IntelPreciseTouchStylusUserClient::clientMemoryForType(UInt32 type, IOOptionBits *options, IOMemoryDescriptor **memory) {
    if (type >= IPTS_BUFFER_NUM)
        return kIOReturnBadArgument;
    
    *memory = driver->getReceiveBufferForIndex(type);
    *options |= kIOMapReadOnly;
    return kIOReturnSuccess;
}

bool IntelPreciseTouchStylusUserClient::initWithTask(task_t owningTask, void *securityToken, UInt32 type, OSDictionary *properties) {
    if (!owningTask)
        return false;
    if (!super::initWithTask(owningTask, securityToken, type))
        return false;
    
    task = owningTask;
    return true;
}

bool IntelPreciseTouchStylusUserClient::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    
    driver = OSDynamicCast(IntelPreciseTouchStylusDriver, provider);
    if (!driver)
        return false;
    
    return true;
}

void IntelPreciseTouchStylusUserClient::stop(IOService* provider) {
    super::stop(provider);
}

IOReturn IntelPreciseTouchStylusUserClient::clientClose(void) {
    if (!isInactive())
        terminate();
    return kIOReturnSuccess;
}

IOReturn IntelPreciseTouchStylusUserClient::sMethodGetDeviceInfo(OSObject *target, void *ref, IOExternalMethodArguments *args) {
    IntelPreciseTouchStylusUserClient *that = OSDynamicCast(IntelPreciseTouchStylusUserClient, target);
    if (!that)
        return kIOReturnError;
    return that->getDeviceInfo(ref, args);
}

IOReturn IntelPreciseTouchStylusUserClient::getDeviceInfo(void *ref, IOExternalMethodArguments *args) {
    IPTSDeviceInfo *info = reinterpret_cast<IPTSDeviceInfo *>(args->structureOutput);
    info->vendor_id = driver->getVendorID();
    info->product_id = driver->getDeviceID();
    info->max_contacts = driver->getMaxContacts();
    return kIOReturnSuccess;
}

IOReturn IntelPreciseTouchStylusUserClient::sMethodReceiveInput(OSObject *target, void *ref, IOExternalMethodArguments *args) {
    IntelPreciseTouchStylusUserClient *that = OSDynamicCast(IntelPreciseTouchStylusUserClient, target);
    if (!that)
        return kIOReturnError;
    return that->receiveInput(ref, args);
}

IOReturn IntelPreciseTouchStylusUserClient::receiveInput(void *ref, IOExternalMethodArguments *args) {
    UInt8 buffer_idx;
    if (driver->getCurrentInputBuffer(&buffer_idx) != kIOReturnSuccess)
        return kIOReturnError;
    args->scalarOutput[0] = buffer_idx;
    return kIOReturnSuccess;
}

IOReturn IntelPreciseTouchStylusUserClient::sMethodSendHIDReport(OSObject *target, void *ref, IOExternalMethodArguments *args) {
    IntelPreciseTouchStylusUserClient *that = OSDynamicCast(IntelPreciseTouchStylusUserClient, target);
    if (!that)
        return kIOReturnError;
    return that->sendHIDReport(ref, args);
}

IOReturn IntelPreciseTouchStylusUserClient::sendHIDReport(void *ref, IOExternalMethodArguments *args) {
    driver->handleHIDReport(reinterpret_cast<const IPTSHIDReport *>(args->structureInput));
    return kIOReturnSuccess;
}
