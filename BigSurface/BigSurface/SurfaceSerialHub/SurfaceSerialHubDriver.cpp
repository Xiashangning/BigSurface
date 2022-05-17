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

#define LOG(str, ...)    IOLog("%s::" str "\n", getName(), ##__VA_ARGS__)

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

IOReturn SurfaceSerialHubDriver::processMessage() {
    if (_pos < 10) {
        sendNAK();
        ERR_DUMP_MSG("Message received incomplete! Protential data loss!");
        return kIOReturnError;
    }
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(msg_cache);
    SurfaceSerialCommand *command;
    UInt16 payload_crc, crc;
    WaitingRequest *waiting_pos = &waiting_requests;
    PendingCommand *pending_pos = &pending_commands;
    bool found = false;
    if (msg->syn != SYN_BYTES) {
        sendNAK();
        ERR_DUMP_MSG("syn btyes error!");
        return kIOReturnError;
    }
    if (msg->frame_crc != crc_ccitt_false(CRC_INITIAL, msg_cache+2, sizeof(SurfaceSerialFrame))) {
        sendNAK();
        ERR_DUMP_MSG("frame crc error!");
        return kIOReturnError;
    }
    if (_pos != msg->frame.length+10) {
        sendNAK();
        ERR_DUMP_MSG("data length error!");
        return kIOReturnError;
    }
    switch (msg->frame.type) {
        case FRAME_TYPE_ACK:
            if (msg_cache[8]!=0xFF || msg_cache[9]!=0xFF) {
                sendNAK();
                ERR_DUMP_MSG("payload crc for ACK should be 0xFFFF!");
                return kIOReturnError;
            }
            while (pending_pos->next) {
                PendingCommand *cmd = pending_pos->next;
                SurfaceSerialMessage *pending_msg = reinterpret_cast<SurfaceSerialMessage *>(cmd->buffer);
                if (pending_msg->frame.seq_id == msg->frame.seq_id) {
                    found = true;
                    cmd->timer->cancelTimeout();
                    cmd->timer->disable();
                    work_loop->removeEventSource(cmd->timer);
                    OSSafeReleaseNULL(cmd->timer);
                    delete[] cmd->buffer;
                    
                    pending_pos->next = cmd->next;
                    if (!pending_pos->next)
                        last_pending = pending_pos;
                    delete cmd;
                    break;
                }
                pending_pos = pending_pos->next;
            }
            if (!found)
                LOG("Warning, no pending command found for seq_id %d", msg->frame.seq_id);
            break;
        case FRAME_TYPE_NAK:
            LOG("Warning, NAK received! Resending all pending messages!");
            while (pending_pos->next) {
                PendingCommand *cmd = pending_pos->next;
                cmd->trial_count = 0;
                cmd->timer->cancelTimeout();
                cmd->timer->setTimeoutMS(1);
                pending_pos = pending_pos->next;
            }
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
                sendNAK();
                ERR_DUMP_MSG("payload crc error!");
                return kIOReturnError;
            }
            if (command->request_id >= REQID_MIN) {
                while (waiting_pos->next) {
                    WaitingRequest *req = waiting_pos->next;
                    if (req->req_id == command->request_id) {
                        found = true;
                        command_gate->commandWakeup(&req->waiting);
                        waiting_pos->next = req->next;
                        if (!waiting_pos->next)
                            last_waiting = waiting_pos;
                        break;
                    }
                    waiting_pos = waiting_pos->next;
                }
                // sometimes due to delay in sending back ACK, there are duplicated responses, we ignore them here.
//                if (!found)
//                    LOG("Warning, received data with unknown tc %x, cid %x", command->target_category, command->command_id);
            } else {
                if (event_handler[command->request_id]) {
                    event_handler[command->request_id]->eventReceived(command->target_category, command->target_id_in, command->instance_id, command->command_id, rx_data, rx_data_len);
                } else {
                    ERR_DUMP_MSG("Event unregistered!");
                }
            }
            break;
        default:
            sendNAK();
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

IOReturn SurfaceSerialHubDriver::sendNAK() {
    UInt8 buffer[10];
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(buffer);
    msg->syn = SYN_BYTES;
    msg->frame.type = FRAME_TYPE_NAK;
    msg->frame.length = 0;
    msg->frame.seq_id = 0;
    msg->frame_crc = crc_ccitt_false(CRC_INITIAL, buffer+2, sizeof(SurfaceSerialFrame));
    buffer[8] = 0xFF;
    buffer[9] = 0xFF;
    return uart_controller->transmitData(buffer, 10);
}

UInt16 SurfaceSerialHubDriver::sendCommand(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 payload_len, bool seq) {
    UInt16 len = DATA_OFFSET+payload_len+2;
    UInt8 *tx_buffer = new UInt8[len];
    
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(tx_buffer);
    msg->syn = SYN_BYTES;
    msg->frame.type = seq?FRAME_TYPE_DATA_SEQ:FRAME_TYPE_DATA_NSQ;
    msg->frame.length = payload_len + 8;
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
    
    if (payload_len > 0)
        memcpy(tx_buffer+DATA_OFFSET, payload, payload_len);
    
    *(reinterpret_cast<UInt16 *>(tx_buffer+DATA_OFFSET+payload_len)) = crc_ccitt_false(CRC_INITIAL, tx_buffer+sizeof(SurfaceSerialMessage), sizeof(SurfaceSerialCommand)+payload_len);
    
    if (command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceSerialHubDriver::sendCommandGated), tx_buffer, &len, &seq) != kIOReturnSuccess) {
        LOG("Sending command failed!");
        return 0;
    }
    
    return cmd->request_id;
}

