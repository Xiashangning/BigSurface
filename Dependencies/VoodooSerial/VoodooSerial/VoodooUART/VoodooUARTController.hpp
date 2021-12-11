//
//  VoodooUARTController.hpp
//  BigSurface
//
//  Created by Xavier on 2021/9/23.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//
/* Implements an Intel LPSS Designware UART Controller
 * type 16550A (8250)
 *
 * This is the base class from which all implementations of a physical
 * Intel LPSS Designware UART Controller should inherit from.
 * The members of this class are responsible for low-level interfacing with the physical hardware.
 * The code is based on Linux 8250_dw.c
 */

#ifndef VoodooUARTController_hpp
#define VoodooUARTController_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "../utils/VoodooACPIResourcesParser/VoodooACPIResourcesParser.hpp"
#include "../utils/helpers.hpp"
#include "VoodooUARTConstants.h"

#define UART_IDLE_LONG_TIMEOUT  200
#define UART_IDLE_TIMEOUT       50
#define UART_ACTIVE_TIMEOUT     5

enum UART_State {
    UART_SLEEP=0,
    UART_IDLE,
    UART_ACTIVE
};

enum VoodooUARTState {
    kVoodooUARTStateOff = 0,
    kVoodooUARTStateOn = 1
};

struct VoodooUARTPhysicalDevice {
    UART_State state;
    const char* name;
    IOPCIDevice* device;
    IOMemoryMap* mmap;
};

struct VoodooUARTMessage {
    UInt8* buffer;
    UInt16 length;
};

struct VoodooUARTBus {
    UInt32 interrupt_source;
    bool command_error;
    UInt32 baud_rate;
    UInt8 data_bits;
    UInt8 stop_bits;
    UInt8 parity;
    UInt32 tx_buffer_length;
    VoodooUARTMessage* tx_buffer;
    UInt32 tx_fifo_depth;
    UInt8 *rx_buffer;
};

class EXPORT VoodooUARTClient : public IOService {
    OSDeclareAbstractStructors(VoodooUARTClient);
    
public:
    virtual void dataReceived(UInt8 *buffer, UInt16 length) = 0;
};

/* Implements an Intel LPSS Designware 16550A compatible UART Controller
 *
 * This is the base class from which all implementations of a physical
 * Intel LPSS Designware UART Controller should inherit from. The members of this class
 * are responsible for low-level interfacing with the physical hardware. For the driver implementing
 * the properiety Designware UART controller interface, see <VoodooUARTControllerDriver>.
 */
class EXPORT VoodooUARTController : public IOService {
  OSDeclareDefaultStructors(VoodooUARTController);

 public:
    bool init(OSDictionary* properties) override;
    
    IOService* probe(IOService* provider, SInt32* score) override;
    
    bool start(IOService* provider) override;

    void stop(IOService* provider) override;
    
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;
    
    IOReturn requestConnect(VoodooUARTClient *_client, UInt32 baud_rate, UInt8 data_bits, UInt8 stop_bits, UInt8 parity);

    void requestDisconnect(VoodooUARTClient *_client);

    IOReturn transmitData(UInt8 *buffer, UInt16 length);

    VoodooUARTPhysicalDevice physical_device;

 protected:

    IOReturn mapMemory();

    IOReturn unmapMemory();
    
    IOReturn configureDevice();
    
    void resetDevice();

 private:
    VoodooUARTClient* client {nullptr};
    IOWorkLoop* work_loop {nullptr};
    IOCommandGate* command_gate {nullptr};
    IOLock* lock {nullptr};
    bool is_interrupt_enabled {false};
    IOInterruptEventSource *interrupt_source {nullptr};
    
    IOTimerEventSource* interrupt_simulator {nullptr};
    AbsoluteTime last_activate_time {0};
    bool is_polling {false};
    bool ready {false};
    
    int fifo_size {0};
    UInt32 fcr {0};
    UInt32 mcr {0};
    UInt32 ier {0};
    
    VoodooUARTBus bus;
    bool write_complete {false};
    
    inline UInt32 readRegister(int offset);

    inline void writeRegister(UInt32 value, int offset);
    
    inline void startUARTClock();
    
    inline void stopUARTClock();

    void releaseResources();
    
    IOReturn transmitDataGated(UInt8* buffer, UInt16* length);
    
    /* Set permitted Interrupt type by IER*/
    IOReturn prepareCommunication();
    
    void stopCommunication();
    
    IOReturn waitUntilNotBusy();
    
    void handleInterrupt(OSObject* target, void* refCon, IOService* nubDevice, int source);
    
    void simulateInterrupt(OSObject* owner, IOTimerEventSource* timer);

    IOReturn startInterrupt();

    void stopInterrupt();
    
    void toggleInterruptType(UInt32 type, bool enable);
};

#endif /* VoodooUARTController_hpp */
