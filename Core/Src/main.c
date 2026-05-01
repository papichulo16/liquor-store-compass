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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "project.h"
#include "Fusion.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define RX_BUFFER_SIZE 1024
#define SAMPLE_RATE 100  // Hz, must match your timer

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c3;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim16;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart1_rx;

/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 8,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE BEGIN PV */

osThreadId_t lookup_task_handle;
const osThreadAttr_t lookup_task_attributes = {
  .name = "lookup_task",
  .stack_size = 128 * 8,
  .priority = (osPriority_t) osPriorityHigh,
};

uint8_t g_rxbuf[RX_BUFFER_SIZE];

location_t* g_target = NULL;

// temporary hardcoded @ florida tech
float g_lat = 28.065804;
float g_lon = -80.620512;

//float g_lat = 0.0f;
//float g_lon = 0.0f;

MahonyState g_mahony;

FusionAhrs ahrs;
FusionBias offset;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C3_Init(void);
static void MX_TIM16_Init(void);
void StartDefaultTask(void *argument);

/* USER CODE BEGIN PFP */

void lookup_task(void* args);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int __io_putchar(int ch)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, 100);
    return ch;
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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_I2C3_Init();
  MX_TIM16_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim16);

  //ST7796S_Init(&hspi1);
  //ST7796S_FillScreen(&hspi1, 0xf800);

  printf("[main.c] LCD init\r\n");
  LCD_Init();

  LCD_Set_Cursor(0,0);
  LCD_Print("Welcome to");
  LCD_Set_Cursor(1,0);
  LCD_Print("Liquor Compass");

  HAL_Delay(10000);

  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_rxbuf, RX_BUFFER_SIZE);

  bool ver =  gy271m_verify(&hi2c1);
  printf("[main.c] GY271M VERIFY %s\r\n", ver ? "GOOD" : "!!! BAD !!!");

  if (!ver)
    while(1);

  bool init =  gy271m_init(&hi2c1);
  printf("[main.c] GY271M INIT %s\r\n", init ? "GOOD" : "!!! BAD !!!");

  if (!init)
    while(1);

  init = MPU6050_init(&hi2c3);
  printf("[main.c] MPU6050 INIT %s\r\n", init ? "GOOD" : "!!! BAD !!!");

  if (!init)
    while(1);

  uint32_t ret;
  uint16_t test = 0;

  ret = w25qxx_init(&hspi1);
  
  printf("[main.c] W25QXX init %s --- exit code %x\r\n", ret == W25QXX_OK ? "OK" : "!!! FAIL !!!", ret );

  Mahony_Init(&g_mahony);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

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
  //gps_q = osMessageQueueNew(1, RX_BUFFER_SIZE, NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  lookup_task_handle = osThreadNew(lookup_task, NULL, &lookup_task_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_10;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00B07CB4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.Timing = 0x00B07CB4;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 319;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 999;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_3 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // Push-pull output
  GPIO_InitStruct.Pull = GPIO_NOPULL;           // No pull-up/down
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_0 | GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   // Push-pull output
  GPIO_InitStruct.Pull = GPIO_NOPULL;           // No pull-up/down
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/*
  
  for(;;)
  {

    w25qxx_read_device_id(&hspi1, &test);
    printf("[main.c] DEVICE ID %x\r\n", test);

    osDelay(1500);
  }
*/

void compass_init(void)
{
    FusionBiasInitialise(&offset);
    FusionAhrsInitialise(&ahrs);

    const FusionAhrsSettings settings = {
        .convention          = FusionConventionNwu,  // North-West-Up
        .gain                = 0.5f,                 // Like Kp, lower = smoother
        .gyroscopeRange      = 2000.0f,              // Match your gyro's configured DPS range
        .accelerationRejection= 10.0f,
        .magneticRejection   = 10.0f,
        .recoveryTriggerPeriod= 5 * SAMPLE_RATE,
    };
    FusionAhrsSetSettings(&ahrs, &settings);
}

float compass_get_heading(void)
{
    // Let Fusion do the tilt-compensated compass heading directly
    // This is more reliable than extracting yaw from Euler angles
    const FusionQuaternion quat = FusionAhrsGetQuaternion(&ahrs);
    const FusionVector earth    = FusionAhrsGetEarthAcceleration(&ahrs);

    FusionEuler euler = FusionQuaternionToEuler(quat);

    float heading = euler.angle.yaw;
    if (heading < 0.0f)    heading += 360.0f;
    if (heading >= 360.0f) heading -= 360.0f;

    return heading;
}