IOReturn SurfaceSerialHubDriver::sendCommandGated(UInt8 *tx_buffer, UInt16 *len, bool *seq) {
    PendingCommand *cmd = new PendingCommand;
    cmd->buffer = tx_buffer;
    cmd->len = *len;
    cmd->trial_count = *seq ? 1 : 0;   // if no ACK is needed, count is set to 0
    cmd->timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &SurfaceSerialHubDriver::commandTimeout));
    cmd->next = nullptr;
    if (!cmd->timer) {
        LOG("Could not create timer for sending command!");
        delete cmd;
        return kIOReturnError;
    }
    last_pending->next = cmd;
    last_pending = cmd;
    
    work_loop->addEventSource(cmd->timer);
    cmd->timer->enable();
    cmd->timer->setTimeoutUS(1);
    return kIOReturnSuccess;
}

void SurfaceSerialHubDriver::commandTimeout(OSObject* owner, IOTimerEventSource* timer) {
    PendingCommand *pos = &pending_commands;
    bool found = false;
    while (pos->next) {
        if (pos->next->timer == timer) {
            found = true;
            PendingCommand *cmd = pos->next;
            SurfaceSerialCommand *cmd_data = reinterpret_cast<SurfaceSerialCommand *>(cmd->buffer+sizeof(SurfaceSerialMessage));
            if (cmd->trial_count == 0) {
                if (uart_controller->transmitData(cmd->buffer, cmd->len) != kIOReturnSuccess)
                    LOG("Sending NSQ command failed for tc %x, tid %x, cid %x, iid %x!", cmd_data->target_category, cmd_data->target_id_out, cmd_data->command_id, cmd_data->instance_id);
            } else if (cmd->trial_count > 3) {
                LOG("Receive no ACK for command tc %x, tid %x, cid %x, iid %x!", cmd_data->target_category, cmd_data->target_id_out, cmd_data->command_id, cmd_data->instance_id);
            } else {
                if (cmd->trial_count > 1)
                    LOG("Timeout, trial count: %d", cmd->trial_count);
                if (uart_controller->transmitData(cmd->buffer, cmd->len) != kIOReturnSuccess)
                    LOG("Sending SEQ command failed for tc %x, tid %x, cid %x, iid %x!", cmd_data->target_category, cmd_data->target_id_out, cmd_data->command_id, cmd_data->instance_id);
                cmd->trial_count++;
                cmd->timer->setTimeoutMS(ACK_TIMEOUT);
                break;
            }
            cmd->timer->disable();
            work_loop->removeEventSource(cmd->timer);
            OSSafeReleaseNULL(cmd->timer);
            delete[] cmd->buffer;
            pos->next = cmd->next;
            if (!pos->next)
                last_pending = pos;
            delete cmd;
            break;
        }
        pos = pos->next;
    }
    if (!found)
        LOG("Warning, timeout occurred but find no timer");
}

