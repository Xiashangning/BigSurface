//
//  MEIProtocol.h
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/5/29.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef MEIProtocol_h
#define MEIProtocol_h

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

#ifndef BIT
#define BIT(nr) (1UL << (nr))
#endif

#ifndef GENMASK
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (64 - 1 - (h))))
#endif

/*
 * IPTS MEI constants and communication protocol ported from linux
 */

/* Host Firmware Status Registers in PCI Config Space */
#define MEI_PCI_CFG_1           0x40
#define MEI_PCI_CFG_1_D0I3      0x80000000

/* MEI registers */
#define MEI_H_CB_WW    0x00     /* Host Circular Buffer Write Window register */
#define MEI_H_CSR      0x04     /* Host Control Status register */
#define MEI_ME_CB_RW   0x08     /* ME Circular Buffer Read Window register */
#define MEI_ME_CSR     0x0C     /* ME Control Status Host Access register */
#define MEI_H_PG_CSR   0x10     /* Host Power Gate Control Status register */
#define MEI_H_D0I3C    0x800    /* Host D0I3 Control  */

/* Control Status register bits for Host and ME */
#define MEI_CSR_BUF_DEPTH         0xFF000000    /* Circular Buffer Depth */
#define MEI_CSR_BUF_WPOINTER      0x00FF0000    /* Circular Buffer Write Pointer */
#define MEI_CSR_BUF_RPOINTER      0x0000FF00    /* Circular Buffer Read Pointer */

/* Host Control Status register bits */
#define MEI_H_CSR_INT_ENABLE        BIT(0)    /* Interrupt Enable */
#define MEI_H_CSR_INT_STA           BIT(1)    /* Interrupt Status */
#define MEI_H_CSR_INT_GEN           BIT(2)    /* Interrupt Generate */
#define MEI_H_CSR_READY             BIT(3)    /* Ready */
#define MEI_H_CSR_RESET             BIT(4)    /* Reset */
#define MEI_H_CSR_D0I3C_INT_ENABLE  BIT(5)    /* D0I3 Interrupt Enable */
#define MEI_H_CSR_D0I3C_INT_STA     BIT(6)    /* D0I3 Interrupt Status */
#define MEI_H_CSR_INT_ENABLE_MASK   (MEI_H_CSR_INT_ENABLE | MEI_H_CSR_D0I3C_INT_ENABLE)
#define MEI_H_CSR_INT_STA_MASK      (MEI_H_CSR_INT_STA | MEI_H_CSR_D0I3C_INT_STA)

/* ME Control Status Host Access register bits */
#define MEI_ME_CSR_INT_ENABLE           BIT(0)    /* Interrupt Enable */
#define MEI_ME_CSR_INT_STA              BIT(1)    /* Interrupt Status */
#define MEI_ME_CSR_INT_GEN              BIT(2)    /* Interrupt Generate */
#define MEI_ME_CSR_READY                BIT(3)    /* Ready */
#define MEI_ME_CSR_RESET                BIT(4)    /* Reset */
#define MEI_ME_CSR_POWER_GATE_ISO_CAP   BIT(6)    /* Power Gate Isolation Capability */

/* Host Power Gate Control Status register bits */
#define MEI_H_HPG_CSR_POWER_GATING_EXIT     BIT(0)
#define MEI_H_HPG_CSR_POWER_GATING_ENTER    BIT(1)

/* Host D0I3C register bits */
#define MEI_H_D0I3C_CIP         BIT(0)
#define MEI_H_D0I3C_IR          BIT(1)
#define MEI_H_D0I3C_I3          BIT(2)
#define MEI_H_D0I3C_RR          BIT(3)

#define MEI_SLOT_SIZE               sizeof(UInt32)
#define MEI_SLOT_TYPE               UInt32
#define MEI_RX_MSG_BUF_SIZE         (128 * MEI_SLOT_SIZE)
#define MEI_DATA_TO_SLOTS(len)      (((len)+MEI_SLOT_SIZE-1)/MEI_SLOT_SIZE)
#define MEI_SLOTS_TO_DATA(slots)    ((slots) * MEI_SLOT_SIZE)

#define MEI_MAX_CLIENT_NUM      256     /* SHOULD be dividable by 8 */
#define MEI_MAX_CONSEC_RESET    3

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

