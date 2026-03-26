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

#include "userfunctions.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

#if defined(ENABLE_SPIKE_DETECTION) && !defined(ENABLE_BANDPASS_FILTER)
#error "ENABLE_SPIKE_DETECTION requires ENABLE_BANDPASS_FILTER to be defined"
#endif

#if defined(ENABLE_BANDPASS_FILTER) || defined(ENABLE_NOTCH_FILTER)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
	float b0, b1, b2, a1, a2;
	float x1[NUM_SAMPLED_CHANNELS];
	float x2[NUM_SAMPLED_CHANNELS];
	float y1[NUM_SAMPLED_CHANNELS];
	float y2[NUM_SAMPLED_CHANNELS];
} BiquadSection;

static uint16_t filtered_output[NUM_SAMPLED_CHANNELS];

static inline float biquad_process(BiquadSection *s, int ch, float input)
{
	float output = s->b0 * input + s->b1 * s->x1[ch] + s->b2 * s->x2[ch]
	             - s->a1 * s->y1[ch] - s->a2 * s->y2[ch];
	s->x2[ch] = s->x1[ch];
	s->x1[ch] = input;
	s->y2[ch] = s->y1[ch];
	s->y1[ch] = output;
	return output;
}

#ifdef ENABLE_BANDPASS_FILTER

#define NUM_BIQUAD_SECTIONS (FILTER_ORDER / 2)

typedef struct {
	BiquadSection hpf[NUM_BIQUAD_SECTIONS];
	BiquadSection lpf[NUM_BIQUAD_SECTIONS];
} BandpassFilter;

static BandpassFilter bandpass_filter;

#ifdef ENABLE_SPIKE_DETECTION
static uint16_t spike_refractory_samples;
static uint16_t spike_led_pulse_samples;
static uint16_t refractory_counter[NUM_SAMPLED_CHANNELS];
static uint16_t led_pulse_counter;
#endif

void init_bandpass_filter(float sample_rate)
{
	for (int i = 0; i < NUM_BIQUAD_SECTIONS; i++) {
		float angle = (float)M_PI * (2.0f * i + 1.0f) / (2.0f * FILTER_ORDER);
		float Q = 1.0f / (2.0f * cosf(angle));

		float K_hp = tanf((float)M_PI * FILTER_LOW_CUTOFF_HZ / sample_rate);
		float norm_hp = 1.0f / (1.0f + K_hp / Q + K_hp * K_hp);
		bandpass_filter.hpf[i].b0 = norm_hp;
		bandpass_filter.hpf[i].b1 = -2.0f * norm_hp;
		bandpass_filter.hpf[i].b2 = norm_hp;
		bandpass_filter.hpf[i].a1 = 2.0f * (K_hp * K_hp - 1.0f) * norm_hp;
		bandpass_filter.hpf[i].a2 = (1.0f - K_hp / Q + K_hp * K_hp) * norm_hp;

		float K_lp = tanf((float)M_PI * FILTER_HIGH_CUTOFF_HZ / sample_rate);
		float norm_lp = 1.0f / (1.0f + K_lp / Q + K_lp * K_lp);
		bandpass_filter.lpf[i].b0 = K_lp * K_lp * norm_lp;
		bandpass_filter.lpf[i].b1 = 2.0f * K_lp * K_lp * norm_lp;
		bandpass_filter.lpf[i].b2 = K_lp * K_lp * norm_lp;
		bandpass_filter.lpf[i].a1 = 2.0f * (K_lp * K_lp - 1.0f) * norm_lp;
		bandpass_filter.lpf[i].a2 = (1.0f - K_lp / Q + K_lp * K_lp) * norm_lp;

		for (int ch = 0; ch < NUM_SAMPLED_CHANNELS; ch++) {
			bandpass_filter.hpf[i].x1[ch] = 0.0f;
			bandpass_filter.hpf[i].x2[ch] = 0.0f;
			bandpass_filter.hpf[i].y1[ch] = 0.0f;
			bandpass_filter.hpf[i].y2[ch] = 0.0f;
			bandpass_filter.lpf[i].x1[ch] = 0.0f;
			bandpass_filter.lpf[i].x2[ch] = 0.0f;
			bandpass_filter.lpf[i].y1[ch] = 0.0f;
			bandpass_filter.lpf[i].y2[ch] = 0.0f;
		}
	}

#ifdef ENABLE_SPIKE_DETECTION
	spike_refractory_samples = (uint16_t)(SPIKE_REFRACTORY_MS * sample_rate / 1000.0f);
	spike_led_pulse_samples = (uint16_t)(SPIKE_LED_PULSE_MS * sample_rate / 1000.0f);
	led_pulse_counter = 0;
	for (int ch = 0; ch < NUM_SAMPLED_CHANNELS; ch++) {
		refractory_counter[ch] = 0;
	}
#endif
}

