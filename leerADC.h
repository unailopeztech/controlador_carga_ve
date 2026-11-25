/*
 * leerADC.h
 *
 *  Created on: 24 nov 2026
 *      Author: ehu
 */

#ifndef INC_LEERADC_H_
#define INC_LEERADC_H_

void cambiar_canal(uint32_t canal);
uint32_t adc_to_mv(uint32_t adc_binary);

#endif /* INC_LEERADC_H_ */
