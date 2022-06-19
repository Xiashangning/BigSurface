//
//  SurfaceManagementEngineDriver.cpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/05/28.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "SurfaceManagementEngineDriver.hpp"
#include "SurfaceManagementEngineClient.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", "SurfaceManagementEngineDriver", ##__VA_ARGS__)

#define super IOService
OSDefineMetaClassAndStructors(SurfaceManagementEngineDriver, IOService);

void bitmap_set_bit(UInt8 *map, UInt addr, bool set) {
    UInt i = addr / 8;
    UInt offset = addr % 8;
    if (set)
        map[i] |= BIT(offset);
    else
        map[i] &= ~BIT(offset);
}

bool bitmap_get_bit(UInt8 *map, UInt addr) {
    UInt i = addr / 8;
    UInt offset = addr % 8;
    return (map[i] & BIT(offset)) != 0;
}

UInt bitmap_find_next_bit(UInt8 *map, UInt map_size, UInt start_addr) {
    UInt pos = start_addr;
    while (pos < map_size && !bitmap_get_bit(map, pos))
        pos++;
    return pos;
}

bool SurfaceManagementEngineDriver::init(OSDictionary* properties) {
    if (!super::init(properties))
        return false;

    memset(&device, 0, sizeof(MEIPhysicalDevice));
    memset(&bus, 0, sizeof(MEIBus));
    queue_init(&bus.tx_queue);
    
    return true;
}

IOService* SurfaceManagementEngineDriver::probe(IOService* provider, SInt32* score) {
    if (!super::probe(provider, score))
        return nullptr;
    
    device.pci_dev = OSDynamicCast(IOPCIDevice, provider);
    if (!device.pci_dev)
        return nullptr;

    return this;
}

bool SurfaceManagementEngineDriver::start(IOService* provider) {
    if (!super::start(provider))
        return false;
    
    int interrupt_type, interrupt_idx=0;
    
    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Could not get work loop");
        goto exit;
    }
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate) {
        LOG("Could not open command gate");
        goto exit;
    }
    work_loop->addEventSource(command_gate);
    client_msg_action = OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceManagementEngineDriver::sendClientMessageGated);
    reset_work = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceManagementEngineDriver::scheduleReset));
    rescan_work = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceManagementEngineDriver::scheduleRescan));
    resume_work = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceManagementEngineDriver::scheduleResume));
    if (!reset_work || !rescan_work || !resume_work) {
        LOG("Could not create scheduled work");
        goto exit;
    }
    work_loop->addEventSource(reset_work);
    work_loop->addEventSource(rescan_work);
    work_loop->addEventSource(resume_work);
    
    if (!device.pci_dev->open(this)) {
        LOG("Could not open provider");
        goto exit;
    }
    if (initDevice() != kIOReturnSuccess)
        goto exit;
    
    while (device.pci_dev->getInterruptType(interrupt_idx, &interrupt_type) == kIOReturnSuccess) {
        if (interrupt_type & kIOInterruptTypePCIMessaged)
            break;
        interrupt_idx++;
    }
    interrupt_source = IOFilterInterruptEventSource::filterInterruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceManagementEngineDriver::handleInterrupt), OSMemberFunctionCast(IOFilterInterruptAction, this, &SurfaceManagementEngineDriver::filterInterrupt), device.pci_dev, interrupt_idx);
    if (!interrupt_source) {
        LOG("WTF? Cannot register PCI MSI interrupt!");
        goto exit;
    }
    work_loop->addEventSource(interrupt_source);
    interrupt_source->enable();
    
    if (command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceManagementEngineDriver::startDeviceGated)) != kIOReturnSuccess) {
        LOG("Initialise hardware failed");
        goto exit;
    }
    
    PMinit();
    device.pci_dev->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);

    device.pci_dev->retain();
    registerService();
    return true;
exit:
    releaseResources();
    return false;
}

void SurfaceManagementEngineDriver::stop(IOService *provider) {
    if (!queue_empty(&bus.tx_queue)) {
        LOG("Warning, there are still pending transactions!");
        MEIClientTransaction *tx;
        qe_foreach_element_safe(tx, &bus.tx_queue, entry) {
            completeTransaction(tx);
        }
        IOSleep(100);
    }
    command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceManagementEngineDriver::stopDeviceGated));
    PMstop();
    releaseResources();
    super::stop(provider);
}

void SurfaceManagementEngineDriver::free() {
    super::free();
}

IOReturn SurfaceManagementEngineDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceManagementEngineDriver::stopDeviceGated));
            disableInterrupts();
            interrupt_source->disable();
            device.pci_dev->enablePCIPowerManagement(kPCIPMCSPowerStateD3);
            awake = false;
            LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            device.pci_dev->enablePCIPowerManagement(kPCIPMCSPowerStateD0);
            interrupt_source->enable();
            if (command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceManagementEngineDriver::restartDeviceGated)) != kIOReturnSuccess)
                LOG("Restarting device failed! Retry later");
            init_timeout->enable();
            LOG("Woke up");
        }
    }
    return kIOPMAckImplied;
}

void SurfaceManagementEngineDriver::releaseResources() {
    if (interrupt_source) {
        disableInterrupts();
        interrupt_source->disable();
        work_loop->removeEventSource(interrupt_source);
        OSSafeReleaseNULL(interrupt_source);
    }
    if (init_timeout) {
        init_timeout->cancelTimeout();
        init_timeout->disable();
        work_loop->removeEventSource(init_timeout);
        OSSafeReleaseNULL(init_timeout);
    }
    if (idle_timeout) {
        idle_timeout->cancelTimeout();
        idle_timeout->disable();
        work_loop->removeEventSource(idle_timeout);
        OSSafeReleaseNULL(idle_timeout);
    }
    if (reset_work) {
        reset_work->disable();
        work_loop->removeEventSource(reset_work);
        OSSafeReleaseNULL(reset_work);
    }
    if (rescan_work) {
        rescan_work->disable();
        work_loop->removeEventSource(rescan_work);
        OSSafeReleaseNULL(rescan_work);
    }
    if (resume_work) {
        resume_work->disable();
        work_loop->removeEventSource(resume_work);
        OSSafeReleaseNULL(resume_work);
    }
    unmapMemory();
    if (device.pci_dev->isOpen(this))
        device.pci_dev->close(this);
    if (command_gate) {
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    OSSafeReleaseNULL(work_loop);
    if (ipts_client) {
        ipts_client->stop(this);
        ipts_client->detach(this);
        OSSafeReleaseNULL(ipts_client);
    }
    
    OSSafeReleaseNULL(device.pci_dev);
}

IOReturn SurfaceManagementEngineDriver::startDeviceGated() {
    IOReturn ret;
    
    clearInterrupts();
    configDevice();
    
    device.reset_cnt = 0;
    do {
        device.state = MEIDeviceInitializing;
        ret = resetDevice();

        if (ret == kIOReturnDeviceError || device.state == MEIDeviceDisabled) {
            LOG("Reset failed!");
            goto err;
        }
    } while (ret != kIOReturnSuccess);
    
    // wait mei host bus started
    if (bus.state <= MEIBusStarting) {
        AbsoluteTime abstime, deadline;
        nanoseconds_to_absolutetime(MEI_HOST_BUS_MSG_TIMEOUT * 1000000000ULL, &abstime);
        clock_absolutetime_interval_to_deadline(abstime, &deadline);
        ret = command_gate->commandSleep(&wait_bus_start, deadline, THREAD_INTERRUPTIBLE);
        if (ret == THREAD_TIMED_OUT && bus.state <= MEIBusStarting) {
            bus.state = MEIBusIdle;
            LOG("Timeout waiting for mei bus to start!");
            goto err;
        }
    }

    if (!isHostReady()) {
        LOG("Host is not ready");
        goto err;
    }
    if (!isHardwareReady()) {
        LOG("ME is not ready");
        goto err;
    }

    if (!isBusMessageVersionSupported()) {
        LOG("Device start failed");
        goto err;
    }

    LOG("Link layer initialization succeeded");
    return kIOReturnSuccess;
err:
    LOG("Link layer initialization failed");
    device.state = MEIDeviceDisabled;
    return kIOReturnDeviceError;
}

void SurfaceManagementEngineDriver::stopDeviceGated() {
    device.state = MEIDevicePowerDown;
    
    init_timeout->disable();

    resetDevice();
    device.state = MEIDeviceDisabled;
}

IOReturn SurfaceManagementEngineDriver::restartDeviceGated() {
    device.state = MEIDevicePowerUp;
    device.reset_cnt = 0;
    IOReturn ret = resetDevice();
    
    if (ret == kIOReturnDeviceError ||
        device.state == MEIDeviceDisabled) {
        LOG("Error! Device disabled");
        return kIOReturnDeviceError;
    }
    if (ret != kIOReturnSuccess)
        reset_work->interruptOccurred(nullptr, this, 0);
    
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::mapMemory() {
    if (device.pci_dev->getDeviceMemoryCount() == 0) {
        return kIOReturnDeviceError;
    } else {
        device.mmap = device.pci_dev->mapDeviceMemoryWithIndex(0);
        if (!device.mmap) return kIOReturnDeviceError;
        return kIOReturnSuccess;
    }
}

void SurfaceManagementEngineDriver::unmapMemory() {
    OSSafeReleaseNULL(device.mmap);
}

inline UInt32 SurfaceManagementEngineDriver::readRegister(int offset) {
    if (device.mmap) {
         IOVirtualAddress address = device.mmap->getVirtualAddress();
         if (address != 0)
             return *(const volatile UInt32 *)(address + offset);
     }
     return kIOReturnSuccess;
}

inline void SurfaceManagementEngineDriver::writeRegister(UInt32 value, int offset) {
    if (device.mmap) {
        IOVirtualAddress address = device.mmap->getVirtualAddress();
        if (address != 0)
            *(volatile UInt32 *)(address + offset) = value;
    }
}

UInt8 SurfaceManagementEngineDriver::calcFilledSlots() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    SInt8 read_ptr = (hcsr & MEI_CSR_BUF_RPOINTER) >> 8;
    SInt8 write_ptr = (hcsr & MEI_CSR_BUF_WPOINTER) >> 16;
    
    return write_ptr - read_ptr;
}

IOReturn SurfaceManagementEngineDriver::findEmptySlots(UInt8 *empty_slots) {
    UInt8 filled_slots = calcFilledSlots();
    if (filled_slots > device.tx_buf_depth)
        return kIOReturnOverrun;
    
    *empty_slots = device.tx_buf_depth - filled_slots;
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::countRxSlots(UInt8 *filled_slots) {
    UInt32 mecsr = readRegister(MEI_ME_CSR);
    UInt8 buffer_depth = (mecsr & MEI_CSR_BUF_DEPTH) >> 24;
    SInt8 read_ptr = (mecsr & MEI_CSR_BUF_RPOINTER) >> 8;
    SInt8 write_ptr = (mecsr & MEI_CSR_BUF_WPOINTER) >> 16;
    UInt8 slots = write_ptr - read_ptr;
    
    if (slots > buffer_depth)
        return kIOReturnOverrun;
    
    *filled_slots = slots;
    return kIOReturnSuccess;
}

bool SurfaceManagementEngineDriver::acquireWriteBuffer() {
    if (device.pg_state == MEIPowerGatingOn || (device.pg_event >= MEIPowerGatingEventWait && device.pg_event <= MEIPowerGatingInterruptWait))
        return false;
    if (!bus.tx_buf_ready)
        return false;
    bus.tx_buf_ready = false;
    return true;
}

bool SurfaceManagementEngineDriver::isHardwareReady() {
    UInt32 mecsr = readRegister(MEI_ME_CSR);
    return (mecsr & MEI_ME_CSR_READY) == MEI_ME_CSR_READY;
}

bool SurfaceManagementEngineDriver::isHostReady() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    return (hcsr & MEI_H_CSR_READY) == MEI_H_CSR_READY;
}

bool SurfaceManagementEngineDriver::isBusMessageVersionSupported() {
    return (device.version_major < MEI_HBM_MAJOR_VERSION) ||
            (device.version_major == MEI_HBM_MAJOR_VERSION &&
             device.version_minor <= MEI_HBM_MINOR_VERSION);
}

bool SurfaceManagementEngineDriver::isWriteQueueEmpty() {
    return device.state == MEIDeviceEnabled && queue_empty(&bus.tx_queue);
}

bool SurfaceManagementEngineDriver::isDMARingAllocated() {
    return device.ring_desc[MEIDMADescriptorHost].vaddr != nullptr;
}

inline void SurfaceManagementEngineDriver::setHostCSR(UInt32 hcsr) {
    hcsr &= ~MEI_H_CSR_INT_STA_MASK;
    writeRegister(hcsr, MEI_H_CSR);
}

void SurfaceManagementEngineDriver::clearInterrupts() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    if (hcsr & MEI_H_CSR_INT_STA_MASK)
        writeRegister(hcsr, MEI_H_CSR);
}

