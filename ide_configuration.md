
This guide will explain how to install and configure VSCode for use with arduino-cli. It assumes the Arduino plug-in has already been installed in VSCode.

# Installing and Configuring arduino-cli

Using arduino-cli is more flexible and reliable than the IDE.

## Install Arduino CLI

Instructions are available at https://arduino.github.io/arduino-cli/latest/. Essentially:
`brew update`
`brew install arduino-cli`

## Post Installation

Initialize the installation:
`arduino-cli config init`


Enable unsafe installation so that local zip files can be installed:
`arduino-cli config set library.enable_unsafe_install true`

## Updating Arduino CLI

To update an existing installation of Arduino CLI:
`brew update`
`brew upgrade arduino-cli`

# Installing and Updating ESP Core

## Install

Add the ESP32 board manager packages:
`arduino-cli config set board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

Update the index:
`arduino-cli core update-index`

Install ESP32 core, version 2.0.11:
`arduino-cli core install esp32:esp32@2.0.11`

Verify the installation was successful, and optionally remove any other cores:
`arduino-cli core list`

Expect:


| ID | Installed | Latest | Name |
| --- | --- | --- | --- |
| esp32:esp32 | 2.0.11 | 2.0.11 | esp32 |


## Updating
 Uninstall the current ESP core:
`arduino-cli core uninstall esp32:esp32`

Specify the version number to upgrade the ESP to 2.0.11:
`arduino-cli core install esp32:esp32@2.0.11`

# Installing and Configuring Libraries
Check the libraries to ensure no libraries are installed:
`arduino-cli lib list`

Expect:
```
No libraries installed.
```
If there are any installed libraries, uninstall them before proceeding.
  

### Download required libraries
Download each library below as a zip file.

| Library | Version | URL |
|---------|---------|-----|
| Adafruit_BusIO | 1.14.1 | https://github.com/adafruit/Adafruit_BusIO |
| Adafruit-GFX-Library | 1.11.5 | https://github.com/adafruit/Adafruit-GFX-Library |
| Adafruit_SSD1306 | 2.5.7 | https://github.com/adafruit/Adafruit_SSD1306 |
| ArduinoJson | 6.20.1 | https://github.com/bblanchon/ArduinoJson |
| BrentIO_AsyncWebServer_ESP32_W5500 | 2024.3.1 | https://github.com/BrentIO/AsyncWebServer_ESP32_W5500 |
| BrentIO_AsyncTCP | 2024.2.1 | https://github.com/BrentIO/AsyncTCP |
| BrentIO_ESP Async WebServer | 2024.3.2 | https://github.com/BrentIO/ESPAsyncWebServer |
| BrentIO_PCA95x5 | 2023.10.2 | https://github.com/BrentIO/PCA95x5 |
| BrentIO_PCT2075 | 2023.10.3 | https://github.com/BrentIO/PCT2075 |
| Ethernet | 2.0.2 | https://github.com/arduino-libraries/Ethernet |
| I2C_EEPROM | 1.7.1 | https://github.com/RobTillaart/I2C_EEPROM |
| LinkedList | 1.3.3 | https://github.com/ivanseidel/LinkedList |
| NTPClient | 3.2.1 | https://github.com/arduino-libraries/NTPClient |
| PCA9685_RT | 0.4.1 | https://github.com/RobTillaart/PCA9685_RT |
| PubSubClient | 2.8 | https://github.com/knolleary/pubsubclient |
| Regexp | 0.1.1 | https://github.com/nickgammon/Regexp |

Install each library above using the following command:
`arduino-cli lib install --zip-path /my/downloads/directory/library_name.zip`


# Flash Partition
FireFly Controller uses a custom board, typically using the ESP32 WROVER-E Module featuring 16MB flash storage (ESP32-WROVER-E-N16R8).  It uses a standard partition image included with the ESP32 core, and its use is defined in the custom boards.txt.  The partition image used is `Default (6.25MB APP/OTA/3.43MB SPIFFS)`, which is defined in `~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.11/tools/partitions/default_16MB.csv`.  The file should **not** be modified.

The partition table will be defined by ESP Core as:

| Name | Type | SubType | Offset | Size | Flags |
|--|--|--|--|--|--|
| nvs | data | nvs | 0x9000 | 0x5000 |
| otadata | data | ota | 0xe000 | 0x2000 |
| app0 | app | ota_0 | 0x10000 | 0x640000 |
| app1 | app | ota_1 | 0x650000 | 0x640000 |
| spiffs | data | spiffs | 0xc90000 | 0x360000 |
| coredump | data | coredump | 0xFF0000 | 0x10000 |


# Add Custom Board to boards.txt

The board must be added to the boards.txt file, found in the Espressif ESP Core version-specific folder. The stub for the custom board is included in the Hardware-Registration-and-Configuration project.

Steps:

1. Close Visual Studio Code

2. Merge the sub custom board into the main boards file. Example for ESP Core version 2.0.11:
`cp ~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.11/boards.txt ~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.11/boards.original.txt`

	`cat ./Hardware-Registration-and-Configuration/boards.txt >> ~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.11/boards.txt`

3. Open Visual Studio Code. Select the board labeled `P5 Software FireFly Controller`.

4. For new boards only, ensure the option `Hardware-Registration-and-Configuration` is set to `Enabled`. Subsequent flashes of that chip should be set to `Disabled`.

5. Flash the Hardware-Registration-and-Configuration.ino project.


# Visual Studio IDE Configuration

 To use Visual Studio Code to compile FireFly-Controller, several files must be modified, all of which are located in `/.vscode/`. The folder may be hidden, but the file should be created automatically by VSCode.

## c_cpp_properties.json
No changes should be required for this file, as it is generated automatically.

## settings.json
```
{
	"arduino.commandPath": "arduino-cli",
	"arduino.useArduinoCli": true,
	"arduino.path": "/usr/local/bin/",
	"arduino.logLevel": "info",
	"arduino.defaultTimestampFormat": "%H:%m:%S.%L "
}
```

## arduino.json
Contents of this file control the data sent to the compiler, and *do not* affect IntelliSense. IntelliSense is updated automatically from the compiler's output.

Example File Contents:
```
{
	"sketch": "Hardware-Registration-and-Configuration.ino",
	"configuration": "FlashFreq=80,PartitionScheme=default,UploadSpeed=921600,DebugLevel=none,EraseFlash=none",
	"board": "esp32:esp32:firefly_controller",
	"buildPreferences": [
		[
			"build.extra_flags",
			"-DASYNCWEBSERVER_REGEX -DPRODUCT_HEX=0x08062305 -DESP32 -DSSD1306_NO_SPLASH -I~/GitHub/P5Software/FireFly-Controller"
		]
	],
	"port": "/dev/tty.SLAB_USBtoUART",
	"output": "../.cache",
	"programmer": "esptool"
}
```

### board
Defines the custom board configured in the Custom Boards section, above: 
`esp32:esp32:firefly_controller`


### build.extra_flags
**`ASYNCWEBSERVER_REGEX`** Allows regex paths in the URL.

**`PRODUCT_HEX`** This configuration indicates the hardware product ID expressed as a hexadecimal and is required. If it is not included, the compiler will trigger an error. Change the `0x08062305` value in the example shown above to match the actual hardware product ID, with `0x` prefixed. This allows for a product ID beginning with zero.

**`ESP32`** The hardware type must also be set for the Adafruit libraries to be configured correctly. Use `-DESP32` flag to set the hardware to ESP32. Without it, you can expect to receive errors such as ```fatal error: util/delay.h: No such file or directory```

**`SSD1306_NO_SPLASH`** To prohibit the adafruit logo from being displayed, use `-DSSD1306_NO_SPLASH` to disable it from compiling.

You must also include the parent directory of FireFly-Controller using the `-I/my/path/to/project/FireFly-Controller` parameter. The folder structure should look like this:

```
/my/path/to/project/FireFly-Controller
-> .vscode
-> Controller
	---> Controller.ino
	---> ...
