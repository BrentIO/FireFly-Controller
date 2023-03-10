# Configuring Microsoft Visual Studio Code IDE

This guide will explain how to install and configure VSCode for use with arduino-cli.  It assumes the Arduino plug-in has already been installed in VSCode.

---

## **Installing and Configuring arduino-cli**

Using arduino-cli is more flexible and reliable than the IDE.

### Install Arduino CLI

Instructions are available at https://arduino.github.io/arduino-cli/latest/.  Essentially:

```
brew update
brew install arduino-cli
```

### Init the installation:

`arduino-cli config init`

Enable unsafe installation so that local zip files can be installed:

`arduino-cli config set library.enable_unsafe_install true`

### Configure boards and libraries

Add the ESP32 board manager packages:

`arduino-cli config set board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

Install ESP32 core, version 2.0.6:

`arduino-cli core install esp32:esp32@2.0.6`

Check the libraries to ensure nothing is installed.

`arduino-cli lib list`

Expect:
```
No libraries installed.
```

If there are any installed libraries, uninstall them before proceeding.

### Install the required libraries

Download each library manually as a zip file.  Install each required library from disk.

`arduino-cli lib install ArduinoJson@6.20.1`

`arduino-cli lib install --zip-path /my/downloads/directory/ArduinoJson-6.20.1.zip`

`arduino-cli lib install --zip-path  /my/downloads/directory/I2C_EEPROM-1.7.1.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/PCA95x5-0.1.3.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/PCA9685_RT-0.4.1.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/PCT2075-main.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/Regexp-0.1.1.zip`

---

## **Visual Studio IDE Configuration**

To use Visual Studio Code to compile FireFly-Controller, several files must be modified, all of which are located in `/.vscode/`.  The folder may be hidden, but the file should be created automatically by VSCode.

### c_cpp_properties.json

No changes should be required for this file, as it is generated automatically.

###  settings.json

```
{
    "arduino.commandPath": "arduino-cli",
    "arduino.useArduinoCli": true,
    "arduino.path": "/usr/local/bin/",
    "arduino.logLevel": "info",
    "arduino.defaultTimestampFormat": "%H:%m:%S.%L "
}
```

### arduino.json

Contents of this file control the data sent to the compiler, and *do not* affect IntelliSense.  IntelliSense is updated automatically from the compiler's output.

Example File Contents:

```
{
    "sketch": "Controller.ino",
    "configuration": "PartitionScheme=default,FlashMode=qio,FlashFreq=80,UploadSpeed=921600,DebugLevel=none,EraseFlash=none",
    "board": "esp32:esp32:esp32wrover",
    "buildPreferences": [
        [
            "build.extra_flags",
            "-DPRODUCT_ID=32322211 -I/my/path/to/project/FireFly-Controller"
        ]
    ],
    "port": "/dev/tty.SLAB_USBtoUART",
    "output": "../.cache"
}
```

### **build.extra_flags**

This configuration indicates the hardware product ID and is required.  If it is not included, the compiler will trigger an error.  Change the `32322211` value in the example shown above to match the actual hardware product ID.

You must also include the parent directory of FireFly-Controller using the `-I/my/path/to/project/FireFly-Controller` parameter.  The folder structure should look like this:

```
/my/path/to/project/FireFly-Controller
    -> .vscode
    -> Controller
        -> Controller.ino
        -> ...
    -> Hardware-Registration-and-Configuration
        -> Hardware-Registration-and-Configuration.ino
        -> ...
    -> common
        -> hardwareDefinitions.h
        -> ...
```

---

## How to include files

To include additional files, place them in the correct directories.  At the top of the .ino file, include the files, such as:

```
#include <PCA95x5.h>
#include "common/hardwareDefinitions.h"
```

# Troubleshooting

## Upload won't work

If the upload function does not work, but the application compiles correctly, re-select the port on the bottom right side of the screen.