/* Timeouts in Seconds */
#define MEI_HW_READY_TIMEOUT            2  /* Timeout on ready message */
#define MEI_CONNECT_TIMEOUT             3  /* HPS: at least 2 seconds */

#define MEI_CLIENT_CONNECT_TIMEOUT      15  /* HPS: Client Connect Timeout */
#define MEI_CLIENTS_INIT_TIMEOUT        15  /* HPS: Clients Enumeration Timeout */

#define MEI_POWER_GATING_ISO_TIMEOUT    1  /* PG Isolation time response 1 sec */
#define MEI_D0I3_TIMEOUT                5  /* D0i3 set/unset max response time */
#define MEI_HOST_BUS_MSG_TIMEOUT        1  /* 1 second */

#define MEI_DEVICE_IDLE_TIMEOUT         5

#define MEI_CLIENT_SEND_MSG_TIMEOUT     500 /* 500 ms*/

/*
 * MEI Version
 */
#define MEI_HBM_MINOR_VERSION               2
#define MEI_HBM_MAJOR_VERSION               2
/*
 * MEI version with PGI support
 */
#define MEI_HBM_MINOR_VERSION_PGI           1
#define MEI_HBM_MAJOR_VERSION_PGI           1
/*
 * MEI version with Dynamic clients support
 */
#define MEI_HBM_MINOR_VERSION_DC            0
#define MEI_HBM_MAJOR_VERSION_DC            2
/*
 * MEI version with immediate reply to enum request support
 */
#define MEI_HBM_MINOR_VERSION_IE            0
#define MEI_HBM_MAJOR_VERSION_IE            2
/*
 * MEI version with disconnect on connection timeout support
 */
#define MEI_HBM_MINOR_VERSION_DOT           0
#define MEI_HBM_MAJOR_VERSION_DOT           2
/*
 * MEI version with notification support
 */
#define MEI_HBM_MINOR_VERSION_EV            0
#define MEI_HBM_MAJOR_VERSION_EV            2
/*
 * MEI version with fixed address client support
 */
#define MEI_HBM_MINOR_VERSION_FA            0
#define MEI_HBM_MAJOR_VERSION_FA            2
/*
 * MEI version with OS ver message support
 */
#define MEI_HBM_MINOR_VERSION_OS            0
#define MEI_HBM_MAJOR_VERSION_OS            2
/*
 * MEI version with dma ring support
 */
#define MEI_HBM_MINOR_VERSION_DR            1
#define MEI_HBM_MAJOR_VERSION_DR            2
/*
 * MEI version with vm tag support
 */
#define MEI_HBM_MINOR_VERSION_VT            2
#define MEI_HBM_MAJOR_VERSION_VT            2
/*
 * MEI version with capabilities message support
 */
#define MEI_HBM_MINOR_VERSION_CAP           2
#define MEI_HBM_MAJOR_VERSION_CAP           2
/*
 * MEI version with client DMA support
 */
#define MEI_HBM_MINOR_VERSION_CD            2
#define MEI_HBM_MAJOR_VERSION_CD            2

/* Host bus message command opcode */
#define MEI_HBM_CMD_OP_MSK                  0x7f
/* Host bus message command RESPONSE */
#define MEI_HBM_CMD_RES_MSK                 0x80

/*
 * MEI Bus Message Command IDs
 */
#define MEI_HOST_START_REQ_CMD              0x01
#define MEI_HOST_START_RES_CMD              0x81

#define MEI_HOST_STOP_REQ_CMD               0x02
#define MEI_HOST_STOP_RES_CMD               0x82

#define MEI_ME_STOP_REQ_CMD                 0x03

#define MEI_HOST_ENUM_REQ_CMD               0x04
#define MEI_HOST_ENUM_RES_CMD               0x84

#define MEI_HOST_CLIENT_PROP_REQ_CMD        0x05
#define MEI_HOST_CLIENT_PROP_RES_CMD        0x85

#define MEI_CLIENT_CONNECT_REQ_CMD          0x06
#define MEI_CLIENT_CONNECT_RES_CMD          0x86

#define MEI_CLIENT_DISCONNECT_REQ_CMD       0x07
#define MEI_CLIENT_DISCONNECT_RES_CMD       0x87

#define MEI_FLOW_CONTROL_CMD                0x08

