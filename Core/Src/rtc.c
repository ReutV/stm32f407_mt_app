/*
 * rtc.c
 *
 *  Created on: Jan 13, 2025
 *      Author: vaknin
 */
#include <stdint.h>
#include <stdlib.h>

#include "main.h"

void time_configure(void);
void date_configure(void);
void rtc_q_print_time(void);
void rtc_report_time_stop(void);

/* RTC message buffer */
char rtc_time_buff[64] = {0};
char rtc_time_buff_cb[64] = {0};

/* Errors */
static char* rtc_error_cmd      	 = "error: Invalid rtc input command\n";
static char* rtc_error_invalid_state = "error: Invalid state\n";

/* Messages */
static char* rtc_hours_msg   = "Enter hours(0-23): ";
static char* rtc_minutes_msg = "Enter minutes(0-59): ";
static char* rtc_seconds_msg = "Enter seconds(0-59): ";
static char* rtc_day_msg     = "Enter date(1-31): ";
static char* rtc_month_msg   = "Enter month(1-12): ";
static char* rtc_weekDay_msg = "Enter day(1-7): ";
static char* rtc_year_msg    = "Enter year(0-99): ";
static char* rtc_report_msg  = "Enable reporting y/n ";

char weekDays[7][4] = {"Sun","Mon","Tue","Wed","Thu", "Fri","Sat"};

typedef enum {
	Time_hhState,		/* hours update state   */
	Time_mmState,		/* minutes update state */
	Time_ssState,		/* seconds update state */
	Time_stateTerm		/* Terminate state      */
}eTimeState_t;

typedef enum {
	Date_ddState,		/* date update state    */
	Date_mmState,		/* month update state   */
	Date_wdState,		/* WeekDay update state */
	Date_yyState,		/* year update state    */
	Date_stateTerm		/* Terminate state      */
}eDateState_t;

/**
 * @brief This function validate the date values
 *
 * @param state of the values update
 *
 * @return	pdFALSE if value exceeds
 *
 * */
BaseType_t validate_date_value(uint32_t value, eDateState_t state){

	switch(state){
		case Date_ddState:
			if(value < 1 || value > 31){
				return pdFALSE;
			}
			break;
		case Date_mmState:
			if(value < 1 || value > 12){
				return pdFALSE;
			}
			break;
		case Date_wdState:
			if(value < 1 || value > 7){
				return pdFALSE;
			}
			break;
		case Date_yyState:
			if(value < 0 || value > 99){
				return pdFALSE;
			}
			break;
		case Date_stateTerm:
			return pdTRUE;
		default:
			xQueueSend(q_print, &rtc_error_invalid_state, 0);
			return pdFALSE;
	}

	return pdTRUE;
}

/**
 * @brief This function validate the time values
 *
 * @param state of the values update
 *
 * @return	pdFALSE if value exceeds
 *
 * */
BaseType_t validate_time_value(uint32_t value, eTimeState_t state){

	switch(state){
		case Time_hhState:
			if(value < 0 || value > 23){
				return pdFALSE;
			}
			break;
		case Time_mmState:
			if(value < 0 || value > 59){
				return pdFALSE;
			}
			break;
		case Time_ssState:
			if(value < 0 || value > 59){
				return pdFALSE;
			}
			break;
		case Time_stateTerm:
			return pdTRUE;
		default:
			xQueueSend(q_print, &rtc_error_invalid_state, 0);
			return pdFALSE;
	}

	return pdTRUE;
}

/**
 * @brief This function reads the time from the rtc and print values to the console
 *
 * */
void rtc_q_print_time(void){
	RTC_TimeTypeDef sTimeCheck;
	HAL_RTC_GetTime(&hrtc, &sTimeCheck, RTC_FORMAT_BIN);
	printf("\nDebug: Hours: %02d, Minutes: %02d, Seconds: %02d\n", sTimeCheck.Hours, sTimeCheck.Minutes, sTimeCheck.Seconds);
}

/**
 * @brief This function reads the time and date and push to the queue
 *
 * */
