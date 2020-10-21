/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */
#define OK 0
#define EQ 0

#define _red_  "\033[1;31m"
#define _yel_  "\033[1;33m"
#define _blu_  "\033[1;34m"
#define _end_  "\033[0m"
#define DEBUG_HEADER    _end_ "[" _yel_ "%22.22s" _end_ ":" _red_ "%04d" _end_ "][" _blu_ "%-24.24s" _end_ "] "

#define dbug0(T,fmt)  if(T) printf( DEBUG_HEADER fmt, __FILE__, __LINE__, __FUNCTION__ )
#define dbug(T,fmt, args...)  if(T) printf( DEBUG_HEADER fmt, __FILE__, __LINE__, __FUNCTION__, ##args)

#define BUFSIZ 	128

char rcvBuf[BUFSIZ] = {0,};
char rcvChar;
int rcvStart, rcvEnd, rcvCnt;

char sndBuf[BUFSIZ] = {0,};
int sndStart, sndEnd, toSndCnt;

typedef struct {
	uint8_t cmdType;
	uint8_t command;

} CMD;

uint8_t bufFull;

TaskHandle_t hTaskUsart3Rx;
TaskHandle_t hTaskCmdWorker;

const char *taskRxName = "UART3-RX Handler";
const char *taskWorkerName = "Command-Worker";

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, uint8_t *p, int len) {
	HAL_UART_Transmit(&huart3, p, len, 10);
	return len;
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == huart3.Instance)  {
	  if (rcvStart <= rcvEnd)
		  rcvCnt = rcvEnd - rcvStart + 1;
	  else
		  rcvCnt = BUFSIZ - (rcvStart - rcvEnd) + 1;

	  if (rcvCnt < BUFSIZ) {
		  if (rcvEnd < BUFSIZ - 1)
			  rcvEnd++;
		  else
			  rcvEnd = 0;
		  bufFull = 0;
	  }
	  else {
		  bufFull = 1;
	  }

	  HAL_UART_Receive_IT(&huart3, &rcvChar, 1);
	  rcvBuf[rcvEnd] = rcvChar;
	  rcvCnt++;
  }
}

uint8_t GetRcvChar(uint8 *ecode)
{
	uint8_t rCh;

	*ecode = 0;
	if (rcvCnt == 0) {
		*ecode = -1;
		return 0;
	}

	if (rcvCnt == 1) {
		rCh = rcvBuf[rcvStart];
		rcvStart = rcvEnd = -1;
	}
	else {
		if (rcvStart == BUFSIZ-1) {
			rCh = rcvBuf[rcvStart];
			rcvStart = 0;
		}
		else {
			rCh = rcvBuf[rcvStart++];
		}
	}
	rcvCnt--;

	return rCh;
}

uint16_t GetRcvLine(uint8_t *pBuf, uint16_t *len)
{
	uint8_t ecode, start, end;
	uint8_t ch;
	uint16_t i;

	i = start = end = 0;

	ch = GetRcvChar(&ecode);
	while (ecode == 0 && end == 0) {
		if (i > *len-1) {
			return -2; // 버퍼 부족 에러
		}
		switch (ch) {
			case '/':
				ch0 = GetRcvChar(&ecode);
				if (ecode == 0 && ch0 == '/')
					pBuf[i++] = '/';
				else if (ecode == 0 && ch0 == '\n')
					pBuf[i++] = '\n';
				else {
					pBuf[i++] = ch;
					start = 1;
				}
				break;
			case '\n':
				pBuf[i++] = 0;
				end = 1;
				break;
			default:
				if (start)
					pBuf[i++] = ch;
		}
		ch = GetRcvChar(&ecode);
	}
	*len = i;

	return 0;
}

uint16_t parseCmdLine(uint8_t lineCmd, )
{

	return 0;
}

//void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
//{
//	if (huart->Instance == huart3.Instance)  {
//		// lock
//		if (sndStart < sndEnd) {
//			HAL_UART_Transmit_IT(&huart3, &sndBuf[sndStart++], 1);
//			toSndCnt = sndEnd - sndStart;
//		}
//		else if (sndStart == sndEnd && sndStart != -1) {
//			HAL_UART_Transmit_IT(&huart3, &sndBuf[sndStart], 1);
//			sndStart = sndEnd = -1;
//			toSndCnt = 0;
//		}
//		else { // (sndStart > sndEnd)
//			HAL_UART_Transmit_IT(&huart3, &sndBuf[sndStart++], 1);
//			if (sndStart > BUFSIZ)
//				sndStart = 0;
//			toSndCnt = BUFSIZ - (sndStart - sndEnd);
//		}
//		// unlock
//	}
//}

void Uart3Send(UART_HandleTypeDef *huart, uint8_t *data, uint16_t len)
{
	HAL_UART_Transmit(huart, data,  len, 20);
}

void vTaskUart3RxCmdHandler(void *pvParam)
{
	uint16_t err;
	uint8_t line[64];
	CMD CmdData;
	uint8_t cmdType, command;

	const char *pchTaskParam = (char*)pvParam;

	dbug(1, "Task Name: %s\n", pchTaskParam );

	bufFull = 0;
	rcvStart = rcvEnd = -1;   // 함수화
	HAL_UART_Receive_IT(&huart3, &rcvChar, 1);

	while (1)
	{
		memset(line, 0, siz=sizeof(line));
		if ((err = GetRcvLine(line, &siz)) == OK)  {
			HAL_UART_Transmit(&huart3, line, siz, 10);  // TODO: 10을 define
			err = parseCmdLine(line, &cmdData);
			if (err == OK) {
				cmdType = cmdData.cmdType;
				command = cmdData.command;
				switch (cmdType) {
				case 'C':  // 모듈 명령
					// message send to module process
					break;
				case 'M':  // 동작 제어
					// message send to manager process
					break;
				case 'P':  // 폴링
					// 응답 전송
					break;
				}
			}
			else {
				dbg(1, "E-PARSECMDLINE:%d\n", err);
			}
		}
		else {
			dbg(1, "E-GETLINE:%d\n", err);
		}

		vTaskDelay(500);
	}

}



void vTaskCmdWorker(void *pvParam)
{
	uint16_t err;


	const char *pchTaskParam = (char*)pvParam;

	dbug(1, "Task Name: %s\n", pchTaskParam );

	while (1) {

		vTaskDelay(500);
	}

}

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
  MX_I2C2_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  sndStart = sndEnd = toSndCnt = -1;
  rcvStart = rcvEnd = rcvCnt = -1;
  memset(sndBuf,  0, BUFSIZ);
  memset(rcvBuf,  0, BUFSIZ);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();
  /* Start scheduler */
  osKernelStart();


  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */

  /* USER CODE BEGIN WHILE */

  xTaskCreate(vTaskUart3RxCmdHandler, "vTaskUart3RxCmdHandler", 200, (void*)taskRxName,     1, &hTaskUsart3Rx);
  xTaskCreate(vTaskCmdWorker,         "vTaskCmdWorker",         200, (void*)taskWorkerName, 1, &hTaskCmdWorker);
  vTaskStartScheduler();

  while (1)
  {
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
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