#define MEI_PG_ISOLATION_ENTRY_REQ_CMD      0x0a
#define MEI_PG_ISOLATION_ENTRY_RES_CMD      0x8a
#define MEI_PG_ISOLATION_EXIT_REQ_CMD       0x0b
#define MEI_PG_ISOLATION_EXIT_RES_CMD       0x8b

#define MEI_ADD_CLIENT_REQ_CMD              0x0f
#define MEI_ADD_CLIENT_RES_CMD              0x8f

#define MEI_NOTIFY_REQ_CMD                  0x10
#define MEI_NOTIFY_RES_CMD                  0x90
#define MEI_NOTIFICATION_CMD                0x11

#define MEI_DMA_SETUP_REQ_CMD               0x12
#define MEI_DMA_SETUP_RES_CMD               0x92

#define MEI_CAPABILITIES_REQ_CMD            0x13
#define MEI_CAPABILITIES_RES_CMD            0x93

#define MEI_CLIENT_DMA_MAP_REQ_CMD          0x14
#define MEI_CLIENT_DMA_MAP_RES_CMD          0x94

#define MEI_CLIENT_DMA_UNMAP_REQ_CMD        0x15
#define MEI_CLIENT_DMA_UNMAP_RES_CMD        0x95

enum MEIHostBusMessageReturnType : UInt8 {
    MEIHostBusMessageReturnSuccess        = 0,
    MEIHostBusMessageReturnClientNotFound = 1,
    MEIHostBusMessageReturnAlreadyExists  = 2,  /* connection already established */
    MEIHostBusMessageReturnRejected       = 3,  /* connection is rejected */
    MEIHostBusMessageReturnInvalidParam   = 4,
    MEIHostBusMessageReturnNotAllowed     = 5,
    MEIHostBusMessageReturnAlreadyStarted = 6,  /* system is already started */
    MEIHostBusMessageReturnNotStarted     = 7,  /* system not started */
    MEIHostBusMessageReturnTypeLength,
};

enum MEIExtendedHeaderType : UInt8 {
    MEIExtendedHeaderNone = 0,
    MEIExtendedHeaderVtag = 1,
};

/**
 * struct MEIExtendedHeaderDescriptor - extend header descriptor (TLV)
 * @type: header type
 * @length: header length excluding descriptor SHOULD BE 1
 * @data: payload data
 */
struct PACKED MEIBusExtendedHeader {
    MEIExtendedHeaderType type;
    UInt8 length;
    UInt8 data[2];
};

#define MEI_EXTHEADER_DATA_VTAG_IDX  0

/**
 * struct MEIExtendedMetaHeader - extend header meta data
 * @count: number of headers
 * @size: total slot count of the extended header list excluding meta header
 * @reserved: reserved
 * @hdrs: extended headers TLV list
 */
struct PACKED MEIBusExtendedMetaHeader {
    UInt8 count;
    UInt8 size;
    UInt8 reserved[2];
    UInt8 hdrs[];
};

/*
 * Extended header iterator functions
 */
static inline MEIBusExtendedHeader *mei_ext_begin(MEIBusExtendedMetaHeader *meta)
{
    return reinterpret_cast<MEIBusExtendedHeader *>(meta->hdrs);
}

/**
 * mei_ext_last - check if the ext is the last one in the TLV list
 *
 * @meta: meta header of the extended header list
 * @hdr: a meta header on the list
 *
 * Return: true if ext is the last header on the list
 */
static inline bool mei_ext_last(MEIBusExtendedMetaHeader *meta, MEIBusExtendedHeader *hdr)
{
    return reinterpret_cast<UInt8 *>(hdr) >= reinterpret_cast<UInt8 *>(meta) + sizeof(MEIBusExtendedMetaHeader) + MEI_SLOTS_TO_DATA(meta->size);
}

/**
 * struct MEIMessageHeader - MEI Bus Interface Section size=4
 *
 * @me_addr: device address
 * @host_addr: host address
 * @length: message length
 * @reserved: reserved
 * @extended: message has extended header
 * @dma_ring: message is on dma ring
 * @internal: message is internal
 * @msg_complete: last packet of the message
 * @extension: extension of the header
 */
struct PACKED MEIBusMessageHeader {
    UInt32 me_addr:8;
    UInt32 host_addr:8;
    UInt32 length:9;
    UInt32 reserved:3;
    UInt32 extended:1;
    UInt32 dma_ring:1;
    UInt32 internal:1;
    UInt32 msg_complete:1;
    UInt32 extension[];
};

