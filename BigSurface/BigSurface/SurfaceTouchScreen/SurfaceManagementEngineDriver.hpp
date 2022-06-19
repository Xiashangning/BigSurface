//
//  SurfaceManagementEngineDriver.hpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/05/28.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef SurfaceManagementEngineDriver_hpp
#define SurfaceManagementEngineDriver_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/pci/IOPCIDevice.h>

#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>

#include "MEIProtocol.h"

#define kIOPMPowerOff                       0
#define kIOPMNumberPowerStates              2
static IOPMPowerState MyIOPMPowerStates[kIOPMNumberPowerStates] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

enum MEIDeviceState {
    MEIDeviceInitializing = 0,
    MEIDeviceInitClients,
    MEIDeviceEnabled,
    MEIDeviceResetting,
    MEIDeviceDisabled,
    MEIDevicePowerDown,
    MEIDevicePowerUp,
};

/**
 * enum MEIBusState - host bus message protocol state
 *
 * @MEIBusIdle              : protocol not started
 * @MEIBusStarting          : start request message was sent
 * @MEIBusSetupCap          : capabilities request message was sent
 * @MEIBusSetupDMARing      : dma ring setup request message was sent
 * @MEIBusEnumerateClients  : enumeration request was sent
 * @MEIBusRequestingClientProps : acquiring clients properties
 * @MEIBusStarted           : enumeration was completed
 * @MEIBusStopped           : stopping exchange
 */
enum MEIBusState {
    MEIBusIdle = 0,
    MEIBusStarting,
    MEIBusSetupCap,
    MEIBusSetupDMARing,
    MEIBusEnumerateClients,
    MEIBusRequestingClientProps,
    MEIBusStarted,
    MEIBusStopped,
};

/**
 * enum MEIPowerGatingEvent - power gating transition events
 *
 * @MEIPowerGatingEventIdle         : the driver is not in power gating transition
 * @MEIPowerGatingEventWait         : the driver is waiting for a pg event to complete
 * @MEIPowerGatingEventReceived     : the driver received pg event
 * @MEIPowerGatingInterruptWait     : the driver is waiting for a pg event interrupt
 * @MEIPowerGatingInterruptReceived : the driver received pg event interrupt
 */
enum MEIPowerGatingEvent {
    MEIPowerGatingEventIdle,
    MEIPowerGatingEventWait,
    MEIPowerGatingEventReceived,
    MEIPowerGatingInterruptWait,
    MEIPowerGatingInterruptReceived,
};

/**
 * enum MEIPowerGatingState - device internal power gating state
 *
 * @MEIPowerGatingOff   : device is not power gated - it is active
 * @MEIPowerGatingOn    : device is power gated - it is in lower power state
 */
enum MEIPowerGatingState {
    MEIPowerGatingOff = 0,
    MEIPowerGatingOn  = 1,
};

struct MEIDMARingInfo {
    IOBufferMemoryDescriptor*   buffer;
    IODMACommand*               dma_cmd;
    void*                       vaddr;
    IOPhysicalAddress           paddr;
    UInt32                      size;
};

struct MEIPhysicalDevice {
    IOPCIDevice*        pci_dev;
    IOMemoryMap*        mmap;
    MEIDeviceState      state;
    MEIDMARingInfo      ring_desc[MEIDMADescriptorSize];
    MEIPowerGatingEvent pg_event;
    MEIPowerGatingState pg_state;
    queue_head_t        write_q;
    queue_head_t        write_waiting_q;
    queue_head_t        write_ctrl_q;
    queue_head_t        read_ctrl_q;
    int     reset_cnt;
    UInt8   tx_buf_depth;
    UInt8   version_minor;
    UInt8   version_major;
    bool    pg_supported;       // power gating isolation
    bool    dc_supported;       // dynamic clients
    bool    dot_supported;      // disconnect on timeout
    bool    ev_supported;       // event notification
    bool    fa_supported;       // fixed address client
    bool    ie_supported;       // immediate reply to enum request
    bool    os_supported;       // support OS ver message
    bool    dr_supported;       // dma ring
    bool    vt_supported;       // vtag
    bool    cap_supported;      // capabilities message
    bool    cd_supported;       // client dma
};

class SurfaceManagementEngineClient;

struct MEIClientTransaction {
    queue_entry entry;
    SurfaceManagementEngineClient *client;
    UInt8*  data;
    UInt16  data_len;
    bool    completed;
    bool    blocking;
    bool    wait;
};

struct MEIBus {
    MEIBusState     state;
    UInt8           rx_msg_buf[MEI_RX_MSG_BUF_SIZE];
    MEI_SLOT_TYPE   rx_msg_hdr[MEI_RX_MSG_BUF_SIZE];
    int             rx_msg_hdr_len;
    bool            tx_buf_ready;
    UInt8           tx_queue_limit;
    queue_head_t    tx_queue;
};

UUID_DEFINE(SURFACE_IPTS_CLIENT_UUID, 0x70, 0x08, 0x8d, 0x3e, 0x1a, 0x27, 0x08, 0x42, 0x8e, 0xb5, 0x9a, 0xcb, 0x94, 0x02, 0xae, 0x04);

