VoodooInput Changelog
=====================
#### v1.1.2
- Improved touch state abstraction

#### v1.1.1
- Remove angle calculation

#### v1.1.0
- Eliminate leftover scaling of physical dimensions by x10

#### v1.0.9
- Add trackpoint device from VoodooTrackpoint

#### v1.0.8
- Initial MacKernelSDK and Xcode 12 compatibility

#### v1.0.7
- Allowed to set finger type externally to fix swiping desktops when holding a dragged item
- Added a message to allow client to set gesture orientation when rotating a touchscreen (thx @Goshin)

#### v1.0.6
- Reduced memory consumption and CPU usage
- Fixed dragging issues on some touchpads

#### v1.0.5
- Don't read extra finger if there is no stylus
- Improve handling of slow to respond devices 

#### v1.0.4
- Improved compatibility with MT2 emulation (thx @Goshin)
- Improved compatibility with VoodooI2C (thx @kprinssu)
- Fix dragging/selection instability while touchpad button is pressed
- Bundle SDK in resources for DEBUG builds

#### v1.0.3
- Fixed interpreting transducer type data

#### v1.0.2
- Minor deployment fixes
- Resolved loading issues on some configurations (thx @Sniki)

#### v1.0.1
- Added support for macOS versions prior to 10.15

#### v1.0.0
- Initial release