/* The length is up to 9 bits */
#define MEI_MSG_MAX_LEN_MASK GENMASK(9, 0)

/**
 * struct MEIBusMessage - Generic MEI bus message
 *
 * @cmd: bus message command ID
 * @data: generic data
 */
struct PACKED MEIBusMessage {
    UInt8 cmd;
    UInt8 data[];
};

#define MEI_TO_MSG(specific_msg)    (reinterpret_cast<MEIBusMessage *>(specific_msg))

struct PACKED MEIBusGenericMessage {
    UInt8 cmd;
    UInt8 reserved[3];
};

typedef MEIBusGenericMessage MEIBusPowerGatingRequest;
typedef MEIBusGenericMessage MEIBusPowerGatingResponse;

struct PACKED MEIBusHostVersionRequest {
    UInt8 cmd;
    UInt8 reserved;
    UInt8 host_version_minor;
    UInt8 host_version_major;
};

struct PACKED MEIBusHostVersionResponse {
    UInt8 cmd;
    UInt8 host_version_supported;
    UInt8 me_max_version_minor;
    UInt8 me_max_version_major;
};

enum MEIStopReason : UInt8 {
    DriverStopRequest = 0x00,
    DeviceD1Entry     = 0x01,
    DeviceD2Entry     = 0x02,
    DeviceD3Entry     = 0x03,
    SystemS1Entry     = 0x04,
    SystemS2Entry     = 0x05,
    SystemS3Entry     = 0x06,
    SystemS4Entry     = 0x07,
    SystemS5Entry     = 0x08,
};

struct PACKED MEIBusHostStopRequest {
    UInt8 cmd;
    MEIStopReason reason;
    UInt8 reserved[2];
};

struct PACKED MEIBusHostStopResponse {
    UInt8 cmd;
    UInt8 reserved[3];
};

struct PACKED MEIBusMEStopRequest {
    UInt8 cmd;
    MEIStopReason reason;
    UInt8 reserved[2];
};

/**
 * @MEI_HBM_ENUM_ALLOW_ADD_FLAG: allow dynamic clients add
 * @MEI_HBM_ENUM_IMMEDIATE_FLAG: allow FW to send answer immediately
 */
#define MEI_HBM_ENUM_ALLOW_ADD_FLAG     BIT(0)
#define MEI_HBM_ENUM_IMMEDIATE_FLAG     BIT(1)

struct PACKED MEIBusHostEnumerationRequest {
    UInt8 cmd;
    UInt8 flags;
    UInt8 reserved[2];
};

struct PACKED MEIBusHostEnumerationResponse {
    UInt8 cmd;
    UInt8 reserved[3];
    UInt8 valid_addresses[MEI_MAX_CLIENT_NUM/8];
};

/**
 * struct MEIClientProperty - mei client properties
 *
 * @uuid: guid of the client
 * @protocol_version: client protocol version
 * @max_connection_num: number of possible connections. 0: fixed clients
 * @fixed_address: fixed me address (0 if the client is dynamic)
 * @single_recv_buf: 1 if all connections share a single receive buffer.
 * @vt_supported: the client support vtag
 * @reserved: reserved
 * @max_msg_length: MTU of the client
 */
struct PACKED MEIClientProperty {
    uuid_t uuid;
    UInt8  protocol_version;
    UInt8  max_connection_num;
    UInt8  fixed_address;
    UInt8  single_recv_buf:1;
    UInt8  vt_supported:1;
    UInt8  reserved:6;
    UInt32 max_msg_length;
};

struct PACKED MEIBusClientPropertyRequest {
    UInt8 cmd;
    UInt8 me_addr;
    UInt8 reserved[2];
};

struct PACKED MEIBusClientPropertyResponse {
    UInt8 cmd;
    UInt8 me_addr;
    MEIHostBusMessageReturnType status;
    UInt8 reserved;
    MEIClientProperty client_properties;
};

/**
 * struct MEIBusAddClientRequest - request to add a client
 *     might be sent by fw after enumeration has already completed
 *
 * @cmd: bus message command ID
 * @me_addr: address of the client in ME
 * @reserved: reserved
 * @client_properties: client properties
 */
struct PACKED MEIBusAddClientRequest {
    UInt8 cmd;
    UInt8 me_addr;
    UInt8 reserved[2];
    MEIClientProperty client_properties;
};

