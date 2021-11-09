//
//  BatteryManagerState.h
//  SurfaceBattery
//
//  Copyright © 2018 usersse2. All rights reserved.
//

#ifndef BatteryManagerState_hpp
#define BatteryManagerState_hpp

/**
 *  Aggregated battery information
 */
struct BatteryInfo {
	/**
	 *  Unknown value (according to ACPI, -1)
	 */
	static constexpr UInt32 ValueUnknown = 0xFFFFFFFF;

	/**
	 *  Maximum integer value (according to ACPI)
	 */
	static constexpr UInt32 ValueMax = 0x80000000;

	/**
	 *  Must be no more than kSMBusMaximumDataSize
	 */
	static const size_t MaxStringLen = 32;

	/**
	 *  Default voltage to assume
	 */
	static constexpr UInt32 DummyVoltage = 12000;

	/**
	 *  Smaller state substruct for quick updates
	 */
	struct State {
		UInt32 state {0};
		UInt32 presentVoltage {0};
		UInt32 presentRate {0};
		UInt32 averageRate {0};
		UInt32 remainingCapacity {0};
		UInt32 lastFullChargeCapacity {0};
		UInt32 lastRemainingCapacity {0};
		UInt32 designVoltage {0};
		UInt32 runTimeToEmpty {0};
		UInt32 averageTimeToEmpty {0};
		UInt32 averageTimeToEmptyHW {0};
		SInt32 signedPresentRate {0};
		SInt32 signedAverageRate {0};
		SInt32 signedAverageRateHW {0};
		UInt32 timeToFull {0};
		UInt32 timeToFullHW {0};
		UInt8 chargeLevel {0};
		UInt32 designCapacityWarning {0};
		UInt32 designCapacityLow {0};
		UInt16 temperatureDecikelvin {2931};
		UInt16 chargingCurrent {0};
		UInt16 chargingVoltage {8000};
		double temperature {0};
		bool powerUnitIsWatt {false};
		bool calculatedACAdapterConnected {false};
		bool bad {false};
		bool bogus {false};
		bool critical {false};
		bool batteryIsFull {true};
		bool publishTemperatureKey {false};
        AbsoluteTime lastUpdateTime {0};
	};

	/**
	 *  Battery manufacturer data
	 *  All fields are big endian numeric values.
	 *  While such values are not available from ACPI directly, we are faking
	 *  them by leaving them as 0, as seen in System Profiler.
	 */
	struct BatteryManufacturerData {
		UInt16 PackLotCode {0};
		UInt16 PCBLotCode {0};
		UInt16 FirmwareVersion {0};
		UInt16 HardwareVersion {0};
		UInt16 BatteryVersion {0};
	};

	/**
	 *  Complete battery information
	 */
	State state {};
	UInt16 manufactureDate {0};
	UInt32 designCapacity {0};
	UInt32 technology {0};
	UInt32 cycle {0};
	char deviceName[MaxStringLen] {};
	char serial[MaxStringLen] {};
	char batteryType[MaxStringLen] {};
	char manufacturer[MaxStringLen] {};
	bool connected {false};
	BatteryManufacturerData batteryManufacturerData {};

	/**
	 *  Validate battery information and set the defaults
	 *
	 *  @param id        battery id
	 */
	void validateData(SInt32 id=-1);
};

/**
 *  Aggregated adapter information
 */
struct ACAdapterInfo {
	bool connected {false};
};

/**
 *  Aggregated battery manager state
 */
struct BatteryManagerState {
	static constexpr UInt8 MaxBatteriesSupported = 4;
	static constexpr UInt8 MaxAcAdaptersSupported = 2;
	BatteryInfo btInfo[MaxBatteriesSupported] {};
	ACAdapterInfo acInfo[MaxAcAdaptersSupported] {};
};

#endif /* BatteryManagerState_hpp */