#endif /* ENABLE_BANDPASS_FILTER */

#ifdef ENABLE_NOTCH_FILTER

static BiquadSection notch_filter;

void init_notch_filter(float sample_rate)
{
	float w0 = 2.0f * (float)M_PI * NOTCH_FREQUENCY_HZ / sample_rate;
	float alpha = sinf(w0) / (2.0f * NOTCH_Q_FACTOR);

	float a0 = 1.0f + alpha;
	notch_filter.b0 =  1.0f / a0;
	notch_filter.b1 = -2.0f * cosf(w0) / a0;
	notch_filter.b2 =  1.0f / a0;
	notch_filter.a1 = -2.0f * cosf(w0) / a0;
	notch_filter.a2 =  (1.0f - alpha) / a0;

	for (int ch = 0; ch < NUM_SAMPLED_CHANNELS; ch++) {
		notch_filter.x1[ch] = 0.0f;
		notch_filter.x2[ch] = 0.0f;
		notch_filter.y1[ch] = 0.0f;
		notch_filter.y2[ch] = 0.0f;
	}
}

#endif /* ENABLE_NOTCH_FILTER */

#endif /* ENABLE_BANDPASS_FILTER || ENABLE_NOTCH_FILTER */

// Specify condition that should result in the main while loop ending.
// By default, escape once NUMBER_OF_SECONDS_TO_ACQUIRE seconds of data has been gathered.
int loop_escape(void)
{
	// Escape once sample memory capacity (default 1 second of data) has been reached.
#ifdef OFFLINE_TRANSFER
	return sample_counter > per_channel_sample_memory_capacity;
#else
	return 0;
#endif
}


// Write any desired data from this sequence to memory.
// By default, only the result corresponding to a CONVERT on FIRST_SAMPLED_CHANNEL is saved per sequence.
void write_data_to_memory(void)
{
#ifdef OFFLINE_TRANSFER
	// Save single sample to sample_memory array.
	for (int i = 0; i < NUM_SAMPLED_CHANNELS; i++) {
		sample_memory[(sample_counter * NUM_SAMPLED_CHANNELS) + i] = command_sequence_MISO[FIRST_SAMPLED_CHANNEL + i + 2];
	}
	sample_counter++;

//	// Read results of aux command slots (not used in this sample example).
//	// For more advanced programs that require reading of aux command results, those would be read and saved here.
//	uint16_t aux0_result = command_sequence_MISO[34]; // Result of AUX SLOT 1 from this command sequence
//	uint16_t aux1_result = command_sequence_MISO[0];  // Result of AUX SLOT 2 from the previous command sequence
//	uint16_t aux2_result = command_sequence_MISO[1];  // Result of AUX SLOT 3 from the previous command sequence
#endif
}


// Determine if data is ready to be transmitted, and if so, transmit (for example via USART).
void transmit_data_realtime(void)
{
#ifndef OFFLINE_TRANSFER

#if defined(ENABLE_BANDPASS_FILTER) || defined(ENABLE_NOTCH_FILTER)

#ifdef ENABLE_SPIKE_DETECTION
	if (led_pulse_counter > 0) {
		led_pulse_counter--;
		if (led_pulse_counter == 0) {
			write_pin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, false);
		}
	}
