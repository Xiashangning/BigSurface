//
//  SurfaceSerialHubDriver.cpp
//  SurfaceSerialHub
//
//  Created by Xavier on 2021/10/29.
//  Copyright © 2021 Xia Shangning. All rights reserved.
//

#include "SurfaceSerialHubDriver.hpp"
#include "../../../Dependencies/VoodooSerial/VoodooSerial/utils/VoodooACPIResourcesParser/VoodooACPIResourcesParser.hpp"
#include "../SurfaceSerialHubDevices/SurfaceBatteryNub.hpp"
#include "../SurfaceSerialHubDevices/SurfaceLaptop3Nub.hpp"

OSDefineMetaClassAndAbstractStructors(SurfaceSerialHubClient, IOService);

#define super IOService
OSDefineMetaClassAndStructors(SurfaceSerialHubDriver, IOService);

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

// We try our best to determine if it is the real syn bytes... Let's hope we are not that unlucky
bool judge_sync(UInt8 *buffer, UInt16 len) {
    if (buffer[0] == SSH_SYN_BYTE_2) {
        if (len >= 2 && buffer[1]!=SSH_FRAME_TYPE_NAK && buffer[1]!=SSH_FRAME_TYPE_ACK && buffer[1]!=SSH_FRAME_TYPE_DATA_SEQ && buffer[1]!=SSH_FRAME_TYPE_DATA_NSQ)
            return false;
        // minimum length required for data transfer is 0x08=sizeof(SurfaceSerialFrame)
        if (len >= 4 && buffer[1]==SSH_FRAME_TYPE_DATA_NSQ && buffer[2]<sizeof(SurfaceSerialFrame) && buffer[3]==0x00)
            return false;
        return true;
    }
    return false;
}

int find_sync_bytes(UInt8 *buffer, UInt16 len) {
    for (int i=0; i<len-1; i++) {
        if (buffer[i] == SSH_SYN_BYTE_1 && judge_sync(buffer+i+1, len-i-1))
            return i;
    }
    return -1;
}

void SurfaceSerialHubDriver::bufferReceived(VoodooUARTController *sender, UInt8 *buffer, UInt16 length) {
    if (last->next == current && current->buffer_len) {
        LOG("Overrrun!");
        return;
    }
    
    CircleBuffer* buf = last->next;
    memcpy(buf->buffer, buffer, length);
    buf->buffer_len = length;
    last = last->next;
    uart_interrupt->interruptOccurred(nullptr, this, 0);
}

void SurfaceSerialHubDriver::processReceivedBuffer(IOInterruptEventSource *sender, int count) {
    while (current->buffer_len) {
        _process(current->buffer, current->buffer_len);
        current->buffer_len = 0;
        current = current->next;
    }
}

void SurfaceSerialHubDriver::_process(UInt8 *buffer, UInt16 length) {
    if (length == 0)
        return;
    if (rx_msg.partial_syn) {
        if (judge_sync(buffer, length)) {
            processMessage();
            rx_msg.pos = 1;
            rx_msg.cache[0] = SSH_SYN_BYTE_1;
        } else {
            rx_msg.cache[rx_msg.pos++] = SSH_SYN_BYTE_1;
        }
        rx_msg.partial_syn = false;
    }
    int end = find_sync_bytes(buffer, length);
    if (end != -1) {    // found sync bytes
        if (end > 0) {
            memcpy(rx_msg.cache+rx_msg.pos, buffer, end);
            rx_msg.pos += end;
        }
        if (rx_msg.pos > 0) {  // a message is completed
            processMessage();
            buffer += end;
            length -= end;
        }
        rx_msg.pos = 2;
        rx_msg.cache[0] = SSH_SYN_BYTE_1;
        rx_msg.cache[1] = SSH_SYN_BYTE_2;
        _process(buffer+2, length-2);
    } else {
        if (buffer[length-1] == SSH_SYN_BYTE_1) {
            memcpy(rx_msg.cache+rx_msg.pos, buffer, length-1);
            rx_msg.pos += length-1;
            rx_msg.partial_syn = true;
        } else {
            memcpy(rx_msg.cache+rx_msg.pos, buffer, length);
            rx_msg.pos += length;
        }
        if (rx_msg.pos >= 5 && rx_msg.len == SSH_MSG_LENGTH_UNKNOWN)
            rx_msg.len = reinterpret_cast<SurfaceSerialMessage *>(rx_msg.cache)->frame.length+10;
        if (rx_msg.pos == rx_msg.len) {  // a message is completed
            processMessage();
            if (rx_msg.partial_syn) {  // certainly another message
                rx_msg.pos = 1;
                rx_msg.cache[0] = SSH_SYN_BYTE_1;
                rx_msg.partial_syn = false;
            }
        }
    }
}