IOReturn SurfaceSerialHubDriver::getResponse(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 payload_len, bool seq, UInt8 *buffer, UInt16 buffer_len) {
    UInt16 req_id = sendCommand(tc, tid, iid, cid, payload, payload_len, seq);
    
    if (req_id != 0)
        return command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceSerialHubDriver::waitResponse), &req_id, buffer, &buffer_len);
    return kIOReturnError;
}

IOReturn SurfaceSerialHubDriver::waitResponse(UInt16 *req_id, UInt8 *buffer, UInt16 *buffer_len) {
    AbsoluteTime abstime, deadline;
    IOReturn sleep;
    
    WaitingRequest *w = new WaitingRequest;
    w->waiting = false;
    w->req_id = *req_id;
    w->next = nullptr;
    last_waiting->next = w;
    last_waiting = w;
    
    nanoseconds_to_absolutetime(500000000, &abstime); // 500ms
    clock_absolutetime_interval_to_deadline(abstime, &deadline);
    sleep = command_gate->commandSleep(&w->waiting, deadline, THREAD_INTERRUPTIBLE);
    
    if (sleep == THREAD_TIMED_OUT) {
        LOG("Timeout waiting for response");
        WaitingRequest *pos = &waiting_requests;
        while (pos->next) {
            if (pos->next == w) {
                LOG("Droping waiting request anyway");
                pos->next = pos->next->next;
                if (!pos->next)
                    last_waiting = pos;
                break;
            }
            pos = pos->next;
        }
        delete w;
        return kIOReturnTimeout;
    }
    delete w;
    UInt16 len = rx_data_len;
    if (*buffer_len < rx_data_len)
        len = *buffer_len;
    if (*buffer_len != rx_data_len)
        LOG("Warning, buffer size(%d) and rx_data_len(%d) mismatched!", *buffer_len, rx_data_len);
    memcpy(buffer, rx_data, len);
    return kIOReturnSuccess;
}

IOReturn SurfaceSerialHubDriver::registerEvent(UInt8 tc, UInt8 iid, SurfaceSerialHubClient *client) {
    if (!client)
        return kIOReturnError;
    if (event_handler[tc]) {
        LOG("Already has an event handler registered!");
        return kIOReturnNoResources;
    }
    SurfaceEventPayload payload;
    UInt8 ret;
    payload.target_category = tc;
    payload.instance_id = iid;
    payload.request_id = tc;
    payload.flags = EVENT_FLAG_SEQUENCED;
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_ENABLE_EVENT, reinterpret_cast<UInt8 *>(&payload), sizeof(SurfaceEventPayload), true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        LOG("Unexpected response from event-enable request");
        return kIOReturnError;
    }
    event_handler[tc] = client;
    return kIOReturnSuccess;
}

void SurfaceSerialHubDriver::unregisterEvent(UInt8 tc, UInt8 iid, SurfaceSerialHubClient *client) {
    if (event_handler[tc]) {
        if (event_handler[tc] == client) {
            SurfaceEventPayload payload;
            UInt8 ret;
            payload.target_category = tc;
            payload.instance_id = iid;
            payload.request_id = tc;
            payload.flags = EVENT_FLAG_SEQUENCED;
            if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISABLE_EVENT, reinterpret_cast<UInt8 *>(&payload), sizeof(SurfaceEventPayload), true, &ret, 1) != kIOReturnSuccess || ret != 0) {
                LOG("Unexpected response from event-disable request");
                return;
            }
            event_handler[tc] = nullptr;
        } else {
            LOG("Event not registered for this client!");
        }
    }
}

