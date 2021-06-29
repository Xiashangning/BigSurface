# BigSurface
The name comes from macOS Big Sur.
Big Sur + Surface -> Big Surface (LOL)

PS : If you have a better name, please let me know.

**A proposition for a fully intergrated kext for all Surface Pro related hardwares**

**How to install**

You will need to add the kext into opencore's `config.plist` in the order specified as below
<img width="407" alt="截屏2021-06-29 下午8 26 08" src="https://user-images.githubusercontent.com/18528518/123798086-6feaca00-d919-11eb-9e87-2fb3d6268cfe.png">

## TODO
- Surface Type Cover                            **Done**
  
  > The code is based on VoodooI2CHID.kext, but added **integrated and hot pluggable touchpad&keyboard support**.
- Battery status--Surface Serial Hub            **Most Important**
  > See https://github.com/linux-surface/surface-aggregator-module.
  > To obtain the battery readout, one needs to register the operation handler for _SAN device(called via _SAN.RQST) and send the request to SSH device.
  > SSH is a UART controller, **the low level uart operation(tx, rx)** is needed to be completed. Thus an expert in UART development is needed.
  > Other than that, the rest of the code is not hard to port, mainly how to encode and decode the request packages. 
  >
  > The Surface Laptop and Surface Book things can be deleted.
- Performance mode
  > Depends on Surface Serial Hub driver
- Buttons
  > WIP
- Ambient Light Sensor
  > ACPI device name: ACSD, attached under I2C4
  > The Linux source code is attached in the folder, just one source file, should be 'easy' to port.(Maybe we don't need gesture and proximity)
  > Driver should attach to VoodooI2CDeviceNub and use it to perform I2C IO.
  > See VoodooI2CSynaptics for code reference.
- Cameras
  > ACPI devices: CAMR,CAMF,CAM3(infrared camera)
  > Corresponding device id: OV8865,OV5693,OV7251
  > Not so important, they use I2C to transfert data. Linux code available.
- Touch Screen
  > Device id: 0x34E4
  > Linux uses `mei` to communicate with the touch screen(ipts), don't know the equivalant thing on macOS nor how to communicate with it.