#define ERR_DUMP_MSG(str) err_dump(getName(), str, rx_msg.cache, rx_msg.pos)

IOReturn SurfaceSerialHubDriver::processMessage() {
    if (rx_msg.pos < 10) {
        sendNAK();
        ERR_DUMP_MSG("Message received incomplete! Protential data loss!");
        return kIOReturnError;
    }
    SurfaceSerialMessage *message = reinterpret_cast<SurfaceSerialMessage *>(rx_msg.cache);
    SurfaceSerialCommand *command;
    UInt8 *rx_data;
    UInt16 calc_crc, crc, rx_data_len;
    WaitingRequest *waiting_pos = &waiting_requests;
    PendingCommand *pending_pos = &pending_commands;
    bool found = false;
    if (message->syn != SSH_SYN_BYTES) {
        sendNAK();
        ERR_DUMP_MSG("syn btyes error!");
        return kIOReturnError;
    }
    if (message->frame_crc != crc_ccitt_false(CRC_INITIAL, rx_msg.cache+2, sizeof(SurfaceSerialFrame))) {
        sendNAK();
        ERR_DUMP_MSG("frame crc error!");
        return kIOReturnError;
    }
    if (rx_msg.pos != message->frame.length+10) {
        sendNAK();
        ERR_DUMP_MSG("data length error!");
        return kIOReturnError;
    }
    switch (message->frame.type) {
        case SSH_FRAME_TYPE_ACK:
            if (rx_msg.cache[SSH_PAYLOAD_OFFSET]!=0xFF || rx_msg.cache[SSH_PAYLOAD_OFFSET+1]!=0xFF) {
                sendNAK();
                ERR_DUMP_MSG("payload crc for ACK should be 0xFFFF!");
                return kIOReturnError;
            }
            while (pending_pos->next) {
                PendingCommand *cmd = pending_pos->next;
                SurfaceSerialMessage *pending_msg = reinterpret_cast<SurfaceSerialMessage *>(cmd->buffer);
                if (pending_msg->frame.seq_id == message->frame.seq_id) {
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
                LOG("Warning, no pending command found for seq_id %d", message->frame.seq_id);
            break;
        case SSH_FRAME_TYPE_NAK:
            LOG("Warning, NAK received! Resending all pending messages!");
            while (pending_pos->next) {
                PendingCommand *cmd = pending_pos->next;
                if (cmd->trial_count) {     // not a NSQ command
                    cmd->trial_count = 1;
                    cmd->timer->cancelTimeout();
                    cmd->timer->setTimeoutUS(1);
                }
                pending_pos = pending_pos->next;
            }
            break;
        case SSH_FRAME_TYPE_DATA_SEQ:
            sendACK(message->frame.seq_id);
        case SSH_FRAME_TYPE_DATA_NSQ:
            command = reinterpret_cast<SurfaceSerialCommand *>(rx_msg.cache+SSH_PAYLOAD_OFFSET);
            rx_data = rx_msg.cache + SSH_DATA_OFFSET;
            rx_data_len = message->frame.length - sizeof(SurfaceSerialCommand);
            calc_crc = crc_ccitt_false(CRC_INITIAL, rx_msg.cache+SSH_PAYLOAD_OFFSET, message->frame.length);
            crc = *(reinterpret_cast<UInt16 *>(rx_data+rx_data_len));
            if (crc != calc_crc) {
                sendNAK();
                ERR_DUMP_MSG("payload crc error!");
                return kIOReturnError;
            }
            if (command->request_id >= SSH_REQID_MIN) {
                while (waiting_pos->next) {
                    WaitingRequest *req = waiting_pos->next;
                    if (req->req_id == command->request_id) {
                        found = true;
                        if (rx_data_len) {
                            req->data = new UInt8[rx_data_len];
                            req->data_len = rx_data_len;
                            memcpy(req->data, rx_data, rx_data_len);
                        }
                        command_gate->commandWakeup(&req->waiting);
                        waiting_pos->next = req->next;
                        if (!waiting_pos->next)
                            last_waiting = waiting_pos;
                        break;
                    }
                    waiting_pos = waiting_pos->next;
                }
                if (!found)
                    LOG("Warning, received data with unknown tc %x, cid %x", command->target_category, command->command_id);
            } else {
                if (handler[command->request_id]) {
                    handler[command->request_id]->eventReceived(command->target_category, command->target_id_in, command->instance_id, command->command_id, rx_data, rx_data_len);
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
    
    rx_msg.pos = 0;
    rx_msg.len = SSH_MSG_LENGTH_UNKNOWN;
    
    return kIOReturnSuccess;
}

IOReturn SurfaceSerialHubDriver::sendACK(UInt8 seq_id) {
    UInt8 buffer[SSH_PAYLOAD_OFFSET+2];
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(buffer);
    msg->syn = SSH_SYN_BYTES;
    msg->frame.type = SSH_FRAME_TYPE_ACK;
    msg->frame.length = 0;
    msg->frame.seq_id = seq_id;
    msg->frame_crc = crc_ccitt_false(CRC_INITIAL, buffer+2, sizeof(SurfaceSerialFrame));
    buffer[SSH_PAYLOAD_OFFSET] = 0xFF;
    buffer[SSH_PAYLOAD_OFFSET+1] = 0xFF;
    return uart_controller->transmitData(buffer, SSH_PAYLOAD_OFFSET+2);
}

IOReturn SurfaceSerialHubDriver::sendNAK() {
    UInt8 buffer[SSH_PAYLOAD_OFFSET+2];
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(buffer);
    msg->syn = SSH_SYN_BYTES;
    msg->frame.type = SSH_FRAME_TYPE_NAK;
    msg->frame.length = 0;
    msg->frame.seq_id = 0;
    msg->frame_crc = crc_ccitt_false(CRC_INITIAL, buffer+2, sizeof(SurfaceSerialFrame));
    buffer[SSH_PAYLOAD_OFFSET] = 0xFF;
    buffer[SSH_PAYLOAD_OFFSET+1] = 0xFF;
    return uart_controller->transmitData(buffer, SSH_PAYLOAD_OFFSET+2);
}

UInt16 SurfaceSerialHubDriver::sendCommand(UInt8 tc, UInt8 tid, UInt8 iid, UInt8 cid, UInt8 *payload, UInt16 payload_len, bool seq) {
    if (!awake)
        return 0;
    
    UInt16 len = SSH_DATA_OFFSET+payload_len+2;
    UInt8 *buffer = new UInt8[len];
    
    SurfaceSerialMessage *msg = reinterpret_cast<SurfaceSerialMessage *>(buffer);
    msg->syn = SSH_SYN_BYTES;
    msg->frame.type = seq ? SSH_FRAME_TYPE_DATA_SEQ : SSH_FRAME_TYPE_DATA_NSQ;
    msg->frame.length = payload_len + 8;
    msg->frame.seq_id = seq_counter.getID();
    msg->frame_crc = crc_ccitt_false(CRC_INITIAL, buffer+2, sizeof(SurfaceSerialFrame));
    
    SurfaceSerialCommand *cmd = reinterpret_cast<SurfaceSerialCommand *>(buffer+SSH_PAYLOAD_OFFSET);
    cmd->type = SSH_COMMAND_TYPE;
    cmd->target_category = tc;
    cmd->target_id_out = tid;
    cmd->target_id_in = 0x00;
    cmd->instance_id = iid;
    cmd->request_id = req_counter.getID();
    cmd->command_id = cid;
    
    if (payload_len > 0)
        memcpy(buffer+SSH_DATA_OFFSET, payload, payload_len);
    
    *(reinterpret_cast<UInt16 *>(buffer+SSH_DATA_OFFSET+payload_len)) = crc_ccitt_false(CRC_INITIAL, buffer+SSH_PAYLOAD_OFFSET, sizeof(SurfaceSerialCommand)+payload_len);
    
    if (command_gate->runAction(OSMemberFunctionCast(IOCommandGate::Action, this, &SurfaceSerialHubDriver::sendCommandGated), buffer, &len, &seq) != kIOReturnSuccess) {
        LOG("Sending command failed!");
        delete[] buffer;
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

void SurfaceSerialHubDriver::commandTimeout(IOTimerEventSource* timer) {
    PendingCommand *pos = &pending_commands;
    bool found = false;
    while (pos->next) {
        if (pos->next->timer == timer) {
            found = true;
            PendingCommand *cmd = pos->next;
            SurfaceSerialCommand *cmd_data = reinterpret_cast<SurfaceSerialCommand *>(cmd->buffer+SSH_PAYLOAD_OFFSET);
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
                cmd->timer->setTimeoutMS(SSH_ACK_TIMEOUT);
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
    w->data = nullptr;
    w->data_len = 0;
    w->next = nullptr;
    last_waiting->next = w;
    last_waiting = w;
    
    nanoseconds_to_absolutetime(SSH_WAIT_TIMEOUT * 1000000, &abstime);
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
    if (*buffer_len != w->data_len)
        LOG("Warning, given buffer_len(%d) and received data_len(%d) mismatched!", *buffer_len, w->data_len);
    if (w->data_len) {
        if (*buffer_len < w->data_len)
            w->data_len = *buffer_len;
        memcpy(buffer, w->data, w->data_len);
        delete[] w->data;
    }
    delete w;
    return kIOReturnSuccess;
}

IOReturn SurfaceSerialHubDriver::registerEvent(SurfaceSerialHubClient *client, UInt8 tc, UInt8 iid) {
    if (!client)
        return kIOReturnError;
    if (handler[tc]) {
        LOG("Already has an event handler registered!");
        return kIOReturnNoResources;
    }
    SurfaceEventPayload payload;
    UInt8 ret;
    payload.target_category = tc;
    payload.instance_id = iid;
    payload.request_id = tc;
    payload.flags = SSH_EVENT_FLAG_SEQUENCED;
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_ENABLE_EVENT, reinterpret_cast<UInt8 *>(&payload), sizeof(SurfaceEventPayload), true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        LOG("Unexpected response from event-enable request");
        return kIOReturnError;
    }
    handler[tc] = client;
    return kIOReturnSuccess;
}

void SurfaceSerialHubDriver::unregisterEvent(SurfaceSerialHubClient *client, UInt8 tc, UInt8 iid) {
    if (handler[tc]) {
        if (handler[tc] == client) {
            SurfaceEventPayload payload;
            UInt8 ret;
            payload.target_category = tc;
            payload.instance_id = iid;
            payload.request_id = tc;
            payload.flags = SSH_EVENT_FLAG_SEQUENCED;
            if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISABLE_EVENT, reinterpret_cast<UInt8 *>(&payload), sizeof(SurfaceEventPayload), true, &ret, 1) != kIOReturnSuccess || ret != 0) {
                LOG("Unexpected response from event-disable request");
                return;
            }
            handler[tc] = nullptr;
        } else
            LOG("Event not registered for this client!");
    }
}

bool SurfaceSerialHubDriver::init(OSDictionary *properties) {
    if (!super::init(properties))
        return false;
    
    pending_commands = {nullptr, 0, nullptr, 0, nullptr};
    waiting_requests = {false, 0, nullptr, 0, nullptr};
    last_pending = &pending_commands;
    last_waiting = &waiting_requests;
    memset(handler, 0, sizeof(handler));
    memset(rx_msg.cache, 0, SSH_MSG_CACHE_SIZE);
    rx_msg.pos = 0;
    rx_msg.len = SSH_MSG_LENGTH_UNKNOWN;
    rx_msg.partial_syn = false;
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
    
    gpio_controller = getGPIOController();
    if (!gpio_controller) {
        LOG("Could not find GPIO controller, exiting");
        return nullptr;
    }
    uart_controller = getUARTController();
    if (!uart_controller) {
        LOG("Could not find UART controller, exiting");
        return nullptr;
    }
    // Give the UART controller some time to load
    IOSleep(100);
    
    // Allocate a circle buffer with size SSH_BUFFER_SIZE to store buffer from UART
    current = new CircleBuffer;
    current->buffer = new UInt8[fifo_size];
    current->buffer_len = 0;
    CircleBuffer* pos = current;
    for (int i=0; i < SSH_BUFFER_SIZE-1; i++) {
        CircleBuffer* buf = new CircleBuffer;
        buf->buffer = new UInt8[fifo_size];
        buf->buffer_len = 0;
        pos->next = buf;
        pos = buf;
    }
    pos->next = current;
    last = pos;
    
    LOG("Surface Serial Hub found!");
    return this;
}

bool SurfaceSerialHubDriver::start(IOService *provider) {
    UInt32 version;
    UInt8 ret;
    if (!super::start(provider))
        return false;
    
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
    
    uart_interrupt = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceSerialHubDriver::processReceivedBuffer));
    if (!uart_interrupt) {
        LOG("Could not register uart interrupt!");
        goto exit;
    }
    work_loop->addEventSource(uart_interrupt);
    gpio_interrupt = IOInterruptEventSource::interruptEventSource(this, OSMemberFunctionCast(IOInterruptEventAction, this, &SurfaceSerialHubDriver::gpioWakeUp), this, 0);
    if (!gpio_interrupt) {
        LOG("Could not register gpio interrupt!");
        goto exit;
    }
    work_loop->addEventSource(gpio_interrupt);
    
    if (uart_controller->requestConnect(this, OSMemberFunctionCast(VoodooUARTController::MessageHandler, this, &SurfaceSerialHubDriver::bufferReceived), baudrate, data_bits, stop_bits, parity, flow_control) != kIOReturnSuccess) {
        LOG("Failed to connect to UART controller!");
        goto exit;
    }
    
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_VERSION, nullptr, 0, true, reinterpret_cast<UInt8 *>(&version), 4) != kIOReturnSuccess) {
        LOG("Failed to get SAM version from UART!");
        goto exit_connected;
    }
    LOG("SAM version %u.%u.%u", (version >> 24) & 0xff, (version >> 8) & 0xffff, version & 0xff);
    
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_D0_ENTRY, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        LOG("Unexpected response from D0-entry notification");
        goto exit_connected;
    }
    if (getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISPLAY_ON, nullptr, 0, true, &ret, 1) != kIOReturnSuccess || ret != 0) {
        LOG("Unexpected response from display-on notification");
        goto exit_connected;
    }
    
    // publishing necessary surface device nubs
    battery_nub = OSTypeAlloc(SurfaceBatteryNub);
    if (!battery_nub || !battery_nub->init() || !battery_nub->attach(this)) {
        LOG("Failed to init Surface Battery nub!");
        OSSafeReleaseNULL(battery_nub);
    } else {
        if (!battery_nub->start(this)) {
            LOG("Failed to attach Surface Battery nub!");
            battery_nub->detach(this);
            OSSafeReleaseNULL(battery_nub);
        } else
            LOG("Surface Battery nub published!");
    }
    
    laptop3_nub = OSTypeAlloc(SurfaceLaptop3Nub);
    if (!laptop3_nub || !laptop3_nub->init() || !laptop3_nub->attach(this)) {
        LOG("Failed to init Surface Laptop3 nub!");
        OSSafeReleaseNULL(laptop3_nub);
    } else {
        if (!laptop3_nub->start(this)) {
            LOG("Failed to attach Surface Laptop3 nub! Ignore this if you are not a Laptop3 device");
            laptop3_nub->detach(this);
            OSSafeReleaseNULL(laptop3_nub);
        } else
            LOG("Surface Laptop3 nub published!");
    }

    PMinit();
    uart_controller->joinPMtree(this);
    registerPowerDriver(this, MyIOPMPowerStates, kIOPMNumberPowerStates);
    
    registerService();
    return true;
    
exit_connected:
    uart_controller->requestDisconnect(this);
exit:
    releaseResources();
    return false;
}

