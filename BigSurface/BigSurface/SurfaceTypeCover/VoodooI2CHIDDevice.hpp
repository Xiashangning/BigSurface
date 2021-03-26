//
//  VoodooI2CHIDDevice.hpp
//  VoodooI2CHID
//
//  Created by Alexandre on 25/08/2017.
//  Copyright © 2017 Alexandre Daoud. All rights reserved.
//

#ifndef VoodooI2CHIDDevice_hpp
#define VoodooI2CHIDDevice_hpp

#include <IOKit/IOLib.h>
#include <IOKit/IOKitKeys.h>
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include "../../../Dependencies/helpers.hpp"

#define INTERRUPT_SIMULATOR_TIMEOUT 5
#define INTERRUPT_SIMULATOR_TIMEOUT_BUSY 2
#define INTERRUPT_SIMULATOR_TIMEOUT_IDLE 50

#define I2C_HID_PWR_ON  0x00
#define I2C_HID_PWR_SLEEP 0x01


#define I2C_HID_QUIRK_NO_IRQ_AFTER_RESET    BIT(1)
#define I2C_HID_QUIRK_RESET_ON_RESUME       BIT(5)

#define HID_ANY_ID  0xFFFF

#define USB_VENDOR_ID_ALPS_JP   0x044E

#define I2C_VENDOR_ID_HANTICK       0x0911
#define I2C_PRODUCT_ID_HANTICK_5288 0x5288

#define I2C_VENDOR_ID_RAYDIUM       0x2386
#define I2C_PRODUCT_ID_RAYDIUM_3118 0x3118

#define I2C_VENDOR_ID_SYNAPTICS             0x06cb
#define I2C_PRODUCT_ID_SYNAPTICS_SYNA2393   0x7a13


#define EXPORT __attribute__((visibility("default")))

typedef union {
    UInt8 data[4];
    struct __attribute__((__packed__)) cmd {
        UInt16 reg;
        UInt8 report_type_id;
        UInt8 opcode;
    } c;
} VoodooI2CHIDDeviceCommand;

typedef struct __attribute__((__packed__)) {
    UInt16 wHIDDescLength;
    UInt16 bcdVersion;
    UInt16 wReportDescLength;
    UInt16 wReportDescRegister;
    UInt16 wInputRegister;
    UInt16 wMaxInputLength;
    UInt16 wOutputRegister;
    UInt16 wMaxOutputLength;
    UInt16 wCommandRegister;
    UInt16 wDataRegister;
    UInt16 wVendorID;
    UInt16 wProductID;
    UInt16 wVersionID;
    UInt32 reserved;
} VoodooI2CHIDDeviceHIDDescriptor;

static const struct i2c_hid_quirks {
    UInt16 idVendor;
    UInt16 idProduct;
    UInt32 quirks;
} i2c_hid_quirks[] = {
    { I2C_VENDOR_ID_HANTICK, I2C_PRODUCT_ID_HANTICK_5288,
        I2C_HID_QUIRK_NO_IRQ_AFTER_RESET },
    { I2C_VENDOR_ID_RAYDIUM, I2C_PRODUCT_ID_RAYDIUM_3118,
        I2C_HID_QUIRK_NO_IRQ_AFTER_RESET },
    { USB_VENDOR_ID_ALPS_JP, HID_ANY_ID,
        I2C_HID_QUIRK_RESET_ON_RESUME },
    { I2C_VENDOR_ID_SYNAPTICS, I2C_PRODUCT_ID_SYNAPTICS_SYNA2393,
        I2C_HID_QUIRK_RESET_ON_RESUME },
    { 0, 0 }
};

class VoodooI2CDeviceNub;

/* Implements an I2C-HID device as specified by Microsoft's protocol in the following document: http://download.microsoft.com/download/7/D/D/7DD44BB7-2A7A-4505-AC1C-7227D3D96D5B/hid-over-i2c-protocol-spec-v1-0.docx
 *
 * The members of this class are responsible for issuing I2C-HID commands via the device API as well as interacting with OS X's HID stack.
 */


class EXPORT VoodooI2CHIDDevice : public IOHIDDevice {
  OSDeclareDefaultStructors(VoodooI2CHIDDevice);

 public:
    const char* name;

    /* Initialises a <VoodooI2CHIDDevice> object
     * @properties Contains the properties of the matched provider
     *
     * This is the first function called during the load routine and
     * allocates the memory needed for a <VoodooI2CHIDDevice> object.
     *
     * @return *true* upon successful initialisation, *false* otherwise
     */

    bool init(OSDictionary* properties) override;

    /* Frees the <VoodooI2CHIDDevice> object
     *
     * This is the last function called during the unload routine and
     * frees the memory allocated in <init>.
     */

    void free() override;

    /*
     * Issues an I2C-HID command to get the HID descriptor from the device.
     *
     * @return *kIOReturnSuccess* on sucessfully getting the HID descriptor, *kIOReturnIOError* if the request failed, *kIOReturnInvalid* if the descriptor is invalid
     */
    virtual IOReturn getHIDDescriptor();

    /*
     * Gets the HID descriptor address by evaluating the device's '_DSM' method in the ACPI tables
     *
     * @return *kIOReturnSuccess* on sucessfully getting the HID descriptor address, *kIOReturnNotFound* if neither the '_DSM' method nor the '_XDSM' was found, *kIOReturnInvalid* if the address is invalid
     */

    IOReturn getHIDDescriptorAddress();
    
    IOReturn getReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;
    
    IOReturn parseHIDDescriptor();

