Intan Technologies RHD STM32 Firmware Framework

This Framework was developed with a NUCLEO-U5A5ZJ-Q board. To begin building and running this project
with a connected U5 chip, take the following steps:

- Create an STM32CubeIDE workspace
- In this workspace, create a folder called 'rhd_acquisition'
- In STM32CubeIDE, open this folder as a project (Open Project)
- Project -> Clean
- Run As ... rhd_acquisition Release

If you wish to debug the project, Build (Hammer icon) -> Debug. Then, Debug -> rhd_acquisition Debug.
Note that the slower Debug build is likely to trigger the ITClip Error when running, so it is recommended to significantly
slow down sampling period if debugging is necessary. This can be done by increasing the TIM3 period.



For further customization, the files mentioned in the RHD STM32 Framework document that you may want to change are:
Core
	Inc
		rhdinterface.h
		rhdregisters.h
		stm32u5xx_it.h
		userconfig.h
		userfunctions.h

	Src
		main.c
		rhdinterface.c
		rhdregisters.c
		stm32u5xx_it.c
		userfunctions.c
		


To verify data acquired and transmitted via USART, a MATLAB script (ReadFromSerialPort.m) is included in this directory
to plot the data that is received on the COM port the STM32U5 is connected on. The default port slot is "COM4", but your
system may vary - check which port the STM32U5 is on, and replace "COM4" with the actual port. Then, you should be able
to run the MATLAB script (it will pause waiting for data to come in on the connected port), then run the STM32U5 binary,
and view the data plotting in the MATLAB script.