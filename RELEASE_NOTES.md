
# Release Notes

This package contains two specific applications to demonstrate Bluetooth Periodic Advertising with Response (PAwR) with Electronic Shelf Label (ESL) profile on STM32WB09 MCU. See README file for more details. 
 
## [2.0.0] - 2025-03-04
 
### New Features

- BLE_ESL and BLE_ESL_AP now follow standard Electronic Shelf Label profile (with some limitations). Old projects renamed to BLE_PAwR_Broadcaster and BLE_PAwR_Observer.
- Alignment to STM32CubeWB0 v1.2.0 package.
- Changed name of ST custom board to STEVAL-ESL1KCB.

### Content

Refer to Release Notes of STM32CubeWB0 v1.2.0 package for details of Components (Drivers, Cortex-M CMSIS, STM32 CMSIS, STMWB0x_HAL_Driver, STM32WB0x_Nucleo, Middlewares, Utilities)

### Development Toolchains and Compilers

- IAR Embedded Workbench for ARM (EWARM) toolchain from v9.30.1
- uVision Microcontroller Development Kit (MDK-ARM) from v5.37
- STM32CubeIDE from v1.16.1

### Supported Devices and Boards

- STM32WB09
  - NUCLEO-WB09KE board
  - STEVAL-ESL1KCB board

### Known Limitations

- ESL AP:
  - Missing AT command to provision an ESL. ESL addresses to be provisioned are hard-coded. ESL addresses are derived from the two least significant bytes of public address.
  - Provisioned ESLs are not saved in NVM. After a reset, the AP will assign predefined ESL addresses.
  - Object Transfer Profile not supported.
  - Not able to parse response if it has been received after last transmission of the command: maximum number of retransmissions for unicast cannot be set to 0.
- ESL:
  - Only one timed command for all the LEDs can be queued.
  - Only one timed command for all the Displays can be queued.
  - After a reset, the ESL gets unassociated.
  - Object Transfer Profile not supported: not possible to receive images (support to OTP is not mandatory).
 

 ## [1.0.0] - 2024-06-19
 
### New Features

First release, aligned to official STM32CubeWB0 v1.0.0 package, of Bluetooth PAwR and ESL applications for STM32WB09 MCU.
   
### Content

Refer to Release Notes of STM32CubeWB0 v1.0.0 package for details of Components (Drivers, Cortex-M CMSIS, STM32 CMSIS, STMWB0x_HAL_Driver, STM32WB0x_Nucleo, Middlewares, Utilities)

### Development Toolchains and Compilers

- IAR Embedded Workbench for ARM (EWARM) toolchain from v9.30.1 + ST-Link
- uVision Microcontroller Development Kit (MDK-ARM) from v5.37 + ST-Link

### Supported Devices and Boards

- STM32WB09
  - NUCLEO-WB09KE board
  - STEVAL-ESLBLECB board

### Known Limitations

None
 