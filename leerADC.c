/*
 * leerADC.c
 *
 *  Created on: 24 nov 2026
 *      Author: ehu
 */
#include <stdint.h>
#include "leerADC.h"
#include "main.h"
#define ADC_VREF_MV 3300U //TENSION REF
#define ADC_LEVELS 4096U //2^12
extern ADC_HandleTypeDef hadc1;
uint32_t read_channel(uint32_t canal)
{
	ADC_ChannelConfTypeDef s = {0};
	s.Channel = canal;
	s.Rank = 1;
	s.SamplingTime = ADC_SAMPLETIME_28CYCLES;
	HAL_ADC_ConfigChannel(&hadc1, &s);

	HAL_ADC_Start(&hadc1);//orden de inicio
	HAL_ADC_PollForConversion(&hadc1, 10);//espera el EOC

	return  HAL_ADC_GetValue(&hadc1); //0 a 4095
		 // volts_mv = adc_to_mv(adc_binary);
}
uint32_t adc_to_mv(uint32_t adc_binary)
{
	return adc_binary * ADC_VREF_MV / ADC_LEVELS;
}
