# Configuring Microsoft Visual Studio Code IDE

This guide will explain how to install and configure VSCode for use with arduino-cli.  It assumes the Arduino plug-in has already been installed in VSCode.

---
## **Installing and Configuring arduino-cli**

Using arduino-cli is more flexible and reliable than the IDE.

### Install Arduino CLI

Instructions are available at https://arduino.github.io/arduino-cli/latest/.  Essentially:

`brew update`

`brew install arduino-cli`

---
### Init the installation:

`arduino-cli config init`

Enable unsafe installation so that local zip files can be installed:

`arduino-cli config set library.enable_unsafe_install true`

---
### Configure boards and libraries

Add the ESP32 board manager packages:

`arduino-cli config set board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

Install ESP32 core, version 2.0.9:

`arduino-cli core install esp32:esp32@2.0.9`

Check the libraries to ensure nothing is installed.

`arduino-cli lib list`

Expect:
```
No libraries installed.
```

If there are any installed libraries, uninstall them before proceeding.

---
### Install the required libraries

Download each library manually as a zip file.  Install each required library from disk.

`arduino-cli lib install ArduinoJson@6.20.1`

`arduino-cli lib install --zip-path /my/downloads/directory/ArduinoJson-6.20.1.zip`

`arduino-cli lib install --zip-path  /my/downloads/directory/I2C_EEPROM-1.7.1.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/PCA95x5-0.1.3.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/PCA9685_RT-0.4.1.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/PCT2075-main.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/Regexp-0.1.1.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/Adafruit_SSD1306-2.5.7.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/Adafruit-GFX-Library-1.11.5.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/Adafruit_BusIO-1.14.1.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/ESPAsyncWebServer-master.zip`

`arduino-cli lib install --zip-path /my/downloads/directory/AsyncTCP-master.zip`

---
## **Visual Studio IDE Configuration**

To use Visual Studio Code to compile FireFly-Controller, several files must be modified, all of which are located in `/.vscode/`.  The folder may be hidden, but the file should be created automatically by VSCode.

---
## c_cpp_properties.json

No changes should be required for this file, as it is generated automatically.

---
##  settings.json

```
{
    "arduino.commandPath": "arduino-cli",
    "arduino.useArduinoCli": true,
    "arduino.path": "/usr/local/bin/",
    "arduino.logLevel": "info",
    "arduino.defaultTimestampFormat": "%H:%m:%S.%L "
}
```
---
## arduino.json

Contents of this file control the data sent to the compiler, and *do not* affect IntelliSense.  IntelliSense is updated automatically from the compiler's output.

Example File Contents:

```
{
    "sketch": "Hardware-Registration-and-Configuration.ino",
    "configuration": "FlashFreq=80,PartitionScheme=default,UploadSpeed=921600,DebugLevel=none,EraseFlash=none",
    "board": "esp32:esp32:firefly_controller",
    "buildPreferences": [
        [
            "build.extra_flags",
            "-DASYNCWEBSERVER_REGEX -DPRODUCT_ID=FFC_08062305 -DESP32 -DSSD1306_NO_SPLASH -I/Users/brent/GitHub/P5Software/FireFly-Controller"
        ]
    ],
    "port": "/dev/tty.SLAB_USBtoUART",
    "output": "../.cache",
    "programmer": "esptool"
}
```
---
## build.extra_flags

**`ASYNCWEBSERVER_REGEX`** Required only for Hardware Registration and Configuration application, this allows regex paths in the URL.

**`PRODUCT_ID`** This configuration indicates the hardware product ID and is required.  If it is not included, the compiler will trigger an error.  Change the `FFC_32322211` value in the example shown above to match the actual hardware product ID, with `FFC_` prefixed.  This allows for a product ID beginning with zero.

**`ESP32`** The hardware type must also be set for the Adafruit libraries to be configured correctly.  Use `-DESP32` flag to set the hardware to ESP32.  Without it, you can expect to receive errors such as ```fatal error: util/delay.h: No such file or directory```

**`SSD1306_NO_SPLASH`** To prohibit the adafruit logo from being displayed, use `-DSSD1306_NO_SPLASH` to disable it from compiling.

You must also include the parent directory of FireFly-Controller using the `-I/my/path/to/project/FireFly-Controller` parameter.  The folder structure should look like this:

```
/my/path/to/project/FireFly-Controller
    -> .vscode
    -> Controller
        -> Controller.ino
        -> ...
    -> Hardware-Registration-and-Configuration
        -> Hardware-Registration-and-Configuration.ino
        -> boards.txt
        -> ...
    -> common
        -> hardwareDefinitions.h
        -> ...
```

---
## Add Custom Board to boards.txt
The board must be added to the boards.txt file, found in the Espressif ESP Core version-specifc folder.  The stub for the custom board is included in the Hardware-Registration-and-Configuration project.

Steps:
1. Close Visual Studio Code
2. Merge the sub custom board into the main boards file.  Example for ESP Core version 2.0.9:
   
    `cp ~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.9/boards.txt ~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.9/boards.original.txt`
    
    `cat ./Hardware-Registration-and-Configuration/boards.txt >> ~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.9/boards.txt`

3. Open Visual Studio Code.  Select the board labeled `P5 Software FireFly Controller`.
4. For new boards only, ensure the option `Hardware-Registration-and-Configuration` is set to `Enabled`.  Subsequent flashes of that chip should be set to `Disabled`.
5. Flash the Hardware-Registration-and-Configuration.ino project.

---
# How to include files into the project

To include additional files, place them in the correct directories.  At the top of the .ino file, include the files, such as:

```
#include <PCA95x5.h>
#include "common/hardwareDefinitions.h"
```

# Updating Packages

## Updating Arduino CLI

To update, run these commands in Terminal:

`brew update`

`brew upgrade arduino-cli`

## Updating Arduino CLI

To upgrade the ESP version from 2.0.6 to 2.0.9:

`arduino-cli core uninstall esp32:esp32`

`arduino-cli core install esp32:esp32@2.0.9`

---
# Troubleshooting

## Upload won't work

If the upload function does not work, but the application compiles correctly, re-select the port on the bottom right side of the screen.

---

# Creating the LittleFS image

The size is from the partition table -- 0x360000 = 3538944

`~Library/Arduino15/packages/esp32/tools/mklittlefs/3.0.0-gnu12-dc7f933/mklittlefs -s 3538944 -c /Users/brent/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/data /Users/brent/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/littlefs.bin`

# Flashing the littelfs image

The location is the start location for spiffs 0xC90000

`"/Users/brent/Library/Arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool" --chip esp32 --port "/dev/tty.SLAB_USBtoUART" --baud 921600  --before default_reset --after hard_reset write_flash  -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0xC90000 /Users/brent/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/littlefs.bin`