void rtc_q_print_time_n_date(void){

	char* form;
	uint8_t day_idx;

	const char* hdr = "Current Time&Date ";

	char* data = rtc_time_buff;

	memset(rtc_time_buff, 0, sizeof(rtc_time_buff));

	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;

	memset(&sTime,0,sizeof(sTime));
	memset(&sDate,0,sizeof(sDate));

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	day_idx = sDate.WeekDay == RTC_WEEKDAY_SUNDAY ? 0 : sDate.WeekDay;

	form = sTime.TimeFormat == RTC_HOURFORMAT12_AM ? "AM" : "PM";

	// Display time in 12 hours format
	if(hrtc.Init.HourFormat == RTC_HOURFORMAT_12){
	    sprintf(data, "%02d:%02d:%02d [%s] [%s] %02d-%02d-%04d\n", sTime.Hours, sTime.Minutes,
	    		sTime.Seconds, form, (char*)&weekDays[day_idx], sDate.Date, sDate.Month, 2000 + sDate.Year);
	}
	else{ // 24 hours format (No AM/PM)
		sprintf(data, "%02d:%02d:%02d [%s] %02d-%02d-%04d\n", sTime.Hours, sTime.Minutes,
				sTime.Seconds, (char*)&weekDays[day_idx], sDate.Date, sDate.Month, 2000 + sDate.Year);
	}

	// Send message to queue
	xQueueSend(q_print, &hdr, portMAX_DELAY);
	// Send message to queue
	xQueueSend(q_print, &data, portMAX_DELAY);

}

/**
 * @brief Start rtc_timer
 *
 * */
void rtc_report_time_start(void){
	xTimerStart(rtc_timer, 0);
}

/**
 * @brief This function handle the report time option
 *
 * @note Activates the rtc_timer for periodic reporting
 * */
void rtc_report_time_enable(void){
	uint32_t cmd_value;
	command_t* rx_cmd;

	char option;

	xQueueSend(q_print, &rtc_report_msg, portMAX_DELAY);

	xTaskNotifyWait(0, 0, &cmd_value, portMAX_DELAY);

	rx_cmd = (command_t*)cmd_value;

	if(rx_cmd->len == 1){
		// Get option
		option = rx_cmd->payload[0];
	}
	else{
		// Invalid input
		xQueueSend(q_print, &rtc_error_cmd, 0);
		return;
	}

	if(option == 'y' || option == 'Y'){
		rtc_report_time_start();
	}
	else if(option == 'n' || option == 'N'){
		// Stop rtc timer
		rtc_report_time_stop();
	}
	else{
		// Invalid Input
		xQueueSend(q_print, &rtc_error_cmd, 0);
	}
}

/**
 * @brief This stops the @rtc_timer
 *
 * */
void rtc_report_time_stop(void){
	xTimerStop(rtc_timer, 0);
}

/**
 * @brief This
 *
 * @param sTime Pointer to sTime structure
 *
 * @return None
 *
 * */
void rtc_time_format_set(RTC_TimeTypeDef* sTime){
	uint8_t hours = sTime->Hours;

	if(hrtc.Init.HourFormat == RTC_HOURFORMAT_12){
	    if (hours > 12){ // 13 - 23
	    	sTime->Hours = hours - 12;
	    	sTime->TimeFormat = RTC_HOURFORMAT12_PM;
	    }
	    else if(hours == 12){ // 12
	    	sTime->TimeFormat = RTC_HOURFORMAT12_PM;
	    }
	    else if(hours >= 1 && hours < 12){ // 1 - 11
	    	/* Do nothing */
	    	sTime->TimeFormat = RTC_HOURFORMAT12_AM;
	    }
	    else if(hours == 0){ // 0
	    	sTime->Hours = 12;
	    	sTime->TimeFormat = RTC_HOURFORMAT12_AM;
		}
	}
}

/**
 * @brief This function handle the time configuration
 *
 * */
void time_configure(void){
	BaseType_t ret;
	uint32_t cmd_value;
	uint8_t value;
	command_t* rx_cmd;

	RTC_TimeTypeDef sTime = {0};

	// init state
	eTimeState_t time_state = Time_hhState;
	char* rtc_time_msg =  rtc_hours_msg;

	do{
		xQueueSend(q_print, &rtc_time_msg, portMAX_DELAY);

		xTaskNotifyWait(0, 0, &cmd_value, portMAX_DELAY);

		rx_cmd = (command_t*)cmd_value;

		value = atoi((char*)rx_cmd->payload);

		ret = validate_time_value(value, time_state);
		if(ret == pdFALSE){
			xQueueSend(q_print, &rtc_error_cmd, 0);
			return;
		}

		switch(time_state){
			case Time_hhState:
				sTime.Hours = value;
				time_state = Time_mmState;
				rtc_time_msg = rtc_minutes_msg;
				break;
			case Time_mmState:
				sTime.Minutes = value;
				time_state = Time_ssState;
				rtc_time_msg = rtc_seconds_msg;
				break;
			case Time_ssState:
				sTime.Seconds = value;
				time_state = Time_stateTerm;
				break;
			case Time_stateTerm:
				break;
			default:
				time_state = Time_stateTerm;
				xQueueSend(q_print, &rtc_error_invalid_state, 0);
				return;;
		}
	}
	while(time_state != Time_stateTerm);

	if(time_state != Time_stateTerm){
		// User didn't send the time values as expected. do not set new time.
		return;
	}

	rtc_time_format_set(&sTime);

	// Set the time
	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

}

