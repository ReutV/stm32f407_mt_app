/*
 * led_effect.c
 *
 *  Created on: Jan 13, 2025
 *      Author: vaknin
 */
#include "main.h"

static char* leds_error_msg = "error (leds_effect): invalid input command\n";

volatile eLeds_exec_t exec_flag;

/**
 * @brief This function execute the LEDs functions
 *
 * */
void leds_execute_handler(void){
	switch(exec_flag){
		case exec_e1:
			exec_leds_e1();
			break;
		case exec_e2:
			exec_leds_e2();
			break;
		case exec_e3:
			exec_leds_e3();
			break;
		case exec_e4:
			exec_leds_e4();
			break;
		default:
			break;
	}
}

/**
 * @brief This function reset the LEDs
 *
 * */
void leds_turn_off(){
	HAL_GPIO_WritePin(LED_GPIO_PORT,GREEN_LED_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_GPIO_PORT,BLUE_LED_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_GPIO_PORT,ORANGE_LED_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(LED_GPIO_PORT,RED_LED_PIN, GPIO_PIN_RESET);
}

/**
 * @brief This function toggle LEDs state
 *
 * */
void exec_leds_e1(){

	HAL_GPIO_TogglePin(LED_GPIO_PORT,GREEN_LED_PIN);
	HAL_GPIO_TogglePin(LED_GPIO_PORT,RED_LED_PIN);
	HAL_GPIO_TogglePin(LED_GPIO_PORT,BLUE_LED_PIN);
	HAL_GPIO_TogglePin(LED_GPIO_PORT,ORANGE_LED_PIN);
}

/**
 * @brief This function toggle LEDs state
 *
 * */
void exec_leds_e2(void){
	static uint8_t pattern = 0xA;

	pattern = ~pattern;
	HAL_GPIO_WritePin(LED_GPIO_PORT, ORANGE_LED_PIN, (pattern >> 0) & 0x1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, RED_LED_PIN, 	 (pattern >> 1) & 0x1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, BLUE_LED_PIN, 	 (pattern >> 2) & 0x1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, GREEN_LED_PIN,  (pattern >> 3) & 0x1);
}

/**
 * @brief This function turn on sequence moving pattern clockwise
 *
 * */
void exec_leds_e3(){
	static uint8_t idx = 0;
	uint8_t mv_pattern = 0x1;

	mv_pattern = mv_pattern << idx;
	HAL_GPIO_WritePin(LED_GPIO_PORT, ORANGE_LED_PIN,(mv_pattern >> 0) & 1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, RED_LED_PIN,   (mv_pattern >> 1) & 1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, BLUE_LED_PIN,  (mv_pattern >> 2) & 1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, GREEN_LED_PIN, (mv_pattern >> 3) & 1);
	idx++;
	if(idx > 3){
		idx = 0;
	}
}

/**
 * @brief This function turn on sequence moving pattern counterclockwise
 *
 * */
void exec_leds_e4(){
	static uint8_t idx = 0;
	uint8_t mv_pattern = 0x8;

	mv_pattern = mv_pattern >> idx;
	HAL_GPIO_WritePin(LED_GPIO_PORT, ORANGE_LED_PIN,(mv_pattern >> 0) & 1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, RED_LED_PIN,   (mv_pattern >> 1) & 1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, BLUE_LED_PIN,  (mv_pattern >> 2) & 1);
	HAL_GPIO_WritePin(LED_GPIO_PORT, GREEN_LED_PIN, (mv_pattern >> 3) & 1);
	idx++;
	if(idx > 3){
		idx = 0;
	}
}

/**
 * @brief This function init the LEDs function execution
 *
 * @param option the option of the function to execute
 *
 * @retval uiny32_t Non zero value when exit back to Main Menu
 *
 * */
uint32_t leds_execute(char* option){
	BaseType_t status;
	static uint32_t start_tim_once_flag = 1;

	leds_turn_off();

	if(!strcmp(option, "e1")){
		exec_flag = exec_e1;
	}
	else if(!strcmp(option, "e2")){
		exec_flag = exec_e2;
	}
	else if(!strcmp(option, "e3")){
		exec_flag = exec_e3;
	}
	else if(!strcmp(option, "e4")){
		exec_flag = exec_e4;
	}
	else if(!strcmp(option, "exit")){
		// LEDs effect stop
		HAL_TIM_Base_Stop(&htim7);
		exec_flag = exec_none;
		leds_turn_off();

		// Back to main
		app_curr_state = sMainMenu;
		status = xTaskNotify(menu_task_handle, 0, eNoAction);
		configASSERT(status == pdPASS);
		start_tim_once_flag = 1;
		return 1;
	}
	else{
		// Invalid input
		xQueueSend(q_print, &leds_error_msg, 0);
	}

	// Start timer
	if(start_tim_once_flag){
		HAL_TIM_Base_Start_IT(&htim7);
		start_tim_once_flag = 0;
	}

	return 0;
}