void SurfaceManagementEngineDriver::enableInterrupts() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    hcsr |= MEI_H_CSR_INT_ENABLE_MASK;
    setHostCSR(hcsr);
}

void SurfaceManagementEngineDriver::disableInterrupts() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    hcsr &= ~MEI_H_CSR_INT_ENABLE_MASK;
    setHostCSR(hcsr);
}

void SurfaceManagementEngineDriver::setHostInterrupt() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    hcsr |= MEI_H_CSR_INT_GEN;
    setHostCSR(hcsr);
}

void SurfaceManagementEngineDriver::configDevice() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    device.tx_buf_depth = (hcsr & MEI_CSR_BUF_DEPTH) >> 24;

    device.pg_state = MEIPowerGatingOff;
    UInt32 d0i3c = readRegister(MEI_H_D0I3C);
    if (d0i3c & MEI_H_D0I3C_I3)
        device.pg_state = MEIPowerGatingOn;
}

IOReturn SurfaceManagementEngineDriver::initDevice() {
    device.pci_dev->setMemoryEnable(true);
    device.pci_dev->setBusMasterEnable(true);
    
    if (mapMemory() != kIOReturnSuccess) {
        LOG("Could not map pci_device memory");
        return kIOReturnError;
    }
    // DMA ring
    device.ring_desc[MEIDMADescriptorHost].size = MEI_DMA_RING_HOST_SIZE;
    device.ring_desc[MEIDMADescriptorDevice].size = MEI_DMA_RING_DEVICE_SIZE;
    device.ring_desc[MEIDMADescriptorControl].size = MEI_DMA_RING_CTRL_SIZE;
    
    device.state = MEIDeviceInitializing;
    memset(me_client_map, 0, sizeof(me_client_map));
    device.pg_event = MEIPowerGatingEventIdle;
    
    init_timeout = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceManagementEngineDriver::initialiseTimeout));
    idle_timeout = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceManagementEngineDriver::enterIdle));
    if (!init_timeout || !idle_timeout) {
        LOG("Failed to create timer for device");
        return kIOReturnError;
    }
    work_loop->addEventSource(init_timeout);
    work_loop->addEventSource(idle_timeout);
    
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::resetDevice() {
    IOReturn ret = kIOReturnSuccess;
    UInt32 hcsr;
    MEIDeviceState state = device.state;
    if (state != MEIDeviceInitializing &&
        state != MEIDeviceDisabled &&
        state != MEIDevicePowerDown &&
        state != MEIDevicePowerUp) {
        LOG("Warning! Unexpected reset call from state: %x", device.state);
    }
    clearInterrupts();
    // put bus into idle before actual reset
    init_timeout->cancelTimeout();
    bus.state = MEIBusIdle;
    
    bool interrupts_enabled = state != MEIDevicePowerDown;
    device.state = MEIDeviceResetting;
    if (++device.reset_cnt > MEI_MAX_CONSEC_RESET) {
        LOG("Reached maximal consecutive resets");
        device.state = MEIDeviceDisabled;
        return kIOReturnDeviceError;
    }
    
    if (interrupts_enabled) {
        enableInterrupts();
        ret = exitPowerGatingSync();
        if (ret != kIOReturnSuccess)
            goto sw_reset;
    }
    
    hcsr = readRegister(MEI_H_CSR);
    /*
     * MEI_H_CSR_RESET may be found lit before reset is started,
     * for example if preceding reset flow hasn't completed.
     * In that case asserting MEI_H_CSR_RESET will be ignored, therefore
     * we need to clean MEI_H_CSR_RESET bit to start a successful reset sequence.
     */
    if ((hcsr & MEI_H_CSR_RESET) == MEI_H_CSR_RESET) {
        LOG("Warning, MEI_H_CSR_RESET is set = 0x%08X, previous reset probably failed", hcsr);
        hcsr &= ~MEI_H_CSR_RESET;
        setHostCSR(hcsr);
        hcsr = readRegister(MEI_H_CSR);
    }
    hcsr |= MEI_H_CSR_RESET | MEI_H_CSR_INT_GEN | MEI_H_CSR_INT_STA_MASK;
    if (!interrupts_enabled)
        hcsr &= ~MEI_H_CSR_INT_ENABLE_MASK;

    writeRegister(hcsr, MEI_H_CSR);

    // Host reads the MEI_H_CSR once to ensure that the posted write to MEI_H_CSR completes.
    hcsr = readRegister(MEI_H_CSR);
    if ((hcsr & MEI_H_CSR_RESET) == 0)
        LOG("Warning, MEI_H_CSR_RESET is not set = 0x%08X", hcsr);
    if ((hcsr & MEI_H_CSR_READY) == MEI_H_CSR_READY)
        LOG("Warning, MEI_H_CSR_READY is not cleared 0x%08X", hcsr);

    if (!interrupts_enabled) {
        deresetDevice();
        enterPowerGating();
    }
sw_reset:
    /* fall through and remove the sw state even if hw reset has failed
     * no need to clean up software state in case of power up */
    if (state != MEIDeviceInitializing && state != MEIDevicePowerUp) {
        if (!queue_empty(&bus.tx_queue)) {
            LOG("Flushing all pending transactions!");
            MEIClientTransaction *tx;
            qe_foreach_element_safe(tx, &bus.tx_queue, entry) {
                completeTransaction(tx);
            }
            
            AbsoluteTime abstime, deadline;
            nanoseconds_to_absolutetime(50000000ULL, &abstime);    // 50ms
            clock_absolutetime_interval_to_deadline(abstime, &deadline);
            // Give up work_loop's thread to let pending transactions to finish their work on gate.
            command_gate->commandSleep(&awake, deadline, THREAD_INTERRUPTIBLE);
        }
        if (ipts_client)
            ipts_client->hostRequestDisconnect();
    }
    resetBus();

    memset(bus.rx_msg_hdr, 0, sizeof(bus.rx_msg_hdr));
    if (ret != kIOReturnSuccess) {
        LOG("Hardware reset failed");
        return ret;
    }

    if (state == MEIDevicePowerDown) {
        device.state = MEIDeviceDisabled;
        return kIOReturnSuccess;
    }

    ret = waitDeviceReady();
    if (ret != kIOReturnSuccess)
        return ret;
    LOG("Link is established");
    
    device.state = MEIDeviceInitClients;
    // Send start request
    bus.state = MEIBusIdle;
    ret = sendStartRequest();
    if (ret) {
        LOG("Start request failed");
        device.state = MEIDeviceResetting;
        return ret;
    }

    bus.state = MEIBusStarting;
    init_timeout->setTimeoutMS(MEI_CLIENTS_INIT_TIMEOUT * 1000);

    return kIOReturnSuccess;
}

