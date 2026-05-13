Intan RHD2132 STM32 Firmware - Real-Time DSP Extensions

Extensions to the Intan Technologies RHD STM32 Firmware Framework (v1.2) that add real-time digital signal processing to live neural data acquisition. Built on the Nucleo-144 STM32U5A5 platform with an Intan RHD2132 chip via an LVDS adapter, streaming filtered data to MATLAB over USART at 10 Mbaud.

Overview

The original framework streams raw 16-bit ADC samples from the RHD2132 directly to the host PC. This fork (currently) adds three independently configurable DSP features that run on-chip inside the SPI DMA completion interrupt at the 20 kHz sample rate, with zero external dependencies:

1. Digital bandpass filter - configurable Butterworth IIR
2. Spike detection - threshold-based detection with refractory period and LED indication
3. 60 Hz notch filter - IIR notch for power-line interference rejection

All features are toggled at compile time via `#define`s in `Core/Inc/userconfig.h`. Disabled features are completely removed from the binary - no runtime overhead.

Hardware

- MCU: STM32 Nucleo-144 (U5A5)
- Amplifier: Intan RHD2132 (32-channel neural amplifier)
- Adapter: LVDS adapter board
- Host: MATLAB on Mac/PC via USART (USB-to-serial)

Configuration

All features are configured at compile time in `Core/Inc/userconfig.h`. To enable a feature, leave its `#define ENABLE_*` line uncommented; to disable, comment it out. 

File Map

- `Core/Inc/userconfig.h` - all feature toggles and parameter defines
- `Core/Inc/userfunctions.h` - function declarations for `init_bandpass_filter()` and `init_notch_filter()`
- `Core/Src/userfunctions.c` - filter and spike detection implementations, modified `transmit_data_realtime()`
- `Core/Src/main.c` - one-time filter initialization calls during startup
- All other files preserved from the original Intan v1.2 framework

Building

Open the project in STM32CubeIDE and build as normal. No external libraries required - everything uses standard CMSIS, HAL, and `<math.h>`.

Acknowledgments

Built on top of the [Intan Technologies RHD STM32 Firmware Framework v1.2](http://www.intantech.com).


Intan README:

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
