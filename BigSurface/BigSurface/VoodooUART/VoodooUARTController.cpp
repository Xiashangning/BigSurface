//
//  VoodooUARTController.cpp
//  BigSurface
//
//  Created by 夏尚宁 on 2021/9/23.
//  Copyright © 2021 Alexandre Daoud. All rights reserved.
//

#include "VoodooUARTController.hpp"

#define LOG(str, ...) IOLog("%s::%s " str, getName(), physical_device.name, ##__VA_ARGS__)
// Log only if current thread is interruptible, otherwise we will get a panic.
#define TryLog(args...) do { if (ml_get_interrupts_enabled()) LOG(args); } while (0)

#define CONFIGURED(a) a?"YES":"NO"

#define super IOService
OSDefineMetaClassAndStructors(VoodooUARTController, IOService);

UInt8 test_buffer[18] = {0xAA,0x55,0x80,0x08,0x00,0x00,0x59,0xF0,0x80,0x01,0x01,0x00,0x00,0x00,0x00,0x13,0x2c,0x13};
static int test_read_cnt = 3;

bool VoodooUARTController::init(OSDictionary* properties) {
    if (!super::init(properties)) {
        IOLog("%s::super::init failed\n", getName());
        return false;
    }
    
    memset(&bus, 0, sizeof(VoodooUARTBus));
    memset(&physical_device, 0, sizeof(VoodooUARTPhysicalDevice));
    physical_device.state = UART_IDLE;

    return true;
}

IOReturn VoodooUARTController::mapMemory() {
    if (physical_device.device->getDeviceMemoryCount() == 0) {
        return kIOReturnDeviceError;
    } else {
        physical_device.mmap = physical_device.device->mapDeviceMemoryWithIndex(0);
        if (!physical_device.mmap) return kIOReturnDeviceError;
        return kIOReturnSuccess;
    }
}

IOReturn VoodooUARTController::unmapMemory() {
    OSSafeReleaseNULL(physical_device.mmap);
    return kIOReturnSuccess;
}

IOReturn VoodooUARTController::configureDevice() {
    UInt32 reg;
    LOG("Set PCI power state D0\n");
    auto pci_device = physical_device.device;
    pci_device->enablePCIPowerManagement(kPCIPMCSPowerStateD0);

    /* Apply Forcing D0 patch from VoodooI2C*/
    uint16_t oldPowerStateWord = pci_device->configRead16(0x80 + 0x4);
    uint16_t newPowerStateWord = (oldPowerStateWord & (~0x3)) | 0x0;
    pci_device->configWrite16(0x80 + 0x4, newPowerStateWord);

    pci_device->setBusMasterEnable(true);
    pci_device->setMemoryEnable(true);
    
    if (mapMemory() != kIOReturnSuccess) {
        LOG("Could not map memory\n");
        return kIOReturnError;
    }
    
    /* Reset Device*/
    writeRegister(0, LPSS_PRIV + LPSS_PRIV_RESETS);
    writeRegister(LPSS_PRIV_RESETS_FUNC | LPSS_PRIV_RESETS_IDMA, LPSS_PRIV + LPSS_PRIV_RESETS);
    
    reg = readRegister(DW_UART_UCV);
    LOG("Designware UART version %c.%c%c\n", (reg >> 24) & 0xff, (reg >> 16) & 0xff, (reg >> 8) & 0xff);
    
    reg = readRegister(DW_UART_CPR);
    int abp_width = reg&UART_CPR_ABP_DATA_WIDTH ? (reg&UART_CPR_ABP_DATA_WIDTH)*16 : 8;
    fifo_size = DW_UART_CPR_FIFO_SIZE(reg);
    LOG("UART capabilities:\n");
    LOG("    ABP_DATA_WIDTH      : %d\n", abp_width);
    LOG("    AFCE_MODE           : %s\n", CONFIGURED(reg&UART_CPR_AFCE_MODE));
    LOG("    THRE_MODE           : %s\n", CONFIGURED(reg&UART_CPR_THRE_MODE));
    LOG("    SIR_MODE            : %s\n", CONFIGURED(reg&UART_CPR_SIR_MODE));
    LOG("    SIR_LP_MODE         : %s\n", CONFIGURED(reg&UART_CPR_SIR_LP_MODE));
    LOG("    ADDITIONAL_FEATURES : %s\n", CONFIGURED(reg&UART_CPR_ADDITIONAL_FEATURES));
    LOG("    FIFO_ACCESS         : %s\n", CONFIGURED(reg&UART_CPR_FIFO_ACCESS));
    LOG("    FIFO_STAT           : %s\n", CONFIGURED(reg&UART_CPR_FIFO_STAT));
    LOG("    SHADOW              : %s\n", CONFIGURED(reg&UART_CPR_SHADOW));
    LOG("    ENCODED_PARMS       : %s\n", CONFIGURED(reg&UART_CPR_ENCODED_PARMS));
    LOG("    DMA_EXTRA           : %s\n", CONFIGURED(reg&UART_CPR_DMA_EXTRA));
    LOG("    FIFO_SIZE           : %d\n", fifo_size);
    
    //Start UART
    //Clear FIFO disable all interrupts, they will be renabled in prepareCommunication.
    fcr = 0;
    mcr = 0;
    ier = 0;
    writeRegister(UART_FCR_ENABLE_FIFO | UART_FCR_CLEAR_RX_FIFO | UART_FCR_CLEAR_TX_FIFO, DW_UART_FCR);
    writeRegister(0, DW_UART_FCR);
    readRegister(DW_UART_RX);
    writeRegister(0, DW_UART_IER);
    
    //Clear the interrupt registers.
    readRegister(DW_UART_LSR);
    readRegister(DW_UART_RX);
    readRegister(DW_UART_IIR);
    readRegister(DW_UART_MSR);
    readRegister(DW_UART_USR);
    
    //DMA TBD Here
    
    mcr = UART_MCR_OUT2;
    writeRegister(mcr, DW_UART_MCR);
    
    toggleClockGating(true);
    toggleInterruptType(UART_IER_ENABLE_MODEM_STA_INT, true);
    
    if (!bus.transmit_buffer) {
        bus.transmit_buffer = static_cast<VoodooUARTMessage *>(IOMalloc(sizeof(VoodooUARTMessage)));
        bus.transmit_buffer->length = 0;
    }
    LOG("PCI Interrupt %d\n", physical_device.device->configRead8(0x3c));
    
    return kIOReturnSuccess;
}

