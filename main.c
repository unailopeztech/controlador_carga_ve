/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "leerADC.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum { EST_A, EST_B, EST_C, EST_D, EST_E } estado_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//#define ADC_BUF_LEN 400
#define LINEA_MAX          32U     /* tamaño de linea_rx, contando el '\0' */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//volatile uint16_t adc_buf;
volatile estado_t estado = EST_A;
extern volatile float Vpulso;
volatile float Vtemp;
volatile uint8_t temperatura;
extern volatile uint16_t adc_pulso;
extern volatile uint16_t I;
static uint8_t autorizado = 0;
volatile uint8_t evt_boton = 0;
volatile uint8_t evt_emergencia = 0;
volatile uint8_t emergencia = 0;
uint8_t byte_rx;                          /* donde la HAL deja cada byte */
char linea_rx[LINEA_MAX];                 /* la línea que se va formando */
uint32_t longitud_rx = 0;                 /* caracteres guardados en linea_rx (solo la callback) */
volatile uint8_t flag_linea_lista = 0;    /* 1: hay una línea completa (la pone la callback) */
volatile uint16_t adc_pulso;
volatile float Vpulso;
volatile uint8_t adc_listo=0;
volatile uint16_t adc_raw;
volatile uint32_t canal = ADC_CHANNEL_0;
volatile uint8_t toca_leer_a1 = 0;
extern volatile uint16_t duty;
static char tx_buf[48];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	static uint32_t t_ult = 0;
	if(GPIO_Pin == iniciar_detener_Pin && HAL_GetTick() - t_ult > 20){//antirrebote
		t_ult = HAL_GetTick();
		evt_boton=1;
	}
	else if(GPIO_Pin == emergencia_Pin && HAL_GetTick() - t_ult > 20){//antirrebote
		t_ult = HAL_GetTick();
		evt_boton=1;
		evt_emergencia=1;
	}
}
uint32_t adc_binary1;
uint32_t volts_mv1;

