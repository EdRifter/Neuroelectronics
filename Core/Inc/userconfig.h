/*
  Intan Technologies RHD STM32 Firmware Framework
  Version 1.2

  Copyright (c) 2025 Intan Technologies

  This file is part of the Intan Technologies RHD STM32 Firmware Framework.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the “Software”), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.


  See <http://www.intantech.com> for documentation and product information.
  
 */

#ifndef INC_USERCONFIG_H_
#define INC_USERCONFIG_H_

// If using HAL drivers, leave this uncommented.
// If using LL drivers, leave this commented.
#define USE_HAL

// If acquiring a short period of data, then exiting and transmitting data
// offline is desired, leave this uncommented.
// If instead transmitting data in real-time is desired, leave this commented.
//#define OFFLINE_TRANSFER

// Error detect GPIO, by default used to illuminate red LED
// when an error of any kind is detected.
#define ERROR_DETECTED_PORT 		LED_RED_GPIO_Port
#define ERROR_DETECTED_PIN 			LED_RED_Pin
// If this pin goes high (red LED illuminates), check error code
// bits 0-3 to determine which error code has been flagged.
// Default GPIO assignment:
// PE0: ErrorCode_Bit_3 (MSB)
// PG8: ErrorCode_Bit_2
// PG5: ErrorCode_Bit_1
// PG6: ErrorCode_Bit_0 (LSB)
// Once the 4-bit error code has been determined, consult ErrorCode in
// rhdinterface.h to determine to which error the code maps.

// How many CONVERT commands are sent in a single sequence,
// which occurs every time the period defined by INTERRUPT_TIM occurs (default 20 kHz).
// Default of 32 indicates that 32 amplifier channels are each sampled once per sequence.
#define CONVERT_COMMANDS_PER_SEQUENCE 32
#define AUX_OFFSET CONVERT_COMMANDS_PER_SEQUENCE

// How many AUX commands are sent in a single sequence,
// which occurs every time the period defined by INTERRUPT_TIM occurs (default 20 kHz).
// Default of 3 indicates that 3 auxiliary command lists
// each execute a single command per sequence.
#define AUX_COMMANDS_PER_SEQUENCE 3

// How many AUX commands are contained in a single auxiliary
// command list (excluding zcheck_DAC command lists).
// Default of 128 indicates that for each sequence, each of the
// AUX_COMMANDS_PER_SEQUENCE (default 3) command lists executes a single command,
// so that 128 sequences must occur before an auxiliary command
// list finishes and repeats execution from the beginning again.
#define AUX_COMMAND_LIST_LENGTH 128

// IMPORTANT note regarding sample rate:
// The per-channel sample rate is defined by the period of the INTERRUPT_TIM peripheral,
// which generates the interrupt events that trigger SPI sequence transfers.
// The INTERRUPT_TIM peripheral configuration occurs in the .ioc file, and by default
// is set up with the timer's input frequency of 96 MHz (APB1 Timer clock) and a period
// of 4800 cycles, resulting in 20 kHz.

// When recording offline, how many seconds of data should be acquired before
// the acquisition loop is escaped. With the default sample rate of 20000, 1 second of data
// corresponds to 20000 samples per channel.
// Note that on-chip RAM is limited, so setting this number excessively high will cause
// the chip to run out of memory during program execution.
#define NUMBER_OF_SECONDS_TO_ACQUIRE 0.5

// Which of the RHD chip's amplifier channels is selected as the starting point to have its data
// saved and transmitted via USART.
#define FIRST_SAMPLED_CHANNEL 8

// How many channels (starting from FIRST_SAMPLED_CHANNEL) to have their data saved and transmitted via USART.
#define NUM_SAMPLED_CHANNELS 4

// Digital bandpass filter configuration.
// Uncomment to enable on-chip Butterworth IIR bandpass filtering before USART transmission.
#define ENABLE_BANDPASS_FILTER

// Bandpass frequency limits in Hz.
#define FILTER_LOW_CUTOFF_HZ   20.0f
#define FILTER_HIGH_CUTOFF_HZ  30.0f

// Filter order: 2, 4, 6, or 8. Higher order = sharper rolloff but more computation.
#define FILTER_ORDER 4

// Spike detection configuration (requires ENABLE_BANDPASS_FILTER).
// Uncomment to enable real-time spike detection on the blue LED (PB7).
#define ENABLE_SPIKE_DETECTION

// Voltage threshold in microvolts. Negative value = detect negative-going spikes.
#define SPIKE_THRESHOLD_UV  -75.0f

// Dead time after detection in milliseconds. Prevents double-counting the same spike.
#define SPIKE_REFRACTORY_MS  1.5f

// How long the blue LED stays on after a spike is detected (in milliseconds).
#define SPIKE_LED_PULSE_MS  250.0f

// 60 Hz notch filter configuration. Fully independent of bandpass and spike detection.
// Uncomment to enable a 2nd-order IIR notch filter that removes power-line interference.
#define ENABLE_NOTCH_FILTER

// Centre frequency to reject, in Hz. Use 50.0f for regions with 50 Hz mains.
#define NOTCH_FREQUENCY_HZ  60.0f

// Quality factor. Higher Q = narrower notch. 30 gives ~2 Hz bandwidth at 60 Hz.
#define NOTCH_Q_FACTOR  30.0f

// Which peripherals/handles should be used by software, depending on if HAL or LL drivers are used.
// If the user wishes to use a different peripheral, for example SPI2 instead of SPI3, then that
// change should be made here (in addition to configuring that peripheral properly in the .ioc file).
#ifdef USE_HAL
#define USART huart1
#define SPI hspi3
#define INTERRUPT_TIM htim3
#else
#define USART USART1
#define SPI SPI3
#define INTERRUPT_TIM TIM3
#define DMA GPDMA1
#define DMA_TX_CHANNEL LL_DMA_CHANNEL_12
#define DMA_RX_CHANNEL LL_DMA_CHANNEL_13
#define DMA_USART_CHANNEL LL_DMA_CHANNEL_0
#endif

#endif /* INC_USERCONFIG_H_ */
