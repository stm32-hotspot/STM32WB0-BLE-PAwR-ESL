## __BLE_ESL_AP Application Description__

This example demonstrates how to use an STM32WB0 to communicate with Electronic Shelf Label devices using Periodic Advertising with Responses (PAwR) feature. 
In particular, it implements the Access Point role of the standard profile (Electronic Shelf Label, aka ESL).

For the sake of simplicity, the device has a defined list of Peripherals to connect to, the ESLs. Within ESL commands, each ESL is address by a Group ID and an ESL ID. Once connected to the Peripheral, the AP configures the ESL by writing its ESL ID and assigning it to a group. Then the Periodic Advertising Synchronization Transfer (PAST) procedure is started to let the Peripheral synchronize with the PAwR train, without the need to do an energy-intensive scan.
Once the procedure is completed, the link can be disconnected.

To control the AP, an AT-like command interface is used.
Each command ends with \<CR\>. 
An "OK" or "ERROR" is given after a command is received. When a response is received from the peer, this is notified on the terminal with a string like 
*+\<RESP\>:\<group_id\>,\<esl_id\>,\<status\>\[,\<return_params\>\]*. 
Commands which do not require a response, e.g. LED Control, can be sent in broadcast with all the ESLs in the group, by specifying 0xFF as ESL ID. 
Broadcast commands are retransmitted to increase reliability.

*ATE* can be typed to enable local echo.

For this demo \<group_id\> and \<esl_id\> are limited to a value between 0 and 2. Only for \<esl_id\> there is the possibility to use FF for broadcast messages.

The following AT commands can be used to send standard ESL commands (see Electronic Shelf Label Service specification):
- *AT+PING=\<group_id\>,\<esl_id\>*: Ping
- *AT+UNASSOC=\<group_id\>,\<esl_id\>*: Unassociate from AP
- *AT+SRVRST=\<group_id\>,\<esl_id\>*: Service Reset
- *AT+FRST=\<group_id\>,\<esl_id\>*: Factory Reset
- *AT+UPDCMP=\<group_id\>,\<esl_id\>*: Update Complete
- *AT+SENS=\<group_id\>,\<esl_id\>,\<sensor_index\>*: Read Sensor Data
- *AT+REFRESH=\<group_id\>,\<esl_id\>,\<display_index\>*: Refresh Display 
- *AT+IMG=\<group_id\>,\<esl_id\>,\<display_index\>,\<image_index\>*: Display Image
- *AT+IMGTIM=\<group_id\>,\<esl_id\>,\<display_index\>,\<image_index\>,\<absolute_time\>*: Display Timed Image
- *AT+LED=\<group_id\>,\<esl_id\>,\<led_index\>,\<led_component\>,\<pattern\>,\<off_period\>,\<on_period\>,\<repeat\>*: LED Control
- *AT+LEDTIM=\<group_id\>,\<esl_id\>,\<led_index\>,\<led_component\>,\<pattern\>,\<off_period\>,\<on_period\>,\<repeat\>,\<absolute_time\>*: LED Timed Control

In addition to the previous list of AT commands, the following AT commands can be used to send proprietary ESL commands:
- *AT+TXT=\<group_id\>,\<esl_id\>,\<text\>*: Set text
  - \<text\> can be a string with maximum 15 characters
- *AT+PRICE=\<group_id\>,\<esl_id\>,\<price\>*: Set price

The following AT commands can be used to perform other special operations and for tests:
- *AT+RECONF=\<group_id\>,\<esl_id\>,\<new_group_id\>,\<new_esl_id\>*: Reconfigure an ESL with a new address
- *AT+CONN=\<group_id\>,\<esl_id\>*: Connect to an ESL (ESL enters *updating state*) 
- *AT+INFO*: Read all the Information Characteristics from the connected ESL
- *AT+DISPLAYINFO*: Read the Display Information Characteristic from the connected ESL
- *AT+SENSORINFO*: Read the Sensor Information Characteristic from the connected ESL
- *AT+LEDINFO*: Read the LED Information Characteristic from the connected ESL
- *AT+CLRSCDB*: Delete all bonding information
- *AT+ABSTIME?*: Read current absolute time
- *AT+HELP*: List of AT commands

For each AT command, an ESL command is queued and sent as soon as possible with PAwR. 
The AP can also write commands to the ESL Control Point (ECP) characteristic of a connected ESL (while in *Updating state*).

### __Keywords__

Connectivity, BLE, BLE protocol, BLE PAwR, ESL

### __Hardware and Software environment__

  - This application runs on STM32WB0 Nucleo board.
  - Another STM32WB0 Nucleo board or a STEVAL-ESL1KCB is necessary to run BLE_ESL application.
    
### __How to use it?__

In order to make the program work, you must do the following:

 - Open the project with your preferred toolchain
 - Rebuild all files and load your image into target memory
 - Open a serial terminal (with settings 115200-8-N-1 and \<CR\> transmission at end of line).
 - Reset the board to run the example.
 - Run BLE_ESL on another Nucleo board or STEVAL-ESL1KCB.
 - Once the AP has passed the synchronization information to the ESL, you can send commands on the terminal by specifying the assigned GROUP_ID and ESL_ID to:
   - ping the board
   - control an LED
   - read some sensor data
   - set a description (supported only for STEVAL-ESL1KCB)
   - set a price (supported only for STEVAL-ESL1KCB)
   - set an icon (supported only for STEVAL-ESL1KCB)


### __Notes__
                                            
 - On Keil framework, the following compilation setting are applied:
   - diag_suppress L6312W          (Hide "Empty < type> region description for region < region>" warning)
   - diag_suppress L6314W          (Hide "No section matches pattern < module>(< section>" warning)
   - diag_suppress L6329W          (Hide "Pattern < module>(< section>) only matches removed unused sections" warning)