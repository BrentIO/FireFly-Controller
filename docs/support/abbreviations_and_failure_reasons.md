# OLED and Event Log Abbreviations
Abbreviations that can be found in the event log or error display are documented here.

| Abbreviation | Meaning |
| ------------ | ------- |
| Inpt ctl | Input controller |
| Out ctl | Output controller |
| Temp sen | Temperature sensor |


# Failure Reasons
Reason codes that can be raised by the hardware abstraction libraries to the main application.


## External EEPROM
Defined in `externalEEPROM.h`

| Enumeration | Code | Reason |
| ------------| ---- | ------ |
| `SUCCESS_NO_ERROR` | 0 | Request was successful, no error was returned | 
| `DATA_TRANSMIT_BUFFER_ERROR` | 1 | Data too long to fit in transmit buffer |
| `ADDRESS_OFFLINE` | 2 | Received NACK on transmit of address |
| `TRANSMIT_NOT_ACKNOLWEDGED` | 3 | Received NACK on transmit of data |
| `OTHER_ERROR` | 4 | Other error, raised by the I2C bus | 
| `TIMEOUT` | 5 | Timeout |
| `INVALID_HARDWARE_CONFIGURATION` | 10 | Invalid hardware configuration in hardware.h |
| `UNKNOWN_ERROR` | 11 | Unknown/undocumented failure |


## Inputs
Defined in `inputs.h`

| Enumeration | Code | Reason |
| ------------| ---- | ------ |
| `SUCCESS_NO_ERROR` | 0 | Request was successful, no error was returned | 
| `DATA_TRANSMIT_BUFFER_ERROR` | 1 | Data too long to fit in transmit buffer |
| `ADDRESS_OFFLINE` | 2 | Received NACK on transmit of address |
| `TRANSMIT_NOT_ACKNOLWEDGED` | 3 | Received NACK on transmit of data |
| `OTHER_ERROR` | 4 | Other error, raised by the I2C bus | 
| `TIMEOUT` | 5 | Timeout |
| `INVALID_HARDWARE_CONFIGURATION` | 10 | Invalid hardware configuration in hardware.h |
| `UNKNOWN_ERROR` | 11 | Unknown/undocumented failure |


## Outputs
Defined in `outputs.h`

| Enumeration | Code | Reason |
| ------------| ---- | ------ |
| `SUCCESS_NO_ERROR` | 0 | Request was successful, no error was returned | 
| `DATA_TRANSMIT_BUFFER_ERROR` | 1 | Data too long to fit in transmit buffer |
| `ADDRESS_OFFLINE` | 2 | Received NACK on transmit of address |
| `TRANSMIT_NOT_ACKNOLWEDGED` | 3 | Received NACK on transmit of data |
| `OTHER_ERROR` | 4 | Other error, raised by the I2C bus | 
| `TIMEOUT` | 5 | Timeout |
| `INVALID_HARDWARE_CONFIGURATION` | 10 | Invalid hardware configuration in hardware.h |
| `GENERIC_ERROR` | 255 | Inherited from PCA9685 library, generic error |
| `CHANNEL_OUT_OF_RANGE` | 254 | Inherited from PCA9685 library, channel out of range |
| `INVALID_MODE_REGISTER` | 253 | Inherited from PCA9685 library, invalid mode register chosen |
| `GENERIC_I2C_ERROR` | 252 | Inherited from PCA9685 library, i2c communication error |
| `UNKNOWN_ERROR` | 11 | Unknown/undocumented failure |


## OLED
Defined in `oled.h`

| Enumeration | Code | Reason |
| ------------| ---- | ------ |
| `ADDRESS_OFFLINE` | 2 | Device was not found on the bus |
| `UNABLE_TO_START` | 12 | The underlying hardware library returned a fault when attempting to begin communications |


## Temperature Sensor
Defined in `temperature.h`

| Enumeration | Code | Reason |
| ------------| ---- | ------ |
| `SUCCESS_NO_ERROR` | 0 | Request was successful, no error was returned | 
| `DATA_TRANSMIT_BUFFER_ERROR` | 1 | Data too long to fit in transmit buffer |
| `ADDRESS_OFFLINE` | 2 | Received NACK on transmit of address |
| `TRANSMIT_NOT_ACKNOLWEDGED` | 3 | Received NACK on transmit of data |
| `OTHER_ERROR` | 4 | Other error, raised by the I2C bus | 
| `TIMEOUT` | 5 | Timeout |
| `INVALID_HARDWARE_CONFIGURATION` | 10 | Invalid hardware configuration in hardware.h |
| `UNKNOWN_ERROR` | 11 | Unknown/undocumented failure |