/*
 * Implements a MEI based IPTS driver
 * fixed_addr=12 vt_supported=0
 */
class EXPORT SurfaceManagementEngineDriver : public IOService {
    OSDeclareDefaultStructors(SurfaceManagementEngineDriver);

 public:
    bool init(OSDictionary* properties) override;
    
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;

    void stop(IOService* provider) override;

    void free() override;
    
    IOReturn setPowerState(unsigned long whichState, IOService *whatDevice) override;
    
    IOReturn sendClientMessage(SurfaceManagementEngineClient *client, UInt8 *buffer, UInt16 buffer_len, bool blocking);
    
protected:
    IOReturn mapMemory();

    void unmapMemory();

private:
    IOWorkLoop*                     work_loop {nullptr};
    IOCommandGate*                  command_gate {nullptr};
    IOFilterInterruptEventSource*   interrupt_source {nullptr};
    IOInterruptEventSource*         reset_work {nullptr};
    IOInterruptEventSource*         rescan_work {nullptr};
    IOInterruptEventSource*         resume_work {nullptr};
    IOTimerEventSource*             init_timeout {nullptr};
    IOTimerEventSource*             idle_timeout {nullptr};
    SurfaceManagementEngineClient*  ipts_client {nullptr};
    IOCommandGate::Action           client_msg_action {nullptr};
    
    MEIPhysicalDevice   device;
    MEIBus              bus;
    UInt8               me_client_map[MEI_MAX_CLIENT_NUM/8];
    
    bool awake {true};
    bool wait_hw_ready {false};
    bool wait_bus_start {false};
    bool wait_power_gating {false};

    void releaseResources();
    
    IOReturn startDeviceGated();
    void stopDeviceGated();
    IOReturn restartDeviceGated();
    
    inline UInt32 readRegister(int offset);
    inline void writeRegister(UInt32 value, int offset);
    
    UInt8 calcFilledSlots();
    IOReturn findEmptySlots(UInt8 *empty_slots);
    IOReturn countRxSlots(UInt8 *filled_slots);
    bool acquireWriteBuffer();
    
    bool isHardwareReady();
    bool isHostReady();
    bool isBusMessageVersionSupported();
    bool isWriteQueueEmpty();
    bool isDMARingAllocated();
    
    inline void setHostCSR(UInt32 hcsr);
    
    IOReturn initDevice();
    void configDevice();
    IOReturn resetDevice();
    void deresetDevice();
    void enableDevice();
    IOReturn waitDeviceReady();
    void configDeviceFeatures();
    
    void resetBus();
    
    void clearInterrupts();
    void enableInterrupts();
    void disableInterrupts();
    void setHostInterrupt();
    
    IOReturn enterPowerGating();
    IOReturn enterPowerGatingSync();
    IOReturn exitPowerGatingSync();
    
    IOReturn allocateDMARing();
    void freeDMARing();
    void resetDMARing();
    
    void readMessage(UInt8 *buffer, UInt16 buffer_len);
    
    IOReturn writeMessage(UInt8 *header, UInt16 header_len, UInt8 *data, UInt16 data_len);
    inline void setupMessageHeader(MEIBusMessageHeader *header, UInt16 length);
    IOReturn writeHostMessage(MEIBusMessageHeader *header, MEIBusMessage *msg);
    IOReturn sendClientMessageGated(SurfaceManagementEngineClient *client, UInt8 *buffer, UInt16 *buffer_len, bool *blocking);
    IOReturn sendStartRequest();
    IOReturn sendStopRequest();
    IOReturn sendPowerGatingCommand(bool enter);
    IOReturn sendCapabilityRequest();
    IOReturn sendClientEnumerationRequest();
    IOReturn sendClientPropertyRequest(UInt client_idx);
    IOReturn sendDMASetupRequest();
    IOReturn sendAddClientResponse(UInt8 me_addr, MEIHostBusMessageReturnType status);
    
    void scheduleReset(IOInterruptEventSource *sender, int count);
    void scheduleRescan(IOInterruptEventSource *sender, int count);
    void scheduleResume(IOInterruptEventSource *sender, int count);
    
    bool filterInterrupt(IOFilterInterruptEventSource *sender);
    void handleInterrupt(IOInterruptEventSource *sender, int count);
    
    void handlePowerGatingInterrupt(UInt32 source);
    
    IOReturn handleRead(UInt8 *filled_slots);
    IOReturn handleHostMessage(MEIBusMessageHeader *hdr);
    IOReturn handleClientMessage(SurfaceManagementEngineClient *client, MEIBusMessageHeader *mei_hdr, MEIBusExtendedMetaHeader *meta);
    void discardMessage(MEIBusMessageHeader *hdr, UInt16 discard_len);
        
    IOReturn handleWrite();
    void completeTransaction(MEIClientTransaction *tx);
    bool submitTransaction(MEIClientTransaction *tx);
    
    void enterIdle(IOTimerEventSource *timer);
    void initialiseTimeout(IOTimerEventSource *timer);
    
    IOReturn addClient(MEIClientProperty *client_props, UInt8 addr);
};

#endif /* SurfaceManagementEngineDriver_hpp */
