/*
 * tasks_handler.c
 *
 *  Created on: Jan 13, 2025
 *      Author: vaknin
 */
#include <stdint.h>
#include <stdlib.h>
#include "main.h"

extern uint32_t leds_execute(char* option);
extern uint32_t rtc_execute(int option);


char* error_cmd = "error: invalid input command\n";

/**
 * @brief This task handle the UART messages transmission
 *
 * @param cmd - Pointer to the command struct
 *
 * @return Non zero value if the queue is empty or 'end of message' character is missing
 *
 * @note cmd payload size should be equal or larger than queue size
 * */
static int extract_command(command_t* cmd){
	BaseType_t ret;
	char item; 		// data byte
	uint32_t i = 0;

	ret = uxQueueMessagesWaiting(q_data);
	if(ret <= 0){
		return -1;
	}

	do{
		// Pull queue data
		ret = xQueueReceive(q_data, &item, 0);
		if(ret == pdTRUE){
			cmd->payload[i++] = item;
		}
		else return -1;	/* No more items in the q - no '\n' received - Invalid command */
	}while(item != '\n');

	// Handle the end of command string
	cmd->payload[i-1] = '\0'; // Replace '\n' with '\0';
	cmd->len = i-1;

	return 0;
}

/**
 * @brief This
 *
 * @param
 *
 * @return
 *
 * @note
 * */
void process_command(command_t* cmd){

	BaseType_t ret = extract_command(cmd);
	if(ret){
		return;
	}

	switch(app_curr_state){
	case sMainMenu:
		xTaskNotify(menu_task_handle, (uint32_t)cmd, eSetValueWithOverwrite);
		break;
	case sLedEffect:
		xTaskNotify(leds_task_handle, (uint32_t)cmd, eSetValueWithOverwrite);
		break;
	case sRtcMenu:
	case sRtcTimeConfig:
	case sRtcDateConfig:
	case sRtcReport:
		xTaskNotify(rtc_task_handle, (uint32_t)cmd, eSetValueWithOverwrite);
		break;
	}

}

/**
 * This function receives the USRT messages from the queue
 *
 *  */
void command_handle_task_handler(void* params){
	command_t cmd;

	while(1){
		// Wait for data
		if(xTaskNotifyWait(0, 0, NULL, portMAX_DELAY)){
			process_command(&cmd);
		}
	}
}

/**
 * @brief This task handle the UART messages transmission
 *
 * @return void
 * */
void print_task_handler(void* params){
	uint32_t msg; // buffer

	while(1){
		// receive item from the queue
		if(xQueueReceive(q_print, (void*)&msg, portMAX_DELAY))
		{
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen((char*)msg), HAL_MAX_DELAY);
		}
	}
}

/**
 * @brief This
 *
 * @param
 *
 * @return
 *
 * @note
 * */
void menu_task_handler(void* params){
	BaseType_t status;
	command_t* rx_cmd;
	uint32_t cmd_value;
	int option = -1;

	char* menu_msg =  "\n=====================\n"
						"|\tMENU\t\t|\n"
					    "=====================\n"
					    "LED effect\t--> 0\n"
					    "Date and time\t--> 1\n"
					    "Exit\t\t--> 2\n"
					    "Enter your choice here: ";

	while(1){
		// Add the menu start message to the TX queue
		xQueueSend(q_print, &menu_msg, portMAX_DELAY);

		// Wait for the user command
		xTaskNotifyWait(0, 0, &cmd_value, portMAX_DELAY);

		rx_cmd = (command_t*)cmd_value;

		if(rx_cmd->len == 1){
			option = atoi((char*)&rx_cmd->payload[0]);

			switch(option){
				case 0: // LEDs functions
					app_curr_state = sLedEffect;
					status = xTaskNotify(leds_task_handle, 0, eNoAction);
					configASSERT(status == pdPASS);
					break;

				case 1: // Date and time
					app_curr_state = sRtcMenu;
					status = xTaskNotify(rtc_task_handle, 0, eNoAction);
					configASSERT(status == pdPASS);
					break;

				case 2:	//Exit
					printf("deleting menu task\n");
					/* options: delete All tasks, deinit uart*/
					break;

				default:
					break;
			}
		}
		else{
			// Invalid input
			xQueueSend(q_print, &error_cmd, portMAX_DELAY);
			continue;
		}

		// Wait for the user command to start menu again
		status = xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
		configASSERT(status == pdPASS);
	}

}

/**
 * @brief This
 *
 * @param
 *
 * @return
 *
 * @note
 * */
void leds_task_handler(void* params){
	uint32_t ret;
	command_t* rx_cmd;
	tx_command_t tx_msg;
	(void)tx_msg;
	uint32_t cmd_value;
	char option[5];

	char* led_msg = "=====================\n"
					"|\tLEDs\t\t|\n"
				    "=====================\n"
				    "Options: exit, e1, e2, e3, e4\n"
				    "Enter your choice here: ";


	while(1){

		// Wait for the user command to activate LEDS
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY); //Go Blocking state till Notification arrive

		while(1){
			// Print led_msg
			xQueueSend(q_print, &led_msg, portMAX_DELAY);

			// Wait for the user command
			xTaskNotifyWait(0, 0, &cmd_value, portMAX_DELAY);

			rx_cmd = (command_t*)cmd_value;

			if(rx_cmd->len <= 4){
				// Get option
				strncpy(option, (char*)rx_cmd->payload, sizeof(option));
			}
			else{
				// Invalid input
				xQueueSend(q_print, &error_cmd, 0);
				continue;
			}

			// Execute LEDs function
			ret = leds_execute(option);
			if(ret){
				break;
			}
		}

	}
}

/**
 * @brief This function is the RTC task handler
 *
 * @param parameters
 *
 * @return	void
 * */
void rtc_task_handler(void* params){
	uint32_t ret;
	int option;

	tx_command_t tx_msg;
	(void)tx_msg;

	RTC_TimeTypeDef stime;
	RTC_DateTypeDef sdate;
	HAL_StatusTypeDef status;

	uint32_t cmd_value;
	command_t* rx_cmd;

	const char* rtc_hdr_msg =  "=====================\n"
						  	   "|\tRTC\t\t|\n"
						  	   "=====================\n";

	const char* rtc_menu_msg = "Configure time\t\t--> 0\n"
						       "Configure date\t\t--> 1\n"
							   "Enable reporting\t--> 2\n"
							   "Exit\t\t\t--> 3\n"
							   "Debug\t\t\t--> 4\n"
							   "Enter your choice here: ";

	// Get the time
	status = HAL_RTC_GetTime(&hrtc, &stime, RTC_FORMAT_BIN);
	configASSERT(status == HAL_OK);

	// Get the date
	status = HAL_RTC_GetDate(&hrtc, &sdate, RTC_FORMAT_BIN);
	configASSERT(status == HAL_OK);

	while(1){

		// Wait for the user command
		xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);

		while(1){

			xQueueSend(q_print, &rtc_hdr_msg, portMAX_DELAY);
			//rtc_q_print_time_n_date();
			xQueueSend(q_print, &rtc_menu_msg, portMAX_DELAY);

			// Wait for the user choice
			xTaskNotifyWait(0, 0, &cmd_value, portMAX_DELAY);

			rx_cmd = (command_t*)cmd_value;

			if(rx_cmd->len == 1){
				option = atoi((char*)&rx_cmd->payload[0]);
			}
			else{
				// Invalid input
				xQueueSend(q_print, &error_cmd, 0);
				continue;
			}

			// Execute command
			ret = rtc_execute(option);
			if(ret){
				break;
			}

		}
	}

}

