//
//  IPTSKenerlUserShared.h
//  SurfaceTouchScreen
//
//  Created by Xavier on 2022/6/11.
//  Copyright © 2022 Xia Shangning. All rights reserved.
//

#ifndef IPTSKenerlUserShared_h
#define IPTSKenerlUserShared_h

#include <IOKit/IOTypes.h>

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif

/*
 * The amount of buffers that is used for IPTS
 */
#define IPTS_BUFFER_NUM 16

#define IPTS_DATA_TYPE_PAYLOAD      0x0
#define IPTS_DATA_TYPE_ERROR        0x1
#define IPTS_DATA_TYPE_VENDOR_DATA  0x2
#define IPTS_DATA_TYPE_HID_REPORT   0x3
#define IPTS_DATA_TYPE_GET_FEATURES 0x4

#define IPTS_PAYLOAD_FRAME_TYPE_STYLUS  0x6
#define IPTS_PAYLOAD_FRAME_TYPE_HEATMAP 0x8

#define IPTS_REPORT_TYPE_START 0
#define IPTS_REPORT_TYPE_END   0xff

#define IPTS_CONTAINER_TYPE_ROOT    0x00
#define IPTS_CONTAINER_TYPE_HEATMAP 0x01
#define IPTS_CONTAINER_TYPE_REPORT  0xff

#define IPTS_REPORT_TYPE_HEATMAP_DIM  0x03
#define IPTS_REPORT_TYPE_HEATMAP      0x25
#define IPTS_REPORT_TYPE_STYLUS_V1    0x10
#define IPTS_REPORT_TYPE_STYLUS_V2    0x60

#define IPTS_REPORT_TYPE_FREQUENCY_NOISE          0x04
#define IPTS_REPORT_TYPE_PEN_GENERAL              0x57
#define IPTS_REPORT_TYPE_PEN_JNR_OUTPUT           0x58
#define IPTS_REPORT_TYPE_PEN_NOISE_METRICS_OUTPUT 0x59
#define IPTS_REPORT_TYPE_PEN_DATA_SELECTION       0x5a
#define IPTS_REPORT_TYPE_PEN_MAGNITUDE            0x5b
#define IPTS_REPORT_TYPE_PEN_DFT_WINDOW           0x5c
#define IPTS_REPORT_TYPE_PEN_MULTIPLE_REGION      0x5d
#define IPTS_REPORT_TYPE_PEN_TOUCHED_ANTENNAS     0x5e
#define IPTS_REPORT_TYPE_PEN_METADATA             0x5f
#define IPTS_REPORT_TYPE_PEN_DETECTION            0x62
#define IPTS_REPORT_TYPE_PEN_LIFT                 0x63

#define IPTS_STYLUS_REPORT_MODE_BIT_PROXIMITY   0
#define IPTS_STYLUS_REPORT_MODE_BIT_CONTACT     1
#define IPTS_STYLUS_REPORT_MODE_BIT_BUTTON      2
#define IPTS_STYLUS_REPORT_MODE_BIT_RUBBER      3

//FIXME: should get these report IDs by parsing report descriptor and looking for digitizer usage 0x61
#define IPTS_HID_REPORT_IS_CONTAINER(x) (x==7 || x==8 || x==10 || x==11 || x==12 || x==13 || x==26 || x==28)

#define IPTS_HID_REPORT_SINGLETOUCH 0x40
#define IPTS_SINGLETOUCH_MAX_VALUE (1 << 15)

#define IPTS_MAX_X          9600
#define IPTS_MAX_Y          7200
#define IPTS_DIAGONAL       12000
#define IPTS_MAX_PRESSURE   4096

#define IPTS_DFT_NUM_COMPONENTS   9
#define IPTS_DFT_MAX_ROWS         16
#define IPTS_DFT_PRESSURE_ROWS    6

#define IPTS_DFT_ID_POSITION      6
#define IPTS_DFT_ID_BUTTON        9
#define IPTS_DFT_ID_PRESSURE      11

struct PACKED IPTSDataHeader {
    UInt32 type;
    UInt32 size;
    UInt32 buffer;
    UInt8  reserved[52];
};

struct PACKED IPTSPayloadHeader {
    UInt32 counter;
    UInt32 frames;
    UInt8  reserved[4];
};

struct PACKED IPTSPayloadFrame {
    UInt16 index;
    UInt16 type;
    UInt32 size;
    UInt8  reserved[8];
};

