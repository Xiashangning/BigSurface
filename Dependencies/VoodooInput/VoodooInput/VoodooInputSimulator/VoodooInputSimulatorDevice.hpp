//
//  VoodooInputSimulatorDevice.hpp
//  VoodooInput
//
//  Copyright © 2019 Alexandre Daoud and Kishor Prins. All rights reserved.
//

#ifndef VOODOO_SIMULATOR_DEVICE_HPP
#define VOODOO_SIMULATOR_DEVICE_HPP

#include <IOKit/IOService.h>
#include <IOKit/hid/IOHIDDevice.h>

#include <kern/clock.h>

#include "../VoodooInput.hpp"
#include "../VoodooInputMultitouch/VoodooInputTransducer.h"
#include "../VoodooInputMultitouch/VoodooInputEvent.h"
#include "../VoodooInputMultitouch/MultitouchHelpers.h"

#ifndef EXPORT
#define EXPORT __attribute__((visibility("default")))
#endif

#define MT2_MAX_X 8134
#define MT2_MAX_Y 5206

/* State bits reference: linux/drivers/hid/hid-magicmouse.c#L58-L63 */
#define MT2_TOUCH_STATE_BIT_TRANSITION (0x1)
#define MT2_TOUCH_STATE_BIT_NEAR (0x1 << 1)
#define MT2_TOUCH_STATE_BIT_CONTACT (0x1 << 2)

enum TouchStates {
    kTouchStateInactive = 0x0,
    kTouchStateStart = MT2_TOUCH_STATE_BIT_NEAR | MT2_TOUCH_STATE_BIT_TRANSITION,
    kTouchStateActive = MT2_TOUCH_STATE_BIT_CONTACT,
    kTouchStateStop = MT2_TOUCH_STATE_BIT_CONTACT | MT2_TOUCH_STATE_BIT_NEAR | MT2_TOUCH_STATE_BIT_TRANSITION
};

/* Finger Packet
+---+---+---+---+---+---+---+---+---+
|   | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
+---+---+---+---+---+---+---+---+---+
| 0 |           x: SInt13           |
+---+-----------+                   +
| 1 |           |                   |
+---+           +-------------------+
| 2 |           y: SInt13           |
+---+-----------+-----------+       +
| 3 |   state   |   finger  |       |
|   |   UInt3   |   UInt3   |       |
+---+-----------+-----------+-------+
| 4 |       touchMajor: UInt8       |
+---+-------------------------------+
| 5 |       touchMinor: UInt8       |
+---+-------------------------------+
| 6 |          size: UInt8          |
+---+-------------------------------+
| 7 |        pressure: UInt8        |
+---+-----------+---+---------------+
| 8 |   angle   | 0 |    touchID    |
|   |   UInt3   |   |     UInt4     |
+---+-----------+---+---------------+
*/
struct __attribute__((__packed__)) MAGIC_TRACKPAD_INPUT_REPORT_FINGER {
    SInt16 X: 13;
    SInt16 Y: 13;
    UInt8 Finger: 3;
    UInt8 State: 3;
    UInt8 Touch_Major;
    UInt8 Touch_Minor;
    UInt8 Size;
    UInt8 Pressure;
    UInt8 Identifier: 4;
    UInt8 : 1;
    UInt8 Angle: 3;
};

struct __attribute__((__packed__)) MAGIC_TRACKPAD_INPUT_REPORT {
    UInt8 ReportID;
    UInt8 Button;
    UInt8 Unused[5];
    
    UInt8 TouchActive;
    
    UInt8 multitouch_report_id;
    UInt8 timestamp_buffer[3];
    
    MAGIC_TRACKPAD_INPUT_REPORT_FINGER FINGERS[]; // May support more fingers
};

static_assert(sizeof(MAGIC_TRACKPAD_INPUT_REPORT) == 12, "Unexpected MAGIC_TRACKPAD_INPUT_REPORT size");
static_assert(sizeof(MAGIC_TRACKPAD_INPUT_REPORT_FINGER) == 9, "Unexpected MAGIC_TRACKPAD_INPUT_REPORT_FINGER size");

class EXPORT VoodooInputSimulatorDevice : public IOHIDDevice {
    OSDeclareDefaultStructors(VoodooInputSimulatorDevice);
    
public:
    void constructReport(const VoodooInputEvent& multitouch_event);

    IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;

    IOReturn getReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;
    IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;

    OSNumber* newVendorIDNumber() const override;
    OSNumber* newProductIDNumber() const override;
    OSNumber* newVersionNumber() const override;
    OSString* newTransportString() const override;
    OSString* newManufacturerString() const override;
    OSNumber* newPrimaryUsageNumber() const override;
    OSNumber* newPrimaryUsagePageNumber() const override;
    OSString* newProductString() const override;
    OSString* newSerialNumberString() const override;
    OSNumber* newLocationIDNumber() const override;

    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;

    bool start(IOService* provider) override;
    void stop(IOService* provider) override;
    void releaseResources();

private:
    bool ready_for_reports {false};
    VoodooInput* engine {nullptr};
    AbsoluteTime start_timestamp {};
    OSData* new_get_report_buffer {nullptr};
    bool touch_active[15] {false};
    IOWorkLoop* work_loop {nullptr};
    IOCommandGate* command_gate {nullptr};
    IOBufferMemoryDescriptor* input_report_buffer {nullptr};
    MAGIC_TRACKPAD_INPUT_REPORT* input_report {nullptr};

    void sendReport();
    void constructReportGated(const VoodooInputEvent& multitouch_event);
};


#endif
