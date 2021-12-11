//
//  SurfaceSerialHubDriver.cpp
//  BigSurface
//
//  Created by Xavier on 2021/10/29.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#include "SurfaceSerialHubDriver.hpp"
#include "SerialProtocol.h"
#include "../../../Dependencies/VoodooSerial/VoodooSerial/utils/VoodooACPIResourcesParser/VoodooACPIResourcesParser.hpp"

OSDefineMetaClassAndAbstractStructors(SurfaceSerialHubClient, IOService);

#define super VoodooUARTClient
OSDefineMetaClassAndStructors(SurfaceSerialHubDriver, VoodooUARTClient);

void err_dump(const char *name, const char *str, UInt8 *buffer, UInt16 len) {
    IOLog("%s::%s\n", name, str);
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

int find_sync_bytes(UInt8 *buffer, UInt16 len) {
    for (int i=0; i<len-1; i++) {
        if (buffer[i] == SYN_BYTE_1 && buffer[i+1] == SYN_BYTE_2) {
            // We try our best to determine if it is the real syn bytes... Let's hope we are not that unlucky
            if (i < len-2 && buffer[i+2]!=FRAME_TYPE_NAK && buffer[i+2]!=FRAME_TYPE_ACK && buffer[i+2]!=FRAME_TYPE_DATA_SEQ && buffer[i+2]!=FRAME_TYPE_DATA_NSQ)
                continue;
            if (i < len-4 && buffer[i+2]==FRAME_TYPE_DATA_NSQ && buffer[i+3]<0x08 && buffer[i+4]==0x00) // minimum length required for data transfer is 0x08=sizeof(SurfaceSerialFrame)
                continue;
            return i;
        }
    }
    return -1;
}

void SurfaceSerialHubDriver::dataReceived(UInt8 *buffer, UInt16 length) {
    memcpy(rx_buffer, buffer, length);
    rx_buffer_len = length;
    interrupt_source->interruptOccurred(nullptr, this, 0);
}

void SurfaceSerialHubDriver::_process(UInt8 *buffer, UInt16 len) {
    if (len == 0)
        return;
    if (partial_syn) {
        if (buffer[0] == SYN_BYTE_2) {
            processMessage();
            _pos = 1;
            msg_len = -1;
            msg_cache[0] = SYN_BYTE_1;
        } else {
            msg_cache[_pos++] = SYN_BYTE_1;
        }
    }
    partial_syn = false;
    int end = find_sync_bytes(buffer, len);
    if (end != -1) {
        if (end > 0) {
            memcpy(msg_cache+_pos, buffer, end);
            _pos += end;
        }
        if (_pos > 0) {  // has unfinished msg
            processMessage();
            buffer += end;
            len -= end;
        }
        _pos = 2;
        msg_len = -1;
        msg_cache[0] = SYN_BYTE_1;
        msg_cache[1] = SYN_BYTE_2;
        _process(buffer+2, len-2);
    } else {
        if (buffer[len-1] == SYN_BYTE_1) {
            memcpy(msg_cache+_pos, buffer, len-1);
            _pos += len-1;
            partial_syn = true;
        } else {
            memcpy(msg_cache+_pos, buffer, len);
            _pos += len;
        }
        if (_pos >= 5 && msg_len == -1)
            msg_len = reinterpret_cast<SurfaceSerialMessage *>(msg_cache)->frame.length+10;
        if (_pos == msg_len) {
            processMessage();
            if (partial_syn) {
                _pos = 1;
                msg_cache[0] = SYN_BYTE_1;
            } else {
                _pos = 0;
            }
            msg_len = -1;
            partial_syn = false;
        }
    }
}

void SurfaceSerialHubDriver::processReceivedData(OSObject* target, void* refCon, IOService* nubDevice, int source) {
    if (!awake)
        return;
    _process(rx_buffer, rx_buffer_len);
}

#define ERR_DUMP_MSG(str) err_dump(getName(), str, msg_cache, _pos)

IOReturn SurfaceSerialHubDriver::processMessage() { // error send NAK
    if (_pos < 10) {
        ERR_DUMP_MSG("Message received incomplete! Protential data loss!");
        return kIOReturnError;
    }
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(msg_cache);
    SurfaceSerialCommand *command;
    UInt16 payload_crc, crc;
    WaitingRequest t1 = {false, 0, waiting_requests};
    WaitingRequest *waiting_pos = &t1;
    PendingCommand t2 = {nullptr, 0, nullptr, 0, pending_commands};
    PendingCommand *pending_pos = &t2;
    bool found = false;
    if (msg->syn != SYN_BYTES) {
        ERR_DUMP_MSG("syn btyes error!");
        return kIOReturnError;
    }
    if (msg->frame_crc != crc_ccitt_false(CRC_INITIAL, msg_cache+2, sizeof(SurfaceSerialFrame))) {
        ERR_DUMP_MSG("frame crc error!");
        return kIOReturnError;
    }
    if (_pos != msg->frame.length+10) {
        ERR_DUMP_MSG("data length error!");
        return kIOReturnError;
    }
    switch (msg->frame.type) {
        case FRAME_TYPE_ACK:
            if (msg_cache[8]!=0xFF || msg_cache[9]!=0xFF) {
                ERR_DUMP_MSG("payload crc for ACK should be 0xFFFF!");
                return kIOReturnError;
            }
            while (pending_pos->next) {
                PendingCommand *cmd = pending_pos->next;
                SurfaceSerialMessage *pending_msg = reinterpret_cast<SurfaceSerialMessage *>(cmd->buffer);
                if (pending_msg->frame.seq_id == msg->frame.seq_id) {
                    found = true;
                    cmd->timer->disable();
                    work_loop->removeEventSource(cmd->timer);
                    OSSafeReleaseNULL(cmd->timer);
                    delete[] cmd->buffer;
                    
                    pending_pos->next = cmd->next;
                    pending_commands = t2.next;
                    delete cmd;
                    break;
                }
                pending_pos = pending_pos->next;
            }
            if (!found)
                IOLog("%s::Warning, no pending command found for seq_id %d\n", getName(), msg->frame.seq_id);
            break;
        case FRAME_TYPE_NAK:
            break;
        case FRAME_TYPE_DATA_SEQ:
            sendACK(msg->frame.seq_id);
        case FRAME_TYPE_DATA_NSQ:
            command = reinterpret_cast<SurfaceSerialCommand *>(msg_cache+sizeof(SurfaceSerialMessage));
            rx_data = msg_cache+DATA_OFFSET;
            rx_data_len = msg->frame.length - sizeof(SurfaceSerialCommand);
            payload_crc = crc_ccitt_false(CRC_INITIAL, msg_cache+sizeof(SurfaceSerialMessage), msg->frame.length);
            crc = *(reinterpret_cast<UInt16 *>(rx_data+rx_data_len));
            if (crc != payload_crc) {
                ERR_DUMP_MSG("payload crc error!");
                return kIOReturnError;
            }
            if (command->request_id >= REQID_MIN) {
                while (waiting_pos->next) {
                    if (waiting_pos->next->req_id == command->request_id) {
                        found = true;
                        command_gate->commandWakeup(&waiting_pos->next->waiting);
                        waiting_pos->next = waiting_pos->next->next;
                        waiting_requests = t1.next;
                        break;
                    }
                    waiting_pos = waiting_pos->next;
                }
                if (!found)
                    IOLog("%s::Warning, received data with unknown req_id %d\n", getName(), command->request_id);
            } else {
                if (event_handler[command->request_id]) {
                    event_handler[command->request_id]->eventReceived(command->target_category, command->instance_id, command->command_id, rx_data, rx_data_len);
                } else {
                    ERR_DUMP_MSG("Event unregistered!");
                }
            }
            break;
        default:
            ERR_DUMP_MSG("Unknown message type!");
            return kIOReturnError;
    }
    
    return kIOReturnSuccess;
}

IOReturn SurfaceSerialHubDriver::sendACK(UInt8 seq_id) {
    UInt8 buffer[10];
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(buffer);
    msg->syn = SYN_BYTES;
    msg->frame.type = FRAME_TYPE_ACK;
    msg->frame.length = 0;
    msg->frame.seq_id = seq_id;
    msg->frame_crc = crc_ccitt_false(CRC_INITIAL, buffer+2, sizeof(SurfaceSerialFrame));
    buffer[8] = 0xFF;
    buffer[9] = 0xFF;
    return uart_controller->transmitData(buffer, 10);
}

UInt16 SurfaceSerialHubDriver::sendCommand(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 size, bool seq) {
    UInt16 len = DATA_OFFSET+size+2;
    UInt8 *tx_buffer = new UInt8[len];
    
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(tx_buffer);
    msg->syn = SYN_BYTES;
    msg->frame.type = seq?FRAME_TYPE_DATA_SEQ:FRAME_TYPE_DATA_NSQ;
    msg->frame.length = size + 8;
    msg->frame.seq_id = seq_counter.getID();
    msg->frame_crc = crc_ccitt_false(CRC_INITIAL, tx_buffer+2, sizeof(SurfaceSerialFrame));
    
    SurfaceSerialCommand *cmd = reinterpret_cast<SurfaceSerialCommand *>(tx_buffer+sizeof(SurfaceSerialMessage));
    cmd->type = COMMAND_TYPE;
    cmd->target_category = tc;
    cmd->target_id_out = tid;
    cmd->target_id_in = 0x00;
    cmd->instance_id = iid;
    cmd->request_id = req_counter.getID();
    cmd->command_id = cid;
    
    if (size > 0)
        memcpy(tx_buffer+DATA_OFFSET, payload, size);
    
    *(reinterpret_cast<UInt16 *>(tx_buffer+DATA_OFFSET+size)) = crc_ccitt_false(CRC_INITIAL, tx_buffer+sizeof(SurfaceSerialMessage), sizeof(SurfaceSerialCommand)+size);
    
    if (command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceSerialHubDriver::sendCommandGated), tx_buffer, &len, &seq) != kIOReturnSuccess) {
        IOLog("%s::Sending command failed!\n", getName());
        return 0;
    }
    
    return cmd->request_id;
}

