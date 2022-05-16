//
//  SMCSMBusController.cpp
//  SurfaceBattery
//
//  Copyright © 2018 usrsse2. All rights reserved.
//

#include <stdatomic.h>
#include <libkern/c++/OSContainers.h>
#include <IOKit/IOCatalogue.h>
#include <IOKit/IOTimerEventSource.h>

#include "SurfaceSMBusController.hpp"

#define super IOSMBusController
OSDefineMetaClassAndStructors(SurfaceSMBusController, IOSMBusController)

bool SurfaceSMBusController::start(IOService *provider) {
	workLoop = IOWorkLoop::workLoop();
	if (!workLoop) {
        IOLog("%s::workLoop allocation failed\n", getName());
		return false;
	}

	requestQueue = OSArray::withCapacity(0);
	if (!requestQueue) {
        IOLog("%s::requestQueue allocation failure\n", getName());
		OSSafeReleaseNULL(workLoop);
		return false;
	}
	
	if (!super::start(provider)) {
        IOLog("%s::parent start failed\n", getName());
		OSSafeReleaseNULL(workLoop);
		OSSafeReleaseNULL(requestQueue);
		return false;
	}

	setProperty("IOSMBusSmartBatteryManager", kOSBooleanTrue);
	setProperty("_SBS", 1, 32);
	registerService();

	timer = IOTimerEventSource::timerEventSource(this,
        [](OSObject *owner, IOTimerEventSource *) {
            auto ctrl = OSDynamicCast(SurfaceSMBusController, owner);
            if (ctrl) {
                ctrl->handleBatteryCommandsEvent();
            }
        });

	if (timer) {
		getWorkLoop()->addEventSource(timer);
		timer->setTimeoutMS(TimerTimeoutMs);
	} else {
        IOLog("%s::failed to allocate normal timer event\n", getName());
		OSSafeReleaseNULL(workLoop);
		OSSafeReleaseNULL(requestQueue);
		return false;
	}

	BatteryManager::getShared()->subscribe(handleACPINotification, this);
	
	return true;
}

void SurfaceSMBusController::setReceiveData(IOSMBusTransaction *transaction, UInt16 valueToWrite) {
	transaction->receiveDataCount = sizeof(UInt16);
	UInt16 *ptr = reinterpret_cast<UInt16*>(transaction->receiveData);
	*ptr = valueToWrite;
}

