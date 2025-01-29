/*
 * stm32f407x_disc_board.h
 *
 *  Created on: Jan 8, 2025
 *      Author: vaknin
 */

#include "stm32f4xx_hal.h"

// USART API
#define USART2_GPIO_port GPIOA
#define USART2_TX_PIN 	 GPIO_PIN_2 // pa2
#define USART2_RX_PIN    GPIO_PIN_3 // pa3

/* LEDS */
#define LED_GPIO_PORT GPIOD
#define GREEN_LED_PIN	GPIO_PIN_12
#define ORANGE_LED_PIN	GPIO_PIN_13
#define RED_LED_PIN		GPIO_PIN_14
#define BLUE_LED_PIN	GPIO_PIN_15

/* Button */
#define USR_BUTTON_GPIO_PORT GPIOA
#define USR_BUTTON_PIN		 GPIO_PIN_0