IOReturn SurfaceSerialHubDriver::sendCommandGated(UInt8 *tx_buffer, UInt16 *len, bool *seq) {
    PendingCommand *cmd = new PendingCommand;
    cmd->buffer = tx_buffer;
    cmd->len = *len;
    cmd->trial_count = 0;
    cmd->timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceSerialHubDriver::commandTimeout));
    if (!cmd->timer) {
        IOLog("%s::Could not create timer for sending command!\n", getName());
        delete cmd;
        return kIOReturnError;
    }
    cmd->next = pending_commands;
    pending_commands = cmd;
    
    work_loop->addEventSource(cmd->timer);
    cmd->timer->enable();
    cmd->timer->setTimeoutUS(1);
    return kIOReturnSuccess;
}

void SurfaceSerialHubDriver::commandTimeout(OSObject* owner, IOTimerEventSource* timer) {
    PendingCommand temp = {nullptr, 0, nullptr, 0, pending_commands};
    PendingCommand *pos = &temp;
    bool found = false;
    while (pos->next) {
        if (pos->next->timer == timer) {
            found = true;
            PendingCommand *cmd = pos->next;
            SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(cmd->buffer);
            if (cmd->trial_count >= 3) {
                IOLog("%s::receive no ACK for command %d\n", getName(), msg->frame.seq_id);
                cmd->timer->disable();
                work_loop->removeEventSource(cmd->timer);
                OSSafeReleaseNULL(cmd->timer);
                delete[] cmd->buffer;
                pos->next = cmd->next;
                pending_commands = temp.next;
                delete cmd;
            } else {
                cmd->trial_count++;
                if (uart_controller->transmitData(cmd->buffer, cmd->len) != kIOReturnSuccess) {
                    IOLog("%s::Sending command failed! Retrying\n", getName());
                }
                cmd->timer->setTimeoutMS(ACK_TIMEOUT);
            }
            break;
        }
        pos = pos->next;
    }
    if (!found)
        IOLog("%s::warning, timeout occurred but find no timer\n", getName());
}