#endif

	for (int ch = 0; ch < NUM_SAMPLED_CHANNELS; ch++) {
		float sample = (float)command_sequence_MISO[FIRST_SAMPLED_CHANNEL + ch + 2] - 32768.0f;

#ifdef ENABLE_BANDPASS_FILTER
		for (int s = 0; s < NUM_BIQUAD_SECTIONS; s++) {
			sample = biquad_process(&bandpass_filter.hpf[s], ch, sample);
		}

		for (int s = 0; s < NUM_BIQUAD_SECTIONS; s++) {
			sample = biquad_process(&bandpass_filter.lpf[s], ch, sample);
		}
#endif

#ifdef ENABLE_NOTCH_FILTER
		sample = biquad_process(&notch_filter, ch, sample);
#endif

#ifdef ENABLE_SPIKE_DETECTION
		float sample_uv = sample * 0.195f;
		if (refractory_counter[ch] > 0) {
			refractory_counter[ch]--;
		} else if (sample_uv < SPIKE_THRESHOLD_UV) {
			write_pin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, true);
			led_pulse_counter = spike_led_pulse_samples;
			refractory_counter[ch] = spike_refractory_samples;
		}
#endif

		float result = sample + 32768.0f;
		if (result < 0.0f) result = 0.0f;
		if (result > 65535.0f) result = 65535.0f;
		filtered_output[ch] = (uint16_t)result;
	}
	transmit_dma_to_usart(&filtered_output[0], NUM_SAMPLED_CHANNELS * sizeof(uint16_t));
#else
	transmit_dma_to_usart(&command_sequence_MISO[FIRST_SAMPLED_CHANNEL + 2], NUM_SAMPLED_CHANNELS * sizeof(uint16_t));
#endif

#endif
}


// Transmit accumulated data after acquisition has finished (for example via USART).
void transmit_data_offline(void)
{
	// This is a relatively large transfer, too much for a single HAL DMA function call.
	// Ideally, we'd do something like:
	//	if (HAL_UART_Transmit(&USART, (uint8_t*) &sample_memory[0], NUM_SAMPLED_CHANNELS * SAMPLES_IN_MEMORY * sizeof(uint16_t), HAL_MAX_DELAY) != HAL_OK)
	//	{
	//		Error_Handler();
	//	}
	// but, 160,000 byte (if NUM_SAMPLED_CHANNELS is 4 and SAMPLES_IN_MEMORY is 20000) transfer too much for a single HAL function call.

	// 2*samples_per_chunk needs to fit into a uint16_t (max value 65535), so the max value of samples_per_chunk
	// is 32767. Ideally, total_samples_in_memory divides into this value cleanly, so 20000 is a reasonable candidate.
	// However, for reasons that are unclear, at high Baud rates, large transfers seem more likely to fail. So, dividing
	// into very small chunks seems to be the most reliable at high Baud rates.

	// We do the same thing for LL, for consistency - optimized performance is not critical for offline transfers, so there is likely
	// no significant downside to chunking data into many smaller transfers.

	const uint16_t samples_per_chunk = 1;
	const uint32_t total_samples_in_memory = NUM_SAMPLED_CHANNELS * calculate_sample_rate() * NUMBER_OF_SECONDS_TO_ACQUIRE;
	const uint32_t num_chunks = floor(total_samples_in_memory / samples_per_chunk);
	const uint16_t remaining_samples = total_samples_in_memory % samples_per_chunk;

	// Transmit multiple complete chunks of data
	for (int i = 0; i < num_chunks; i++) {
		uart_ready = false;
		transmit_dma_to_usart(&sample_memory[samples_per_chunk * i], samples_per_chunk * sizeof(uint16_t));
		while (!uart_ready) {}
	}

	// Transmit any remaining data too small to fit in a complete chunk
	if (remaining_samples > 0) {
		uart_ready = false;
		transmit_dma_to_usart(&sample_memory[samples_per_chunk * num_chunks], remaining_samples * sizeof(uint16_t));
		while (!uart_ready) {}
	}
}


