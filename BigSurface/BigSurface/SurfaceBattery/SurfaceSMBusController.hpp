//
//  SurfaceSMBusController.hpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#ifndef SurfaceSMBusController_hpp
#define SurfaceSMBusController_hpp

#include <IOKit/IOService.h>
#include <IOKit/IOSMBusController.h>
#include <IOKit/battery/AppleSmartBatteryCommands.h>

#include "../../../Dependencies/VoodooSerial/VoodooSerial/utils/helpers.hpp"
#include "BatteryManager.hpp"

class EXPORT SurfaceSMBusController : public IOSMBusController {
	OSDeclareDefaultStructors(SurfaceSMBusController)

	/**
	 *  This is here to avoid collisions in case of future expansion of IOSMBusController class.
	 *  WARNING! It is important that this is the first member of the class!
	 */
	void *reserved[8] {};

	/**
	 Set received data for a transaction to a 16-bit integer value

	 @param transaction SMBus transaction
	 @param valueToWrite Value to write
	 */
	static void setReceiveData(IOSMBusTransaction *transaction, UInt16 valueToWrite);

	/**
	 *  Sensible defaults for timer probing and rechecking the new commands.
	 *  Note: when it was 1000, after extending the code it failed to read all data in 10 seconds,
	 *  so AppleSmartBatteryManager aborted polling.
	 */
	static constexpr UInt32 TimerTimeoutMs = 100;

	/**
	 *  A workloop in charge of handling timer events with requests.
	 */
	IOWorkLoop *workLoop {nullptr};

	/**
	 *  Timer event source for processing requests.
	 */
	IOTimerEventSource *timer {nullptr};

	/**
	 *  Registered SMBus requests.
	 */
	OSArray *requestQueue {nullptr};

	/**
	 *  Create battery date in AppleSmartBattery format
	 *
	 *  @param day     manufacturing date
	 *  @param month   manufacturing month
	 *  @param year    manufacturing year
	 *
	 *  @return date in AppleSmartBattery format
	 */
	static constexpr UInt16 makeBatteryDate(UInt16 day, UInt16 month, UInt16 year) {
		return (day & 0x1FU) | ((month & 0xFU) << 5U) | (((year - 1980U) & 0x7FU) << 9U);
	}

public:	
	/**
	 *  Start the driver by setting up the workloop and preparing the matching.
	 *
	 *  @param provider  parent IOService object
	 *
	 *  @return true on success
	 */
	bool start(IOService *provider) override;

	/**
	 *  Handle external notification from Surface battery instance
	 *
	 *  @param target  SurfaceSMBusController instance
	 *
	 *  @return kIOReturnSuccess
	 */
	static IOReturn handleACPINotification(void *target);
	
protected:
	/**
	 *  Returns the current work loop.
	 *  We should properly override this, as our provider has no work loop.
	 *
	 *  @return current workloop for request handling.
	 */
	IOWorkLoop *getWorkLoop() const override {
		return workLoop;
	}

	/**
	 *  Request handling routine used for handling battery commands.
	 *
	 *  @param request request to handle
	 *
	 *  @return request handling result.
	 */
	IOSMBusStatus startRequest(IOSMBusRequest *request) override;

	/**
	 *  Timer event used to handled queued AppleSmartBatteryManager SMBus requests.
	 */
	void handleBatteryCommandsEvent();
	
	/**
	 *  Holds value of batteriesConnected() when the previous notification was sent, must be guarded by stateLock
	 *
	 *  @return true on success
	 */
	bool prevBatteriesConnected {false};

	/**
	 *  Holds value of adaptersConnected() when the previous notification was sent, must be guarded by stateLock
	 *
	 *  @return true on success
	 */
	bool prevAdaptersConnected {false};
};

#endif /* SurfaceSMBusController_hpp */