-> Hardware-Registration-and-Configuration
	---> data
		-----> auxillary_data.json
		-----> ...
	---> Hardware-Registration-and-Configuration.ino
	---> boards.txt
	---> swagger.yaml
	---> ...
-> common
	---> hardware.h
	---> externalEEPROM.h
	---> ...
```

## Troubleshooting

### Uploads won't work

If the upload function does not work, but the application compiles correctly, re-select the port on the bottom right side of the screen.

  
  

# LittleFS
This uses the SPIFFS partition type, which can be found in the partition table included with ESP core. The size of the image depends on the size of the partition. The image must be flashed to the correct starting point from the partition table, see Flash Partition section.

## 16MB Chip

Size = `0x360000`

 Create the image:
`~/Library/Arduino15/packages/esp32/tools/mklittlefs/3.0.0-gnu12-dc7f933/mklittlefs -s 0x360000 -c ~/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/data ~/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/littlefs-16mb.bin`

Location = `0xC90000`

Flash the image:
`~/Library/Arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool --chip esp32 --port "/dev/tty.SLAB_USBtoUART" --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 16MB 0xC90000 ~/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/littlefs-16mb.bin`

 
## 4MB Chip

Size = `0x160000`

Create the image:
`~/Library/Arduino15/packages/esp32/tools/mklittlefs/3.0.0-gnu12-dc7f933/mklittlefs -s 0x160000 -c ~/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/data ~/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/littlefs-4mb.bin`

Location = `0x290000`

Flash the image:
`~/Library/Arduino15/packages/esp32/tools/esptool_py/4.5.1/esptool --chip esp32 --port "/dev/tty.SLAB_USBtoUART" --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 4MB 0x290000 ~/GitHub/P5Software/FireFly-Controller/Hardware-Registration-and-Configuration/littlefs-4mb.bin`