IOSMBusStatus SurfaceSMBusController::startRequest(IOSMBusRequest *request) {
	auto result = IOSMBusController::startRequest(request);
	if (result != kIOSMBusStatusOK)
        return result;
	
	IOSMBusTransaction *transaction = request->transaction;

	if (transaction) {
		transaction->status = kIOSMBusStatusOK;
		UInt32 valueToWrite = 0;
		
		if (transaction->address == kSMBusManagerAddr && transaction->protocol == kIOSMBusProtocolReadWord) {
			switch (transaction->command) {
				case kMStateContCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					valueToWrite = BatteryManager::getShared()->externalPowerConnected() ? kMACPresentBit : 0;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, valueToWrite);
					break;
				}
				case kMStateCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					if (BatteryManager::getShared()->batteriesConnected()) {
						valueToWrite = kMPresentBatt_A_Bit;
						if ((BatteryManager::getShared()->state.btInfo[0].state.state & SurfaceBattery::BSTStateMask) == SurfaceBattery::BSTCharging)
							valueToWrite |= kMChargingBatt_A_Bit;
					}
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, valueToWrite);
					break;
				}
				default: {
					// Nothing should be done here since AppleSmartBattery always calls bzero fo transaction
					// Let's also not set transaction status to error value since it can be an unknown command
					break;
				}
			}
		}

		if (transaction->address == kSMBusBatteryAddr && transaction->protocol == kIOSMBusProtocolReadWord) {
			//FIXME: maybe the incoming data show us which battery is queried about? Or it's in the address?

			switch (transaction->command) {
				case kBBatteryStatusCmd: {
					setReceiveData(transaction, BatteryManager::getShared()->calculateBatteryStatus(0));
					break;
				}
				case kBManufacturerAccessCmd: {
					if (transaction->sendDataCount == 2 &&
						(transaction->sendData[0] == kBExtendedPFStatusCmd ||
						 transaction->sendData[0] == kBExtendedOperationStatusCmd)) {
						// AppleSmartBatteryManager ignores these values.
						setReceiveData(transaction, 0);
					}
					//CHECKME: Should else case be handled?
					break;
				}
				case kBPackReserveCmd:
				case kBDesignCycleCount9CCmd: {
					setReceiveData(transaction, 1000);
					break;
				}
				case kBReadCellVoltage1Cmd:
				case kBReadCellVoltage2Cmd:
				case kBReadCellVoltage3Cmd:
				case kBReadCellVoltage4Cmd:
                case kBVoltageCmd: {
                    IOSimpleLockLock(BatteryManager::getShared()->stateLock);
                    auto value = BatteryManager::getShared()->state.btInfo[0].state.presentVoltage;
                    IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
                    setReceiveData(transaction, value);
                    break;
                }
				case kBCurrentCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.signedPresentRate;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBAverageCurrentCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.signedAverageRate;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBSerialNumberCmd:
				case kBMaxErrorCmd:
					break;
				case kBRunTimeToEmptyCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.runTimeToEmpty;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBAverageTimeToEmptyCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.averageTimeToEmpty;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBTemperatureCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.temperatureDecikelvin;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBDesignCapacityCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].designCapacity;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBCycleCountCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].cycle;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBAverageTimeToFullCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.timeToFull;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBRemainingCapacityCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.remainingCapacity;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBFullChargeCapacityCmd: {
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					auto value = BatteryManager::getShared()->state.btInfo[0].state.lastFullChargeCapacity;
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					setReceiveData(transaction, value);
					break;
				}
				case kBManufactureDateCmd: {
					UInt16 value = 0;
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					if (BatteryManager::getShared()->state.btInfo[0].manufactureDate) {
						value = BatteryManager::getShared()->state.btInfo[0].manufactureDate;
					} else {
						strncpy(reinterpret_cast<char *>(transaction->receiveData), BatteryManager::getShared()->state.btInfo[0].serial, kSMBusMaximumDataSize);
						transaction->receiveData[kSMBusMaximumDataSize-1] = '\0';
					}
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);

					if (!value) {
						const char* p = reinterpret_cast<char *>(transaction->receiveData);
						bool found = false;
						int year = 2016, month = 02, day = 29;
						while ((p = strstr(p, "20")) != nullptr) { // hope that this code will not survive until 22nd century
							if (sscanf(p, "%04d%02d%02d", &year, &month, &day) == 3 || 		// YYYYMMDD (Lenovo)
								sscanf(p, "%04d/%02d/%02d", &year, &month, &day) == 3) {	// YYYY/MM/DD (HP)
								if (1 <= month && month <= 12 && 1 <= day && day <= 31) {
									found = true;
									break;
								}
							}
							p++;
						}
						
						if (!found) { // in case we parsed a non-date
							year = 2016;
							month = 02;
							day = 29;
						}
						value = makeBatteryDate(day, month, year);
					}
					
					setReceiveData(transaction, value);
					break;
				}
				//CHECKME: Should there be a default setting receiveDataCount to 0 or status failure?
			}
		}

		if (transaction->address == kSMBusBatteryAddr &&
			transaction->protocol == kIOSMBusProtocolWriteWord &&
			transaction->command == kBManufacturerAccessCmd) {
			if (transaction->sendDataCount == 2 && (transaction->sendData[0] == kBExtendedPFStatusCmd || transaction->sendData[0] == kBExtendedOperationStatusCmd)) {
				// Nothing can be done here since we don't write any values to hardware, we fake response for this command in handler for
				// kIOSMBusProtocolReadWord/kBManufacturerAccessCmd. Anyway, AppleSmartBattery::transactionCompletion ignores this response.
				// If we can see this log statement, it means that fields sendDataCount and sendData have a proper offset in IOSMBusTransaction
			}
		}

		if (transaction->address == kSMBusBatteryAddr && transaction->protocol == kIOSMBusProtocolReadBlock) {
			switch (transaction->command) {
				case kBManufacturerNameCmd: {
					transaction->receiveDataCount = kSMBusMaximumDataSize;
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					strncpy(reinterpret_cast<char *>(transaction->receiveData), BatteryManager::getShared()->state.btInfo[0].manufacturer, kSMBusMaximumDataSize);
					transaction->receiveData[kSMBusMaximumDataSize-1] = '\0';
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					break;
				}
				case kBAppleHardwareSerialCmd:
					transaction->receiveDataCount = kSMBusMaximumDataSize;
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					strncpy(reinterpret_cast<char *>(transaction->receiveData), BatteryManager::getShared()->state.btInfo[0].serial, kSMBusMaximumDataSize);
					transaction->receiveData[kSMBusMaximumDataSize-1] = '\0';
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					break;
				case kBDeviceNameCmd:
					transaction->receiveDataCount = kSMBusMaximumDataSize;
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					strncpy(reinterpret_cast<char *>(transaction->receiveData), BatteryManager::getShared()->state.btInfo[0].deviceName, kSMBusMaximumDataSize);
					transaction->receiveData[kSMBusMaximumDataSize-1] = '\0';
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					break;
				case kBManufacturerDataCmd:
					transaction->receiveDataCount = sizeof(BatteryInfo::BatteryManufacturerData);
					IOSimpleLockLock(BatteryManager::getShared()->stateLock);
					memcpy(reinterpret_cast<UInt16 *>(transaction->receiveData), &BatteryManager::getShared()->state.btInfo[0].batteryManufacturerData, sizeof(BatteryInfo::BatteryManufacturerData));
					IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);
					break;
				case kBManufacturerInfoCmd:
					break;
				// Let other commands slip if any.
				// receiveDataCount is already 0, and status failure results in retries - it's not what we want.
			}
		}
	}

	if (!requestQueue->setObject(request)) {
        IOLog("%s::startRequest failed to append a request\n", getName());
		return kIOSMBusStatusUnknownFailure;
	}
    
    IOSimpleLockLock(BatteryManager::getShared()->stateLock);
    clock_get_uptime(&BatteryManager::getShared()->lastAccess);
    IOSimpleLockUnlock(BatteryManager::getShared()->stateLock);

	return result;
}

