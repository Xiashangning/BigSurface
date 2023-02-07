//
//  SurfaceBatteryNub.hpp
//  SurfaceSerialHubDevices
//
//  Created by Xavier on 2022/5/19.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceBatteryNub_hpp
#define SurfaceBatteryNub_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>

#include "../SurfaceSerialHub/SurfaceSerialHubDriver.hpp"

#define BIX_LENGTH          119
#define BST_LENGTH          16

enum SurfaceBatteryEventType {
    SurfaceBatteryInformationChanged = 0,
    SurfaceBatteryStatusChanged,
    SurfaceAdaptorStatusChanged,
};

class EXPORT SurfaceBatteryNub : public SurfaceSerialHubClient {
    OSDeclareDefaultStructors(SurfaceBatteryNub);
    
public:
    typedef void (*EventHandler)(OSObject *owner, SurfaceBatteryNub *sender, SurfaceBatteryEventType type);
    
    bool attach(IOService* provider) override;
    
    void detach(IOService* provider) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn registerBatteryEvent(OSObject* owner, EventHandler _handler);
    
    void unregisterBatteryEvent(OSObject* owner);
    
    void eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) override;
    
    /*
     * index: The index of battery, start with 1
     */
    IOReturn getBatteryConnection(UInt8 index, bool *connected);
    
    /*
     * index: The index of battery, start with 1
     * Returned OSArray bix need to be released by the caller!
     */
    IOReturn getBatteryInformation(UInt8 index, OSArray **bix);
    
    /*
     * index: The index of battery, start with 1
     */
    IOReturn getBatteryStatus(UInt8 index, UInt32 *bst, UInt16 *temp);
    
    IOReturn getAdaptorStatus(UInt32 *psr);
    
    IOReturn setPerformanceMode(UInt32 mode);
    
private:
    SurfaceSerialHubDriver* ssh {nullptr};
    OSObject*               target {nullptr};
    EventHandler            handler {nullptr};
};

#endif /* SurfaceBatteryNub_hpp */
