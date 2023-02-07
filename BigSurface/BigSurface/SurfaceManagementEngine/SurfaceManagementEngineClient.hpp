//
//  SurfaceManagementEngineClient.hpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/1.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceManagementEngineClient_hpp
#define SurfaceManagementEngineClient_hpp

#include <IOKit/IOLocks.h>

#include "SurfaceManagementEngineDriver.hpp"

struct MEIClientMessage {
    queue_entry entry;
    UInt8*      msg;
    UInt16      len;
};

class EXPORT SurfaceManagementEngineClient : public IOService {
    OSDeclareDefaultStructors(SurfaceManagementEngineClient);
    friend class SurfaceManagementEngineDriver;
    
public:
    // handle received message
    typedef void (*MessageHandler)(OSObject *owner, SurfaceManagementEngineClient *sender, UInt8 *msg, UInt16 msg_len);
    
    bool attach(IOService* provider) override;
    
    void detach(IOService* provider) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn registerMessageHandler(OSObject *owner, MessageHandler _handler);
    
    void unregisterMessageHandler(OSObject *owner);
    
    IOReturn sendMessage(UInt8 *data, UInt16 data_len, bool blocking);
    
private:
    SurfaceManagementEngineDriver*      api {nullptr};
    IOLock*                             queue_lock {nullptr};
    IOWorkLoop*                         work_loop {nullptr};
    IOInterruptEventSource*             interrupt_source {nullptr};
    MEIClientProperty                   properties;
    
    OSObject*       target {nullptr};
    MessageHandler  handler {nullptr};
    queue_head_t    rx_queue;
    UInt8*          rx_cache {nullptr};
    UInt16          rx_cache_pos {0};
    
    UInt8   addr;
    bool    active {false};
    bool    initial {true};    
    
    void releaseResources();
    
    void resetProperties(MEIClientProperty *client_props, UInt8 me_addr);
    
    void hostRequestDisconnect();
    
    void hostRequestReconnect();
    
    void messageComplete();
    
    void notifyMessage(IOInterruptEventSource *sender, int count);
};

#endif /* SurfaceManagementEngineClient_hpp */