IOReturn SurfaceSerialHubDriver::getResponse(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 size, bool seq, UInt8 *buffer, UInt16 buffer_size) {
    UInt16 req_id = sendCommand(tc, tid, iid, cid, payload, size, seq);
    
    if (req_id != 0)
        return command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceSerialHubDriver::waitResponse), &req_id, buffer, &buffer_size);
    return kIOReturnError;
}

IOReturn SurfaceSerialHubDriver::waitResponse(UInt16 *req_id, UInt8 *buffer, UInt16 *buffer_size) {
    AbsoluteTime abstime, deadline;
    IOReturn sleep;
    
    WaitingRequest *w = new WaitingRequest;
    w->waiting = false;
    w->req_id = *req_id;
    w->next = waiting_requests;
    waiting_requests = w;
    
    nanoseconds_to_absolutetime(500000000, &abstime); // 500ms
    clock_absolutetime_interval_to_deadline(abstime, &deadline);
    sleep = command_gate->commandSleep(&w->waiting, deadline, THREAD_INTERRUPTIBLE);
    
    if (sleep == THREAD_TIMED_OUT) {
        IOLog("%s::Timeout waiting for response\n", getName());
        WaitingRequest temp = {false, 0, waiting_requests};
        WaitingRequest *list_pos = &temp;
        while (list_pos->next) {
            if (list_pos->next == w) {
                IOLog("%s::Droping waiting request anyway\n", getName());
                list_pos->next = list_pos->next->next;
                waiting_requests = temp.next;
                break;
            }
            list_pos = list_pos->next;
        }
        delete w;
        return kIOReturnTimeout;
    }
    delete w;
    UInt16 len = rx_data_len;
    if (*buffer_size < rx_data_len)
        len = *buffer_size;
    if (*buffer_size != rx_data_len)
        IOLog("%s::Warning, buffer size(%d) and rx_data_len(%d) mismatched!\n", getName(), *buffer_size, rx_data_len);
    memcpy(buffer, rx_data, len);
    return kIOReturnSuccess;
}