// Configure and transmit register values.
// Initial register values default to the same default settings in the RHX software.
// Any desired changes to these values added after the 'write_initial_reg_values()' function call.
void configure_registers(void)
{
	write_initial_reg_values(&parameters);

	/* Make any changes that differ from defaults here. For example, configure registers 5 and 7 for impedance check: */

//	// Reg 5: Set zcheck_DAC_power, zcheck_en, zcheck_scale, and zcheck_polarity
//	parameters->zcheck_DAC_power = true;
//	parameters->zcheck_en = true;
//	set_zcheck_scale(parameters, ZcheckCs1pF);
//	set_zcheck_polarity(parameters, ZcheckPositiveInput);
//	write_command(5, get_register_value(parameters, 5));
//
//	// Reg 6: (Actual DAC value which changes over time - instead of setting once here, this should be written sample-by-sample in an aux command list).
//
//	// Reg 7: Set zcheck_select
//	set_zcheck_channel(parameters, FIRST_SAMPLED_CHANNEL);
//	write_command(7, get_register_value(parameters, 7));
}


// Configure the CONVERT commands that are loaded at the beginning of command_sequence_MOSI.
// By default, channels from 0 to CONVERT_COMMANDS_PER_SEQUENCE - 1 (0 to 31) are loaded consecutively (0, 1, 2, 3, ... 31).
void configure_convert_commands(void)
{
	// If default ordering of channel CONVERT commands (0, 1, 2, 3, ... 31) is desired, pass a NULL 2nd parameter to create_convert_sequence().
	create_convert_sequence(NULL);

	// If a custom ordering of channel CONVERT commands is instead desired, create a uint8_t array of size CONVERT_COMMANDS_PER_SEQUENCE
	// and populate each entry with the desired channel number. Then pass this array as the 2nd parameter to create_convert_sequence().
	// For example, if sampling in descending order from CONVERT_COMMANDS_PER_SEQUENCE - 1 (31 to 0) is desired:
	//	uint8_t channel_numbers[CONVERT_COMMANDS_PER_SEQUENCE] = {0};
	//	for (int i = 0; i < CONVERT_COMMANDS_PER_SEQUENCE; i++) {
	//		channel_numbers[i] = (CONVERT_COMMANDS_PER_SEQUENCE - 1) - i;
	//	}
	//	create_convert_sequence(channel_numbers);
}


// Configure the AUX commands that are loaded at the end of command_sequence_MOSI.
// By defaults, command lists from 0 to AUX_COMMANDS_PER_SEQUENCE - 1 (0 to 2) are loaded consecutively (32, 33, 34).
void configure_aux_commands(void)
{
	  // All create_command_list functions return -1 to indicate failure.
	  // Additionally, they should all be used to create command lists of length AUX_COMMAND_LIST_LENGTH, except
	  // for create_command_list_zcheck_DAC. This function returns a command list with a length that depends on the
	  // desired frequency, so if using this command list it's important to set zcheck_DAC_command_slot_position to 0, 1, or
	  // 2 (one of the 3 command slots) to indicate its position, and set zcheck_DAC_command_list_length so that during
	  // execution of this list, after the length has been reached it can begin at 0 again.

	// Slot 0: Write RHD register loading to aux_command_list[0], so that the register values saved in software (parameters) are continually re-written.
	create_command_list_RHD_register_config(&parameters, (uint16_t*) aux_command_list[0], false, AUX_COMMAND_LIST_LENGTH);

	// Slot 1: Write dummy reads to aux_command_list[1], so that register 40 is repeatedly read.
	create_command_list_dummy(&parameters, (uint16_t*) aux_command_list[1], AUX_COMMAND_LIST_LENGTH, read_command(40));

	// Slot 2: Write dummy reads to aux_command_list[2], so that register 41 is repeatedly read.
	create_command_list_dummy(&parameters, (uint16_t*) aux_command_list[2], AUX_COMMAND_LIST_LENGTH, read_command(41));

	// NOTE: If an impedance check command list is desired, it is created and used slightly differently, because its length is not AUX_COMMAND_LIST_LENGTH
	// but rather depends on impedance test signal frequency. For this demonstration, a zcheck_DAC command list is created and populates aux command slot 2.
	// In this case, the above creation of a dummy command on slot 2 would be redundant and should be commented out.

	// Write impedance check DAC control to aux_command_list[2], so that a sine wave is approximated by the DAC.
	// Note that, as opposed to all other command lists which should be AUX_COMMAND_LIST_LENGTH long, these
	// zcheck_DAC commands can have different lengths depending on desired frequency. To handle this, be sure to:
	// a) assign create_command_list_zcheck_DAC()'s return value to zcheck_DAC_command_list_length, and
	// b) assign which command slot the zcheck_DAC command list is in to zcheck_DAC_command_slot_position.
	// In order to use this, uncomment the declarations of the variables below, located in rhdinterface.h
//	zcheck_DAC_command_list_length = create_command_list_zcheck_DAC(parameters, (uint16_t*) aux_command_list[2], 1000.0, 100);
//	zcheck_DAC_command_slot_position = 2;
}


