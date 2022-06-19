//
//  IntelPreciseTouchStylusUserClient.hpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/11.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef IntelPreciseTouchStylusUserClient_hpp
#define IntelPreciseTouchStylusUserClient_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>

#include "IntelPreciseTouchStylusDriver.hpp"

class EXPORT IntelPreciseTouchStylusUserClient : public IOUserClient {
    OSDeclareDefaultStructors(IntelPreciseTouchStylusUserClient);
    
public:
    void stop(IOService* provider) override;
    bool start(IOService* provider) override;
    
    bool initWithTask(task_t owningTask, void* securityToken, UInt32 type, OSDictionary* properties) override;

    IOReturn clientClose(void) override;
    
    IOReturn externalMethod(uint32_t selector, IOExternalMethodArguments* arguments, IOExternalMethodDispatch* dispatch, OSObject* target, void* reference) override;
    
    IOReturn clientMemoryForType(UInt32 type, IOOptionBits* options, IOMemoryDescriptor** memory) override;
    
private:
    static const IOExternalMethodDispatch methods[kNumberOfMethods];
    IntelPreciseTouchStylusDriver*  driver {nullptr};
    task_t task {nullptr};
    
    static IOReturn sMethodGetDeviceInfo(OSObject *target, void *ref, IOExternalMethodArguments *args);
    static IOReturn sMethodReceiveInput(OSObject *target, void *ref, IOExternalMethodArguments *args);
    static IOReturn sMethodSendHIDReport(OSObject *target, void *ref, IOExternalMethodArguments *args);
    
    IOReturn getDeviceInfo(void *ref, IOExternalMethodArguments* args);
    IOReturn receiveInput(void *ref, IOExternalMethodArguments* args);
    IOReturn sendHIDReport(void *ref, IOExternalMethodArguments* args);
};

#endif /* IntelPreciseTouchStylusUserClient_hpp */
