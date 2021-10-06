//
//  VoodooUARTController.hpp
//  BigSurface
//
//  Created by 夏尚宁 on 2021/9/23.
//  Copyright © 2021 Alexandre Daoud. All rights reserved.
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
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pci/IOPCIDevice.h>

#include "../../Dependencies/helpers.hpp"
#include "VoodooUARTConstants.h"

#ifndef kIOPMPowerOff
#define kIOPMPowerOff                       0
#endif
#define kVoodooUARTIOPMNumberPowerStates    2

#define UART_IDLE_LONG_TIMEOUT  500
#define UART_IDLE_TIMEOUT       200
#define UART_ACTIVE_TIMEOUT     2

enum UART_Parity {
    PARITY_NONE=0,
    PARITY_ODD,
    PARITY_EVEN
};

enum UART_State {
    UART_SLEEP=0,
    UART_IDLE,
    UART_ACTIVE
};

enum VoodooUARTState {
    kVoodooUARTStateOff = 0,
    kVoodooUARTStateOn = 1
};

static IOPMPowerState VoodooUARTIOPMPowerStates[kVoodooUARTIOPMNumberPowerStates] = {
    {1, kIOPMPowerOff, kIOPMPowerOff, kIOPMPowerOff, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, kIOPMPowerOn, kIOPMPowerOn, kIOPMPowerOn, 0, 0, 0, 0, 0, 0, 0, 0}
};

typedef struct _VoodooUARTPhysicalDevice {
    UART_State state;
    const char* name;
    IOPCIDevice* device;
    IOMemoryMap* mmap;
} VoodooUARTPhysicalDevice;

typedef struct _VoodooUARTMessage {
    UInt8* buffer;
    UInt16 length;
} VoodooUARTMessage;

typedef struct _VoodooUARTBus {
    UInt32 interrupt_source;
    bool command_error;
    UInt32 baud_rate;
    int data_bits;
    int stop_bits;
    UART_Parity parity;
    UInt32 transmit_buffer_length;
    VoodooUARTMessage* transmit_buffer;
    UInt transmit_fifo_depth;
} VoodooUARTBus;

class EXPORT VoodooUARTClient : IOService {
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

    UInt32 readRegister(int offset);

    void writeRegister(UInt32 value, int offset);

    IOService* probe(IOService* provider, SInt32* score) override;

    bool start(IOService* provider) override;

    void stop(IOService* provider) override;

    void free() override;

    bool init(OSDictionary* properties) override;
    
    IOReturn requestConnect(VoodooUARTClient *_client, UInt32 baud_rate, int data_bits, int stop_bits, UART_Parity parity);

    void requestDisconnect(VoodooUARTClient *_client);

    IOReturn transmitData(UInt8 *buffer, UInt16 length);

    VoodooUARTPhysicalDevice physical_device;

 protected:

    IOReturn mapMemory();

    IOReturn unmapMemory();
    
    IOReturn configureDevice();

 private:
    VoodooUARTClient* client {nullptr};
    IOWorkLoop* work_loop {nullptr};
    IOCommandGate* command_gate {nullptr};
    IOLock* lock {nullptr};
    bool is_interrupt_enabled {false};
    
    IOTimerEventSource* interrupt_simulator {nullptr};
    AbsoluteTime last_activate_time {0};
    bool is_polling {false};
    
    int fifo_size {0};
    UInt32 fcr {0};
    UInt32 mcr {0};
    UInt32 ier {0};
    
    VoodooUARTBus bus;

    void releaseResources();
    
    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;
    
    IOReturn transmitDataGated(UInt8* buffer, UInt16* length);
    
    /* Set permitted Interrupt type by IER*/
    IOReturn prepareCommunication();
    
    void stopCommunication();
    
    IOReturn waitUntilNotBusy();
    
    void handleInterrupt(OSObject* target, void* refCon, IOService* nubDevice, int source);
    
    void simulateInterrupt(OSObject* owner, IOTimerEventSource* timer);

    IOReturn startInterrupt();

    void stopInterrupt();
    
    void logRegister(void* reg);
    
    void toggleInterruptType(UInt32 type, bool enable);
    
    inline void toggleClockGating(bool enable);
};

#endif /* VoodooUARTController_hpp */