IOService* VoodooUARTController::probe(IOService* provider, SInt32* score) {
    if (!super::probe(provider, score)) {
        IOLog("%s::%s super::probe failed\n", getName(), getMatchedName(provider));
        return nullptr;
    }
    
    physical_device.device = OSDynamicCast(IOPCIDevice, provider);
    if (!physical_device.device) {
        IOLog("%s::%s WTF? Provider is not IOPCIDevice\n", getName(), getMatchedName(provider));
        return nullptr;
    }
    physical_device.name = physical_device.device->getName();

    return this;
}

bool VoodooUARTController::start(IOService* provider) {
    IOReturn ret;
    if (!super::start(provider)) {
        LOG("super::start failed\n");
        goto exit;
    }
    LOG("Starting UART controller\n");
    
    lock = IOLockAlloc();
    if (!lock) {
        LOG("Could not allocate lock\n");
        goto exit;
    }
    
    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Could not get work loop\n");
        goto exit;
    }

    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        LOG("Could not open command gate\n");
        goto exit;
    }
    command_gate->enable();
    
    PMinit();
    physical_device.device->joinPMtree(this);
    registerPowerDriver(this, VoodooUARTIOPMPowerStates, kVoodooUARTIOPMNumberPowerStates);
    
    physical_device.device->retain();
    if (!physical_device.device->open(this)) {
        LOG("Could not open provider\n");
        goto exit;
    }

    if (configureDevice() != kIOReturnSuccess) {
        LOG("Configure Device Failed!\n");
        goto exit;
    }
    
    // Don't know why cannot register interrupt for IOPCIDevice...
    ret = physical_device.device->registerInterrupt(0, this, OSMemberFunctionCast(IOInterruptAction, this, &VoodooUARTController::handleInterrupt), 0);
    if (ret != kIOReturnSuccess) {
        LOG("Could not register interrupt! Using polling instead\n");
        is_polling = true;
        interrupt_simulator = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &VoodooUARTController::simulateInterrupt));
        if (!interrupt_simulator) {
            LOG("No! Even timer event source cannot be created!\n");
            goto exit;
        }
        work_loop->addEventSource(interrupt_simulator);
    }
    startInterrupt();
    
    requestConnect(nullptr, 0, 0, 0, PARITY_NONE);
    transmitData(test_buffer, 18);

    registerService();
    return true;
exit:
    releaseResources();
    return false;
}

void VoodooUARTController::stop(IOService *provider) {
    toggleClockGating(false);
    releaseResources();
    PMstop();
    super::stop(provider);
}