void SurfaceManagementEngineDriver::deresetDevice() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    hcsr |= MEI_H_CSR_INT_GEN;
    hcsr &= ~MEI_H_CSR_RESET;
    setHostCSR(hcsr);
}

void SurfaceManagementEngineDriver::enableDevice() {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    hcsr |= MEI_H_CSR_INT_ENABLE_MASK | MEI_H_CSR_INT_GEN | MEI_H_CSR_READY;
    setHostCSR(hcsr);
}

IOReturn SurfaceManagementEngineDriver::waitDeviceReady() {
    AbsoluteTime abstime, deadline;
    IOReturn sleep;
    nanoseconds_to_absolutetime(MEI_HW_READY_TIMEOUT * 1000000000ULL, &abstime);
    clock_absolutetime_interval_to_deadline(abstime, &deadline);
    sleep = command_gate->commandSleep(&wait_hw_ready, deadline, THREAD_INTERRUPTIBLE);
    if (sleep == THREAD_TIMED_OUT) {
        LOG("Timeout waiting for hardware to be ready!");
        return kIOReturnTimeout;
    }

    deresetDevice();
    LOG("Hardware is ready");
    enableDevice();
    return kIOReturnSuccess;
}

void SurfaceManagementEngineDriver::configDeviceFeatures() {
    device.pg_supported = false;
    if (device.version_major > MEI_HBM_MAJOR_VERSION_PGI ||
        (device.version_major == MEI_HBM_MAJOR_VERSION_PGI &&
        device.version_minor >= MEI_HBM_MINOR_VERSION_PGI))
        device.pg_supported = true;

    device.dc_supported = false;
    if (device.version_major >= MEI_HBM_MAJOR_VERSION_DC)
        device.dc_supported = true;

    device.ie_supported = false;
    if (device.version_major >= MEI_HBM_MAJOR_VERSION_IE)
        device.ie_supported = true;

    device.dot_supported = false;
    if (device.version_major >= MEI_HBM_MAJOR_VERSION_DOT)
        device.dot_supported = true;

    device.ev_supported = false;
    if (device.version_major >= MEI_HBM_MAJOR_VERSION_EV)
        device.ev_supported = true;

    device.fa_supported = false;
    if (device.version_major >= MEI_HBM_MAJOR_VERSION_FA)
        device.fa_supported = true;

    device.os_supported = false;
    if (device.version_major >= MEI_HBM_MAJOR_VERSION_OS)
        device.os_supported = true;

    device.dr_supported = false;
    if (device.version_major > MEI_HBM_MAJOR_VERSION_DR ||
        (device.version_major == MEI_HBM_MAJOR_VERSION_DR &&
         device.version_minor >= MEI_HBM_MINOR_VERSION_DR))
        device.dr_supported = true;

    device.vt_supported = false;
    if (device.version_major > MEI_HBM_MAJOR_VERSION_VT ||
        (device.version_major == MEI_HBM_MAJOR_VERSION_VT &&
         device.version_minor >= MEI_HBM_MINOR_VERSION_VT))
        device.vt_supported = true;

    device.cap_supported = false;
    if (device.version_major > MEI_HBM_MAJOR_VERSION_CAP ||
        (device.version_major == MEI_HBM_MAJOR_VERSION_CAP &&
         device.version_minor >= MEI_HBM_MINOR_VERSION_CAP))
        device.cap_supported = true;

    device.cd_supported = false;
    if (device.version_major > MEI_HBM_MAJOR_VERSION_CD ||
        (device.version_major == MEI_HBM_MAJOR_VERSION_CD &&
         device.version_minor >= MEI_HBM_MINOR_VERSION_CD))
        device.cd_supported = true;
    
    device.dr_supported = false;
    device.cd_supported = false;
    
//    LOG("power gating isolation           %d", device.pg_supported);
//    LOG("dynamic clients                  %d", device.dc_supported);
//    LOG("disconnect on timeout            %d", device.dot_supported);
//    LOG("event notification               %d", device.ev_supported);
//    LOG("fixed address client             %d", device.fa_supported);
//    LOG("immediate reply to enum request  %d", device.ie_supported);
//    LOG("support OS ver message           %d", device.os_supported);
//    LOG("dma ring                         %d", device.dr_supported);
//    LOG("vtag                             %d", device.vt_supported);
//    LOG("capabilities message             %d", device.cap_supported);
//    LOG("client dma                       %d", device.cd_supported);
}

void SurfaceManagementEngineDriver::resetBus() {
    if (ipts_client)
        ipts_client->active = false;
    
    init_timeout->cancelTimeout();
    bus.state = MEIBusIdle;
}

IOReturn SurfaceManagementEngineDriver::enterPowerGating() {
    UInt32 reg = readRegister(MEI_H_D0I3C);
    if (!(reg & MEI_H_D0I3C_I3)) {
        reg |= MEI_H_D0I3C_I3;
        reg &= ~MEI_H_D0I3C_IR;
        writeRegister(reg, MEI_H_D0I3C);
    }
    device.pg_state = MEIPowerGatingOn;
    device.pg_event = MEIPowerGatingEventIdle;
    LOG("Enter d0i3 mode");
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::enterPowerGatingSync() {
    IOReturn ret;
    AbsoluteTime abstime, deadline;

    UInt32 reg = readRegister(MEI_H_D0I3C);
    if (reg & MEI_H_D0I3C_I3) {
        LOG("No need to enter d0i3");
        goto on;
    }

    // PGI entry procedure
    device.pg_event = MEIPowerGatingEventWait;
    ret = sendPowerGatingCommand(true);
    if (ret != kIOReturnSuccess) {
        LOG("Failed to send power gating command");
        /* FIXME: should we reset here? */
        goto out;
    }

    nanoseconds_to_absolutetime(MEI_POWER_GATING_ISO_TIMEOUT * 1000000000ULL, &abstime);
    clock_absolutetime_interval_to_deadline(abstime, &deadline);
    command_gate->commandSleep(&wait_power_gating, deadline, THREAD_INTERRUPTIBLE);
    if (device.pg_event != MEIPowerGatingEventReceived) {
        ret = kIOReturnTimeout;
        goto out;
    }
    // end PGI entry procedure
    
    device.pg_event = MEIPowerGatingInterruptWait;
    reg = readRegister(MEI_H_D0I3C);
    reg |= MEI_H_D0I3C_I3;
    reg |= MEI_H_D0I3C_IR;
    writeRegister(reg, MEI_H_D0I3C);
    /* read it to ensure HW consistency */
    reg = readRegister(MEI_H_D0I3C);
    if (!(reg & MEI_H_D0I3C_CIP))
        goto on;

    nanoseconds_to_absolutetime((UInt64)MEI_D0I3_TIMEOUT * 1000000000ULL, &abstime);
    clock_absolutetime_interval_to_deadline(abstime, &deadline);
    command_gate->commandSleep(&wait_power_gating, deadline, THREAD_INTERRUPTIBLE);
    if (device.pg_event != MEIPowerGatingInterruptReceived) {
        reg = readRegister(MEI_H_D0I3C);
        if (!(reg & MEI_H_D0I3C_I3)) {
            ret = kIOReturnTimeout;
            goto out;
        }
    }
on:
    ret = kIOReturnSuccess;
    device.pg_state = MEIPowerGatingOn;
    LOG("Enter d0i3 mode");
out:
    device.pg_event = MEIPowerGatingEventIdle;
    return ret;
}

IOReturn SurfaceManagementEngineDriver::exitPowerGatingSync() {
    IOReturn ret;
    AbsoluteTime abstime, deadline;

    device.pg_event = MEIPowerGatingInterruptWait;
    UInt32 reg = readRegister(MEI_H_D0I3C);
    if (!(reg & MEI_H_D0I3C_I3)) {
        LOG("No need to exit d0i3");
        goto off;
    }

    reg &= ~MEI_H_D0I3C_I3;
    reg |= MEI_H_D0I3C_IR;
    writeRegister(reg, MEI_H_D0I3C);
    reg = readRegister(MEI_H_D0I3C);
    if (!(reg & MEI_H_D0I3C_CIP))
        goto off;

    nanoseconds_to_absolutetime(MEI_D0I3_TIMEOUT * 1000000000ULL, &abstime);
    clock_absolutetime_interval_to_deadline(abstime, &deadline);
    command_gate->commandSleep(&wait_power_gating, deadline, THREAD_INTERRUPTIBLE);
    if (device.pg_event != MEIPowerGatingInterruptReceived) {
        reg = readRegister(MEI_H_D0I3C);
        if (reg & MEI_H_D0I3C_I3) {
            ret = kIOReturnTimeout;
            goto out;
        }
    }
off:
    ret = kIOReturnSuccess;
    device.pg_state = MEIPowerGatingOff;
    LOG("Exit d0i3 mode");
out:
    device.pg_event = MEIPowerGatingEventIdle;
    return ret;
}

IOReturn SurfaceManagementEngineDriver::allocateDMARing() {
    for (int i=0; i < MEIDMADescriptorSize; i++) {
        if (!device.ring_desc[i].size || device.ring_desc[i].vaddr)
            continue;
        
        IOBufferMemoryDescriptor *buffer = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMapInhibitCache, device.ring_desc[i].size, DMA_BIT_MASK(64));
        if (!buffer) {
            LOG("Failed to create DMA buffer for %d", i);
            goto err;
        }
        buffer->prepare();
        
        IODMACommand *cmd = IODMACommand::withSpecification(kIODMACommandOutputHost64, 64, 0, IODMACommand::kMapped, 0, PAGE_SIZE);
        if (!cmd) {
            LOG("Failed to create IODMACommand for %d", i);
            buffer->complete();
            OSSafeReleaseNULL(buffer);
            goto err;
        }
        cmd->setMemoryDescriptor(buffer);

        IODMACommand::Segment64 seg;
        UInt64 offset = 0;
        UInt32 seg_num = 1;
        if (cmd->gen64IOVMSegments(&offset, &seg, &seg_num) != kIOReturnSuccess) {
            LOG("Failed to generate IO virtual address for %d", i);
            OSSafeReleaseNULL(cmd);
            buffer->complete();
            OSSafeReleaseNULL(buffer);
            goto err;
        }
        device.ring_desc[i].paddr = seg.fIOVMAddr;
        device.ring_desc[i].vaddr = buffer->getBytesNoCopy();
        device.ring_desc[i].buffer = buffer;
        device.ring_desc[i].dma_cmd = cmd;
        memset(device.ring_desc[i].vaddr, 0, device.ring_desc[i].size);
    }
    return kIOReturnSuccess;
err:
    freeDMARing();
    return kIOReturnNoMemory;
}

