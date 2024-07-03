# STM32WB09 PAwR/ESL Demo

STM32WB09 PAwR/ESL application demonstrates the PAwR (Periodic Advertising with Response) feature of STM32WB0 Bluetooth stack. With the support of Periodic Advertising with Response, it is possible to have a bidirectional one-to-many communication between a single node and thousands of low-power devices. One of the most innovative applications addressed by this new feature, introduced by Bluetooth Core v5.4, is Electronic Shelf Label. 

This demo is inspired by Bluetooth ESL profile and it follows the same packet format for communicating over PADVB channel. For the sake of simplicity (not only on source code but also for demo set-up) provisioning is not supported in this first version of the demo: ESL addresses are statically decided at compile time. Each ESL is addressed by a Group ID (7-bit value) and an ESL ID (8-bit value), which are defined with two macros: GROUP_ID and ESL_ID (default values of this macros can be found inside esl_profile.h). GROUP_ID and ESL_ID can be defined at project level. ESL ID can be a value between 0x00 and 0xFE. Value 0xFF is reserved for the Broadcast Address.

Since the BD_ADDRESS of the ESLs must be known at compile time, the firmware is made to support only 9 different devices, spread on 3 different groups, with 3 ESLs for each one (so GROUP_ID and ESL_ID can be a number between 0 and 2). However the code can be easily modified to support more groups with more ESLs.

When ESL firmware starts, the device enters advertising state (extended advertising is used, even though legacy advertising is still possible). The Access Point (AP) automatically connects to any of the predefined ESLs. Once it gets connected to the ESL, the ESL can receive the Periodic Advertising Synchronization Info so that it can synchronize with the periodic advertising train and disconnect the link. In particular, the device will only synchronize with the PAwR subevent corresponding to the group (e.g. ESL in group 0 is synchronized only with subevents 0).

Commands can be sent by the PAwR broadcaster Access Point. The human interface to the AP is an AT-like command set over UART interface (115200-8-N-1). See dedicated section below for further details.
Commands can either be unicast, i.e. addressed to a single device, or broadcast if addressed to all the ESLs of a group (ESL ID = 0xFF). Broadcast commands are retransmitted to increase reliability. Unicast commands always have a response from the ESL. If no response is received, a command is automatically retransmitted a limited amount of times, until the response is received from the ESL to confirm the reception of the command.

## Hardware Needed

The ESL application can run on any of these 2 boards:
- NUCLEO-WB09KE (firmware under *Firmware/Projects/NUCLEO-WB09KE/Applications/BLE/BLE_ESL*)
- STEVAL-ESLBLECB (firmware under *Firmware/Projects/STEVAL-ESLBLECB/Applications/BLE_ESL*)

The AP application can run on:
- NUCLEO-WB09KE (firmware under *Firmware/Projects/NUCLEO-WB09KE/Applications/BLE_ESL_AP*)

### STEVAL-ESLBLECB

Even though this demo can run on Nucleo boards, with a reduced set of commands, it can be run also on a dedicated board, STEVAL-ESLBLECB. This board is an example of a real Electronic Shelf Label, with 1.54-inches e-paper display and NFC tag.

![ESL board front](Media/ESL_front_blank.jpg)

![ESL board back](Media/ESL_back.jpg)

These are the main components in the ESL board:
- STM32WB09KE ultra-low power programmable Bluetooth Low Energy SoC
- 1.54-inches e-paper display
- ST25DV, a dynamic NFC/RFID tag IC with I2C interface
- NFC antenna (under the display)
- LED
- Button for user interaction
- Reset button
- Battery holder for CR2032 batteries

## Software Needed