void VoodooUARTController::toggleInterruptType(UInt32 type, bool enable) {
    if (enable) {
        ier |= type;
    } else {
        ier &= ~type;
    }
    writeRegister(ier, DW_UART_IER);
}

inline void VoodooUARTController::toggleClockGating(bool enable) {
    writeRegister(enable, LPSS_PRIVATE_CLOCK_GATING);
}

UInt32 VoodooUARTController::readRegister(int offset) {
    if (physical_device.mmap != 0) {
         IOVirtualAddress address = physical_device.mmap->getVirtualAddress();
         if (address != 0)
             return *(const volatile UInt32 *)(address + offset);
         else
             TryLog("readRegister at offset 0x%x failed to get a virtual address\n", offset);
     } else {
         TryLog("readRegister at offset 0x%x failed since mamory was not mapped\n", offset);
     }
     return 0;
}

void VoodooUARTController::writeRegister(UInt32 value, int offset) {
    if (physical_device.mmap != 0) {
        IOVirtualAddress address = physical_device.mmap->getVirtualAddress();
        if (address != 0)
            *(volatile UInt32 *)(address + offset) = value;
        else
            TryLog("writeRegister at 0x%x failed to get a virtual address\n", offset);
    } else {
        TryLog("writeRegister at 0x%x failed since mamory was not mapped\n", offset);
    }
}

void VoodooUARTController::releaseResources() {
    if (command_gate) {
        command_gate->disable();
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }

    stopInterrupt();

    if (interrupt_simulator) {
        work_loop->removeEventSource(interrupt_simulator);
        OSSafeReleaseNULL(interrupt_simulator);
    }
    if (!is_polling) {
        physical_device.device->unregisterInterrupt(0);
    }

    OSSafeReleaseNULL(work_loop);
    
    unmapMemory();

    physical_device.device->close(this);
    IOFree(bus.transmit_buffer, sizeof(VoodooUARTMessage));
    OSSafeReleaseNULL(physical_device.device);
    
    if (lock) {
        IOLockFree(lock);
    }
}

IOReturn VoodooUARTController::setPowerState(unsigned long whichState, IOService* whatDevice) {
    if (whatDevice != this)
        return kIOPMAckImplied;

    IOLockLock(lock);
    if (whichState == kIOPMPowerOff) {  // index of kIOPMPowerOff state in VoodooUARTIOPMPowerStates
        if (physical_device.state != UART_SLEEP) {
            physical_device.state = UART_SLEEP;
            toggleClockGating(false);
            stopCommunication();
            stopInterrupt();
            unmapMemory();
            LOG("Going to sleep\n");
        }
    } else {
        if (physical_device.state == UART_SLEEP) {
            if (configureDevice() != kIOReturnSuccess)
                LOG("Wakeup Config Failed\n");
            physical_device.state = UART_IDLE;
            prepareCommunication();
            startInterrupt();
            LOG("Woke up\n");
        }
    }
    IOLockUnlock(lock);
    return kIOPMAckImplied;
}

IOReturn VoodooUARTController::requestConnect(VoodooUARTClient *_client, UInt32 baud_rate, int data_bits, int stop_bits, UART_Parity parity) {
    if (client) {
        return kIOReturnNotReady;
    }
    client = _client;
    bus.baud_rate = baud_rate;
    bus.data_bits = data_bits;
    bus.stop_bits = stop_bits;
    bus.parity = parity;
    
    return prepareCommunication();
}

void VoodooUARTController::requestDisconnect(VoodooUARTClient *_client) {
    if (client == _client) {
        client = nullptr;
    }
    stopCommunication();
}

IOReturn VoodooUARTController::transmitData(UInt8 *buffer, UInt16 length) {
    IOLockLock(lock);
//    if (!client)
//        return kIOReturnError;
    IOReturn ret = command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &VoodooUARTController::transmitDataGated), buffer, &length);
    IOLockUnlock(lock);
    return ret;
}

IOReturn VoodooUARTController::transmitDataGated(UInt8* buffer, UInt16* length) {
    IOReturn ret = kIOReturnOverrun;
    int tries;
    for (ret=0, tries=0; tries < 5; tries++) {
        if (!bus.transmit_buffer->length) {
            bus.transmit_buffer->buffer = buffer;
            bus.transmit_buffer->length = *length;
            toggleInterruptType(UART_IER_ENABLE_TX_EMPTY_INT | UART_IER_ENABLE_THRE_INT_MODE, true);
            interrupt_simulator->setTimeoutMS(1000);
            ret = kIOReturnSuccess;
            break;
        }
        IOSleep(100);
    }
    return ret;
}