void SurfaceManagementEngineDriver::freeDMARing() {
    for (int i=0; i < MEIDMADescriptorSize; i++) {
        if (!device.ring_desc[i].vaddr)
            continue;
        
        device.ring_desc[i].dma_cmd->clearMemoryDescriptor();
        OSSafeReleaseNULL(device.ring_desc[i].dma_cmd);
        device.ring_desc[i].buffer->complete();
        OSSafeReleaseNULL(device.ring_desc[i].buffer);
        device.ring_desc[i].vaddr = nullptr;
    }
}

void SurfaceManagementEngineDriver::resetDMARing() {
    MEIBusDMARingControl *ctrl = reinterpret_cast<MEIBusDMARingControl *>(device.ring_desc[MEIDMADescriptorControl].vaddr);
    
    if (ctrl)
        memset(ctrl, 0, sizeof(MEIBusDMARingControl));
}

void SurfaceManagementEngineDriver::readMessage(UInt8 *buffer, UInt16 buffer_len) {
    UInt32 *buf = reinterpret_cast<UInt32 *>(buffer);
    for (; buffer_len >= MEI_SLOT_SIZE; buffer_len -= MEI_SLOT_SIZE)
        *buf++ = readRegister(MEI_ME_CB_RW);
    if (buffer_len > 0) {
        UInt32 reg = readRegister(MEI_ME_CB_RW);
        memcpy(buf, &reg, buffer_len);
    }
    setHostInterrupt();
}

IOReturn SurfaceManagementEngineDriver::writeMessage(UInt8 *header, UInt16 header_len, UInt8 *data, UInt16 data_len) {
    if (!header || !data || header_len % MEI_SLOT_SIZE) {
        LOG("Message invalid!");
        return kIOReturnInvalid;
    }

    UInt8 empty_slots;
    if (findEmptySlots(&empty_slots) != kIOReturnSuccess) {
        LOG("WTF? Filled slots is bigger than host buffer");
        return kIOReturnOverrun;
    }

    UInt8 slots_needed = MEI_DATA_TO_SLOTS(header_len + data_len);
    if (slots_needed > empty_slots) {
        LOG("Message too large! need %d, empty %d", slots_needed, empty_slots);
        return kIOReturnMessageTooLarge;
    }

    const MEI_SLOT_TYPE *buf = reinterpret_cast<MEI_SLOT_TYPE *>(header);
    for (int i = 0; i < header_len / MEI_SLOT_SIZE; i++)
        writeRegister(buf[i], MEI_H_CB_WW);
    buf = reinterpret_cast<MEI_SLOT_TYPE *>(data);
    for (int i = 0; i < data_len / MEI_SLOT_SIZE; i++)
        writeRegister(buf[i], MEI_H_CB_WW);
    UInt8 tail = data_len % MEI_SLOT_SIZE;
    if (tail > 0) {
        UInt32 reg = 0;
        memcpy(&reg, data + data_len - tail, tail);
        writeRegister(reg, MEI_H_CB_WW);
    }

    setHostInterrupt();
    
    if (!isHardwareReady()) {
        LOG("WTF? Hardware is not ready yet");
        return kIOReturnIOError;
    }

    return kIOReturnSuccess;
}

inline void SurfaceManagementEngineDriver::setupMessageHeader(MEIBusMessageHeader *header, UInt16 length) {
    memset(header, 0, sizeof(MEIBusMessageHeader));
    header->length = length;
    header->msg_complete = 1;
}

IOReturn SurfaceManagementEngineDriver::writeHostMessage(MEIBusMessageHeader *header, MEIBusMessage *msg) {
    return writeMessage(reinterpret_cast<UInt8 *>(header), sizeof(MEIBusMessageHeader), reinterpret_cast<UInt8 *>(msg), header->length);
}

IOReturn SurfaceManagementEngineDriver::sendClientMessage(SurfaceManagementEngineClient *client, UInt8 *buffer, UInt16 buffer_len, bool blocking) {
    return command_gate->runAction(client_msg_action, client, buffer, &buffer_len, &blocking);
}

//TODO: DMA ring
IOReturn SurfaceManagementEngineDriver::sendClientMessageGated(SurfaceManagementEngineClient *client, UInt8 *buffer, UInt16 *buffer_len, bool *blocking) {
    if (device.state != MEIDeviceEnabled || !client->active)
        return kIOReturnNoDevice;
    if (*buffer_len > client->properties.max_msg_length)
        return kIOReturnMessageTooLarge;
    if (device.pg_state == MEIPowerGatingOn && exitPowerGatingSync() != kIOReturnSuccess) {
        LOG("Failed to get device active");
        return kIOReturnAborted;
    }
    
    MEIClientTransaction *tx = new MEIClientTransaction;
    tx->client = client;
    tx->data = buffer;
    tx->data_len = *buffer_len;
    tx->completed = false;
    tx->blocking = *blocking;
    
    IOReturn ret = kIOReturnSuccess;
    if (tx->blocking) {
        if (!acquireWriteBuffer() || !submitTransaction(tx)) {   // not finished
            enqueue(&bus.tx_queue, &tx->entry);
            AbsoluteTime abstime, deadline;
            nanoseconds_to_absolutetime(MEI_CLIENT_SEND_MSG_TIMEOUT * 1000000ULL, &abstime);
            clock_absolutetime_interval_to_deadline(abstime, &deadline);
            command_gate->commandSleep(&tx->wait, deadline, THREAD_INTERRUPTIBLE);
            if (!tx->completed) {
                LOG("Failed to send message!");
                ret = kIOReturnTimeout;
            }
            remqueue(&tx->entry);
        }
        delete tx;
    } else {
        if (!acquireWriteBuffer() || !submitTransaction(tx)) {   // not finished
            tx->data = new UInt8[tx->data_len];
            memcpy(tx->data, buffer, tx->data_len);
            enqueue(&bus.tx_queue, &tx->entry);
        }
    }
    
    idle_timeout->setTimeoutMS(MEI_DEVICE_IDLE_TIMEOUT * 1000);
    return ret;
}

IOReturn SurfaceManagementEngineDriver::sendStartRequest() {
    MEIBusMessageHeader header;
    MEIBusHostVersionRequest req;
    setupMessageHeader(&header, sizeof(req));
    memset(&req, 0, sizeof(req));
    req.cmd = MEI_HOST_START_REQ_CMD;
    req.host_version_major = MEI_HBM_MAJOR_VERSION;
    req.host_version_minor = MEI_HBM_MINOR_VERSION;
    
    return writeHostMessage(&header, MEI_TO_MSG(&req));
}

IOReturn SurfaceManagementEngineDriver::sendPowerGatingCommand(bool enter) {
    if (!device.pg_supported)
        return kIOReturnNotPermitted;
    
    UInt8 cmd;
    if (enter)
        cmd = MEI_PG_ISOLATION_ENTRY_REQ_CMD;
    else
        cmd = MEI_PG_ISOLATION_EXIT_REQ_CMD;
    
    MEIBusMessageHeader header;
    MEIBusPowerGatingRequest req;
    setupMessageHeader(&header, sizeof(req));
    memset(&req, 0, sizeof(req));
    req.cmd = cmd;
    
    return writeHostMessage(&header, MEI_TO_MSG(&req));
}

IOReturn SurfaceManagementEngineDriver::sendStopRequest() {
    MEIBusMessageHeader header;
    MEIBusHostStopRequest req;
    setupMessageHeader(&header, sizeof(req));
    memset(&req, 0, sizeof(req));
    req.cmd = MEI_HOST_STOP_REQ_CMD;
    req.reason = DriverStopRequest;
    
    return writeHostMessage(&header, MEI_TO_MSG(&req));
}