uint32_t t_ultimo = 0;
uint32_t t_led = 0;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADC_Start_IT(&hadc1);
  //HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);   // CC1E + CEN
  TIM3->SR = ~TIM_SR_UIF;
  TIM3->DIER |= TIM_DIER_UIE;
  TIM3->CCER |= TIM_CCER_CC1E;
  TIM3->CR1 |= TIM_CR1_CEN;

  void maquina_estados(){
	  if(evt_boton){evt_boton=0; autorizado = !autorizado;}
	  if(evt_emergencia){evt_emergencia=0;emergencia = !emergencia;}
	  switch(estado){
	  	  case EST_A:
	  		  autorizado = 0;
	  		  HAL_GPIO_WritePin(led_verde_GPIO_Port, led_verde_Pin, GPIO_PIN_SET);
	  		  HAL_GPIO_WritePin(led_azul_GPIO_Port, led_azul_Pin, GPIO_PIN_RESET);
	  		  HAL_GPIO_WritePin(led_rojo_GPIO_Port, led_rojo_Pin, GPIO_PIN_RESET);
	  		  if(adc_pulso < 4000){
	  			  if(!autorizado)estado=EST_B;
	  			  else if(autorizado && Vpulso>1.24)estado=EST_C;
	  			  else if(autorizado && Vpulso<1.21 && Vpulso>0.41)estado=EST_D;
	  			  else if(autorizado && Vpulso<0.41)estado=EST_E;
	  		  }
	  		  break;
	  	  case EST_B:

	  		  HAL_GPIO_WritePin(led_verde_GPIO_Port, led_verde_Pin, GPIO_PIN_RESET);
	  		  HAL_GPIO_WritePin(led_rojo_GPIO_Port, led_rojo_Pin, GPIO_PIN_RESET);

	  		  //HAL_Delay(500);
	  		  if(HAL_GetTick() - t_led >= 500)// ha pasado 0.5 segundo?
	  			  {

	  			  		  t_led = HAL_GetTick(); //siguiente instante, sin deriva
	  			  		  //tarea de cada medio segundo
	  			  		  HAL_GPIO_TogglePin(led_azul_GPIO_Port, led_azul_Pin);

	  			  }
	  		if(adc_pulso > 4000)estado=EST_A;
	  		if(autorizado && Vpulso>1.24)estado=EST_C;
	  		if(autorizado && Vpulso<1.21 && Vpulso>0.41)estado=EST_D;
	  		if(emergencia)estado=EST_E;
	  		  		  break;
	  	  case EST_C:
	  		  HAL_GPIO_WritePin(led_verde_GPIO_Port, led_verde_Pin, GPIO_PIN_RESET);
	  		  HAL_GPIO_WritePin(led_azul_GPIO_Port, led_azul_Pin, GPIO_PIN_SET);
	  		  HAL_GPIO_WritePin(led_rojo_GPIO_Port, led_rojo_Pin, GPIO_PIN_RESET);
	  		if(adc_pulso > 4000)estado=EST_A;
	  		else if(!autorizado)estado=EST_B;
	  		else if(autorizado && Vpulso<1.21 && Vpulso>0.41)estado=EST_D;
	  		else if((autorizado && Vpulso<0.41)||emergencia)estado=EST_E;
	  		  		  break;

	  	  case EST_D:
	  		  HAL_GPIO_WritePin(led_verde_GPIO_Port, led_verde_Pin, GPIO_PIN_RESET);
	  		  HAL_GPIO_WritePin(led_azul_GPIO_Port, led_azul_Pin, GPIO_PIN_SET);
	  		  HAL_GPIO_WritePin(led_rojo_GPIO_Port, led_rojo_Pin, GPIO_PIN_RESET);

	  		if(adc_pulso > 4000)estado=EST_A;
	  			  		else if(!autorizado)estado=EST_B;
	  			  		else if(autorizado && Vpulso>1.24)estado=EST_C;
	  			  		else if((autorizado && Vpulso<0.41)||emergencia)estado=EST_E;
	  		  		  break;
	  	  case EST_E:
	  		  HAL_GPIO_WritePin(led_verde_GPIO_Port, led_verde_Pin, GPIO_PIN_RESET);
	  		  HAL_GPIO_WritePin(led_azul_GPIO_Port, led_azul_Pin, GPIO_PIN_RESET);
	  		  HAL_GPIO_WritePin(led_rojo_GPIO_Port, led_rojo_Pin, GPIO_PIN_SET);
	  		if(adc_pulso > 4000 && !emergencia)estado=EST_A;
	  		  		  break;
	  	  }
  }
  HAL_UART_Receive_IT(&huart2, &byte_rx, 2);

   t_ultimo   = HAL_GetTick();
  //uint32_t t_5s   = HAL_GetTick();
   uint32_t last = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(flag_linea_lista == 1U){
		  I = (uint8_t)atoi(&linea_rx[0]);          /* "250" -> 250 */
		  flag_linea_lista=0;

	  }
	  maquina_estados();

	  if((HAL_GetTick() - t_ultimo >= 1000)&& !toca_leer_a1)// ha pasado 1 segundo?
	  {

	  		  t_ultimo = HAL_GetTick(); //siguiente instante, sin deriva
	  		  //tarea de cada segundo
	  		  toca_leer_a1 = 1;

	  }

	  if(adc_listo){
		  adc_listo = 0;

		  if(canal==ADC_CHANNEL_1)
		  {
			  adc_binary1 = adc_raw;
			  Vtemp = adc_to_mv(adc_binary1);
			  temperatura = (Vtemp - 500) / 10;
			  //ya leimos temperatura, volvemos a pa0 para la siguiente conversion del timer
			  canal = ADC_CHANNEL_0;
			  cambiar_canal(canal);

		  }
		  else
		  {
			  adc_pulso = adc_raw;
			  Vpulso = adc_to_mv(adc_pulso) / 1000.0;

			  //si pasó 1 segundo, cambiamos a pa1 la siguiente conversion
			  		  if(toca_leer_a1){
			  			  toca_leer_a1 = 0,
			  			  canal = ADC_CHANNEL_1;
			  			  cambiar_canal(canal);
			  		  }
		  }


		  HAL_ADC_Start_IT(&hadc1);
	  }


	  if(HAL_GetTick() - last >= 50)
	  {
		  last = HAL_GetTick();
		  if (huart2.gState == HAL_UART_STATE_READY)
		  {
			  uint32_t vpot_mv = (uint32_t)(Vpulso * 1000.0f); // 1.65 V → 1650 mV, sin %f

			  int n = snprintf(tx_buf, sizeof(tx_buf), "duty=%u porciento vpot=%lu mV I=%u A\r\n",
					  (unsigned)duty, vpot_mv, (unsigned)I);

			  HAL_UART_Transmit_IT(&huart2, (uint8_t *)tx_buf, n);
		  }
	  }

	//  if(temperatura > 50){
	//	  if(HAL_GetTick() - t_5s >= 5000){// ha pasado 5 segundos?
	//	  		  t_5s = HAL_GetTick(); //siguiente instante, sin deriva
	//	  }
	  //}

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_28CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC2REF;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|led_azul_Pin|led_verde_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(led_rojo_GPIO_Port, led_rojo_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin led_azul_Pin led_verde_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|led_azul_Pin|led_verde_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : emergencia_Pin iniciar_detener_Pin */
  GPIO_InitStruct.Pin = emergencia_Pin|iniciar_detener_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : led_rojo_Pin */
  GPIO_InitStruct.Pin = led_rojo_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_rojo_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC1) {

		adc_raw = HAL_ADC_GetValue(hadc);

		adc_listo = 1; // solo leer y avisar

		HAL_ADC_Stop_IT(hadc);
	}
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (huart == &huart2)
	  {
	    if ((byte_rx == '\r') || (byte_rx == '\n'))              /* Enter */
	    {
	      if ((longitud_rx > 0U) && (flag_linea_lista == 0U))
	      {
	        linea_rx[longitud_rx] = '\0';
	        flag_linea_lista = 1;
	        longitud_rx = 0;
	      }
	    }
	    else if ((flag_linea_lista == 0U) && (longitud_rx < (LINEA_MAX - 1U)))   /* cabe */
	    {
	      linea_rx[longitud_rx] = (char)byte_rx;
	      longitud_rx++;
	    }
	    HAL_UART_Receive_IT(&huart2, &byte_rx, 1);              /* volver a armar */
	  }
  }
//void HAL_UART_TxCpltCallback(){
	//tx_done = 1;
//}
  void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
  {
	  if (huart == &huart2)
	    {
	      longitud_rx = 0;                                        /* descartar la línea */
	      HAL_UART_Receive_IT(&huart2, &byte_rx, 1);              /* volver a armar */
	    }
  }
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