void SurfaceSMBusController::handleBatteryCommandsEvent() {
	timer->setTimeoutMS(TimerTimeoutMs);

	// requestQueue contains objects owned by two parties:
	// 1. requestQueue itself, as the addition of any object retains it
	// 2. IOSMBusController, since it does not release the object after passing it
	// to us in OSMBusController::performTransactionGated.
	// OSArray::removeObject takes away our ownership, and then
	// IOSMBusController::completeRequest releases the request inside.

	while (requestQueue->getCount() != 0) {
		auto request = OSDynamicCast(IOSMBusRequest, requestQueue->getObject(0));
		requestQueue->removeObject(0);
		if (request != nullptr)
			completeRequest(request);
	}
}

IOReturn SurfaceSMBusController::handleACPINotification(void *target) {
	SurfaceSMBusController *self = static_cast<SurfaceSMBusController *>(target);
	if (self) {
		auto &bmgr = *BatteryManager::getShared();
		// TODO: when we have multiple batteries, handle insertion or removal of a single battery
		IOSimpleLockLock(bmgr.stateLock);
		bool batteriesConnected = bmgr.batteriesConnected();
		bool adaptersConnected = bmgr.adaptersConnected();
		self->prevBatteriesConnected = batteriesConnected;
		self->prevAdaptersConnected = adaptersConnected;
		IOSimpleLockUnlock(bmgr.stateLock);
		UInt8 data[] = {kBMessageStatusCmd, batteriesConnected};
		self->messageClients(kIOMessageSMBusAlarm, data, arrsize(data));
		return kIOReturnSuccess;
	}
	return kIOReturnBadArgument;
}