/**
 * @brief This function handle the date configuration
 *
 * */
void date_configure(void){
	BaseType_t ret = pdTRUE;
	uint32_t cmd_value;
	uint8_t value;
	command_t* rx_cmd;

	RTC_DateTypeDef sDate = {0};

	// init state
	eDateState_t date_state = Date_ddState;
	char* rtc_date_msg =  rtc_day_msg;

	do{
		xQueueSend(q_print, &rtc_date_msg, portMAX_DELAY);

		xTaskNotifyWait(0, 0, &cmd_value, portMAX_DELAY);

		rx_cmd = (command_t*)cmd_value;

		value = atoi((char*)rx_cmd->payload);

		ret = validate_date_value(value, date_state);
		if(ret == pdFALSE){
			// Invalid Input
			xQueueSend(q_print, &rtc_error_cmd, 0);
			return;
		}

		switch(date_state){
			case Date_ddState:
				sDate.Date = value;
				date_state = Date_mmState;
				rtc_date_msg = rtc_month_msg;
				break;
			case Date_mmState:
				sDate.Month = value;
				date_state = Date_yyState;
				rtc_date_msg = rtc_year_msg;
				break;
			case Date_yyState:
				sDate.Year = value;
				date_state = Date_wdState;
				rtc_date_msg = rtc_weekDay_msg;
				break;
			case Date_wdState:
				sDate.WeekDay = (value + 6) % 7 ? ((value + 6) % 7) : 7;
				date_state = Date_stateTerm;
				break;
			case Date_stateTerm:
				break;
			default:
				date_state = Date_stateTerm;
				xQueueSend(q_print, &rtc_error_invalid_state, 0);
				return;
		}
	}
	while(date_state != Date_stateTerm);

	// Set the date
	HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

/**
 * @brief This function executes the rtc function
 *
 * @param	option excepted by the user
 *
 * @return	Non zero value when exit back to Main Menu
 * 			Zero value in all other options
 *
 * */
uint32_t rtc_execute(int option){

	while(1){
		switch(option){
			case 0: // Configure the time
				app_curr_state = sRtcTimeConfig;
				time_configure();
				option = 4;		// Print the time&date after update
				break;

			case 1: // Configure the date
				app_curr_state = sRtcDateConfig;
				date_configure();
				option = 4;		// Print the time&date after update
				break;

			case 2:	// Enable reporting
				app_curr_state = sRtcReport;
				rtc_report_time_enable();
				return 0;

			case 3:	// Exit
				app_curr_state = sMainMenu;
				xTaskNotify(menu_task_handle, 0,eNoAction);
				return 1;
			case 4: // Print time and date
				//dummy read
				RTC_TimeTypeDef dummy = {0};
				HAL_RTC_GetTime(&hrtc, &dummy, RTC_FORMAT_BIN);
				RTC_DateTypeDef _dummy = {0};
				HAL_RTC_GetDate(&hrtc, &_dummy, RTC_FORMAT_BIN);

				// Add to queue
				rtc_q_print_time_n_date();

				// Debug
				rtc_q_print_time();

				return 0;
			default:
				// Invalid Input
				xQueueSend(q_print, &rtc_error_cmd, 0);
				return 0;
		}
	}

	return 0;
}

/**
 * @brief This function handle the timer interrupt callback
 *
 * @param	xTimer Timer handle
 *
 * @note uncomment xQueueSend(q_print, &msg, portMAX_DELAY); to transmit to uart
 * */
void rtc_timer_callback(TimerHandle_t xTimer){

	char* msg = rtc_time_buff_cb;
	char* form;

	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	// Get Time format
	form = sTime.TimeFormat == RTC_HOURFORMAT12_AM ? "AM" : "PM";

	if(hrtc.Init.HourFormat == RTC_HOURFORMAT_12){
		sprintf(rtc_time_buff_cb, "Current Time&Date %02d:%02d:%02d [%s] %02d-%02d-20%02d\n", sTime.Hours, sTime.Minutes,
						sTime.Seconds, form, sDate.Date, sDate.Month, sDate.Year);
	}
	else{
		sprintf(rtc_time_buff_cb, "Current Time&Date %02d:%02d:%02d , %02d-%02d-20%02d\n", sTime.Hours, sTime.Minutes,
						sTime.Seconds, sDate.Date, sDate.Month, sDate.Year);
	}

	// Report to uart - uncomment this line!
	//xQueueSend(q_print, &msg, portMAX_DELAY);

	// Report to console
	printf("%s", rtc_time_buff_cb);
}