    /* Probes the candidate I2C-HID device to see if this driver can indeed drive it
     * @provider The provider which we have matched against
     * @score    Probe score as specified in the matched personality
     *
     * This function is responsible for querying the ACPI tables for the HID descriptor address and then
     * requesting the HID descriptor from the device.
     *
     * @return A pointer to this instance of VoodooI2CHID upon succesful probe, else NULL
     */

    VoodooI2CHIDDevice* probe(IOService* provider, SInt32* score) override;

    /* Run during the <IOHIDDevice::start> routine
     * @provider The provider which we have matched against
     *
     * We override <IOHIDDevice::handleStart> in order to allocate the work loop and resources
     * so that <IOHIDDevice> code is run synchronously with <VoodooI2CHID> code.
     *
     * @return *true* upon successful start, *false* otherwise
     */

    bool handleStart(IOService* provider) override;
    
    void simulateInterrupt(OSObject* owner, IOTimerEventSource* timer);
    
    /* Sets a few properties that are needed after <IOHIDDevice> finishes starting
     * @provider The provider which we have matched against
     *
     * @return *true* upon successful start, *false* otherwise
     */
    
    bool start(IOService* provider) override;

    /* Stops the I2C-HID Device
     * @provider The provider which we have matched against
     *
     * This function is called before <free> and is responsible for deallocating the resources
     * that were allocated in <start>.
     */

    void stop(IOService* provider) override;

    /* Create and return a new memory descriptor that describes the report descriptor for the HID device
     * @descriptor Pointer to the memory descriptor returned. This memory descriptor will be released by the caller.
     *
     * @return *kIOReturnSuccess* on success, or an error return otherwise.
     */

    IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;

    /* Returns a number object that describes the vendor ID of the HID device.
     *
     * @return A number object. The caller must decrement the retain count on the object returned.
     */

    OSNumber* newVendorIDNumber() const override;

    /* Returns a number object that describes the product ID of the HID device.
     *
     * @return A number object. The caller must decrement the retain count on the object returned.
     */

    OSNumber* newProductIDNumber() const override;

    /* Returns a number object that describes the version number of the HID device.
     *
     * @return A number object. The caller must decrement the retain count on the object returned.
     */

    OSNumber* newVersionNumber() const override;

    /* Returns a string object that describes the transport layer used by the HID device.
     *
     * @return A string object. The caller must decrement the retain count on the object returned.
     */

    OSString* newTransportString() const override;

    /* Returns a string object that describes the manufacturer of the HID device.
     *
     * @return A string object. The caller must decrement the retain count on the object returned.
     */

    OSString* newManufacturerString() const override;
    
    bool open(IOService *forClient, IOOptionBits options = 0, void *arg = 0) override;
    void close(IOService *forClient, IOOptionBits options) override;

 protected:
    bool awake;
    IOWorkLoop* work_loop;
    
    IOLock* client_lock;
    OSArray* clients;

    VoodooI2CHIDDeviceHIDDescriptor hid_descriptor;

    IOReturn resetHIDDeviceGated();

    /* Issues an I2C-HID reset command.
     *
     * @return *kIOReturnSuccess* on successful reset, *kIOReturnTimeout* otherwise
     */

    IOReturn resetHIDDevice();

    /* Issues an I2C-HID power state command.
     * @state The power state that the device should enter
     *
     * @return *kIOReturnSuccess* on successful power state change, *kIOReturnTimeout* otherwise
     */

    IOReturn setHIDPowerState(VoodooI2CState state);

    /* Called by the OS's power management stack to notify the driver that it should change the power state of the deice
     * @whichState The power state the device is expected to enter represented by either
     *  *kIOPMPowerOn* or *kIOPMPowerOff*
     * @whatDevice The power management policy maker
     *
     *
     * @return *kIOPMAckImplied* on succesful state change, *kIOReturnError* otherwise
     */

    IOReturn setPowerState(unsigned long whichState, IOService* whatDevice) override;

    /* Issues an I2C-HID set report command.
     * @report The report data to be sent to the device
     * @reportType The type of HID report to be sent
     * @options Options for the report, the first two bytes are the report ID
     *
     * @return *kIOReturnSuccess* on successful power state change, *kIOReturnTimeout* otherwise
     */

    IOReturn setReport(IOMemoryDescriptor* report, IOHIDReportType reportType, IOOptionBits options) override;

    /*
     * Lookup and set any quirks associated with the I2C HID device.
     */

    void lookupQuirks();

 private:
    IOACPIPlatformDevice* acpi_device;
    VoodooI2CDeviceNub* api;
    IOCommandGate* command_gate;
    UInt16 hid_descriptor_register;
    IOTimerEventSource* interrupt_simulator;
    IOInterruptEventSource* interrupt_source;
    bool ready_for_input;
    bool* reset_event;
    bool is_interrupt_started = false;
    UInt32 quirks = 0;

    /* Queries the I2C-HID device for an input report
     *
     * This function is called from the interrupt handler in a new thread. It is thus not called from interrupt context.
     */

    void getInputReport();

    /*
    * This function is called when the I2C-HID device asserts its interrupt line.
    */
    
    void interruptOccured(OSObject* owner, IOInterruptEventSource* src, int intCount);

    /* Releases resources allocated in <start>
     *
     * This function is called during a graceful exit from <start> and during
     * execution of <stop> in order to release resources retained by <start>.
     */

    void releaseResources();

    void startInterrupt();

    void stopInterrupt();
};


#endif /* VoodooI2CHIDDevice_hpp */