/**
 * struct MEIBusAddClientResponse - response to add a client
 *     sent by the host to report client addition status to fw
 *
 * @cmd: bus message command ID
 * @me_addr: address of the client in ME
 * @status: if MEIHostBusMessageReturnSuccess then the client can now accept connections.
 * @reserved: reserved
 */
struct PACKED MEIBusAddClientResponse {
    UInt8 cmd;
    UInt8 me_addr;
    MEIHostBusMessageReturnType status;
    UInt8 reserved;
};

/**
 * struct MEIBusClientCommand - client specific host bus command
 *    CONNECT, DISCONNECT, FlOW CONTROL, NOTIFICATION
 *
 * @cmd: bus message command ID
 * @me_addr: address of the client in ME
 * @host_addr: address of the client in the driver
 * @data: generic data
 */
struct PACKED MEIBusClientCommand {
    UInt8 cmd;
    UInt8 me_addr;
    UInt8 host_addr;
    UInt8 data;
};

typedef MEIBusClientCommand MEIBusClientConnectionRequest;

enum MEIClientConnectionStatus : UInt8 {
    MEIClientConnectionSuccess         = MEIHostBusMessageReturnSuccess,
    MEIClientConnectionNotFound        = MEIHostBusMessageReturnClientNotFound,
    MEIClientConnectionAlreadyStarted  = MEIHostBusMessageReturnAlreadyExists,
    MEIClientConnectionOutOfResources  = MEIHostBusMessageReturnRejected,
    MEIClientConnectionMessageTooSmall = MEIHostBusMessageReturnInvalidParam,
    MEIClientConnectionNotAllowed      = MEIHostBusMessageReturnNotAllowed,
};

/**
 * struct MEIBusClientConnectionResponse - connect/disconnect response
 *
 * @cmd: bus message command header
 * @me_addr: address of the client in ME
 * @host_addr: address of the client in the driver
 * @status: status of the request
 */
struct PACKED MEIBusClientConnectionResponse {
    UInt8 cmd;
    UInt8 me_addr;
    UInt8 host_addr;
    MEIClientConnectionStatus status;
};

#define MEI_FLOW_CONTROL_MSG_RESERVED_LEN   5

struct PACKED MEIBusFlowControl {
    UInt8 cmd;
    UInt8 me_addr;
    UInt8 host_addr;
    UInt8 reserved[MEI_FLOW_CONTROL_MSG_RESERVED_LEN];
};

#define MEI_HBM_NOTIFICATION_START 1
#define MEI_HBM_NOTIFICATION_STOP  0
/**
 * struct MEIBusNotificationRequest - start/stop notification request
 *
 * @cmd: bus message command ID
 * @me_addr: address of the client in ME
 * @host_addr: address of the client in the driver
 * @start:  start = 1 or stop = 0 asynchronous notifications
 */
struct PACKED MEIBusNotificationRequest {
    UInt8 cmd;
    UInt8 me_addr;
    UInt8 host_addr;
    UInt8 start;
};

/**
 * struct MEIBusNotificationResponse - start/stop notification response
 *
 * @cmd: bus message command header
 * @me_addr: address of the client in ME
 * @host_addr: - address of the client in the driver
 * @status: (mei_hbm_status) response status for the request
 *  - MEIHostBusMessageReturnSuccess:           successful stop/start
 *  - MEIHostBusMessageReturnClientNotFound:    if the connection could not be found.
 *  - MEIHostBusMessageReturnAlreadyStarted:    for start requests for a previously started notification.
 *  - MEIHostBusMessageReturnNotStarted:        for stop request for a connected client for whom asynchronous notifications are currently disabled.
 * @start:  start = 1 or stop = 0 asynchronous notifications
 * @reserved: reserved
 */
struct PACKED MEIBusNotificationResponse {
    UInt8 cmd;
    UInt8 me_addr;
    UInt8 host_addr;
    MEIHostBusMessageReturnType status;
    UInt8 start;
    UInt8 reserved[3];
};

/**
 * struct MEIBusDMAMemDesc - dma ring
 *
 * @addr_hi: the high 32bits of 64 bit address
 * @addr_lo: the low  32bits of 64 bit address
 * @size   : size in bytes (must be power of 2)
 */
struct PACKED MEIBusDMAInfo {
    UInt32 addr_hi;
    UInt32 addr_lo;
    UInt32 size;
};