IOReturn SurfaceSerialHubDriver::registerEvent(UInt8 tc, UInt8 iid, SurfaceSerialHubClient *client) {
    if (!client)
        return kIOReturnError;
    if (event_handler[tc]) {
        IOLog("%s::Already has an event handler registered!\n", getName());
        return kIOReturnNoResources;
    }
    SurfaceEventPayload payload;
    UInt8 ret;
    payload.target_category = tc;
    payload.instance_id = iid;
    payload.request_id = tc;
    payload.flags = EVENT_FLAG_SEQUENCED;
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_ENABLE_EVENT, reinterpret_cast<UInt8 *>(&payload), sizeof(SurfaceEventPayload), true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        IOLog("%s::Unexpected response from event-enable request\n", getName());
        return kIOReturnError;
    }
    event_handler[tc] = client;
    return kIOReturnSuccess;
}

void SurfaceSerialHubDriver::unregisterEvent(UInt8 tc, UInt8 iid, SurfaceSerialHubClient *client) {
    if (event_handler[tc] && event_handler[tc] == client) {
        SurfaceEventPayload payload;
        UInt8 ret;
        payload.target_category = tc;
        payload.instance_id = iid;
        payload.request_id = tc;
        payload.flags = EVENT_FLAG_SEQUENCED;
        if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISABLE_EVENT, reinterpret_cast<UInt8 *>(&payload), sizeof(SurfaceEventPayload), true, &ret, 1) != kIOReturnSuccess || ret != 0) {
            IOLog("%s::Unexpected response from event-disable request\n", getName());
            return;
        }
        event_handler[tc] = nullptr;
    } else {
        IOLog("%s::Event not registered yet!\n", getName());
    }
}

