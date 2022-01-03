# BigSurface
The name comes from macOS Big Sur.
Big Sur + Surface -> Big Surface (LOL)

PS : If you have a better name, please let me know.

**A proposition for a fully intergrated kext for all Surface related hardwares**

## How to install

You will need to first DELETE all the original VoodooI2C series Kext, and then add the kext into opencore's `config.plist` in the order specified as below<img width="932" alt="截屏2022-01-03 下午5 43 49" src="https://user-images.githubusercontent.com/18528518/147917419-bf24e033-d4f4-43b0-b5f0-561b75c34e7c.png">
## What works
- Surface Type Cover
  > The code is based on VoodooI2CHID.kext, but added **integrated and hot pluggable touchpad&keyboard support**.
- Buttons
  > Problem remains to be solved: Power Button fails after wake from sleep (with deep idle enabled). Might need to investigate into Linux code to see if there are resetting codes when wakeup
- Ambient Light Sensor
  > ACPI device name: ACSD, attached under I2C4
  > 
  > You can set baseline of ALI in info.plist, the calculation is `base_ali+ALI value from the sensor`
- Battery status--Surface Serial Hub **Finally works**
  > I have implemented the UART driver as well as MS's SAM module driver. Now it seems to be working fine and so far no problems are found during the test on serval SP7 devices. 
  > 
  > (Other devices **should work** but need some modifications, **post an issue** and I will try my best to help you get it working)
- Performance mode
  > Right now it is set by `PerformanceMode` in `SurfaceBattery` (default), changing it to other values is not observed to have any effects. If you find any difference (fan speed or battery life, please let me know)
  > 
  > It can only be set by changing the plist or using `ioio`
  > We need a userspace software to control it if it actually has something useful.
  
Possible values are:

      State              Value
      
    Recommended          0x01
    
    Battery Saver        0x02 (Only in battery mode)
    
    Better Performance   0x03
    
    Best Performance     0x04
    
## TODO
- Cameras                            Impossible so far
  > ACPI devices: CAMR,CAMF,CAM3(infrared camera)
  > 
  > Corresponding device id: OV8865,OV5693,OV7251
  > 
  > Even Linux failed to drive the cameras on SP7 (IPU4), SP6 and before (IPU3) might be possible but I do not have the device.
- Touch Screen
  > Device id: 0x34E4

## Surface Laptop's keyboard & touchpad
Theoretically works but you need to implement it yourself, check out my battery codes and consult to https://github.com/linux-surface/surface-aggregator-module/blob/master/doc/requests.txt for necessary events registration. In brief, you need to create a virtual keyboard & touchpad IOSerive, register it under `SurfaceSerialHub` and process the returning data(should be in HID format so the only thing needed is to send the data to macOS)
