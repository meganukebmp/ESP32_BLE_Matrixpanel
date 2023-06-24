# ESP32 Matrix Panel with BLE
This is an example implementation of a dot matrix panel driven by an ESP32 microcontroller capable of accepting and displaying a gif transmitted to it from an Android app over Bluetooth Low Energy. Images sent to the microcontroller get saved to it's internal flash storage and are persistent.
The panel also has a GPS module attached to it in order to demo two way BLE communication without any slowdowns.

![preview](preview.gif)

The app is available here: https://github.com/meganukebmp/ESP32_BLE_Matrixpanel_App

### Building
This is a platformio project, simply use the IDE or the commandline to compile and upload to an ESP32:
```
pio run --target upload
```
You also need to have at least one image to begin with so upload the filesystem either through the IDE or with:
```
pio run --target uploadfs
```

If correctly setup the gif should begin playing on the screen

### Some words
This is by no means a perfect, or even good implementation of the concept. It was done purely as a proof of concept. If you would like to use this for something feel free, however I sincerely encourage you to improve upon its faults.

Mainly:
* Better error handling/failstates
* Fault proofing image data transmissions (Right now we just save the gif over the old one and if the transmission gets interrupted we have a corrupt image which might or might not crash the gif processor)
* There is potential to have this use less energy by precisely timing frame advances and sleeping inbetween
* It's completely insecure
* There is no proper BLE negotiation (for example we don't handle MTU requests and just always force max MTU always)

### Libraries used
Thanks to the developers of the following fantastic libraries:
* [ESP32-HUB75-MatrixPanel-DMA](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA) - DMA Driven RGB matrix panel driver, extremely fast
* [AnimatedGIF](https://github.com/bitbank2/AnimatedGIF) - An extremely fast microcontroller GIF processor
* [TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus) - A very easy to use GPS library