void SurfaceSerialHubDriver::stop(IOService *provider) {
    UInt8 ret;
    getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_DISPLAY_OFF, nullptr, 0, true, &ret, 1);
    getResponse(SSH_TC_SAM, SSH_TID_PRIMARY, 0, SSH_CID_SAM_D0_EXIT, nullptr, 0, true, &ret, 1);
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
            uart_interrupt->disable();
            // clear pending commands, rx_buffer and msg cache
            PendingCommand *pending_pos = pending_commands.next;
            if (pending_pos){
                LOG("There are still pending commands!");
                while (pending_pos) {
                    PendingCommand *t = pending_pos;
                    pending_pos = pending_pos->next;
                    t->timer->cancelTimeout();
                    t->timer->disable();
                    work_loop->removeEventSource(t->timer);
                    OSSafeReleaseNULL(t->timer);
                    delete[] t->buffer;
                    delete t;
                }
            }
            if (current->buffer_len) {
                LOG("There are still raw data unprocessed!");
                while (current->buffer_len) {
                    current->buffer_len = 0;
                    current = current->next;
                }
            }
            rx_msg.pos = 0;
            rx_msg.len = SSH_MSG_LENGTH_UNKNOWN;
            rx_msg.partial_syn = false;
            LOG("Going to sleep");
        }
    } else {
        if (!awake) {
            awake = true;
            uart_interrupt->enable();
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
    if (battery_nub) {
        battery_nub->stop(this);
        battery_nub->detach(this);
        OSSafeReleaseNULL(battery_nub);
    }
    if (laptop3_nub) {
        laptop3_nub->stop(this);
        laptop3_nub->detach(this);
        OSSafeReleaseNULL(laptop3_nub);
    }
    WaitingRequest *waiting_pos = waiting_requests.next;
    if (waiting_pos) {
        while (waiting_pos) {
            WaitingRequest *t = waiting_pos;
            waiting_pos = waiting_pos->next;
            if (t->data_len)
                delete[] t->data;
            delete t;
        }
    }
    PendingCommand *pending_pos = pending_commands.next;
    if (pending_pos){
        while (pending_pos) {
            PendingCommand *t = pending_pos;
            pending_pos = pending_pos->next;
            t->timer->cancelTimeout();
            t->timer->disable();
            work_loop->removeEventSource(t->timer);
            OSSafeReleaseNULL(t->timer);
            delete[] t->buffer;
            delete t;
        }
    }
    CircleBuffer* buffer_pos = current;
    for (int i=0; i < SSH_BUFFER_SIZE; i++) {
        CircleBuffer* t = buffer_pos;
        buffer_pos = buffer_pos->next;
        delete[] t->buffer;
        delete t;
    }
    if (uart_interrupt) {
        uart_interrupt->disable();
        work_loop->removeEventSource(uart_interrupt);
        OSSafeReleaseNULL(uart_interrupt);
    }
    if (gpio_interrupt) {
        gpio_interrupt->disable();
        work_loop->removeEventSource(gpio_interrupt);
        OSSafeReleaseNULL(gpio_interrupt);
    }
    if (command_gate) {
        work_loop->removeEventSource(command_gate);
        OSSafeReleaseNULL(command_gate);
    }
    OSSafeReleaseNULL(work_loop);
}

VoodooGPIO* SurfaceSerialHubDriver::getGPIOController() {
    VoodooGPIO* gpio_controller = nullptr;

    // Wait for GPIO controller, up to 1 second
    OSDictionary* name_match = serviceMatching("VoodooGPIO");
    IOService* matched = waitForMatchingService(name_match, 1000000000);
    gpio_controller = OSDynamicCast(VoodooGPIO, matched);
    if (gpio_controller != NULL)
        IOLog("%s::Got GPIO Controller! %s\n", getName(), gpio_controller->getName());
    
    OSSafeReleaseNULL(name_match);
    OSSafeReleaseNULL(matched);

    return gpio_controller;
}

VoodooUARTController* SurfaceSerialHubDriver::getUARTController() {
    // Wait for UART controller, up to 1 second, try for 5 times
    // Sometimes it takes really a long time for UART to load
    for (int i=0; i < 5; i++) {
        OSDictionary* name_match = serviceMatching("VoodooUARTController");
        IOService* matched = waitForMatchingService(name_match, 1000000000);
        VoodooUARTController* uart = OSDynamicCast(VoodooUARTController, matched);
        OSSafeReleaseNULL(name_match);
        OSSafeReleaseNULL(matched);
        if (uart) {
            LOG("Got UART Controller! %s", uart->getName());
            return uart;
        }
        IOSleep(1000);
    }

    return nullptr;
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
    UInt32 length = data->getLength();
    parser.parseACPIResources(crs, 0, length);
    OSSafeReleaseNULL(data);
    
    if (!parser.found_uart || !parser.found_gpio_interrupts)
        return kIOReturnNoResources;
    
    LOG("Found valid UART bus and GPIO interrupt!");
//    LOG("resource_consumer %d", parser.uart_info.resource_consumer);
//    LOG("device_initiated %d", parser.uart_info.device_initiated);
//    LOG("flow_control %d", parser.uart_info.flow_control);
//    LOG("stop_bits %d", parser.uart_info.stop_bits);
//    LOG("data_bits %d", parser.uart_info.data_bits);
//    LOG("big_endian %d", parser.uart_info.big_endian);
//    LOG("baudrate %u", parser.uart_info.baudrate);
//    LOG("rx_fifo %d", parser.uart_info.rx_fifo);
//    LOG("tx_fifo %d", parser.uart_info.tx_fifo);
//    LOG("parity %d", parser.uart_info.parity);
//    LOG("dtd_enabled %d", parser.uart_info.dtd_enabled);
//    LOG("ri_enabled %d", parser.uart_info.ri_enabled);
//    LOG("dsr_enabled %d", parser.uart_info.dsr_enabled);
//    LOG("dtr_enabled %d", parser.uart_info.dtr_enabled);
//    LOG("cts_enabled %d", parser.uart_info.cts_enabled);
//    LOG("rts_enabled %d", parser.uart_info.rts_enabled);
    
    baudrate = parser.uart_info.baudrate;
    data_bits = parser.uart_info.data_bits;
    stop_bits = parser.uart_info.stop_bits;
    parity = parser.uart_info.parity;
    flow_control = parser.uart_info.flow_control != 0;
    // For SP6, UART bus reports a rx_fifo of size 32 yet the UART device reports itself with a fifo size 64
    fifo_size = 64; //parser.uart_info.rx_fifo;
    
    gpio_pin = parser.gpio_interrupts.pin_number;
    gpio_irq = 0x00000001; //parser.gpio_interrupts.irq_type; // Should be Edge rising ?
    
    return kIOReturnSuccess;
}

void SurfaceSerialHubDriver::gpioWakeUp(IOInterruptEventSource *sender, int count) {
    LOG("GPIO wake up event happened!");
}

IOReturn SurfaceSerialHubDriver::enableInterrupt(int source) {
    return gpio_controller->enableInterrupt(gpio_pin);
}

IOReturn SurfaceSerialHubDriver::disableInterrupt(int source) {
    return gpio_controller->disableInterrupt(gpio_pin);
}

IOReturn SurfaceSerialHubDriver::getInterruptType(int source, int* interrupt_type) {
    return gpio_controller->getInterruptType(gpio_pin, interrupt_type);
}

IOReturn SurfaceSerialHubDriver::registerInterrupt(int source, OSObject *target, IOInterruptAction handler, void *refcon) {
    gpio_controller->setInterruptTypeForPin(gpio_pin, gpio_irq);
    return gpio_controller->registerInterrupt(gpio_pin, target, handler, refcon);
}

IOReturn SurfaceSerialHubDriver::unregisterInterrupt(int source) {
    return gpio_controller->unregisterInterrupt(gpio_pin);
}