struct PACKED IPTSReportHeader {
    UInt8  type;
    UInt8  flags;
    UInt16 size;
};

struct PACKED IPTSStylusReportHeader {
    UInt8  elements;
    UInt8  reserved[3];
    UInt32 serial;
};

struct PACKED IPTSStylusReportV1 {
    UInt8  reserved[4];
    UInt8  mode;
    UInt16 x;
    UInt16 y;
    UInt16 pressure;
    UInt8  reserved2;
};

struct PACKED IPTSStylusReportV2 {
    UInt16 timestamp;
    UInt16 mode;
    UInt16 x;
    UInt16 y;
    UInt16 pressure;
    UInt16 altitude;
    UInt16 azimuth;
    UInt8  reserved[2];
};

struct PACKED IPTSStylusHIDReport {
    UInt8 in_range:1;
    UInt8 touch:1;
    UInt8 side_button:1;
    UInt8 inverted:1;
    UInt8 eraser:1;
    UInt8 reserved:3;
    UInt16 x;
    UInt16 y;
    UInt16 tip_pressure;
    UInt16 x_tilt;
    UInt16 y_tilt;
    UInt16 scan_time;
};

#define IPTS_TOUCH_SCREEN_FINGER_CNT    10

struct PACKED IPTSFingerReport {
    UInt8 touch:1;
    UInt8 contact_id:7;
    UInt16 x;
    UInt16 y;
};

struct PACKED IPTSTouchHIDReport {
    IPTSFingerReport fingers[IPTS_TOUCH_SCREEN_FINGER_CNT];
    UInt8 contact_num;
};

struct PACKED IPTSHeatmapDimension {
    UInt8 height;
    UInt8 width;
    UInt8 y_min;
    UInt8 y_max;
    UInt8 x_min;
    UInt8 x_max;
    UInt8 z_min;
    UInt8 z_max;
};

struct PACKED IPTSReportStart {
    UInt8 reserved[2];
    UInt16 count;
    UInt32 timestamp;
};

struct PACKED IPTSReportContainer {
    UInt32 size;
    UInt8 zero;    // always zero
    UInt8 type;
    UInt8 unknown; // 1 for heatmap container, 0 for other containers
};

struct PACKED IPTSReportHeatmap {
    UInt8 unknown1;  // always 8
    UInt32 unknown2; // always 0
    UInt32 size;
};

struct PACKED IPTSStylusMagnitudeData {
    UInt8 unknown1[2]; // always zero
    UInt8 unknown2[2]; // 0 if pen not near screen, 1 or 2 if pen is near screen
    UInt8 flags;       // 0, 1 or 8 (bitflags?)
    UInt8 unknown3[3]; // always 0xff (padding?)
    UInt32 x[64];
    UInt32 y[44];
};

struct PACKED IPTSStylusDFTWindow {
    UInt32 timestamp; // counting at approx 8MHz
    UInt8 num_rows;
    UInt8 seq_num;
    UInt8 unknown1;   // usually 1, can be 0 if there are simultaneous touch events
    UInt8 unknown2;   // usually 1, can be 0 if there are simultaneous touch events
    UInt8 unknown3;   // usually 1, but can be higher (2,3,4) for the first few packets of a pen interaction
    UInt8 data_type;
    UInt16 unknown4;  // always 0xffff (padding?)
};

struct PACKED IPTSStylusDFTWindowRow {
    UInt32 frequency;
    UInt32 magnitude;
    SInt16 real[IPTS_DFT_NUM_COMPONENTS];
    SInt16 imag[IPTS_DFT_NUM_COMPONENTS];
    SInt8  first;
    SInt8  last;
    SInt8  mid;
    SInt8  zero;
};

#define IPTS_TOUCH_REPORT_ID    0x40
#define IPTS_STYLUS_REPORT_ID   0x01

struct PACKED IPTSHIDReport {
    UInt8 report_id;
    union {
        IPTSTouchHIDReport  touch;
        IPTSStylusHIDReport stylus;
    } data;
};

struct PACKED IPTSDeviceInfo {
    UInt16 vendor_id;
    UInt16 product_id;
    UInt8  max_contacts;
};

enum {
    kMethodGetDeviceInfo,
    kMethodReceiveInput,
    kMethodSendHIDReport,
    
    kNumberOfMethods
};

enum {
    kIPTSNumberOfMemoryType = IPTS_BUFFER_NUM,
};

#endif /* IPTSKenerlUserShared_h */
