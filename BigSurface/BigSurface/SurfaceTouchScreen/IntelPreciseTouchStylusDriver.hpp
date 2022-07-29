//
//  IntelPreciseTouchStylusDriver.hpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/5.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef IntelPreciseTouchStylusDriver_hpp
#define IntelPreciseTouchStylusDriver_hpp

#include "SurfaceManagementEngineClient.hpp"
#include "IPTSProtocol.h"

#define IPTS_BUSY_TIMEOUT       2
#define IPTS_ACTIVE_TIMEOUT     5
#define IPTS_IDLE_TIMEOUT       50

#define IPTS_REPORT_DESC_BUFFER_SIZE    0x460

enum IPTSDeviceState {
    IPTSDeviceStateStarting,
    IPTSDeviceStateStarted,
    IPTSDeviceStateStopping,
    IPTSDeviceStateStopped,
};

struct IPTSBufferInfo {
    IOBufferMemoryDescriptor *buffer;
    IODMACommand *dma_cmd;
    void *vaddr;
    IOPhysicalAddress paddr;
};

class SurfaceTouchScreenDevice;

class EXPORT IntelPreciseTouchStylusDriver : public IOService {
    OSDeclareDefaultStructors(IntelPreciseTouchStylusDriver);
    
public:
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;
    
    void stop(IOService* provider) override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;
    
    IOBufferMemoryDescriptor *getReceiveBufferForIndex(int idx);
    
    UInt16 getVendorID();
    UInt16 getDeviceID();
    UInt8  getMaxContacts();
    
    IOReturn getCurrentInputBuffer(UInt8 *buffer_idx);
    
    void handleHIDReport(const IPTSHIDReport *report);
    
private:
    SurfaceManagementEngineClient*  api {nullptr};
    
    IOWorkLoop*                 work_loop {nullptr};
    IOCommandGate*              command_gate {nullptr};
    IOCommandGate::Action       wait_input {nullptr};
    IOInterruptEventSource*     interrupt_source {nullptr};
    IOTimerEventSource*         timer {nullptr};
    SurfaceTouchScreenDevice*   touch_screen {nullptr};
    
    IPTSDeviceState state {IPTSDeviceStateStopped};
    bool awake {true};
    bool touch_screen_started {false};
    bool busy {false};
    bool restart {false};
    bool wait {false};
    UInt32 current_doorbell {0};
    AbsoluteTime last_activate;
    
    UInt32 data_buffer_size {0};
    UInt32 feedback_buffer_size {0};
    IPTSTouchMode mode {IPTSMultitouch};
    IPTSHIDReport report_to_send;
    bool sent {true};
    
    IPTSBufferInfo rx_buffer[IPTS_BUFFER_NUM];
    IPTSBufferInfo feedback_buffer[IPTS_BUFFER_NUM];
    IPTSBufferInfo doorbell_buffer;
    IPTSBufferInfo workqueue_buffer;
    IPTSBufferInfo tx_buffer;
    IPTSBufferInfo report_desc_buffer;
    
    void releaseResources();
      
    void pollTouchData(IOTimerEventSource* sender);
    
    IOReturn startDevice();
    IOReturn restartDevice();
    IOReturn stopDevice();
    
    IOReturn sendIPTSCommand(UInt32 code, UInt8 *data, UInt16 data_len, bool blocking = true);
    IOReturn sendFeedback(UInt32 buffer, bool blocking = true);
    IOReturn sendSetFeatureReport(UInt8 report_id, UInt8 value);
    
    IOReturn allocateDMAMemory(IPTSBufferInfo *info, UInt32 size);
    IOReturn allocateDMAResources();
    void freeDMAMemory(IPTSBufferInfo *info, bool keep_ref = false);
    void freeDMAResources();
    
    void handleMessage(SurfaceManagementEngineClient *sender, UInt8 *msg, UInt16 msg_len);
    bool isResponseError(IPTSResponse *rsp);
    
    IOReturn getCurrentInputBufferGated(UInt8 *buffer_idx);
    void handleHIDReportGated(IOInterruptEventSource *sender, int count);
};

#endif /* IntelPreciseTouchStylusDriver_hpp */
