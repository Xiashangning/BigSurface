//
//  VoodooI2CHIDTransducerWrapper.cpp
//  VoodooI2CHID
//
//  Created by Alexandre on 02/01/2018.
//  Copyright © 2018 Alexandre Daoud. All rights reserved.
//

#include "VoodooI2CHIDTransducerWrapper.hpp"

#define super OSObject
OSDefineMetaClassAndStructors(VoodooI2CHIDTransducerWrapper, OSObject);

bool VoodooI2CHIDTransducerWrapper::init() {
    if (!super::init())
        return false;

    transducers = OSArray::withCapacity(4);
    if (!transducers)
        return false;

    return true;
}

void VoodooI2CHIDTransducerWrapper::free() {
    OSSafeReleaseNULL(transducers);

    super::free();
}

VoodooI2CHIDTransducerWrapper* VoodooI2CHIDTransducerWrapper::wrapper() {
    VoodooI2CHIDTransducerWrapper* wrapper = OSTypeAlloc(VoodooI2CHIDTransducerWrapper);

    if (!wrapper || !wrapper->init())
        OSSafeReleaseNULL(wrapper);

    return wrapper;
}
