//
//  IntelPreciseTouchStylusDriver.cpp
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/5.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#include "IntelPreciseTouchStylusDriver.hpp"
#include "SurfaceTouchScreenDevice.hpp"

#define LOG(str, ...)    IOLog("%s::" str "\n", "IntelPreciseTouchStylusDriver", ##__VA_ARGS__)

#define super IOService
OSDefineMetaClassAndStructors(IntelPreciseTouchStylusDriver, IOService)

void hex_dump(const char *name, UInt8 *buffer, UInt16 len) {
    if (len == 0)
        return;
    IOLog("%s::msg dump: length: %d\n", name, len);
    int row = (len-1)/16 + 1;
    for (int r=0; r<row; r++) {
        UInt8 line[16] = {0};
        for (int i=0; i < 16 && r*16+i < len; i++) {
            line[i] = buffer[r*16+i];
        }
        IOLog("%s::%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n", name, line[0], line[1], line[2], line[3], line[4], line[5], line[6], line[7], line[8], line[9], line[10], line[11], line[12], line[13], line[14], line[15]);
    }
}

IOService* IntelPreciseTouchStylusDriver::probe(IOService* provider, SInt32* score) {
    if (!super::probe(provider, score))
        return nullptr;

    api = OSDynamicCast(SurfaceManagementEngineClient, provider);
    if (!api)
        return nullptr;

    return this;
}

bool IntelPreciseTouchStylusDriver::start(IOService *provider) {
    if (!super::start(provider))
        return false;
    
    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Failed to create work loop");
        return false;
    }
    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate) {
        LOG("Failed to create command gate");
        goto exit;
    }
    work_loop->addEventSource(command_gate);
    wait_input = OSMemberFunctionCast(IOCommandGate::Action, this, &IntelPreciseTouchStylusDriver::getCurrentInputBufferIDGated);
    interrupt_source = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &IntelPreciseTouchStylusDriver::handleHIDReportGated));
    if (!interrupt_source) {
        LOG("Failed to create interrupt source!");
        goto exit;
    }
    work_loop->addEventSource(interrupt_source);
    timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &IntelPreciseTouchStylusDriver::pollTouchData));
    if (!timer) {
        LOG("Failed to create timer");
        goto exit;
    }
    work_loop->addEventSource(timer);
    
    // Publishing touch screen device
    touch_screen = OSTypeAlloc(SurfaceTouchScreenDevice);
    if (!touch_screen || !touch_screen->init() || !touch_screen->attach(this)) {
        LOG("Could not init Surface Touch Screen device");
        goto exit;
    }
    if (api->registerMessageHandler(this, OSMemberFunctionCast(SurfaceManagementEngineClient::MessageHandler, this, &IntelPreciseTouchStylusDriver::handleMessage)) != kIOReturnSuccess) {
        LOG("Failed to register receive handler");
        goto exit;
    }
    if (startDevice() != kIOReturnSuccess) {
        LOG("Failed to start IPTS device!");
        goto exit;
    }
    // Wait 1 second for device to start
    for (int i = 0; i < 40; i++) {
        if (state == IPTSDeviceStateStarted || state == IPTSDeviceStateStopped)
            break;
        IOSleep(25);
    }
    if (state == IPTSDeviceStateStopping || state == IPTSDeviceStateStopped) {
        LOG("Error occurred during starting process");
        goto exit;
    }
    timer->setTimeoutMS(1000);
    
    PMinit();
    api->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    return true;
exit:
    releaseResources();
    return false;
}

void IntelPreciseTouchStylusDriver::stop(IOService *provider) {
    stopDevice();
    // Wait at most 500ms for device to stop
    for (int i = 0; i < 20; i++) {
        if (state == IPTSDeviceStateStopped)
            break;
        IOSleep(25);
    }
    releaseResources();
    super::stop(provider);
}

IOReturn IntelPreciseTouchStylusDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            awake = false;
            timer->cancelTimeout();
            timer->disable();
            current_doorbell = 0;
            stopDevice();
            // Wait at most 500ms for device to stop
            for (int i = 0; i < 20; i++) {
                if (state == IPTSDeviceStateStopped)
                    break;
                IOSleep(25);
            }
            LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            IOReturn ret = kIOReturnSuccess;
            for (int i = 0; i < 5; i++) {
                IOSleep(500);
                ret = startDevice();
                if (ret != kIOReturnNoDevice)
                    break;
            }
            if (ret != kIOReturnSuccess) {
                LOG("Failed to restart IPTS device from sleep!");
            } else {
                timer->enable();
                timer->setTimeoutMS(1000);
            }
            LOG("Woke up");
        }
    }
    return kIOPMAckImplied;
}

void IntelPreciseTouchStylusDriver::releaseResources() {
    if (timer) {
        timer->cancelTimeout();
        timer->disable();
        work_loop->removeEventSource(timer);
        OSSafeReleaseNULL(timer);
    }
    if (interrupt_source) {
        interrupt_source->disable();
        work_loop->removeEventSource(interrupt_source);
        OSSafeReleaseNULL(interrupt_source);
    }
    if (command_gate) {
        command_gate->disable();
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    OSSafeReleaseNULL(work_loop);
    
    if (touch_screen) {
        touch_screen->stop(this);
        touch_screen->detach(this);
        OSSafeReleaseNULL(touch_screen);
    }
}

IOBufferMemoryDescriptor *IntelPreciseTouchStylusDriver::getReceiveBufferForIndex(int idx) {
    if (idx < 0 || idx >= IPTS_BUFFER_NUM)
        return nullptr;
    
    rx_buffer[idx].buffer->retain();
    return rx_buffer[idx].buffer;
}

UInt16 IntelPreciseTouchStylusDriver::getVendorID() {
    return touch_screen->vendor_id;
}

UInt16 IntelPreciseTouchStylusDriver::getDeviceID() {
    return touch_screen->device_id;
}

UInt8 IntelPreciseTouchStylusDriver::getMaxContacts() {
    return touch_screen->max_contacts;
}

IOReturn IntelPreciseTouchStylusDriver::getCurrentInputBufferID(UInt8 *buffer_idx) {
    return command_gate->runAction(wait_input, buffer_idx);
}

IOReturn IntelPreciseTouchStylusDriver::getCurrentInputBufferIDGated(UInt8 *buffer_idx) {
    waiting = true;
    IOReturn ret = command_gate->commandSleep(&wait);
    if (ret != THREAD_AWAKENED)
        return kIOReturnError;
    *buffer_idx = (current_doorbell - 1) % IPTS_BUFFER_NUM;
    return kIOReturnSuccess;
}

void IntelPreciseTouchStylusDriver::handleHIDReportGated(IOInterruptEventSource *sender, int count) {
    if (sent)
        return;
    
    IOBufferMemoryDescriptor *buffer;
    switch (report_to_send.report_id) {
        case IPTS_TOUCH_REPORT_ID:
            buffer = IOBufferMemoryDescriptor::withBytes(&report_to_send, sizeof(IPTSTouchHIDReport)+1, kIODirectionNone);
            break;
        case IPTS_STYLUS_REPORT_ID:
            buffer = IOBufferMemoryDescriptor::withBytes(&report_to_send, sizeof(IPTSStylusHIDReport)+1, kIODirectionNone);
            break;
        default:
            return;
    }
    touch_screen->handleReport(buffer);
    OSSafeReleaseNULL(buffer);
    sent = true;
}

void IntelPreciseTouchStylusDriver::handleHIDReport(const IPTSHIDReport *report) {
    report_to_send = *report;
    sent = false;
    interrupt_source->interruptOccurred(nullptr, this, 0);
}

void IntelPreciseTouchStylusDriver::pollTouchData(IOTimerEventSource *sender) {
    if (state != IPTSDeviceStateStarted) // IPTS not started yet
        timer->setTimeoutMS(1000);
    
    UInt32 doorbell;
    memcpy(&doorbell, doorbell_buffer.vaddr, sizeof(UInt32));
    
    if (doorbell == current_doorbell) {
        if (!busy) {
            timer->setTimeoutMS(IPTS_IDLE_TIMEOUT);
            return;
        }
        AbsoluteTime cur_time;
        UInt64 nsecs;
        clock_get_uptime(&cur_time);
        SUB_ABSOLUTETIME(&cur_time, &last_activate);
        absolutetime_to_nanoseconds(cur_time, &nsecs);
        if (nsecs < 500000000) // < 500ms
            timer->setTimeoutMS(IPTS_BUSY_TIMEOUT);
        else if (nsecs < 1500000000) // < 1.5s
            timer->setTimeoutMS(IPTS_ACTIVE_TIMEOUT);
        else {
            busy = false;
            timer->setTimeoutMS(IPTS_IDLE_TIMEOUT);
        }
    } else if (doorbell < current_doorbell) {  // MEI device has been reset
        LOG("MEI device has reset! Flushing buffers...");
        for (int i = 0; i < IPTS_BUFFER_NUM; i++)
            sendFeedback(i);
        current_doorbell = doorbell;
        timer->setTimeoutMS(IPTS_BUSY_TIMEOUT);
    } else {
        if (waiting) {
            command_gate->commandWakeup(&wait);
            waiting = false;
        }
        sendFeedback(current_doorbell % IPTS_BUFFER_NUM, false);   // non blocking feedback
        current_doorbell++;
        busy = true;
        clock_get_uptime(&last_activate);
        timer->setTimeoutMS(IPTS_BUSY_TIMEOUT);
    }
}

IOReturn IntelPreciseTouchStylusDriver::sendIPTSCommand(UInt32 code, UInt8 *data, UInt16 data_len, bool blocking) {
    IPTSCommand cmd;

    memset(&cmd, 0, sizeof(IPTSCommand));
    cmd.code = code;

    if (data && data_len > 0)
        memcpy(&cmd.payload, data, data_len);

    IOReturn ret = api->sendMessage(reinterpret_cast<UInt8 *>(&cmd), sizeof(cmd.code) + data_len, blocking);
    if (ret != kIOReturnSuccess && (ret != kIOReturnNoDevice || state != IPTSDeviceStateStopping))
        LOG("Error while sending: 0x%X:%d", code, ret);
    
    return ret;
}

IOReturn IntelPreciseTouchStylusDriver::sendFeedback(UInt32 buffer, bool blocking)
{
    IPTSFeedbackCommand cmd;

    memset(&cmd, 0, sizeof(IPTSFeedbackCommand));
    cmd.buffer = buffer;

    return sendIPTSCommand(IPTS_CMD_FEEDBACK, reinterpret_cast<UInt8 *>(&cmd), sizeof(IPTSFeedbackCommand), blocking);
}

IOReturn IntelPreciseTouchStylusDriver::sendSetFeatureReport(UInt8 report_id, UInt8 value)
{
    IPTSFeedbackBuffer *feedback;

    memset(tx_buffer.vaddr, 0, feedback_buffer_size);
    feedback = reinterpret_cast<IPTSFeedbackBuffer *>(tx_buffer.vaddr);

    feedback->cmd_type = IPTSFeedbackCommandTypeNone;
    feedback->data_type = IPTSFeedbackDataTypeSetFeatures;
    feedback->buffer = IPTS_HOST2ME_BUFFER;
    feedback->size = 2;
    feedback->payload[0] = report_id;
    feedback->payload[1] = value;

    return sendFeedback(IPTS_HOST2ME_BUFFER);
}

IOReturn IntelPreciseTouchStylusDriver::startDevice()
{
    if (state != IPTSDeviceStateStopped)
        return kIOReturnBusy;
    
    state = IPTSDeviceStateStarting;
    restart = false;
    
    return sendIPTSCommand(IPTS_CMD_GET_DEVICE_INFO, nullptr, 0);
}

IOReturn IntelPreciseTouchStylusDriver::stopDevice()
{
    if (state == IPTSDeviceStateStopping || state == IPTSDeviceStateStopped)
        return kIOReturnBusy;
    
    state = IPTSDeviceStateStopping;
    freeDMAResources();
    IOReturn ret = sendFeedback(0);
    if (ret == kIOReturnNoDevice)
        state = IPTSDeviceStateStopped;
    
    return ret;
}

IOReturn IntelPreciseTouchStylusDriver::restartDevice()
{
    if (restart)
        return kIOReturnBusy;
    restart = true;
    
    return stopDevice();
}

void IntelPreciseTouchStylusDriver::handleMessage(SurfaceManagementEngineClient *sender, UInt8 *msg, UInt16 msg_len) {    
    IPTSResponse *rsp = reinterpret_cast<IPTSResponse *>(msg);
    if (isResponseError(rsp))
        return;
    
    IOReturn ret = kIOReturnSuccess;
    switch (rsp->code) {
        case IPTS_RSP_GET_DEVICE_INFO: {
            IPTSGetDeviceInfoResponse device_info;
            memcpy(&device_info, rsp->payload, sizeof(IPTSGetDeviceInfoResponse));
            touch_screen->vendor_id = device_info.vendor_id;
            touch_screen->device_id = device_info.device_id;
            touch_screen->version = device_info.hw_rev;
            touch_screen->max_contacts = device_info.max_contacts;
            data_buffer_size = device_info.data_size;
            feedback_buffer_size = device_info.feedback_size;
            mode = device_info.mode;
            
            if (allocateDMAResources() != kIOReturnSuccess) {
                LOG("Failed to allocate resources");
                ret = kIOReturnNoMemory;
                break;
            }
            
//            IPTSGetReportDescriptor get_desc;
//            memset(&get_desc, 0, sizeof(IPTSGetReportDescriptor));
//            memset(report_desc_buffer.vaddr, 0, IPTS_REPORT_DESC_BUFFER_SIZE);
//            get_desc.addr_lower = report_desc_buffer.paddr & 0xffffffff;
//            get_desc.addr_upper = report_desc_buffer.paddr >> 32;
//            get_desc.padding_len = IPTS_REPORT_DESC_PADDING;
//            ret = sendIPTSCommand(IPTS_CMD_GET_REPORT_DESC, reinterpret_cast<UInt8 *>(&get_desc), sizeof(IPTSGetReportDescriptor));
//            break;
//        }
//        case IPTS_RSP_GET_REPORT_DESC: {
//            IPTSDataHeader header;
//            memcpy(&header, report_desc_buffer.vaddr, sizeof(IPTSDataHeader));
//            UInt16 desc_len = header.size - IPTS_REPORT_DESC_PADDING;
//            touch_screen->hid_desc = IOBufferMemoryDescriptor::withBytes((UInt8 *)report_desc_buffer.vaddr + sizeof(IPTSDataHeader) + IPTS_REPORT_DESC_PADDING, desc_len, kIODirectionNone);
            
            IPTSSetModeCommand set_mode_cmd;
            memset(&set_mode_cmd, 0, sizeof(IPTSSetModeCommand));
            set_mode_cmd.mode = IPTSMultitouch;
            ret = sendIPTSCommand(IPTS_CMD_SET_MODE, reinterpret_cast<UInt8 *>(&set_mode_cmd), sizeof(IPTSSetModeCommand));
            break;
        }
        case IPTS_RSP_SET_MODE: {
            mode = IPTSMultitouch;
            
            IPTSSetMemoryWindowCommand set_mem_cmd;
            memset(&set_mem_cmd, 0, sizeof(IPTSSetMemoryWindowCommand));
            for (int i = 0; i < IPTS_BUFFER_NUM; i++) {
                set_mem_cmd.data_buffer_addr_lower[i] = rx_buffer[i].paddr & 0xffffffff;
                set_mem_cmd.data_buffer_addr_upper[i] = rx_buffer[i].paddr >> 32;

                set_mem_cmd.feedback_buffer_addr_lower[i] = feedback_buffer[i].paddr & 0xffffffff;
                set_mem_cmd.feedback_buffer_addr_upper[i] = feedback_buffer[i].paddr >> 32;
            }
            set_mem_cmd.workqueue_addr_lower = workqueue_buffer.paddr & 0xffffffff;
            set_mem_cmd.workqueue_addr_upper = workqueue_buffer.paddr >> 32;

            set_mem_cmd.doorbell_addr_lower = doorbell_buffer.paddr & 0xffffffff;
            set_mem_cmd.doorbell_addr_upper = doorbell_buffer.paddr >> 32;

            set_mem_cmd.host2me_addr_lower = tx_buffer.paddr & 0xffffffff;
            set_mem_cmd.host2me_addr_upper = tx_buffer.paddr >> 32;

            set_mem_cmd.workqueue_size = IPTS_WORKQUEUE_SIZE;
            set_mem_cmd.workqueue_item_size = IPTS_WORKQUEUE_ITEM_SIZE;

            ret = sendIPTSCommand(IPTS_CMD_SET_MEM_WINDOW, reinterpret_cast<UInt8 *>(&set_mem_cmd), sizeof(IPTSSetMemoryWindowCommand));
            break;
        }
        case IPTS_RSP_SET_MEM_WINDOW:
            if (!touch_screen_started) {
                if (!touch_screen->start(this)) {
                    touch_screen->detach(this);
                    LOG("Could not start Surface Touch Screen device");
                    ret = kIOReturnError;
                    break;
                }
                touch_screen_started = true;
            }
            
            LOG("IPTS Device is ready");
            state = IPTSDeviceStateStarted;
            
            ret = sendIPTSCommand(IPTS_CMD_READY_FOR_DATA, nullptr, 0);
            if (ret != kIOReturnSuccess)
                break;
            ret = sendSetFeatureReport(0x5, 0x1);   // magic command to enable multitouch on SP7
            break;
        case IPTS_RSP_FEEDBACK: {
            if (state != IPTSDeviceStateStopping)
                break;

            IPTSFeedbackResponse feedback;
            memcpy(&feedback, rsp->payload, sizeof(IPTSFeedbackResponse));
            if (feedback.buffer < IPTS_BUFFER_NUM - 1) {
                ret = sendFeedback(feedback.buffer + 1);
                break;
            }
            ret = sendIPTSCommand(IPTS_CMD_CLEAR_MEM_WINDOW, nullptr, 0);
            break;
        }
        case IPTS_RSP_CLEAR_MEM_WINDOW:
            state = IPTSDeviceStateStopped;
            if (restart)
                ret = startDevice();
            break;
        default:
            break;
    }
    if (ret != kIOReturnSuccess) {
        LOG("Error while handling response 0x%08x", rsp->code);
        stopDevice();
    }
}

bool IntelPreciseTouchStylusDriver::isResponseError(IPTSResponse *rsp) {
    bool error;
    switch (rsp->status) {
    case IPTSCommandSuccess:
    case IPTSCommandCompatCheckFail:
        error = false;
        break;
    case IPTSCommandInvalidParams:
        error = rsp->code != IPTS_RSP_FEEDBACK;
        break;
    case IPTSCommandSensorDisabled:
        error = state != IPTSDeviceStateStopping;
        break;
    default:
        error = true;
        break;
    }
    if (!error)
        return false;

    LOG("Command 0x%08x failed: %d", rsp->code, rsp->status);
    if (rsp->status == IPTSCommandExpectedReset || rsp->status == IPTSCommandUnexpectedReset) {
        LOG("Sensor was reset");
        if (restartDevice())
            LOG("Failed to restart IPTS");
    }
    return true;
}

IOReturn IntelPreciseTouchStylusDriver::allocateDMAMemory(IPTSBufferInfo *info, UInt32 size) {
    IOBufferMemoryDescriptor *buf = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(kernel_task, kIODirectionInOut | kIOMemoryPhysicallyContiguous | kIOMemoryKernelUserShared | kIOMapInhibitCache, size, DMA_BIT_MASK(64));
    if (!buf)
        return kIOReturnNoMemory;
    buf->prepare();
    
    IODMACommand *cmd = IODMACommand::withSpecification(kIODMACommandOutputHost64, 64, 0, IODMACommand::kMapped, 0, 4);
    if (!cmd) {
        buf->complete();
        OSSafeReleaseNULL(buf);
        return kIOReturnNoMemory;
    }
    cmd->setMemoryDescriptor(buf);

    IODMACommand::Segment64 seg;
    UInt64 offset = 0;
    UInt32 seg_num = 1;
    if (cmd->gen64IOVMSegments(&offset, &seg, &seg_num) != kIOReturnSuccess) {
        OSSafeReleaseNULL(cmd);
        buf->complete();
        OSSafeReleaseNULL(buf);
        return kIOReturnNoMemory;
    }
    info->paddr = seg.fIOVMAddr;
    info->vaddr = buf->getBytesNoCopy();
    info->buffer = buf;
    info->dma_cmd = cmd;

    return kIOReturnSuccess;
}

IOReturn IntelPreciseTouchStylusDriver::allocateDMAResources()
{
    IPTSBufferInfo *buffers = rx_buffer;
    for (int i = 0; i < IPTS_BUFFER_NUM; i++) {
        if (allocateDMAMemory(buffers+i, data_buffer_size) != kIOReturnSuccess)
            goto release_resources;
    }

    buffers = feedback_buffer;
    for (int i = 0; i < IPTS_BUFFER_NUM; i++) {
        if (allocateDMAMemory(buffers+i, feedback_buffer_size) != kIOReturnSuccess)
            goto release_resources;
    }

    if (allocateDMAMemory(&doorbell_buffer, sizeof(UInt32)) != kIOReturnSuccess ||
        allocateDMAMemory(&workqueue_buffer, sizeof(UInt32)) != kIOReturnSuccess ||
        allocateDMAMemory(&tx_buffer, feedback_buffer_size) != kIOReturnSuccess ||
        allocateDMAMemory(&report_desc_buffer, IPTS_REPORT_DESC_BUFFER_SIZE) != kIOReturnSuccess)
        goto release_resources;

    return kIOReturnSuccess;
release_resources:
    freeDMAResources();
    return kIOReturnNoMemory;
}

void IntelPreciseTouchStylusDriver::freeDMAMemory(IPTSBufferInfo *info) {
    info->dma_cmd->clearMemoryDescriptor();
    OSSafeReleaseNULL(info->dma_cmd);
    info->buffer->complete();
    OSSafeReleaseNULL(info->buffer);
    info->vaddr = nullptr;
    info->paddr = 0;
}

void IntelPreciseTouchStylusDriver::freeDMAResources()
{
    IPTSBufferInfo *buffers = rx_buffer;
    for (int i = 0; i < IPTS_BUFFER_NUM; i++) {
        if (!buffers[i].vaddr)
            continue;
        freeDMAMemory(buffers+i);
    }
    buffers = feedback_buffer;
    for (int i = 0; i < IPTS_BUFFER_NUM; i++) {
        if (!buffers[i].vaddr)
            continue;
        freeDMAMemory(buffers+i);
    }

    if (doorbell_buffer.vaddr)
        freeDMAMemory(&doorbell_buffer);
    if (workqueue_buffer.vaddr)
        freeDMAMemory(&workqueue_buffer);
    if (tx_buffer.vaddr)
        freeDMAMemory(&tx_buffer);
    if (report_desc_buffer.vaddr)
        freeDMAMemory(&report_desc_buffer);
}