- Serial Terminal program, like [Tera Term](https://teratermproject.github.io/index-en.html)

- IDE, required only to compile source code:

  - [IAR EWARM](https://www.iar.com/products/architectures/arm/iar-embedded-workbench-for-arm/)

  - [Keil MDK-ARM](https://developer.arm.com/Tools%20and%20Software/Keil%20MDK)

- [STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) to program the prebuilt binaries (not required if an IDE is used).


## User's Guide

This section describes the steps to set-up and run the demo.

1. Flash ESL boards (either Nucleo-WB09KE or STEVAL-ESLBLECB), with ESL firmware binaries. Each board needs to have its own ESL address, which consists of 15 bits, made of Group ID (7 bits) and ESL ID (8 bits). Set group ID and ESL ID by defining GROUP_ID and ESL_ID in the project and compile it, or use precompiled binaries. Firmware can be flashed either with IDE or STM32CubeProgrammer.  
_Note_: if the board was already programmed with a firmware using Deepstop mode, it may be not possible to use SWD interface to program a new firmware. In this case the STM32WB09 device can be forced to enter bootloader mode to make it possible to program a new firmware. On STEVAL-ESLBLECB, bootloader mode can be entered by pressing reset button while keeping user button pressed, while on Nucleo-WB09KE the reset button must be pressed after placing a jumper on JP1 in *Bootloader mode* position.

2. For the AP on the Nucleo board compile and flash the firmware with and IDE or use the STM32CubeProgrammer to flash the precompiled binaries.  
_Note_: if the board was already programmed with a firmware using Deepstop mode, it may be not possible to use SWD interface to program a new firmware. In this case the STM32WB09 device can be forced to enter bootloader mode to make it possible to program a new firmware. On Nucleo-WB09KE, bootloader mode can be entered by pressing reset button while keeping JP1 in *Bootloader* position.

3. Optional: open a terminal to view messages on the COM port associated to the ESL boards. UART settings are:
    - Baudrate 921600 bps
    - Data bits: 8
    - Parity: none
    - Stop bits: 1.

4. Reset ESL boards by pushing reset button. On STEVAL-ESLBLECB this ensures that display is properly initialized. In fact the display image is kept in RAM if a reset occurs for watchdog reason or after an unexpected condition has happened (e.g. an HardFault).  
You will see some prints on the terminal, that show that the board is advertising.
![ESL terminal](Media/terminal_ESL.png)
The firmware will stop advertising after 90 seconds, to save energy. Press the button (B1 on Nucleo board) to start again advertising.

5. Open a terminal on COM port associated to AP board. UART settings are:
    - Baudrate 115200 bps
    - Data bits: 8
    - Parity: none
    - Stop bits: 1.
Make sure also to set the terminal to send a CR character after a new line.

6. Reset AP board to see some info on the terminal. The firmware tries automatically to connect to the pre-configured ESL boards to pass PAwR train sync info. Once an ESL received the Synchronization information, it immediately disconnects the link.  
This is the log on the Access Point after the connection with the first ESL.
![AP terminal](Media/terminal_AP.png)  
Below the log on the ESL after the connection to the AP.
![ESL terminal](Media/terminal_ESL2.png)  


7. Insert AT commands on the AP terminal to send commands to ESLs. See section below for a list of supported commands.  
Type ATE to enable local echo.  

For example, you can:
  - Turn on an LED on ESL with address 0,0 (Group ID 0, ESL ID 0) by typing:  
    *AT+LED=0,0,1*
  - Turn off the LED on ESL with address 0,0 with:  
    *AT+LED=0,0,0*
  - Read the battery level on ESL 0,2 with:  
    *AT+BATT=0,2*
  - Send a short text, e.g. "Peaches", to be displayed on ESL 1,0 (available only on STEVAL-ESLBLECB):  
    *AT+TXT=1,0,Peaches*
  - Set the price on ESL 1,0 (available only on STEVAL-ESLBLECB):  
    *AT+TXT=1,0,4.95*
  - Set an icon for discount on ESL 2,0 (available only on STEVAL-ESLBLECB):  
    *AT+IMG=2,0,1*
  - Turn on the LED on all the ESLs on Group 2 with:  
    *AT+LED=2,ff,1*  

Below the log on the Access Point:
![AP terminal LED command](Media/terminal_AP_LED.png)  
This the corresponding log on the ESL after receiving the previous commands:
![ESL terminal LED command](Media/terminal_ESL_LED.png) 

When not synchronized with the Access Point, after a loss of synchronization, the ESL is in low power advertising state for a limited amount of time (15 minutes). After exiting advertising state, short press the user button to start advertising again (B1 on Nucleo board).  

On STEVAL-ESLBLECB, a long press on the button (> 1.5 s) clears the screen (useful to be done before storing the board for a long time to avoid damage to the display).

## AT commands

Each command starts with *AT* and ends with a \<CR> character (ASCII: 13).  
An *"OK\<CR>\<LF>"* or *"ERROR\<CR>\<LF>"* string is given after a command is received and processed locally.  
All the AT commands to send ESL commands have *\<group_id>* and *\<esl_id>* as the first two parameters. *\<group_id>* identifies the Group of the ESL and in this demo is limited to a value between 0 and 2 (inclusive). *\<esl_id>* identifies the ESL ID and for this demo is limited to a value between 0 and 2 (inclusive) for unicast commands. Commands which do not require a response, e.g. LED Control, can be sent in broadcast to all the ESLs in the group, by giving FF as *\<esl_id>*. Broadcast commands are retransmitted to increase reliability.

When a response is received from the peer, this is notified on the terminal with a string like  
*+\<RESP>:\<group_id>,\<esl_id>,\<status>\[,params]\<CR>\<LF>*  
where status is 0 if the response has been received successfully. *status* may be followed by a list of other parameters, depending on the command. 

Commands which do not require a response, e.g. LED Control, can be sent in broadcast to all the ESLs in the group, by specifying ESL_ID 0xFF. Broadcast commands are retransmitted to increase reliability.

Here is the list of supported commands on the Access Point.

- Enable local echo:  
  *ATE*
- Ping (only to request a response from the ESL):  
  *AT+PING=\<group_id>,\<esl_id>*  
  When a response is received from the peer, the following string is given:  
  *+STATE:\<group_id>,\<esl_id>,\<status>\<state>\<CR>\<LF>*  
  where *\<state>* is the basic state of the ESL (currently not used and always set to 0).
- LED control:  
  *AT+LED=\<group_id>,\<esl_id>,\<led_level>*  
  where *\<led_level>* can be 0 (off), 1 (on), 2 (blinking). The LED is constantly ON only for a limited amount of time to avoid wasting too much energy.
  When a response is received from the peer, the following string is given:  
  *+LED:\<group_id>,\<esl_id>,\<status>\<CR>\<LF>*
- Read battery level:  
  *AT+BATT=\<group_id>,\<esl_id>*  
  When a response is received from the peer, the following string is given:  
  *+BATT:\<group_id>,\<esl_id>,\<status>,\<battery_level>*  
  where *\<battery_level>* is the measured battery voltage in millivolts.
- Send a text:  
  *AT+TXT=\<group_id>,\<esl_id>,\<text>*  
  where *\<text>* can be a string with a maximum of 15 characters (only 10 characters can be actually displayed on the STEVAL-ESLBLECB screen).  
  When a response is received from the peer, the following string is given:  
  *+TXT:\<group_id>,\<esl_id>,\<status>*  
- Send a price:  
  *AT+PRICE=\<group_id>,\<esl_id>,\<price>*  
  where *\<price>* is a decimal number with 2 digits after the decimal point, up to 999.99.  
  When a response is received from the peer, the following string is given:  
  *+PRICE:\<group_id>,\<esl_id>,\<status>*
- Select a special icon:  
 *AT+IMG=\<group_id>,\<esl_id>,\<img_index>*  
  where *\<img_index>* can be 0 (no icon), 1 (discount icon), 2 (star icon, e.g. a special item).  
  When a response is received from the peer, the following string is given:  
  *+IMG:\<group_id>,\<esl_id>,\<status>*

## Demo on STEVAL-ESLBLECB

When the firmware on the ESL board start to run after a reset or a power-up, the display is initialized by showing only static background, without any text or price.

![ESL board front](Media/ESL_front_init.jpg)

On the bottom-right corner of the display, Group ID and ESL ID, which are statically assigned to the ESL, are shown.  
On the top-right corner, a circle is used to show the current state of the ESL connectivity:
- No circle: no Bluetooth activity (no advertising, not synchronized with AP).
- Empty circle: ESL advertising, not synchronized.
- Full circle: ESL synchronized with AP.

By launching some ESL commands from Access Point, it is possible to do the following operation on the ESL:
- Setting a text for a short description of the item (a maximum of 10 characters)
- Setting the price of the item (up to 999.99)
- Setting a special icon (e.g. to show a discount)
- Activate an LED: ON (automatically switched off after 1 minute) or blinking.

The NFC tag is used to program an URI that is pointing to more product info on a website. The product can be identified by the ESL address (Group ID and ESL ID).

Below an image showing an ESL board with item description and price set.

![ESL board front](Media/ESL_front.jpg)


## Performances

The PAwR train can be customized by changing several parameters. To shortly describe a PAwR advertising, it is composed of PAwR events occurring at fixed intervals, called *periodic advertising intervals*. Each PAwR events is made by subevents, where packets are transmitted. The interval between two consecutive subevents is called *periodic advertising subevent interval*. This is illustrated in the figure below.
![PAwR event](Media/pawr_event.png)

At the beginning of a subevent, the Broadcaster transmits one packet, and after a *periodic advertising response slot delay* it can receive responses from observer devices inside the so called *response slots* (one response packet in a single response slot). The interval between the start of two consecutive response slots is the *periodic advertising response slot spacing*. 
![PAwR subevent](Media/pawr_subevent.png) 

All these parameters can be controlled on the Access Point through macros defined in *app.conf.h*.

![app_conf.h](Media/app_conf.png) 

- **PAWR_NUM_SUBEVENTS**: the number of subevents. It limits the number of supported groups, because subevent #n is used to address only devices belonging to Group ID #n. The maximum number of subevents supported by the Bluetooth stack is equal to CFG_BLE_NUM_PAWR_SUBEVENTS.
- **PAWR_SUBEVENT_INTERVAL_MS**: the interval (in milliseconds) between the start of two PAwR subevents.
- **PAWR_RESPONSE_SLOT_DELAY_MS**: the interval between the start of the subevent and the first response slot.
- **PAWR_RESPONSE_SLOT_SPACING_US**: the interval in microseconds between the beginning of two response slots. It must be long enough to let the observer send the response packet.
- **PAWR_NUM_RESPONSE_SLOTS**: the number of response slots in the subevent. 

The latest four parameters must be chosen so that the all the response slots can fit inside a subevent: *PAWR_SUBEVENT_INTERVAL_MS > PAWR_RESPONSE_SLOT_DELAY_MS + (PAWR_RESPONSE_SLOT_SPACING_US * PAWR_NUM_RESPONSE_SLOTS)/1000*.

- **PAWR_INTERVAL_MS**: the interval in milliseconds between the starts of two PAwR events. Default value it set to *(PAWR_SUBEVENT_INTERVAL_MS \* PAWR_NUM_SUBEVENTS + 200)* to make sure the periodic advertising interval is long enough to accommodate all the subevents, leaving 200 ms of interval at the end of the last subevent.

With the default values of the macros, i.e. 4 subevents with 30 ms of subevent interval, the periodic advertising interval is set to 320 ms. This a value that gives fast responses from ESLs. However, in a real scenario, where ESLs must be working for years before replacing the battery, this value may be too aggressive. It is more common to have periodic intervals in the order of seconds, since there is no need to have a short response time. Moreover, more ESLs may be addressed in the same PAwR subevent.

## NFC (on STEVAL-BLEESLCB)

This section only applies to STEVAL-BLEESLCB board. This board has an NFC tag, which is programmed with an URL. When an user taps the ESL screen with his phone, a notification will take the user on a webpage. The URL contains some info on the ESL, like ESL address and price, together with an identifier of the NFC tag.

## Troubleshooting

**Caution** : Issues and the pull-requests are **not supported** to submit problems or suggestions related to the software delivered in this repository. The STM32WB09 PAwR/ESL example is being delivered as-is, and not necessarily supported by ST.

**For any other question** related to the product, the hardware performance or characteristics, the tools, the environment, you can submit it to the **ST Community** on the STM32 MCUs related [page](https://community.st.com/s/topic/0TO0X000000BSqSWAW/stm32-mcus).