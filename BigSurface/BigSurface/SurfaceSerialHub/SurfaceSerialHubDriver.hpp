//
//  SurfaceSerialHubDriver.hpp
//  SurfaceSerialHub
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

#include "../../../Dependencies/VoodooGPIO/VoodooGPIO/VoodooGPIO.hpp"
#include "../../../Dependencies/VoodooSerial/VoodooSerial/VoodooUART/VoodooUARTController.hpp"
#include "SerialProtocol.h"

#define SSH_REQID_MIN           34
#define SSH_MSG_CACHE_SIZE      256     // max length for a single message
#define SSH_MSG_LENGTH_UNKNOWN  (SSH_MSG_CACHE_SIZE+1)
#define SSH_RING_BUFFER_SIZE    10
#define SSH_RING_BUFFER_NEXT(pos)   ((pos) + 1) % SSH_RING_BUFFER_SIZE
#define SSH_ACK_TIMEOUT         50
#define SSH_CMD_TRAIL_CNT       3
#define SSH_WAIT_TIMEOUT        (SSH_ACK_TIMEOUT * SSH_CMD_TRAIL_CNT)

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
    queue_entry entry;
    bool    waiting;
    UInt16  req_id;
    UInt8*  data;
    UInt16  data_len;
};

struct PendingCommand {
    queue_entry entry;
    UInt8*  buffer {nullptr};
    UInt16  len {0};
    UInt8   trial_count {0};
    IOTimerEventSource* timer {nullptr};
};

struct SerialMessage {
    UInt8   cache[SSH_MSG_CACHE_SIZE];
    UInt16  len;
    UInt16  pos;
    bool    partial_syn;
};

struct RingBuffer {
    UInt8* buffer;
    UInt16 filled_len;
};

class EXPORT SurfaceSerialHubClient : public IOService {
    OSDeclareAbstractStructors(SurfaceSerialHubClient);
    
public:
    /*
     * This method is called when SSH receives corresponding registered events
     * it should NOT block the thread
     * the ACTUAL event handling codes should be done in device driver's own workloop thread
     */
    virtual void eventReceived(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *data_buffer, UInt16 length) = 0;
};

class SurfaceBatteryNub;
class SurfaceHIDNub;

class EXPORT SurfaceSerialHubDriver : public IOService {
    OSDeclareDefaultStructors(SurfaceSerialHubDriver);
    
public:
    UInt16 sendCommand(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 payload_len, bool seq);

    IOReturn getResponse(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 payload_len, bool seq, UInt8 *buffer, UInt16 buffer_len);

    IOReturn registerEvent(SurfaceSerialHubClient *client, UInt8 tc, UInt8 iid);
    
    void unregisterEvent(SurfaceSerialHubClient *client, UInt8 tc, UInt8 iid);
    
    bool init(OSDictionary* properties) override;
    
    IOService* probe(IOService* provider, SInt32* score) override;

    bool start(IOService* provider) override;

    void stop(IOService* provider) override;
    
    void free() override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;

    IOReturn enableInterrupt(int source) override;
    
    IOReturn disableInterrupt(int source) override;

    IOReturn getInterruptType(int source, int *interruptType) override;
    
    IOReturn registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refcon) override;
    
    IOReturn unregisterInterrupt(int source) override;
    
private:
    IOWorkLoop*             work_loop {nullptr};
    IOCommandGate*          command_gate {nullptr};
    IOInterruptEventSource* uart_interrupt {nullptr};
    IOInterruptEventSource* gpio_interrupt {nullptr};
    IOACPIPlatformDevice*   acpi_device {nullptr};
    VoodooUARTController*   uart_controller {nullptr};
    VoodooGPIO*             gpio_controller {nullptr};
    SurfaceBatteryNub*      battery_nub {nullptr};
    SurfaceHIDNub*          hid_nub {nullptr};
    SurfaceSerialHubClient* handler[SSH_REQID_MIN];
    
    bool            awake {true};
    RingBuffer      ring_buffer[SSH_RING_BUFFER_SIZE];
    int             current {0};
    int             last {SSH_RING_BUFFER_SIZE-1};
    SerialMessage   rx_msg;
    queue_head_t    pending_list;
    queue_head_t    waiting_list;
    
    CircleIDCounter seq_counter {CircleIDCounter(0x00, 0xff)};
    CircleIDCounter req_counter {CircleIDCounter(SSH_REQID_MIN, 0xffff)};
    
    UInt32  baudrate {0};
    UInt8   data_bits {0};
    UInt8   stop_bits {0};
    UInt8   parity {0};
    bool    flow_control {false};
    UInt16  fifo_size {0};
    UInt16  gpio_pin {0};
    UInt16  gpio_irq {0};
    
    void bufferReceived(VoodooUARTController *sender, UInt8 *buffer, UInt16 length);
    
    IOReturn sendACK(UInt8 seq_id);
    
    IOReturn sendNAK();
    
    IOReturn processMessage();
    
    void processReceivedBuffer(IOInterruptEventSource *sender, int count);
    
    void gpioWakeUp(IOInterruptEventSource *sender, int count);
    
    void _process(UInt8* buffer, UInt16 length);
    
    IOReturn sendCommandGated(UInt8 *tx_buffer, UInt16 *len, bool *seq);
    
    void commandTimeout(IOTimerEventSource* timer);
    
    IOReturn waitResponse(UInt16 *req_id, UInt8 *buffer, UInt16 *buffer_len);
    
    IOReturn getDeviceResources();
    
    VoodooUARTController* getUARTController();
    
    VoodooGPIO* getGPIOController();
    
    IOReturn flushCacheGated();
    
    void releaseResources();
};

#endif /* SurfaceSerialHubDriver_hpp */