void parse_gprmc(char *sentence) {
    if (strncmp((char *) g_rxbuf, "$GPRMC", 6)) return;
    //printf("[main.c] parsing GPRMC\r\n");
    //printf("%s\r\n", sentence);

    char *token;
    char buf[100];
    strncpy(buf, sentence, sizeof(buf));

    int field = 0;
    float lat = 0, lon = 0;
    char latDir, lonDir, status;

    token = strtok(buf, ",");
    while (token != NULL) {
        switch (field) {
            case 2: status  = token[0]; break;  // A = valid, V = invalid
            case 3: lat     = atof(token); break;
            case 4: latDir  = token[0]; break;
            case 5: lon     = atof(token); break;
            case 6: lonDir  = token[0]; break;
        }
        token = strtok(NULL, ",");
        field++;
    }

    if (status != 'A') return;  // No GPS fix yet

    int latDeg = (int)(lat / 100);
    float latMin = lat - latDeg * 100;
    float latitude  = latDeg + latMin / 60.0f;
    if (latDir == 'S') latitude  = -latitude;

    int lonDeg = (int)(lon / 100);
    float lonMin = lon - lonDeg * 100;
    float longitude = lonDeg + lonMin / 60.0f;
    if (lonDir == 'W') longitude = -longitude;

    //taskENTER_CRITICAL();

    g_lat = latitude;
    g_lon = longitude;

    //taskEXIT_CRITICAL();

}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {

    if (huart->Instance == USART1) {

        if (!strncmp((char *) g_rxbuf, "$GPRMC", 6)) {

          parse_gprmc((char *) g_rxbuf);
        }

        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, g_rxbuf, RX_BUFFER_SIZE);
    }

}

void update_target() {

  location_t* l;

  if (g_lat == 0.0f && g_lon == 0.0f)
    return;

  l = find_nearest(g_lat, g_lon);

  if (!g_target || l->lat != g_target->lat || l->lon != g_target->lon) {

    g_target = l;

    printf("[main.c] new target found\r\n");
  }
}

void lookup_task(void* args) {


  while(1) {

      update_target();
  
      osDelay(1500);
  }
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */

void handle_orientation_log() {

  LCD_Set_Cursor(0,0);
  LCD_Print("Dis: ");
  
  double d = distance_miles(g_lat, g_lon, g_target->lat, g_target->lon);
  LCD_Print(format_distance(d));

  LCD_Set_Cursor(1,0);
  LCD_Print("Bear:");

  char* b = get_bearing(g_lat, g_lon, g_target->lat, g_target->lon);
  LCD_Print(b);

}

void handle_gps_log() {

  if (g_lat == 0.0f && g_lon == 0.0f) {

    printf("[main.c] WAITING FOR SATELLITE FIX...\r\n");
    
    LCD_Set_Cursor(0,0);
    LCD_Print("Waiting for");
    LCD_Set_Cursor(1,0);
    LCD_Print("satellite fix...");
    return;
  }

  printf("[main.c] GPS LAT %d --- LON %d\r\n", (int) (g_lat * 1000000), (int) (g_lon * 1000000));
  printf("[main.c] GPS TARGET LAT %d --- LON %d\r\n", (int) (g_target->lat * 1000000), (int) (g_target->lon * 1000000));

/*
  location_t* test = find_nearest(g_lat, g_lon);

  printf("NEAREST LIQUOR STORE: %d, %d\r\n", (int) (test->lat * 10000000), (int) (test->lon * 10000000));
  printf("DISTANCE %d\r\n", (int) (distance_miles(g_lat, g_lon, test->lat, test->lon) * 100));
*/
  handle_orientation_log();

}


/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;) {

    LCD_Clear();
    handle_gps_log();
    
    osDelay(500);
  }

  /* USER CODE END 5 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

    if (htim->Instance != TIM16) return;

    float gx, gy, gz;  // From your gyro driver,  in radians/sec
    float ax, ay, az;  // From your accel driver,  any unit
    int16_t raw_mx, raw_my, raw_mz;

    MPU6050_Read_Gyro(&hi2c3, &gx, &gy, &gz);
    MPU6050_Read_Accel(&hi2c3, &ax, &ay, &az);
    gy271m_read(&hi2c1, &raw_mx, &raw_my, &raw_mz);

    // Hard iron correction
    float mx = (float)raw_mx - (MAG_X_MIN + MAG_X_MAX) / 2.0f;
    float my = (float)raw_my - (MAG_Y_MIN + MAG_Y_MAX) / 2.0f;
    float mz = (float)raw_mz;

    float gx_dps = (float)gx / GYRO_SENSITIVITY;
    float gy_dps = (float)gy / GYRO_SENSITIVITY;
    float gz_dps = (float)gz / GYRO_SENSITIVITY;

    float ax_g = (float)ax / ACCEL_SENSITIVITY;
    float ay_g = (float)ay / ACCEL_SENSITIVITY;
    float az_g = (float)az / ACCEL_SENSITIVITY;

    // Fill in your gyro values in degrees/sec (NOT radians for this library)
    FusionVector gyroscope     = { .axis = { gx_dps, gy_dps, gz_dps }};

    // Fill in your accelerometer values in g
    FusionVector accelerometer = { .axis = { ax_g, ay_g, az_g }};

    // Magnetometer (calibrated)
    FusionVector magnetometer  = { .axis = { mx, my, mz }};

    // Gyro offset correction (handles bias drift)
    gyroscope = FusionBiasUpdate(&offset, gyroscope);

    // Update filter
    FusionAhrsUpdate(&ahrs, gyroscope, accelerometer, magnetometer, 1.0f / SAMPLE_RATE);

  /* USER CODE END Callback 1 */
}

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