#define DMA_BIT_MASK(n)    (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))

// SHOULD be power of 2
#define MEI_DMA_RING_HOST_SIZE      0x00020000  // 128k
#define MEI_DMA_RING_DEVICE_SIZE    0x00020000
#define MEI_DMA_RING_CTRL_SIZE      PAGE_SIZE   // 4k

enum MEIDMADescriptor {
    MEIDMADescriptorHost = 0,
    MEIDMADescriptorDevice = 1,
    MEIDMADescriptorControl = 2,
    MEIDMADescriptorSize,
};

/**
 * struct MEIBusDMASetupRequest - dma setup request
 *
 * @cmd: bus message command ID
 * @reserved: reserved for alignment
 * @dma_dscr: dma descriptor for HOST, DEVICE, and CTRL
 */
struct PACKED MEIBusDMASetupRequest {
    UInt8 cmd;
    UInt8 reserved[3];
    MEIBusDMAInfo dma_info[MEIDMADescriptorSize];
};

/**
 * struct hbm_dma_setup_response - dma setup response
 *
 * @cmd: bus message command ID
 * @status: 0 on success; otherwise DMA setup failed.
 * @reserved: reserved for alignment
 */
struct PACKED MEIBusDMASetupResponse {
    UInt8 cmd;
    MEIHostBusMessageReturnType status;
    UInt8 reserved[2];
};

/**
 * struct MEIBusDMARingControl - dma ring control block
 *
 * @hbuf_wr_idx: host circular buffer write index in slots
 * @reserved1: reserved for alignment
 * @hbuf_rd_idx: host circular buffer read index in slots
 * @reserved2: reserved for alignment
 * @dbuf_wr_idx: device circular buffer write index in slots
 * @reserved3: reserved for alignment
 * @dbuf_rd_idx: device circular buffer read index in slots
 * @reserved4: reserved for alignment
 */
struct PACKED MEIBusDMARingControl {
    UInt32 hbuf_wr_idx;
    UInt32 reserved1;
    UInt32 hbuf_rd_idx;
    UInt32 reserved2;
    UInt32 dbuf_wr_idx;
    UInt32 reserved3;
    UInt32 dbuf_rd_idx;
    UInt32 reserved4;
};

/* virtual tag supported */
#define MEI_HBM_CAP_VTAG        BIT(0)
/* client dma supported */
#define MEI_HBM_CAP_CLIENT_DMA  BIT(2)

/**
 * struct MEIBusCapabilityRequest - capability request from host to fw
 *
 * @cmd : bus message command ID
 * @capability_requested: bitmask of capabilities requested by host
 */
struct PACKED MEIBusCapabilityRequest {
    UInt8 cmd;
    UInt8 capability_requested[3];
};

/**
 * struct MEIBusCapabilityResponse - capability response from fw to host
 *
 * @cmd : bus message command ID
 * @capability_granted: bitmask of capabilities granted by FW
 */
struct PACKED MEIBusCapabilityResponse {
    UInt8 cmd;
    UInt8 capability_granted[3];
};

/**
 * struct MEIBusClientDMAMapRequest - client dma map request from host to fw
 *
 * @cmd: bus message command ID
 * @client_buffer_id: client buffer id
 * @reserved: reserved
 * @address_lsb: DMA address LSB
 * @address_msb: DMA address MSB
 * @size: DMA size
 */
struct PACKED MEIBusClientDMAMapRequest {
    UInt8 cmd;
    UInt8 client_buffer_id;
    UInt8 reserved[2];
    UInt32 address_lsb;
    UInt32 address_msb;
    UInt32 size;
};

/**
 * struct MEIBusClientDMAUnmapRequest - client dma unmap request from the host to the fw
 *
 * @cmd: bus message command ID
 * @status: unmap status
 * @client_buffer_id: client buffer id
 * @reserved: reserved
 */
struct PACKED MEIBusClientDMAUnmapRequest {
    UInt8 cmd;
    UInt8 status;
    UInt8 client_buffer_id;
    UInt8 reserved;
};

/**
 * struct MEIBusClientDMAResponse - client dma map/unmap response from the fw to the host
 *
 * @cmd: bus message command ID
 * @status: command status
 */
struct PACKED MEIBusClientDMAResponse {
    UInt8 cmd;
    MEIClientConnectionStatus status;
};

#endif /* MEIProtocol_h */
