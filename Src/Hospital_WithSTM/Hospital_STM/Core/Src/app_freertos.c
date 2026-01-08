/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os2.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "main.h"
#include "common_data.h"
#include "ultrasonic.h"
#include "fsr.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern UART_HandleTypeDef huart1;
extern TIM_HandleTypeDef htim4;
extern ADC_HandleTypeDef hadc4;

// 센서 객체
fsr_t g_fsr;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 1024 * 4
};
/* Definitions for GUI_Task */
osThreadId_t GUI_TaskHandle;
const osThreadAttr_t GUI_Task_attributes = {
  .name = "GUI_Task",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 8192 * 4
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern portBASE_TYPE IdleTaskHook(void* p);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
extern void TouchGFX_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationIdleHook(void);

/* USER CODE BEGIN 2 */
void vApplicationIdleHook( void )
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
  
   vTaskSetApplicationTaskTag(NULL, IdleTaskHook);
}
/* USER CODE END 2 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of GUI_Task */
  GUI_TaskHandle = osThreadNew(TouchGFX_Task, NULL, &GUI_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_StartDefaultTask */
/**
* @brief Function implementing the defaultTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN defaultTask */
	// ---------------------------------------------------------
	  // 1. 센서 초기화
	  // ---------------------------------------------------------
	  // 초음파: Delay용 타이머는 NULL(HAL_Delay씀), 캡처는 TIM4, 채널2
	  // 주의: TRIG 핀 포트/핀 번호는 main.h의 정의를 따름 (TRIG_GPIO_Port, TRIG_Pin)
	  Ultrasonic_Init(NULL, &htim4, TIM_CHANNEL_2, TRIG_GPIO_Port, TRIG_Pin);

	  // FSR: ADC4, 임계값(On:2000, Off:1000), Alpha 0.2
	  FSR_Init(&g_fsr, &hadc4, 2000, 1000, 0.2f);

	  // 초기화 후 잠시 대기
	  HAL_Delay(100);

	  // ---------------------------------------------------------
	  // 2. 무한 루프 (0.5초 주기)
	  // ---------------------------------------------------------
	  char tx_buffer[128];

	  for(;;)
	  {

	      // --- [A] 센서 측정 ---
	      // 1. 초음파 트리거 발사 & 측정 시작
	      Ultrasonic_TriggerAndStart();

	      // 2. FSR 업데이트 (ADC 읽기)
	      FSR_Update(&g_fsr);

	      // --- [B] 데이터 포맷팅 ---
	      // 포맷: 초음파(cm) @ 착석여부(1/0) @ 버튼명령(Enum) \n
	      // (버튼 명령은 전송 후 0으로 초기화해야 함)

	      float dist = Ultrasonic_Distance_cm;
	      int seat_status = (FSR_GetSeat(&g_fsr) == FSR_SEAT_OCCUPIED) ? 1 : 0;

	      // g_btn_cmd_queue는 common_data.h에 선언된 전송용 큐
	      int btn_cmd = (int)g_btn_cmd_queue;

	      // snprintf로 문자열 생성
	      int len = snprintf(tx_buffer, sizeof(tx_buffer), "%.1f@%d@%d\n", dist, seat_status, btn_cmd);

	      // --- [C] UART 전송 ---
	      if(len > 0) {
	          HAL_UART_Transmit(&huart1, (uint8_t*)tx_buffer, len, 100);
	      }

	      // 전송했으니 버튼 명령 큐 초기화 (한 번만 보내기 위함)
	      if (btn_cmd != 0) {
	          g_btn_cmd_queue = BTN_CMD_NONE;
	      }

	      //usart 테스트 코드
		  //const char* test_msg = "TEST\r\n";
		  //HAL_UART_Transmit(&huart1, (uint8_t*)test_msg, 6, 100);

	      // --- [D] 0.5초 대기 (RTOS Delay) ---
	      osDelay(500);
	  }
  /* USER CODE END defaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