bool SurfaceSerialHubDriver::init(OSDictionary *properties) {
    if (!super::init(properties))
        return false;
    
    pending_commands = {nullptr, 0, nullptr, 0, nullptr};
    waiting_requests = {false, 0, nullptr};
    last_pending = &pending_commands;
    last_waiting = &waiting_requests;
    memset(event_handler, 0, sizeof(event_handler));
    memset(msg_cache, 0, sizeof(msg_cache));
    return true;
}

IOService* SurfaceSerialHubDriver::probe(IOService *provider, SInt32 *score) {
    if (!super::probe(provider, score))
        return nullptr;
    
    acpi_device = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (!acpi_device)
        return nullptr;
    
    if (getDeviceResources() != kIOReturnSuccess) {
        LOG("Could not parse ACPI resources!");
        return nullptr;
    }
    
    uart_controller = getUARTController();
    if (!uart_controller) {
        LOG("Could not find UART controller, exiting");
        return nullptr;
    }
    // Give the UART controller some time to load
    IOSleep(100);
        
    LOG("Surface Serial Hub found!");
    return this;
}

bool SurfaceSerialHubDriver::start(IOService *provider) {
    UInt32 version;
    if (!super::start(provider))
        return false;
    
    work_loop = IOWorkLoop::workLoop();
    if (!work_loop) {
        LOG("Could not get work loop");
        goto exit;
    }

    command_gate = IOCommandGate::commandGate(this);
    if (!command_gate || (work_loop->addEventSource(command_gate) != kIOReturnSuccess)) {
        LOG("Could not open command gate");
        goto exit;
    }
    
    interrupt_source = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceSerialHubDriver::processReceivedData));
    if (!interrupt_source || (work_loop->addEventSource(interrupt_source)!=kIOReturnSuccess)) {
        LOG("Could not register interrupt!");
        goto exit;
    }
    
    rx_buffer = new UInt8[fifo_size];
    rx_buffer_len = 0;
    
    if (uart_controller->requestConnect(this, baudrate, data_bits, stop_bits, parity) != kIOReturnSuccess) {
        LOG("UARTController already occupied!");
        goto exit;
    }
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_VERSION, nullptr, 0, true, reinterpret_cast<UInt8 *>(&version), 4) != kIOReturnSuccess) {
        LOG("Failed to get SAM version from UART!");
        goto exit;
    }
    LOG("SAM version %u.%u.%u", (version >> 24) & 0xff, (version >> 8) & 0xffff, version & 0xff);
    
    PMinit();
    uart_controller->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    return true;
exit:
    releaseResources();
    return false;
}

void SurfaceSerialHubDriver::stop(IOService *provider) {
    UInt8 ret;
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISPLAY_OFF, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        LOG("Unexpected response from display-off notification");
    }
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_D0_EXIT, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        LOG("Unexpected response from d0-exit notification");
    }
    uart_controller->requestDisconnect(this);
    
    PMstop();
    releaseResources();
    super::stop(provider);
}

void SurfaceSerialHubDriver::free() {
    super::free();
}

IOReturn SurfaceSerialHubDriver::setPowerState(unsigned long whichState, IOService *whatDevice) {
    if (whatDevice != this)
        return kIOReturnInvalid;
    if (whichState == 0) {
        if (awake) {
            UInt8 ret;
            if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISPLAY_OFF, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
                LOG("Unexpected response from display-off notification");
            }
            if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_D0_EXIT, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
                LOG("Unexpected response from d0-exit notification");
            }
            awake = false;
            interrupt_source->disable();
            LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            interrupt_source->enable();
            UInt8 ret;
            if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_D0_ENTRY, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
                LOG("Unexpected response from D0-entry notification");
            }
            if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISPLAY_ON, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
                LOG("Unexpected response from display-on notification");
            }
            LOG("Woke up");
        }
    }
    return kIOPMAckImplied;
}