IOReturn VoodooUARTController::prepareCommunication() {
    if (waitUntilNotBusy() != kIOReturnSuccess)
        return kIOReturnBusy;
    
    toggleClockGating(false);
    
    writeRegister(UART_LCR_DLAB, DW_UART_LCR);
    writeRegister(0x01, DW_UART_DLL);
    writeRegister(0, DW_UART_DLH);
    
    UInt32 lcr = UART_LCR_WLEN8;
    writeRegister(lcr, DW_UART_LCR);
    
    // enable FIFO and set threshold trigger
    fcr = UART_FCR_ENABLE_FIFO | UART_FCR_T_TRIG_HALF | UART_FCR_R_TRIG_HALF;
    writeRegister(fcr, DW_UART_FCR);
    
    // Request to send
    mcr |= UART_MCR_DTR | UART_MCR_RTS | UART_MCR_AFC;
    writeRegister(mcr, DW_UART_MCR);
    for (int tries=0; tries < 1; tries++) {
        if (readRegister(DW_UART_MSR)&UART_MSR_DSR) {
            LOG("Received Data Set Ready Signal!\n");
            break;
        }
//        IODelay(100);
    }
    for (int tries=0; tries < 1; tries++) {
        if (readRegister(DW_UART_MSR)&UART_MSR_CTS) {
            LOG("Received Clear To Send Signal!\n");
            break;
        }
//        IODelay(100);
    }
    
    toggleClockGating(true);
    // Enable rx interrupt
    toggleInterruptType(UART_IER_ENABLE_RX_AVAIL_INT | UART_IER_ENABLE_ERR_INT, true);
    
    return kIOReturnSuccess;
}

void VoodooUARTController::stopCommunication() {
    toggleInterruptType(UART_IER_ENABLE_RX_AVAIL_INT | UART_IER_ENABLE_ERR_INT, false);
}

IOReturn VoodooUARTController::waitUntilNotBusy() {
    int timeout = TIMEOUT * 150, firstDelay = 100;

    while (readRegister(DW_UART_USR) & UART_USR_BUSY) {
        if (timeout <= 0) {
            LOG("Warning: Timeout waiting for UART not to be busy\n");
            return kIOReturnBusy;
        }
        timeout--;
        if (firstDelay-- >= 0)
            IODelay(100);
        else
            IOSleep(1);
    }

    return kIOReturnSuccess;
}

// Unused due to failure in registering hardware interrupt.
void VoodooUARTController::handleInterrupt(OSObject* target, void* refCon, IOService* provider, int source) {
    /* Direct interrupt context. */
    stopInterrupt();
    if (physical_device.state == UART_SLEEP)
        return;
    
    UInt32 iir = readRegister(DW_UART_IIR);
    command_gate->attemptAction(OSMemberFunctionCast(IOCommandGate::Action, this, &VoodooUARTController::logRegister), (void *)iir);

    //clear interrupts
    readRegister(DW_UART_LSR);
    readRegister(DW_UART_RX);
    readRegister(DW_UART_MSR);
exit:
    startInterrupt();
}

