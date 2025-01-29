
**General discription:**
This application is a practical example of multitasking application using FreeRTOS running on STM32F407 Discovery board. 
The application uses:
1. queues and task notification api for syncronization 
2. interrupts, HW timer, SW timer, RTC, uart communication
3. debug options - print out to the IDE Consol (via ST-Link)

**Requierments:**
1. STM32F407 Discovery Board + USB Power cable
2. FTDI TTL-232R-PCB: USB to TTL serial UART converter
3. STM32CubeIDE (free)
4. Serial Terminal (free)

**HW setup:**
1. Establish uart connection to STM32f407 Discovery board
a. Connect PA2 to RX pad 
b. Connect PA3 to TX pad
c. Connect the GND to FTDI GND pad
2. Connect the TTL-232R-PCB (or similar) to the host PC USB connector
3. Make sure the driver installed properly

**SW Setup**
1. Download STM32CubeIDE
2. Download uart terminal such as Termite (on Windows) of TeraTerm (on Linux)

**Debug configuration**
The application is using printf. In order to see the printf messages the Debug session you must activate the Debugging session. therefore this part is essential.
1. Choose Debug prob - ST-LINK (ST-LINK GDB Server)
2. Enable the SWV in the debug configuration Debugger tab
3. Enable trace: when the debugging session starts, enable the trace (push the red dot) and set port-0 by pushing the settings button (do once)

**Launcing the application**
1. Make sure that the board is connected to the PC and the Serial coonnection established as expected
2. Open the Project on the STM32CubeIDE by selecting new "STM32 Project from existing STM32CubeMX configuration file (*.ioc)"
3. goto the project directory and select the sample_app.ioc file
4. Open the uart terminal and configure the serial COM port (search it on the device manager)
5. Configure the serial communicatin:
a. baudrate to 115200
b. Data bits: 8
c. stop bits: 1
d. Parity: None
e. Flow control: none
f. Apend LF
6. Build & Run/Debug
 
**Functionality**
The application is interactive and could activate LEDs functions or RTC date and time update according to the user choices. 