IOReturn SurfaceManagementEngineDriver::sendCapabilityRequest() {
    MEIBusMessageHeader header;
    MEIBusCapabilityRequest req;
    setupMessageHeader(&header, sizeof(req));
    memset(&req, 0, sizeof(req));
    req.cmd = MEI_CAPABILITIES_REQ_CMD;
    if (device.vt_supported)
        req.capability_requested[0] |= MEI_HBM_CAP_VTAG;
    if (device.cd_supported)
        req.capability_requested[0] |= MEI_HBM_CAP_CLIENT_DMA;

    IOReturn ret = writeHostMessage(&header, MEI_TO_MSG(&req));
    if (ret != kIOReturnSuccess)
        return ret;
    bus.state = MEIBusSetupCap;
    init_timeout->setTimeoutMS(MEI_CLIENTS_INIT_TIMEOUT * 1000);
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::sendClientEnumerationRequest() {
    MEIBusMessageHeader header;
    MEIBusHostEnumerationRequest req;
    setupMessageHeader(&header, sizeof(req));
    memset(&req, 0, sizeof(req));
    req.cmd = MEI_HOST_ENUM_REQ_CMD;
    req.flags |= device.dc_supported ? MEI_HBM_ENUM_ALLOW_ADD_FLAG : 0;
    req.flags |= device.ie_supported ? MEI_HBM_ENUM_IMMEDIATE_FLAG : 0;

    IOReturn ret = writeHostMessage(&header, MEI_TO_MSG(&req));
    if (ret != kIOReturnSuccess)
        return ret;
    bus.state = MEIBusEnumerateClients;
    init_timeout->setTimeoutMS(MEI_CLIENTS_INIT_TIMEOUT * 1000);
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::sendClientPropertyRequest(UInt start_idx) {
    UInt addr = bitmap_find_next_bit(me_client_map, MEI_MAX_CLIENT_NUM, start_idx);
    // We got all client properties
    if (addr == MEI_MAX_CLIENT_NUM) {
        bus.state = MEIBusStarted;
        device.state = MEIDeviceEnabled;
        device.reset_cnt = 0;
        rescan_work->interruptOccurred(nullptr, this, 0);
        // Enable idle (d0i3 mode)
        idle_timeout->setTimeoutMS(MEI_DEVICE_IDLE_TIMEOUT * 1000);
        return kIOReturnSuccess;
    }
    
    MEIBusMessageHeader header;
    MEIBusClientPropertyRequest req;
    setupMessageHeader(&header, sizeof(req));
    memset(&req, 0, sizeof(req));
    req.cmd = MEI_HOST_CLIENT_PROP_REQ_CMD;
    req.me_addr = addr;
    
    IOReturn ret = writeHostMessage(&header, MEI_TO_MSG(&req));
    if (ret != kIOReturnSuccess)
        return ret;
    init_timeout->setTimeoutMS(MEI_CLIENTS_INIT_TIMEOUT * 1000);
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::sendDMASetupRequest() {
    MEIBusMessageHeader header;
    MEIBusDMASetupRequest req;
    setupMessageHeader(&header, sizeof(req));
    memset(&req, 0, sizeof(req));
    req.cmd = MEI_DMA_SETUP_REQ_CMD;
    for (int i = 0; i < MEIDMADescriptorSize; i++) {
        IOPhysicalAddress paddr = device.ring_desc[i].paddr;
        req.dma_info[i].addr_hi = paddr >> 32;
        req.dma_info[i].addr_lo = paddr & 0xffffffff;
        req.dma_info[i].size = device.ring_desc[i].size;
    }
    resetDMARing();
    
    IOReturn ret = writeHostMessage(&header, MEI_TO_MSG(&req));
    if (ret != kIOReturnSuccess)
        return ret;
    bus.state = MEIBusSetupDMARing;
    init_timeout->setTimeoutMS(MEI_CLIENTS_INIT_TIMEOUT * 1000);
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::sendAddClientResponse(UInt8 me_addr, MEIHostBusMessageReturnType status) {
    MEIBusMessageHeader header;
    MEIBusAddClientResponse res;
    setupMessageHeader(&header, sizeof(res));
    memset(&res, 0, sizeof(res));
    res.cmd = MEI_ADD_CLIENT_RES_CMD;
    res.me_addr = me_addr;
    res.status = status;
    
    return writeHostMessage(&header, MEI_TO_MSG(&res));
}

void SurfaceManagementEngineDriver::scheduleReset(IOInterruptEventSource *sender, int count) {
    clearInterrupts();
    
    IOReturn ret = resetDevice();
    
    if (device.state == MEIDeviceDisabled) {
        LOG("Error! Device disabled");
        return;
    }
    // retry in case of failure
    if (ret != kIOReturnSuccess)
        reset_work->interruptOccurred(nullptr, this, 0);
}

void SurfaceManagementEngineDriver::scheduleRescan(IOInterruptEventSource *sender, int count) {
    if (!ipts_client->active) {
        LOG("Seems the client has been removed by hardware");
        ipts_client->stop(this);
        ipts_client->detach(this);
        OSSafeReleaseNULL(ipts_client);
    } else if (!ipts_client->initial) {
        // Disabled due to bus reset
        LOG("Reconnecting client...");
        ipts_client->hostRequestReconnect();
        return;
    } else {
        LOG("Starting client...");
        if (!ipts_client->attach(this) || !ipts_client->start(this)) {
            LOG("Failed to start client");
            ipts_client->detach(this);
            OSSafeReleaseNULL(ipts_client);
        }
    }
}

void SurfaceManagementEngineDriver::scheduleResume(IOInterruptEventSource *sender, int count) {
    if (exitPowerGatingSync() != kIOReturnSuccess) {
        LOG("Warning! Exit d0i3 failed. Resetting...");
        reset_work->interruptOccurred(nullptr, this, 0);
    }
}

bool SurfaceManagementEngineDriver::filterInterrupt(IOFilterInterruptEventSource *sender) {
    UInt32 hcsr = readRegister(MEI_H_CSR);
    return (hcsr & MEI_H_CSR_INT_STA_MASK) != 0;
}

void SurfaceManagementEngineDriver::handleInterrupt(IOInterruptEventSource *sender, int count) {
    UInt8 rx_slots = 0;
    
    disableInterrupts();
    
    UInt32 hcsr = readRegister(MEI_H_CSR);
    
    clearInterrupts();
    
    /* check if ME wants a reset */
    if (!isHardwareReady() && device.state != MEIDeviceResetting) {
        LOG("Hardware not ready! Resetting...");
        reset_work->interruptOccurred(nullptr, this, 0);
        goto end;
    }

    if (readRegister(MEI_ME_CSR) & MEI_ME_CSR_RESET)
        setHostInterrupt();

    handlePowerGatingInterrupt(hcsr & MEI_H_CSR_INT_STA_MASK);
    
    //  Check if we need to start the device
    if (!isHostReady()) {
        if (isHardwareReady()) {
            LOG("We need to start the device");
            command_gate->commandWakeup(&wait_hw_ready);
        } else {
            LOG("Spurious Interrupt");
        }
        goto end;
    }
    
    // Check slots available for reading
    if (countRxSlots(&rx_slots) == kIOReturnSuccess) {
        while (rx_slots > 0) {
            IOReturn ret = handleRead(&rx_slots);
            /* There is a race between ME write and interrupt delivery:
             * Not all data is always available immediately after the
             * interrupt, so try to read again on the next interrupt.
             */
            if (ret == kIOReturnNotReadable)
                break;

            if (ret != kIOReturnSuccess &&
                (device.state != MEIDeviceResetting && device.state != MEIDevicePowerDown)) {
                LOG("Read message failed! Resetting...");
                reset_work->interruptOccurred(nullptr, this, 0);
                goto end;
            }
        }
    }

    bus.tx_buf_ready = calcFilledSlots() == 0;
    /*
     * During PG handshake only allowed write is the replay to the
     * PG exit message, so block calling write function
     * if the pg event is in PG handshake
     */
    if (device.pg_event != MEIPowerGatingEventWait && device.pg_event != MEIPowerGatingEventReceived) {
        handleWrite();
        bus.tx_buf_ready = calcFilledSlots() == 0;
    }
end:
    enableInterrupts();
}

void SurfaceManagementEngineDriver::handlePowerGatingInterrupt(UInt32 source) {
    if (device.pg_event == MEIPowerGatingInterruptWait && source & MEI_H_CSR_D0I3C_INT_STA) {
        device.pg_event = MEIPowerGatingInterruptReceived;
        if (device.pg_state == MEIPowerGatingOn) {
            device.pg_state = MEIPowerGatingOff;
            if (bus.state != MEIBusIdle) {
                // Force MEI_H_CSR_READY because it could be wiped off during PG
                LOG("d0i3 set host ready");
                enableDevice();
            }
        } else
            device.pg_state = MEIPowerGatingOn;
        command_gate->commandWakeup(&wait_power_gating);
    }

    if (device.pg_state == MEIPowerGatingOn && (source & MEI_H_CSR_INT_STA)) {
        /*
         * HW sent some data and we are in D0i3,
         * so we got here because of HW initiated exit from D0i3.
         */
        LOG("Resuming from d0i3");
        resume_work->interruptOccurred(nullptr, this, 0);
    }
}

IOReturn SurfaceManagementEngineDriver::handleRead(UInt8 *filled_slots) {
    MEIBusMessageHeader *msg_hdr;
    MEIBusExtendedMetaHeader *meta_hdr = nullptr;
    IOReturn ret = kIOReturnSuccess;

    if (!bus.rx_msg_hdr[0]) {
        bus.rx_msg_hdr[0] = readRegister(MEI_ME_CB_RW);
        bus.rx_msg_hdr_len = 1;
        (*filled_slots)--;

        UInt16 expected_len = 0;
        msg_hdr = reinterpret_cast<MEIBusMessageHeader *>(bus.rx_msg_hdr);
        if (msg_hdr->dma_ring)
            expected_len += MEI_SLOT_SIZE;
        if (msg_hdr->extended)
            expected_len += MEI_SLOT_SIZE;
        if (!bus.rx_msg_hdr[0] || msg_hdr->reserved || msg_hdr->length < expected_len) {
            LOG("Error! Corrupted message header 0x%08X", bus.rx_msg_hdr[0]);
            return kIOReturnInvalid;
        }
    }

    msg_hdr = reinterpret_cast<MEIBusMessageHeader *>(bus.rx_msg_hdr);
    if (MEI_SLOTS_TO_DATA(*filled_slots) < msg_hdr->length) {
        LOG("Error! Less data available than length=%08x.", *filled_slots);
        /* we can't read the message right now */
        return kIOReturnNotReadable;
    }
    
    int ext_hdr_len = 1;
    UInt16 hdr_size_left = msg_hdr->length;
    if (msg_hdr->extended) {
        LOG("Got extended header");
        if (!bus.rx_msg_hdr[1]) {
            bus.rx_msg_hdr[1] = readRegister(MEI_ME_CB_RW);
            bus.rx_msg_hdr_len++;
            (*filled_slots)--;
        }
        meta_hdr = reinterpret_cast<MEIBusExtendedMetaHeader *>(&bus.rx_msg_hdr[1]);
        UInt16 hdr_size_ext = sizeof(MEIBusExtendedMetaHeader) + MEI_SLOTS_TO_DATA(meta_hdr->size);
        
        if (hdr_size_left < hdr_size_ext) {
            LOG("Error! Corrupted message header len %d", msg_hdr->length);
            return kIOReturnInvalid;
        }
        hdr_size_left -= hdr_size_ext;
        ext_hdr_len = meta_hdr->size + 2; // 2 = MEIBusMessageHeader + MEIBusExtendedMetaHeader
        for (int i = bus.rx_msg_hdr_len; i < ext_hdr_len; i++) {
            bus.rx_msg_hdr[i] = readRegister(MEI_ME_CB_RW);
            bus.rx_msg_hdr_len++;
            (*filled_slots)--;
        }
    }
    if (msg_hdr->dma_ring) {
        LOG("Got dma ring set");
        if (hdr_size_left != MEI_SLOT_SIZE) {
            LOG("Error! Corrupted message header len %d", msg_hdr->length);
            return kIOReturnInvalid;
        }
        bus.rx_msg_hdr[ext_hdr_len] = readRegister(MEI_ME_CB_RW);
        bus.rx_msg_hdr_len++;
        (*filled_slots)--;
        msg_hdr->length -= MEI_SLOT_SIZE;
    }

    if (msg_hdr->host_addr == 0 && msg_hdr->me_addr == 0) {
        //  Host message
        ret = handleHostMessage(msg_hdr);
        if (ret != kIOReturnSuccess) {
            LOG("Handle message failed!");
            return ret;
        }
    } else {
        // Client message
        if (!ipts_client || !ipts_client->active || msg_hdr->me_addr != ipts_client->addr) {
            /*
             * A message for not connected fixed address clients should be silently discarded
             * On power down client may be force cleaned, silently discard such messages
             */
            if ((msg_hdr->host_addr == 0 && msg_hdr->me_addr != 0) || device.state == MEIDevicePowerDown) {
                discardMessage(msg_hdr, msg_hdr->length);
                ret = kIOReturnSuccess;
            } else {
                LOG("No destination client found 0x%08X",  bus.rx_msg_hdr[0]);
                return kIOReturnInvalid;
            }
            
        } else
            ret = handleClientMessage(ipts_client, msg_hdr, meta_hdr);
    }
    // Reset the number of slots and header
    memset(bus.rx_msg_hdr, 0, sizeof(bus.rx_msg_hdr));
    bus.rx_msg_hdr_len = 0;
    if (countRxSlots(filled_slots) != kIOReturnSuccess) {
        LOG("Resetting due to slots overflow");
        return kIOReturnOverrun;
    }
    return ret;
}

IOReturn SurfaceManagementEngineDriver::handleHostMessage(MEIBusMessageHeader *header) {
    MEIBusHostVersionResponse *version_res;
    MEIBusClientPropertyResponse *props_res;
    MEIBusHostEnumerationResponse *enum_res;
    MEIBusDMASetupResponse *dma_setup_res;
    MEIBusAddClientRequest *add_client_req;
    MEIBusCapabilityResponse *cap_res;
    
//    MEIBusClientConnectionResponse *connect_res;
//    MEIBusClientConnectionRequest *disconnect_req;
//    MEIBusFlowControl *flow_ctrl;
//    MEIBusNotificationResponse *notif_res;
//    MEIBusClientDMAResponse *client_dma_res;
    
    MEIHostBusMessageReturnType status;
    IOReturn ret;

    // read the message to our buffer
    if (header->length >= sizeof(bus.rx_msg_buf)) {
        LOG("WTF? Message length larger than rx_msg_buf");
        return kIOReturnError;
    }
    readMessage(bus.rx_msg_buf, header->length);
    
    MEIBusMessage *mei_msg = reinterpret_cast<MEIBusMessage *>(bus.rx_msg_buf);
//    MEIBusClientCommand *cl_cmd = reinterpret_cast<MEIBusClientCommand *>(mei_msg);

    /*
     * ignore spurious message and prevent reset nesting
     * bus is put to idle during system reset
     */
    if (bus.state == MEIBusIdle) {
        LOG("Bus state is idle, ignore spurious messages");
        return kIOReturnSuccess;
    }

    switch (mei_msg->cmd) {
        case MEI_HOST_START_RES_CMD:
            LOG("Start response message received");
            init_timeout->cancelTimeout();
            version_res = reinterpret_cast<MEIBusHostVersionResponse *>(mei_msg);
//            LOG("MEI Bus Version: DRIVER=%02d:%02d DEVICE=%02d:%02d", MEI_HBM_MAJOR_VERSION, MEI_HBM_MINOR_VERSION,
//                version_res->me_max_version_major, version_res->me_max_version_minor);
            if (version_res->host_version_supported) {
                device.version_major = MEI_HBM_MAJOR_VERSION;
                device.version_minor = MEI_HBM_MINOR_VERSION;
            } else {
                device.version_major = version_res->me_max_version_major;
                device.version_minor = version_res->me_max_version_minor;
            }

            if (!isBusMessageVersionSupported()) {
                LOG("Warning, start version mismatch - stopping the driver");
                bus.state = MEIBusStopped;
                if (sendStopRequest() != kIOReturnSuccess) {
                    LOG("Error! Failed to send stop request");
                    return kIOReturnIOError;
                }
                break;
            }
            
            configDeviceFeatures();
            
            if (device.state != MEIDeviceInitClients || bus.state != MEIBusStarting) {
                if (device.state == MEIDevicePowerDown) {
                    LOG("Start on shutdown, ignoring");
                    return kIOReturnSuccess;
                }
                LOG("Error! Start state mismatch, [%d, %d]", device.state, bus.state);
                return kIOReturnError;
            }

            if (device.cap_supported) {
                if (sendCapabilityRequest() != kIOReturnSuccess) {
                    LOG("Capabilities request failed");
                    return kIOReturnIOError;
                }
                command_gate->commandWakeup(&wait_bus_start);
                break;
            }

            if (device.dr_supported) {
                if (allocateDMARing() == kIOReturnSuccess) {
                    if (sendDMASetupRequest() != kIOReturnSuccess){
                        LOG("DMA setup request request failed");
                        return kIOReturnIOError;
                    }
                    command_gate->commandWakeup(&wait_bus_start);
                    break;
                }
            }
            device.dr_supported = false;
            
            if (sendClientEnumerationRequest() != kIOReturnSuccess) {
                LOG("Enumeration request failed");
                return kIOReturnIOError;
            }
            command_gate->commandWakeup(&wait_bus_start);
            break;
            
        case MEI_CAPABILITIES_RES_CMD:
            LOG("Capabilities response message received");
            init_timeout->cancelTimeout();
            if (device.state != MEIDeviceInitClients || bus.state != MEIBusSetupCap) {
                if (device.state == MEIDevicePowerDown) {
                    LOG("Capabilities response on shutdown, ignoring");
                    return kIOReturnSuccess;
                }
                LOG("Error! Capabilities response state mismatch, [%d, %d]", device.state, bus.state);
                return kIOReturnError;
            }

            cap_res = reinterpret_cast<MEIBusCapabilityResponse *>(mei_msg);
            if (!(cap_res->capability_granted[0] & MEI_HBM_CAP_VTAG))
                device.vt_supported = false;
            if (!(cap_res->capability_granted[0] & MEI_HBM_CAP_CLIENT_DMA))
                device.cd_supported = false;

            if (device.dr_supported) {
                if (allocateDMARing() == kIOReturnSuccess) {
                    if (sendDMASetupRequest() != kIOReturnSuccess){
                        LOG("DMA setup request request failed");
                        return kIOReturnIOError;
                    }
                    break;
                }
            }
            device.dr_supported = false;

            if (sendClientEnumerationRequest() != kIOReturnSuccess) {
                LOG("Enumeration request failed");
                return kIOReturnIOError;
            }
            break;

        case MEI_DMA_SETUP_RES_CMD:
            LOG("DMA setup response message received");
            init_timeout->cancelTimeout();
            if (device.state != MEIDeviceInitClients || bus.state != MEIBusSetupDMARing) {
                if (device.state == MEIDevicePowerDown) {
                    LOG("DMA setup response on shutdown, ignoring");
                    return kIOReturnSuccess;
                }
                LOG("Error! DMA setup response: state mismatch, [%d, %d]", device.state, bus.state);
                return kIOReturnError;
            }

            dma_setup_res = reinterpret_cast<MEIBusDMASetupResponse *>(mei_msg);
            if (dma_setup_res->status != MEIHostBusMessageReturnSuccess) {
                if (dma_setup_res->status == MEIHostBusMessageReturnNotAllowed) {
                    //FIXME: why not allowed ?
                    LOG("Warning, DMA setup not allowed");
                } else {
                    LOG("DMA setup failed %d", dma_setup_res->status);
                }
                device.dr_supported = false;
                freeDMARing();
            }

            if (sendClientEnumerationRequest() != kIOReturnSuccess) {
                LOG("Enumeration request failed");
                return kIOReturnIOError;
            }
            break;
            
        case MEI_HOST_ENUM_RES_CMD:
            LOG("Enumeration response message received");
            init_timeout->cancelTimeout();
            enum_res = reinterpret_cast<MEIBusHostEnumerationResponse *>(mei_msg);
            memcpy(me_client_map, enum_res->valid_addresses, sizeof(enum_res->valid_addresses));
            if (device.state != MEIDeviceInitClients || bus.state != MEIBusEnumerateClients) {
                if (device.state == MEIDevicePowerDown) {
                    LOG("enumeration response on shutdown, ignoring");
                    return kIOReturnSuccess;
                }
                LOG("Error! Enumeration response state mismatch, [%d, %d]", device.state, bus.state);
                return kIOReturnError;
            }

            bus.state = MEIBusRequestingClientProps;
            // First client property request
            if (sendClientPropertyRequest(0) != kIOReturnSuccess) {
                LOG("Error! Properties request failed");
                return kIOReturnIOError;
            }
            break;

        case MEI_HOST_CLIENT_PROP_RES_CMD:
            LOG("Property response message received");
            init_timeout->cancelTimeout();
            if (device.state != MEIDeviceInitClients || bus.state != MEIBusRequestingClientProps) {
                if (device.state == MEIDevicePowerDown) {
                    LOG("Properties response on shutdown, ignoring");
                    return kIOReturnSuccess;
                }
                LOG("Error! Properties response state mismatch, [%d, %d]", device.state, bus.state);
                return kIOReturnError;
            }

            props_res = reinterpret_cast<MEIBusClientPropertyResponse *>(mei_msg);
            if (props_res->status == MEIHostBusMessageReturnClientNotFound) {
                LOG("Warning, client %d not found", props_res->me_addr);
            } else if (props_res->status != MEIHostBusMessageReturnSuccess) {
                LOG("Error! Properties response wrong status = %d", props_res->status);
                return kIOReturnError;
            } else {
                if (addClient(&props_res->client_properties, props_res->me_addr) == kIOReturnInvalid) {
                    uuid_t swapped_uuid;
                    uuid_string_t uuid_str;
                    uuid_copy(swapped_uuid, props_res->client_properties.uuid);
                    *(reinterpret_cast<UInt32 *>(swapped_uuid)) = OSSwapInt32(*(reinterpret_cast<UInt32 *>(swapped_uuid)));
                    *(reinterpret_cast<UInt16 *>(swapped_uuid) + 2) = OSSwapInt16(*(reinterpret_cast<UInt16 *>(swapped_uuid) + 2));
                    *(reinterpret_cast<UInt16 *>(swapped_uuid) + 3) = OSSwapInt16(*(reinterpret_cast<UInt16 *>(swapped_uuid) + 3));
                    uuid_unparse_lower(swapped_uuid, uuid_str);
                    LOG("Unknown MEI client with uuid %s", uuid_str);
                    LOG("Ignore non IPTS client, continue to request client properties");
                }
            }
            // Request property for next client
            if (sendClientPropertyRequest(props_res->me_addr + 1) != kIOReturnSuccess) {
                LOG("Error! Properties request failed");
                return kIOReturnIOError;
            }
            break;
            
        case MEI_PG_ISOLATION_ENTRY_RES_CMD:
            LOG("Power gating isolation entry response received");
            if (device.pg_state != MEIPowerGatingOff || device.pg_event != MEIPowerGatingEventWait) {
                LOG("Power gating entry response, state mismatch [%d, %d]", device.pg_state, device.pg_event);
                return kIOReturnError;
            }

            device.pg_event = MEIPowerGatingEventReceived;
            command_gate->commandWakeup(&wait_power_gating);
            break;

        case MEI_PG_ISOLATION_EXIT_REQ_CMD:
            LOG("Power gating isolation exit request received");
            if (device.pg_state != MEIPowerGatingOn ||
                (device.pg_event != MEIPowerGatingEventWait && device.pg_event != MEIPowerGatingEventIdle)) {
                LOG("Power gating exit response state mismatch [%d, %d]", device.pg_state, device.pg_event);
                return kIOReturnError;
            }

            switch (device.pg_event) {
                case MEIPowerGatingEventWait:
                    device.pg_event = MEIPowerGatingEventReceived;
                    command_gate->commandWakeup(&wait_power_gating);
                    break;
                case MEIPowerGatingEventIdle:
                    /*
                    * If the driver is not waiting on this then
                    * this is HW initiated exit from PG.
                    */
                    device.pg_event = MEIPowerGatingEventReceived;
                    resume_work->interruptOccurred(nullptr, this, 0);
                    break;
                default:
                    return kIOReturnError;
            }
            break;

        case MEI_HOST_STOP_RES_CMD:
            LOG("Stop response message received");
            init_timeout->cancelTimeout();
            if (bus.state != MEIBusStopped) {
                LOG("Error! Stop response state mismatch, [%d, %d]", device.state, bus.state);
                return kIOReturnError;
            }
            
            device.state = MEIDevicePowerDown;
            // force the reset
            return kIOReturnError;

        case MEI_ME_STOP_REQ_CMD:
            LOG("Stop request message received");
            bus.state = MEIBusStopped;
            if (sendStopRequest() != kIOReturnSuccess) {
                LOG("Error! Stop request failed");
                return kIOReturnIOError;
            }
            break;
            
        case MEI_ADD_CLIENT_REQ_CMD:
            LOG("Add client request received");
            // After the host receives the enum resp message clients may be added or removed
            if (bus.state <= MEIBusEnumerateClients || bus.state >= MEIBusStopped) {
                LOG("Error! Add client state mismatch, [%d, %d]", device.state, bus.state);
                return kIOReturnError;
            }
            add_client_req = reinterpret_cast<MEIBusAddClientRequest *>(mei_msg);
            status = MEIHostBusMessageReturnSuccess;
            ret = addClient(&add_client_req->client_properties, add_client_req->me_addr);
            if (ret == kIOReturnInvalid)
                status = MEIHostBusMessageReturnRejected;
            else if (ret != kIOReturnSuccess)
                status = MEIHostBusMessageReturnClientNotFound;
            
            if (device.state == MEIDeviceEnabled)
                rescan_work->interruptOccurred(nullptr, this, 0);
            
            if (sendAddClientResponse(add_client_req->me_addr, status) != kIOReturnSuccess) {
                LOG("Error! Failed to send add client response");
                return kIOReturnIOError;
            }
            break;
            
//        case MEI_CLIENT_CONNECT_RES_CMD:
//            LOG("Client connect response message received");
//            if (!ipts_client || ipts_client->addr != cl_cmd->me_addr) {
//                LOG("WTF? Client not found");
//                break;
//            }
//
//            connect_res = reinterpret_cast<MEIBusClientConnectionResponse *>(cl_cmd);
//            if (connect_res->status == MEIClientConnectionSuccess)
//                ipts_client->state = MEI_FILE_CONNECTED;
//            else {
//                LOG("Client connection failed, response status %d", connect_res->status);
//                ipts_client->state = MEI_FILE_DISCONNECT_REPLY;
//                if (connect_res->status == MEIClientConnectionNotFound) {
//                    LOG("Invalid client!");
//                    ipts_client->active = false;
//                    if (device.state == MEI_DEV_ENABLED)
//                        rescan_work->interruptOccurred(nullptr, this, 0);
//                }
//            }
//            switch (connect_res->status) {
//                case MEIClientConnectionSuccess:
//                    ipts_client->status = kIOReturnSuccess;
//                    break;
//                case MEIClientConnectionNotFound:
//                    ipts_client->status = kIOReturnNoResources;
//                    break;
//                case MEIClientConnectionAlreadyStarted:
//                case MEIClientConnectionOutOfResources:
//                case MEIClientConnectionNotAllowed:
//                    ipts_client->status = kIOReturnBusy;
//                    break;
//                case MEIClientConnectionMessageTooSmall:
//                default:
//                    ipts_client->status = kIOReturnInvalid;
//                    break;
//            }
//
//            ipts_client->connection_timeout->cancelTimeout();
//            command_gate->commandWakeup(&ipts_client->ctrl_wait);
//            break;
//
//        case MEI_CLIENT_DISCONNECT_RES_CMD:
//            LOG("Client disconnect response message received");
//            if (!ipts_client || ipts_client->addr != cl_cmd->me_addr) {
//                LOG("WTF? Client not found");
//                break;
//            }
//
//            connect_res = reinterpret_cast<MEIBusClientConnectionResponse *>(cl_cmd);
//            if (connect_res->status == MEIClientConnectionSuccess)
//                ipts_client->state = MEI_FILE_DISCONNECT_REPLY;
//            ipts_client->status = kIOReturnSuccess;
//
//            ipts_client->connection_timeout->cancelTimeout();
//            command_gate->commandWakeup(&ipts_client->ctrl_wait);
//            break;
//
//        case MEI_FLOW_CONTROL_CMD:
//            LOG("Client flow control response message received");
//            if (!ipts_client || ipts_client->addr != cl_cmd->me_addr) {
//                LOG("WTF? Client not found");
//                break;
//            }
//
//            flow_ctrl = reinterpret_cast<MEIBusFlowControl *>(cl_cmd);
//            if (!flow_ctrl->host_addr && !ipts_client->properties.single_recv_buf) {
//                LOG("This client does not share receive buffer!");
//                break;
//            }
//            ipts_client->tx_credits++;
//            break;
//
//        case MEI_CLIENT_DISCONNECT_REQ_CMD:
//            LOG("Disconnect request message received");
//            if (!ipts_client || ipts_client->addr != cl_cmd->me_addr) {
//                LOG("WTF? Client not found");
//                break;
//            }
//
//            disconnect_req = reinterpret_cast<MEIBusClientConnectionRequest *>(cl_cmd);
//            ipts_client->state = MEI_FILE_DISCONNECTING;
//            ipts_client->connection_timeout->cancelTimeout();
//
//            mei_cl_enqueue_ctrl_wr_cb(cl, 0, MEI_FOP_DISCONNECT_RSP,
//            break;
//
//        case MEI_NOTIFY_RES_CMD:
//            LOG("Notify response received");
//            if (!ipts_client || ipts_client->addr != cl_cmd->me_addr) {
//                LOG("WTF? Client not found");
//                break;
//            }
//
//            notif_res = reinterpret_cast<MEIBusNotificationResponse *>(cl_cmd);
//            if (notif_res->start) {
//                LOG("Notify start");
//                if (notif_res->status == MEIHostBusMessageReturnSuccess || notif_res->status == MEIHostBusMessageReturnAlreadyStarted) {
//                    ipts_client->notify_enabled = true;
//                    ipts_client->status = kIOReturnSuccess;
//                } else
//                    ipts_client->status = kIOReturnInvalid;
//            } else {
//                LOG("Notify stop");
//                if (notif_res->status == MEIHostBusMessageReturnSuccess || notif_res->status == MEIHostBusMessageReturnNotStarted) {
//                    ipts_client->notify_enabled = false;
//                    ipts_client->status = kIOReturnSuccess;
//                } else
//                    ipts_client->status = kIOReturnInvalid;
//            }
//
//            ipts_client->connection_timeout->cancelTimeout();
//            command_gate->commandWakeup(&ipts_client->ctrl_wait);
//            break;
//
//        case MEI_NOTIFICATION_CMD:
//            LOG("Notification");
//            if (!ipts_client || ipts_client->addr != cl_cmd->me_addr) {
//                LOG("WTF? Client not found");
//                break;
//            }
//
//            if (!ipts_client->notify_enabled) {
//                LOG("Notification not enabled yet");
//                break;
//            }
//
//            cl->notify_ev = true;
//            if (!mei_cl_bus_notify_event(cl))
//                wake_up_interruptible(&cl->ev_wait);
//            if (cl->ev_async)
//                kill_fasync(&cl->ev_async, SIGIO, POLL_PRI);
//            break;
//
//        case MEI_CLIENT_DMA_MAP_RES_CMD:
//            LOG("Client DMA map response message received");
//            client_dma_res = reinterpret_cast<MEIBusClientDMAResponse *>(mei_msg);
//            mei_hbm_cl_dma_map_res(dev, client_dma_res);
//            break;
//
//        case MEI_CLIENT_DMA_UNMAP_RES_CMD:
//            LOG("Client DMA unmap response message received");
//            client_dma_res = reinterpret_cast<MEIBusClientDMAResponse *>(mei_msg);
//            mei_hbm_cl_dma_unmap_res(dev, client_dma_res);
//            break;

        default:
            LOG("WTF? Unknown command %d", mei_msg->cmd);
            return kIOReturnError;

    }
    return kIOReturnSuccess;
}

IOReturn SurfaceManagementEngineDriver::handleClientMessage(SurfaceManagementEngineClient *client, MEIBusMessageHeader *mei_hdr, MEIBusExtendedMetaHeader *meta) {
    UInt16 data_len;
    UInt16 length = mei_hdr->length;
    
    //TODO: extended & dma ring
    if (mei_hdr->extended) {
        LOG("Got extended header");
        goto discard;
    }
    if (mei_hdr->dma_ring) {
        LOG("Got dma ring");
        goto discard;
    }

    data_len = length + client->rx_cache_pos;
    if (client->properties.max_msg_length < data_len) {
        LOG("Warning, message overflow. client max msg size %d, rx msg size %d", client->properties.max_msg_length, data_len);
        goto discard;
    }

    readMessage(client->rx_cache + client->rx_cache_pos, length);
    client->rx_cache_pos += length;

    if (mei_hdr->msg_complete)
        client->messageComplete();
    
    idle_timeout->setTimeoutMS(MEI_DEVICE_IDLE_TIMEOUT * 1000);

    return kIOReturnSuccess;
discard:
    discardMessage(mei_hdr, length);
    return kIOReturnSuccess;
}

void SurfaceManagementEngineDriver::discardMessage(MEIBusMessageHeader *hdr, UInt16 discard_len) {
    if (hdr->dma_ring) {
        //TODO: DMA ring read
//        mei_dma_ring_read(dev, NULL, hdr->extension[dev->rd_msg_hdr_count - 2]);
        discard_len = 0;
    }
    readMessage(bus.rx_msg_buf, discard_len);
}

IOReturn SurfaceManagementEngineDriver::handleWrite() {
    if (!acquireWriteBuffer())
        return kIOReturnAborted;
    
    MEIClientTransaction *tx;
    qe_foreach_element_safe(tx, &bus.tx_queue, entry) {
        if (submitTransaction(tx))
            completeTransaction(tx);
    }
    return kIOReturnSuccess;
}

void SurfaceManagementEngineDriver::completeTransaction(MEIClientTransaction *tx) {
    if (tx->blocking)
        command_gate->commandWakeup(&tx->wait);
    else {
        remqueue(&tx->entry);
        delete[] tx->data;
        delete tx;
    }
}

bool SurfaceManagementEngineDriver::submitTransaction(MEIClientTransaction *tx) {
    MEIBusMessageHeader msg_hdr;
    msg_hdr.host_addr = 0;
    msg_hdr.me_addr = tx->client->addr;
    msg_hdr.internal = false;
    msg_hdr.extended = false;
    msg_hdr.length = 0;
    
    UInt16 hdr_len = sizeof(MEIBusMessageHeader) + msg_hdr.length;
    UInt16 data_len;
    UInt8 empty_slots;
    if (findEmptySlots(&empty_slots) != kIOReturnSuccess) {
        LOG("Write buffer overflow detected");
        return false;
    }
    UInt16 empty_len = MEI_SLOTS_TO_DATA(empty_slots) & MEI_MSG_MAX_LEN_MASK;
    
    /**
     * Split the message only if we can write the whole host buffer
     * otherwise wait for next time the host buffer is empty.
     */
    if (hdr_len + tx->data_len <= empty_len) {
        data_len = tx->data_len;
        msg_hdr.msg_complete = 1;
    } else if (empty_slots == device.tx_buf_depth) {
        data_len = empty_len - hdr_len;
        msg_hdr.msg_complete = 0;
    } else
        return false;
    
    msg_hdr.length += data_len;
    
    if (writeMessage(reinterpret_cast<UInt8 *>(&msg_hdr), hdr_len, tx->data, data_len) != kIOReturnSuccess)
        return true;    // delete this transaction in case of error but leave tx->completed=false
    
    if (msg_hdr.msg_complete) {
        tx->completed = true;
        return true;
    } else {
        tx->data += data_len;
        tx->data_len -= data_len;
        return false;
    }
}

void SurfaceManagementEngineDriver::enterIdle(IOTimerEventSource *timer) {
    IOReturn ret;
    if (isWriteQueueEmpty())
        ret = enterPowerGatingSync();
    else
        ret = kIOReturnBusy;
    
    if (ret != kIOReturnSuccess && ret != kIOReturnBusy) {
        LOG("Warning! Enter d0i3 failed. Resetting...");
        reset_work->interruptOccurred(nullptr, this, 0);
    } else if (ret == kIOReturnBusy)
        idle_timeout->setTimeoutMS(MEI_DEVICE_IDLE_TIMEOUT * 1000);
}

void SurfaceManagementEngineDriver::initialiseTimeout(IOTimerEventSource *timer) {
    if (device.state == MEIDeviceInitClients && bus.state != MEIBusIdle) {
        LOG("Init clients timeout");
        resetDevice();
    }
}

IOReturn SurfaceManagementEngineDriver::addClient(MEIClientProperty *client_props, UInt8 addr) {
    if (uuid_compare(SURFACE_IPTS_CLIENT_UUID, client_props->uuid) != 0)
        return kIOReturnInvalid;

    if (!ipts_client) {
        ipts_client = OSTypeAlloc(SurfaceManagementEngineClient);
        if (!ipts_client || !ipts_client->init()) {
            LOG("Failed to create ipts client");
            OSSafeReleaseNULL(ipts_client);
            return kIOReturnError;
        }
    }
    ipts_client->resetProperties(client_props, addr);
    return kIOReturnSuccess;
}