// Use DMA to transmit num_bytes of data from memory pointer tx_data directly to USART.
// Non-blocking, so it may be helpful to set the 'uart_ready' variable to 0 prior to this function call,
// monitor it, and hold off on further transmissions until the USART Tx complete callback sets it to 1.
void transmit_dma_to_usart(volatile const uint16_t* const tx_data, uint16_t num_bytes)
{
#ifdef USE_HAL
	if (HAL_UART_Transmit_DMA(&USART, (uint8_t*) tx_data, num_bytes) != HAL_OK)
	{
		Error_Handler();
	}
#else
	// Configure the DMA channel data size
	LL_DMA_SetBlkDataLength(DMA, DMA_USART_CHANNEL, num_bytes);

	// Clear all interrupt flags
	LL_DMA_ClearFlag_TC(DMA, DMA_USART_CHANNEL);
	LL_DMA_ClearFlag_HT(DMA, DMA_USART_CHANNEL);
	LL_DMA_ClearFlag_DTE(DMA, DMA_USART_CHANNEL);
	LL_DMA_ClearFlag_ULE(DMA, DMA_USART_CHANNEL);
	LL_DMA_ClearFlag_USE(DMA, DMA_USART_CHANNEL);
	LL_DMA_ClearFlag_SUSP(DMA, DMA_USART_CHANNEL);
	LL_DMA_ClearFlag_TO(DMA, DMA_USART_CHANNEL);

	// Configure DMA channel source address
	LL_DMA_SetSrcAddress(DMA, DMA_USART_CHANNEL, (uint32_t) tx_data);

	// Configure DMA channel destination address
	LL_DMA_SetDestAddress(DMA, DMA_USART_CHANNEL, LL_USART_DMA_GetRegAddr(USART, LL_USART_DMA_REG_DATA_TRANSMIT));

	// Enable common interrupts: Transfer Complete and Transfer Errors ITs
	LL_DMA_EnableIT_TC(DMA, DMA_USART_CHANNEL);
	LL_DMA_EnableIT_DTE(DMA, DMA_USART_CHANNEL);
	LL_DMA_EnableIT_ULE(DMA, DMA_USART_CHANNEL);
	LL_DMA_EnableIT_USE(DMA, DMA_USART_CHANNEL);
	LL_DMA_EnableIT_TO(DMA, DMA_USART_CHANNEL);

	// Write LAP low
	LL_DMA_SetLinkAllocatedPort(DMA, DMA_USART_CHANNEL, LL_DMA_LINK_ALLOCATED_PORT0);

	// Clear TC flag in ICR register
	LL_USART_ClearFlag_TC(USART);
	LL_USART_EnableIT_TC(USART);

	// Enable DMA channel
	LL_DMA_EnableChannel(DMA, DMA_USART_CHANNEL);

	// Enable DMA transfer for transmit request by setting DMAT bit in UART CR3 register
	LL_USART_EnableDMAReq_TX(USART);
#endif
}