IOService* SurfaceSerialHubDriver::probe(IOService *provider, SInt32 *score) {
    if (!super::probe(provider, score))
        return nullptr;
    
    acpi_device = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!acpi_device)
        return nullptr;
    
    if (getDeviceResources() != kIOReturnSuccess) {
        IOLog("%s::Could not parse ACPI resources!\n", getName());
        return nullptr;
    }
        
    IOLog("%s::Surface Serial Hub found!\n", getName());

    return this;
}

bool SurfaceSerialHubDriver::start(IOService *provider) {
    UInt8 sam_version[4];
    UInt8 ret;
    UInt32 version;
    if (!super::start(provider))
        return false;
    
    uart_controller = getUARTController();
    if (!uart_controller) {
        IOLog("%s::Could not find UART controller, exiting\n", getName());
        return false;
    }
    // Give the UART controller some time to load
    IOSleep(100);
    
    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        IOLog("%s::Could not get work loop\n", getName());
        goto exit;
    }

    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        IOLog("%s::Could not open command gate\n", getName());
        goto exit;
    }
    
    interrupt_source = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceSerialHubDriver::processReceivedData));
    if (!interrupt_source || (work_loop->addEventSource(interrupt_source)!=kIOReturnSuccess)) {
        IOLog("%s::Could not register interrupt!\n", getName());
        goto exit;
    }
    interrupt_source->enable();
    
    rx_buffer = new UInt8[fifo_size];
    rx_buffer_len = 0;
    
    memset(event_handler, 0, sizeof(event_handler));
    
    PMinit();
    uart_controller->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);

    if (uart_controller->requestConnect(this, baudrate, data_bits, stop_bits, parity) != kIOReturnSuccess) {
        IOLog("%s::UARTController already occupied!\n", getName());
        goto exit;
    }
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_VERSION, nullptr, 0, true, sam_version, 4) != kIOReturnSuccess) {
        IOLog("%s::Failed to get SAM version from UART!\n", getName());
        goto exit;
    }
    version = *(reinterpret_cast<UInt32 *>(sam_version));
    IOLog("%s::SAM version %u.%u.%u\n", getName(), (version >> 24) & 0xff, (version >> 8) & 0xffff, version & 0xff);
    
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_D0_ENTRY, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        IOLog("%s::Unexpected response from D0-entry notification\n", getName());
        goto exit;
    }
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISPLAY_ON, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        IOLog("%s::Unexpected response from display-on notification\n", getName());
        goto exit;
    }
    
    registerService();
    return true;
exit:
    releaseResources();
    return false;
}

void SurfaceSerialHubDriver::stop(IOService *provider) {
    IOLog("%s::stopped\n", getName());
    if (uart_controller) {
        UInt8 ret;
        if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISPLAY_OFF, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
            IOLog("%s::Unexpected response from display-off notification\n", getName());
        }
        if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_D0_EXIT, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
            IOLog("%s::Unexpected response from d0-exit notification\n", getName());
        }
        uart_controller->requestDisconnect(this);
    }
    PMstop();
    releaseResources();
    super::stop(provider);
}

IOReturn SurfaceSerialHubDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            interrupt_source->disable();
            awake = false;
            IOLog("%s::Going to sleep\n", getName());
        }
    } else {
        if (!awake) {
            awake = true;
            interrupt_source->enable();
            IOLog("%s::Woke up\n", getName());
        }
    }
    return kIOPMAckImplied;
}

