# BigSurface
The name comes from macOS Big Sur.
Big Sur + Surface -> Big Surface (LOL)

PS : If you have a better name, please let me know.

**A proposition for a fully intergrated kext for all Surface related hardwares**

## How to install

You will need to first DELETE all the original VoodooI2C series Kext, and then add the kext into opencore's `config.plist` in the order specified as below
<img width="407" alt="截屏2021-06-29 下午8 26 08" src="https://user-images.githubusercontent.com/18528518/123798086-6feaca00-d919-11eb-9e87-2fb3d6268cfe.png">

## TODO
- Surface Type Cover                            **Done**
  
  > The code is based on VoodooI2CHID.kext, but added **integrated and hot pluggable touchpad&keyboard support**.
- Battery status--Surface Serial Hub            **Most Important**
  > See https://github.com/linux-surface/surface-aggregator-module.
- Performance mode
  > Depends on Surface Serial Hub driver
- Buttons                            **Done**
  
  > Problem remains to be solved: Power Button fails after wake from sleep (with deep idle enabled). Might need to investigate into Linux code to see if there are resetting codes when wakeup
- Ambient Light Sensor                            **Done**
  > ACPI device name: ACSD, attached under I2C4
  > 
  > Problem remains to be solved: Only works after a hot reboot from Windows, otherwise its readout are around 0-3 (in normal situation: 200-300 in room)
- Cameras                            Impossible so far
  > ACPI devices: CAMR,CAMF,CAM3(infrared camera)
  > 
  > Corresponding device id: OV8865,OV5693,OV7251
  > 
  > Even Linux failed to drive the cameras on SP7 (IPU4), SP6 and before (IPU3) might be possible but I do not have the device.
- Touch Screen
  > Device id: 0x34E4