void VoodooUARTController::simulateInterrupt(OSObject* owner, IOTimerEventSource* timer) {
    AbsoluteTime    cur_time;
    clock_get_uptime(&cur_time);
    if (physical_device.state == UART_SLEEP) {
        stopInterrupt();
        return;
    }
    UInt32 status = readRegister(DW_UART_IIR)&UART_IIR_INT_MASK;
    UInt32 lsr = readRegister(DW_UART_LSR);
    if (status != UART_IIR_NO_INT) {
        LOG("IIR: 0x%x\n", status);
    }
    if (bus.transmit_buffer->length) {
        physical_device.state = UART_ACTIVE;
        last_activate_time = cur_time;
        UInt32 data;
        if (!(lsr&UART_LSR_TX_FIFO_FULL)) {
            do {
                data = *bus.transmit_buffer->buffer++;
                writeRegister(data, DW_UART_TX);
                bus.transmit_buffer->length--;
                lsr = readRegister(DW_UART_LSR);
                LOG("tx: %x, LSR status: 0x%x\n", data, lsr);
            } while (!(lsr&UART_LSR_TX_FIFO_FULL) && bus.transmit_buffer->length);
            if (!bus.transmit_buffer->length) {
                toggleInterruptType(UART_IER_ENABLE_TX_EMPTY_INT, false);
                LOG("tx finished\n");
                LOG("TFL: 0x%x\n", readRegister(0x80));
            } else {
                LOG("WTF?\n");
            }
        }
    }
//    if (test_read_cnt > 0) {
//        test_read_cnt--;
//        physical_device.state = UART_ACTIVE;
//        last_activate_time = cur_time;
//        UInt8* buffer = static_cast<UInt8 *>(IOMalloc(sizeof(UInt8)*fifo_size));
//        UInt8* pos = buffer;
//        UInt16 length = 0;
//        while (length<18) {
//            *pos++ = readRegister(DW_UART_RX);
//            length++;
//        }
//        LOG("rx received %d bytes\n", length);
//        LOG("rx data dump: ");
//        int line = 0, len=length;
//        while (len) {
//            for (int i=0;i<16;i++,len--) {
//                if (!len)
//                    break;
//                IOLog("%x ", buffer[line*16+i]);
//            }
//            IOLog("\n");
//            if (len)
//                LOG("              ");
//            line++;
//        }
//        IOFree(buffer, sizeof(UInt8)*fifo_size);
//        interrupt_simulator->setTimeoutMS(200);
//        return;
//    }
//    if (status==UART_IIR_DATA_AVAIL_INT || status==UART_IIR_CHAR_TIMEOUT_INT) {
    if (lsr & (UART_LSR_DATA_READY|UART_LSR_BREAK_INT)) {
        physical_device.state = UART_ACTIVE;
        last_activate_time = cur_time;
        UInt8* buffer = static_cast<UInt8 *>(IOMalloc(sizeof(UInt8)*fifo_size));
        UInt8* pos = buffer;
        UInt16 length = 0;
        do {
            *pos++ = readRegister(DW_UART_RX);
            length++;
        } while (readRegister(DW_UART_LSR) & (UART_LSR_DATA_READY|UART_LSR_BREAK_INT) && length<64);
        LOG("rx received %d bytes\n", length);
        LOG("rx data dump: ");
        int line = 0, len=length;
        while (len) {
            for (int i=0;i<16;i++,len--) {
                if (!len)
                    break;
                IOLog("%x ", buffer[line*16+i]);
            }
            IOLog("\n");
            if (len)
                LOG("              ");
            line++;
        }
        if (client) {
            client->dataReceived(buffer, length);
        }
        IOFree(buffer, sizeof(UInt8)*fifo_size);
    } else if (status==UART_IIR_RX_LINE_STA_INT) {
        LOG("Receiving data error! LSR: 0x%x\n", readRegister(DW_UART_LSR));
    } else if (status==UART_IIR_MODEM_STA_INT) {
        LOG("Modem status changed! MSR: 0x%x\n", readRegister(DW_UART_MSR));
    } else if (status==UART_IIR_BUSY_DETECT) {
        LOG("Detected writing LCR while busy! USR: 0x%x\n", readRegister(DW_UART_USR));
    }
    
    if (physical_device.state == UART_ACTIVE) {
        uint64_t nsecs;
        SUB_ABSOLUTETIME(&cur_time, &last_activate_time);
        absolutetime_to_nanoseconds(cur_time, &nsecs);
        if (nsecs < 1500000000) { // < 1.5s
            interrupt_simulator->setTimeoutMS(UART_ACTIVE_TIMEOUT);
        } else {
            LOG("Enter Idle state\n");
            physical_device.state=UART_IDLE;
        }
    }
    if (physical_device.state == UART_IDLE) {
        interrupt_simulator->setTimeoutMS(UART_IDLE_TIMEOUT);
    }
}

IOReturn VoodooUARTController::startInterrupt() {
    if (is_interrupt_enabled) {
        return kIOReturnStillOpen;
    }
    if (is_polling) {
        interrupt_simulator->setTimeoutMS(UART_IDLE);
        interrupt_simulator->enable();
    } else {
        physical_device.device->enableInterrupt(0);
    }
    is_interrupt_enabled = true;
    return kIOReturnSuccess;
}

void VoodooUARTController::stopInterrupt() {
    if (is_interrupt_enabled) {
        if (is_polling) {
            interrupt_simulator->disable();
        } else {
            physical_device.device->disableInterrupt(0);
        }
        is_interrupt_enabled = false;
    }
}

void VoodooUARTController::logRegister(void *iir) {
    LOG("IIR: 0x%lx, MSR: 0x%x", (long)iir, readRegister(DW_UART_MSR));
}