void SurfaceSerialHubDriver::releaseResources() {
    WaitingRequest *waiting_pos = waiting_requests.next;
    if (waiting_pos) {
        LOG("There are still commands waiting for response!");
        while (waiting_pos) {
            WaitingRequest *t = waiting_pos;
            waiting_pos = waiting_pos->next;
            delete t;
        }
    }
    PendingCommand *pending_pos = pending_commands.next;
    if (pending_pos){
        LOG("There are still pending commands!");
        while (pending_pos) {
            PendingCommand *t = pending_pos;
            pending_pos = pending_pos->next;
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
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    OSSafeReleaseNULL(work_loop);
}

VoodooUARTController* SurfaceSerialHubDriver::getUARTController() {
    VoodooUARTController* uart = nullptr;

    // Wait for UART controller, up to 5 second (sometimes it takes really a long time for UART to load)
    OSDictionary* name_match = serviceMatching("VoodooUARTController");
    IOService* matched = waitForMatchingService(name_match, 5000000000);
    uart = OSDynamicCast(VoodooUARTController, matched);

    if (uart) {
        LOG("Got UART Controller! %s", uart->getName());
    }
    OSSafeReleaseNULL(name_match);
    OSSafeReleaseNULL(matched);
    return uart;
}

IOReturn SurfaceSerialHubDriver::getDeviceResources() {
    VoodooACPIResourcesParser parser;
    OSObject *result = nullptr;
    OSData *data = nullptr;
    if (acpi_device->evaluateObject("_CRS", &result) != kIOReturnSuccess ||
        !(data = OSDynamicCast(OSData, result))) {
        LOG("Could not find or evaluate _CRS method");
        OSSafeReleaseNULL(result);
        return kIOReturnNotFound;
    }

    UInt8 const* crs = reinterpret_cast<UInt8 const*>(data->getBytesNoCopy());
    unsigned int length = data->getLength();
    parser.parseACPIResources(crs, 0, length);
    
    OSSafeReleaseNULL(data);
    
    if (parser.found_uart) {
        LOG("Found valid UART Bus!");
//        LOG("resource_consumer %d", parser.uart_info.resource_consumer);
//        LOG("device_initiated %d", parser.uart_info.device_initiated);
//        LOG("flow_control %d", parser.uart_info.flow_control);
//        LOG("stop_bits %d", parser.uart_info.stop_bits);
//        LOG("data_bits %d", parser.uart_info.data_bits);
//        LOG("big_endian %d", parser.uart_info.big_endian);
//        LOG("baudrate %u", parser.uart_info.baudrate);
//        LOG("rx_fifo %d", parser.uart_info.rx_fifo);
//        LOG("tx_fifo %d", parser.uart_info.tx_fifo);
//        LOG("parity %d", parser.uart_info.parity);
//        LOG("dtd_enabled %d", parser.uart_info.dtd_enabled);
//        LOG("ri_enabled %d", parser.uart_info.ri_enabled);
//        LOG("dsr_enabled %d", parser.uart_info.dsr_enabled);
//        LOG("dtr_enabled %d", parser.uart_info.dtr_enabled);
//        LOG("cts_enabled %d", parser.uart_info.cts_enabled);
//        LOG("rts_enabled %d", parser.uart_info.rts_enabled);
        
        baudrate = parser.uart_info.baudrate;
        data_bits = parser.uart_info.data_bits;
        stop_bits = parser.uart_info.stop_bits;
        parity = parser.uart_info.parity;
        fifo_size = parser.uart_info.rx_fifo;
        return kIOReturnSuccess;
    }
    return kIOReturnNoResources;
}