void SurfaceSerialHubDriver::releaseResources() {
    if (waiting_requests) {
        IOLog("%s::There are still commands waiting for response!\n", getName());
        while (waiting_requests) {
            WaitingRequest *t = waiting_requests;
            waiting_requests = waiting_requests->next;
            delete t;
        }
    }
    if (pending_commands){
        IOLog("%s::There are still pending commands!\n", getName());
        while (pending_commands) {
            PendingCommand *t = pending_commands;
            pending_commands = pending_commands->next;
            delete[] t->buffer;
            t->timer->disable();
            work_loop->removeEventSource(t->timer);
            OSSafeReleaseNULL(t->timer);
            delete t;
        }
    }
    if (rx_buffer)
        delete[] rx_buffer;
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
}

VoodooUARTController* SurfaceSerialHubDriver::getUARTController() {
    VoodooUARTController* uart = nullptr;

    // Wait for UART controller, up to 2 second
    OSDictionary* name_match = IOService::serviceMatching("VoodooUARTController");
    IOService* matched = waitForMatchingService(name_match, 2000000000);
    uart = OSDynamicCast(VoodooUARTController, matched);

    if (uart) {
        IOLog("%s::Got UART Controller! %s\n", getName(), uart->getName());
    }
    name_match->release();
    OSSafeReleaseNULL(matched);
    return uart;
}

IOReturn SurfaceSerialHubDriver::getDeviceResources() {
    VoodooACPIResourcesParser parser;
    OSObject *result = nullptr;
    OSData *data = nullptr;
    if (acpi_device->evaluateObject("_CRS", &result) != kIOReturnSuccess ||
        !(data = OSDynamicCast(OSData, result))) {
        IOLog("%s::Could not find or evaluate _CRS method\n", getName());
        OSSafeReleaseNULL(result);
        return kIOReturnNotFound;
    }

    UInt8 const* crs = reinterpret_cast<UInt8 const*>(data->getBytesNoCopy());
    unsigned int length = data->getLength();
    parser.parseACPIResources(crs, 0, length);
    
    OSSafeReleaseNULL(data);
    
    if (parser.found_uart) {
        IOLog("%s::Found valid UART Bus!\n", getName());
//        IOLog("%s::resource_consumer %d\n", getName(), parser.uart_info.resource_consumer);
//        IOLog("%s::device_initiated %d\n", getName(), parser.uart_info.device_initiated);
//        IOLog("%s::flow_control %d\n", getName(), parser.uart_info.flow_control);
//        IOLog("%s::stop_bits %d\n", getName(), parser.uart_info.stop_bits);
//        IOLog("%s::data_bits %d\n", getName(), parser.uart_info.data_bits);
//        IOLog("%s::big_endian %d\n", getName(), parser.uart_info.big_endian);
//        IOLog("%s::baudrate %u\n", getName(), parser.uart_info.baudrate);
//        IOLog("%s::rx_fifo %d\n", getName(), parser.uart_info.rx_fifo);
//        IOLog("%s::tx_fifo %d\n", getName(), parser.uart_info.tx_fifo);
//        IOLog("%s::parity %d\n", getName(), parser.uart_info.parity);
//        IOLog("%s::dtd_enabled %d\n", getName(), parser.uart_info.dtd_enabled);
//        IOLog("%s::ri_enabled %d\n", getName(), parser.uart_info.ri_enabled);
//        IOLog("%s::dsr_enabled %d\n", getName(), parser.uart_info.dsr_enabled);
//        IOLog("%s::dtr_enabled %d\n", getName(), parser.uart_info.dtr_enabled);
//        IOLog("%s::cts_enabled %d\n", getName(), parser.uart_info.cts_enabled);
//        IOLog("%s::rts_enabled %d\n", getName(), parser.uart_info.rts_enabled);
        
        baudrate = parser.uart_info.baudrate;
        data_bits = parser.uart_info.data_bits;
        stop_bits = parser.uart_info.stop_bits;
        parity = parser.uart_info.parity;
        fifo_size = parser.uart_info.rx_fifo;
        return kIOReturnSuccess;
    }
    return kIOReturnNoResources;
}
