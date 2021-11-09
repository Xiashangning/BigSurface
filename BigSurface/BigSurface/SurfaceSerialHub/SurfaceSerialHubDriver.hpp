//
//  SurfaceSerialHubDriver.hpp
//  BigSurface
//
//  Created by Xavier on 2021/10/29.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#ifndef SurfaceSerialHubDriver_hpp
#define SurfaceSerialHubDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "../../../Dependencies/VoodooSerial/VoodooSerial/VoodooUART/VoodooUARTController.hpp"

#define REQID_MIN 34

#define ACK_TIMEOUT 200

class CircleIDCounter {
private:
    UInt16 min;
    UInt16 max;
    UInt16 id;
public:
    CircleIDCounter(UInt16 _min, UInt16 _max){
        min = _min;
        max = _max;
        id = min-1;
    }
    
    UInt16 getID() {
        UInt16 ret = ++id;
        if (id == max)
            id = min-1;
        return ret;
    }
};

struct WaitingRequest {
    bool waiting;
    UInt16 req_id;
    WaitingRequest *next;
};

struct PendingCommand {
    UInt8 *buffer {nullptr};
    UInt16 len {0};
    IOTimerEventSource* timer {nullptr};
    UInt8 trial_count {0};
    PendingCommand *next;
};

class EXPORT SurfaceSerialHubClient : public IOService {
    OSDeclareAbstractStructors(SurfaceSerialHubClient);
    
public:
    virtual void eventReceived(UInt8 tc, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) = 0;
};

class EXPORT SurfaceSerialHubDriver : public VoodooUARTClient {
    OSDeclareDefaultStructors(SurfaceSerialHubDriver);
    
public:
    void dataReceived(UInt8 *buffer, UInt16 length) override;
    
    UInt16 sendCommand(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 size, bool seq);

    IOReturn getResponse(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 size, bool seq, UInt8 *buffer, UInt16 buffer_size);

    IOReturn registerEvent(UInt8 tc, UInt8 iid, SurfaceSerialHubClient *client);
    
    void unregisterEvent(UInt8 tc, UInt8 iid, SurfaceSerialHubClient *client);
    
    IOService* probe(IOService* provider, SInt32* score) override;

    bool start(IOService* provider) override;

    void stop(IOService* provider) override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;
    
private:
    IOWorkLoop*             work_loop {nullptr};
    IOCommandGate*          command_gate {nullptr};
    IOInterruptEventSource* interrupt_source {nullptr};
    IOLock*                 lock {nullptr};
    IOACPIPlatformDevice*   acpi_device {nullptr};
    VoodooUARTController*   uart_controller {nullptr};
    bool                    awake {true};
    
    UInt8*                  rx_buffer {nullptr};
    UInt16                  rx_buffer_len {0};
    UInt8*                  rx_data {nullptr};
    UInt16                  rx_data_len {0};
    UInt8                   msg_cache[256];
    unsigned int            _pos {0};
    int                     msg_len {-1};
    bool                    partial_syn {false};
    PendingCommand*         pending_commands {nullptr};
    WaitingRequest*         waiting_requests {nullptr};
    SurfaceSerialHubClient* event_handler[33] {nullptr};
    CircleIDCounter         seq_counter {CircleIDCounter(0x00, 0xff)};
    CircleIDCounter         req_counter {CircleIDCounter(REQID_MIN, 0xffff)};
    UInt32                  baudrate {0};
    UInt8                   data_bits {0};
    UInt8                   stop_bits {0};
    UInt8                   parity {0};
    UInt16                  fifo_size {0};
    
    IOReturn sendACK(UInt8 seq_id);
    
    IOReturn processMessage();
    
    void processReceivedData(OSObject* target, void* refCon, IOService* nubDevice, int source);
    
    void _process(UInt8* buffer, UInt16 len);
    
    IOReturn sendCommandGated(UInt8 *tx_buffer, UInt16 *len, bool *seq);
    
    void commandTimeout(OSObject* owner, IOTimerEventSource* timer);
    
    IOReturn waitResponse(UInt16 *req_id, UInt8 *buffer, UInt16 *buffer_size);
    
    IOReturn getDeviceResources();
    
    VoodooUARTController* getUARTController();
    
    void releaseResources();
};

#endif /* SurfaceSerialHubDriver_hpp */
