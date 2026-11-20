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
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/*
 * has_ 欄位表示資料是否有效：
 * 0：尚未取得資料，顯示 N/A，
 * 1：已取得資料，顯示實際數值
 */
typedef struct
{
    uint8_t id[3];
    uint8_t status;
    uint32_t address;
    uint8_t data[4];

    uint8_t has_id;
    uint8_t has_status;
    uint8_t has_address;
    uint8_t has_data;

} FlashInfo;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FLASH_CS_GPIO_Port GPIOA
#define FLASH_CS_Pin GPIO_PIN_4
#define TFT_CS_GPIO_Port GPIOE
#define TFT_CS_Pin GPIO_PIN_7
#define TFT_RST_GPIO_Port GPIOB
#define TFT_RST_Pin GPIO_PIN_0
#define TFT_DC_GPIO_Port GPIOB
#define TFT_DC_Pin GPIO_PIN_1

#define TFT_BLACK  0x0000
#define TFT_WHITE  0xFFFF
#define TFT_CYAN   0x07FF
#define TFT_GREEN  0x07E0
#define TFT_YELLOW 0xFFE0
#define TFT_GRAY   0x8410

#define KEY_ROW_COUNT 4
#define KEY_COL_COUNT 3
#define KEY_DEBOUNCE_MS 30
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define SEL_TFT()   HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET)
#define UNSEL_TFT()   HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET)


#define SEL_FLASH()   HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET)
#define UNSEL_FLASH()   HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */
// 5x7 字型，索引依照下面 chars 的順序
static const char chars[] =
    " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:/.-#";

static const uint8_t font[][5] = {
    {0,0,0,0,0},          // space
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x41,0x41,0x7F,0x41,0x41}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x7F,0x20,0x18,0x20,0x7F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x03,0x04,0x78,0x04,0x03}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z

    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9

    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x14,0x7F,0x14,0x7F,0x14}  // #
};

// P1 (PA9) is unused; the measured row-column contacts suffice.
// 0-based keypad pins: P5/P4/P3/P2 = rows; P8/P7/P6 = columns.
static GPIO_TypeDef *const keypadRowPorts[KEY_ROW_COUNT] = {GPIOD, GPIOD, GPIOC, GPIOC};
static const uint16_t keypadRowPins[KEY_ROW_COUNT] = {GPIO_PIN_13, GPIO_PIN_15, GPIO_PIN_7, GPIO_PIN_9};
static GPIO_TypeDef *const keypadColPorts[KEY_COL_COUNT] = {GPIOB, GPIOD, GPIOD};
static const uint16_t keypadColPins[KEY_COL_COUNT] = {GPIO_PIN_15, GPIO_PIN_9, GPIO_PIN_11};
static const char keypadMap[KEY_ROW_COUNT][KEY_COL_COUNT] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'R', '0', '#'} // R = REDIAL, since the font does not include '*'
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */
static void TFT_Command(uint8_t cmd);
static void TFT_Data(uint8_t data);
static void TFT_Init(void);
static void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
static void TFT_FillColor(uint16_t color);
static void TFT_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bg);
static void TFT_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg);
static void TFT_ShowFlashInfo(const FlashInfo *info);
static void TFT_ShowKeypad(char key);
static void Keypad_Init(void);
static char Keypad_Read(void);
static char Keypad_ReadDebounced(void);
static HAL_StatusTypeDef readFlashStatus(FlashInfo *flash);
static HAL_StatusTypeDef readMemory(FlashInfo *flash, uint32_t address);
static HAL_StatusTypeDef waitFlashReady(FlashInfo *flash, uint32_t timeout);
static HAL_StatusTypeDef writeMemory(FlashInfo *flash, uint32_t address, uint8_t data);
static void Flash_RunDemo(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void Keypad_SetRowInput(uint8_t row)
{
    GPIO_InitTypeDef cfg = {0};
    cfg.Pin = keypadRowPins[row];
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(keypadRowPorts[row], &cfg);
}

static void Keypad_Init(void)
{
    for (uint8_t r = 0; r < KEY_ROW_COUNT; r++)
        Keypad_SetRowInput(r);

    GPIO_InitTypeDef cfg = {0};
    cfg.Mode = GPIO_MODE_INPUT;
    cfg.Pull = GPIO_PULLUP;
    for (uint8_t c = 0; c < KEY_COL_COUNT; c++) {
        cfg.Pin = keypadColPins[c];
        HAL_GPIO_Init(keypadColPorts[c], &cfg);
    }
}

static char Keypad_Read(void)
{
    for (uint8_t r = 0; r < KEY_ROW_COUNT; r++) {
        Keypad_Init();
        HAL_GPIO_WritePin(keypadRowPorts[r], keypadRowPins[r], GPIO_PIN_RESET);
        GPIO_InitTypeDef cfg = {0};
        cfg.Pin = keypadRowPins[r];
        cfg.Mode = GPIO_MODE_OUTPUT_OD;
        cfg.Pull = GPIO_NOPULL;
        cfg.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(keypadRowPorts[r], &cfg);
        HAL_Delay(1);

        for (uint8_t c = 0; c < KEY_COL_COUNT; c++) {
            if (HAL_GPIO_ReadPin(keypadColPorts[c], keypadColPins[c]) == GPIO_PIN_RESET) {
                Keypad_Init();
                return keypadMap[r][c];
            }
        }
    }
    Keypad_Init();
    return '\0';
}

// Non-blocking debouncer: returns the stable current key, or NUL if released.
static char Keypad_ReadDebounced(void)
{
    static char candidate = '\0';
    static char stable = '\0';
    static uint32_t changedAt = 0;
    char raw = Keypad_Read();
    uint32_t now = HAL_GetTick();

    if (raw != candidate) {
        candidate = raw;
        changedAt = now;
    }
    if ((uint32_t)(now - changedAt) >= KEY_DEBOUNCE_MS)
        stable = candidate;
    return stable;
}

static void TFT_Command(uint8_t cmd) {
	SEL_TFT();
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi1, &cmd, 1, 1000);
	UNSEL_TFT();
}

static void TFT_Data(uint8_t data) {
	SEL_TFT();
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET);
	HAL_SPI_Transmit(&hspi1, &data, 1, 1000);
	UNSEL_TFT();
}

static void TFT_Init(void) {
	HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);

	// software reset
	TFT_Command(0x01);
	HAL_Delay(150);

	// sleep out
	TFT_Command(0x11);
	HAL_Delay(120);

	// 16-bit color(color representing format)
	TFT_Command(0x3A);
	TFT_Data(0x05);

	//Memory access control
	TFT_Command(0x36);
	TFT_Data(0xC0); // rotate 180

	// Normal display mode 螢幕進入正常顯示模式，而不是 Partial Display Mode（局部顯示模式）
	TFT_Command(0x13);
	HAL_Delay(10);

	// Display ON
	TFT_Command(0x29);
	HAL_Delay(100);
}


/* read status register*/
static HAL_StatusTypeDef readFlashStatus(FlashInfo* flash){
	if (flash == NULL)
	    return HAL_ERROR;

	flash->has_status = 0;

	SEL_FLASH();
	uint8_t command = 0x05;
	HAL_StatusTypeDef commute_status = HAL_SPI_Transmit(&hspi1, &command, 1, 1000);
	if (commute_status == HAL_OK) {
		commute_status = HAL_SPI_Receive(&hspi1, &flash->status, 1, 1000);
	}
	UNSEL_FLASH();

	if (commute_status == HAL_OK) {
		flash->has_status = 1;
	}
	return commute_status;
}


static HAL_StatusTypeDef readMemory(FlashInfo* flash, uint32_t address){
	if (flash == NULL || address > 0xFFFFFC)
	    return HAL_ERROR;
	flash->has_data = 0;
	flash->has_address = 0;


	uint8_t command[4] = {
			0x03,
			(uint8_t)(address >> 16),
			(uint8_t)(address >> 8),
			(uint8_t)address
	};
	SEL_FLASH();
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, command, 4, 1000);
	if (status == HAL_OK) {
		status = HAL_SPI_Receive(&hspi1, flash->data, 4, 1000);
	}
	if (status == HAL_OK) {
		flash->has_data = 1;
		flash->address = address;
		flash->has_address = 1;
	}
	UNSEL_FLASH();
	return status;
}


static HAL_StatusTypeDef waitFlashReady(FlashInfo *flash, uint32_t timeout)
{
    uint32_t start = HAL_GetTick();

    while (1)
    {
        HAL_StatusTypeDef result = readFlashStatus(flash);

        if (result != HAL_OK)
            return result;

        if (!(flash->status & 0x01))
            return HAL_OK;

        if (HAL_GetTick() - start >= timeout)
            return HAL_TIMEOUT;
    }
}

static HAL_StatusTypeDef writeMemory(FlashInfo* flash, uint32_t address, uint8_t data){
	if (flash == NULL || address > 0xFFFFFC)
	    return HAL_ERROR;


	// until not busy
	HAL_StatusTypeDef readStat;
	readStat = waitFlashReady(flash, 1000);
	if (readStat != HAL_OK) return readStat;

	// write enable
	SEL_FLASH();
	uint8_t command = 0x06;
	readStat = HAL_SPI_Transmit(&hspi1, &command, 1, 1000);
	UNSEL_FLASH();
	if (readStat != HAL_OK) return readStat;

	// until enable
	uint32_t start = HAL_GetTick();
	while(1){
		readStat = readFlashStatus(flash);
		if (readStat != HAL_OK)
			return readStat;

		if (flash->status & 0x02) break;

		if (HAL_GetTick() - start >= 1000)
			return HAL_TIMEOUT;
	}

	// write
	SEL_FLASH();
	uint8_t writeCmd[5] = {
			0x02,
			(uint8_t)(address >> 16),
			(uint8_t)(address >> 8),
			(uint8_t)address,
			data
	};
	readStat = HAL_SPI_Transmit(&hspi1, writeCmd, 5, 1000);
	UNSEL_FLASH();
	if (readStat != HAL_OK) return readStat;

	// until not busy
	return waitFlashReady(flash, 1000);
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
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  TFT_Init();

  Keypad_Init();
  TFT_FillColor(TFT_BLACK);
  TFT_ShowKeypad('\0');
  // Flash_RunDemo(); // Optional: enable flash read/write demo instead of keypad UI.

  FlashInfo flash = {0};

  // read ID
  SEL_FLASH();
  uint8_t command = 0x9F;
  HAL_StatusTypeDef commute_status = HAL_SPI_Transmit(&hspi1, &command, 1, 1000);
  if (commute_status == HAL_OK)
	  commute_status = HAL_SPI_Receive(&hspi1, flash.id, 3, 1000);
  UNSEL_FLASH();
  if (commute_status == HAL_OK) flash.has_id = 1;

  readFlashStatus(&flash);


  // read memory
  readMemory(&flash, 0x123410);
  TFT_ShowFlashInfo(&flash);
  HAL_Delay(5000);

  // write and read
  writeMemory(&flash, 0x123410, 0xAB);
  readMemory(&flash, 0x123410);
  TFT_ShowFlashInfo(&flash);


  HAL_Delay(5000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    char key = Keypad_ReadDebounced();
    static char lastDisplayed = '\0';
    if (key != lastDisplayed)
    {
      TFT_ShowKeypad(key);
      lastDisplayed = key;
    }
    HAL_Delay(5);
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
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TFT_RST_Pin|TFT_DC_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : FLASH_CS_Pin */
  GPIO_InitStruct.Pin = FLASH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(FLASH_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TFT_RST_Pin TFT_DC_Pin */
  GPIO_InitStruct.Pin = TFT_RST_Pin|TFT_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : TFT_CS_Pin */
  GPIO_InitStruct.Pin = TFT_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TFT_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : keypad_Pin */
  GPIO_InitStruct.Pin = keypad_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(keypad_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : keypadD9_Pin keypadD11_Pin keypadD13_Pin keypadD15_Pin */
  GPIO_InitStruct.Pin = keypadD9_Pin|keypadD11_Pin|keypadD13_Pin|keypadD15_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : keypadC7_Pin keypadC9_Pin */
  GPIO_InitStruct.Pin = keypadC7_Pin|keypadC9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : keypadA9_Pin */
  GPIO_InitStruct.Pin = keypadA9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(keypadA9_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void TFT_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    TFT_Command(0x2A);
    TFT_Data(x0 >> 8);
    TFT_Data(x0 & 0xFF);
    TFT_Data(x1 >> 8);
    TFT_Data(x1 & 0xFF);

    TFT_Command(0x2B);
    TFT_Data(y0 >> 8);
    TFT_Data(y0 & 0xFF);
    TFT_Data(y1 >> 8);
    TFT_Data(y1 & 0xFF);
}

static void TFT_FillColor(uint16_t color)
{
    TFT_SetAddressWindow(0, 0, 127, 159);
    TFT_Command(0x2C);

    uint8_t pixel[2] = {
        (uint8_t)(color >> 8),
        (uint8_t)(color & 0xFF)
    };

    SEL_TFT();
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_SET);

    for (int i = 0; i < 128 * 160; i++)
    {
        HAL_SPI_Transmit(&hspi1, pixel, 2, 1000);
    }

    UNSEL_TFT();
}


static void TFT_DrawChar(uint16_t x, uint16_t y, char c,
                  uint16_t color, uint16_t bg)
{
    if (x + 5 >= 128 || y + 7 >= 160)
        return;

    // Find character in font table
    const uint8_t *glyph = font[0];

    for (unsigned int i = 0; i < sizeof(chars) - 1; i++)
    {
        if (chars[i] == c)
        {
            glyph = font[i];
            break;
        }
    }

    // 6x8 pixels, RGB565 = 2 bytes per pixel
    uint8_t buffer[6 * 8 * 2];

    uint16_t index = 0;

    for (uint8_t row = 0; row < 8; row++)
    {
        for (uint8_t col = 0; col < 6; col++)
        {
            uint16_t pixelColor = bg;

            if (col < 5 && row < 7)
            {
                if (glyph[col] & (1 << row))
                {
                    pixelColor = color;
                }
            }

            buffer[index++] = (uint8_t)(pixelColor >> 8);
            buffer[index++] = (uint8_t)(pixelColor & 0xFF);
        }
    }

    // Set address window once for entire character
    TFT_SetAddressWindow(x, y, x + 5, y + 7);

    // Start memory write
    TFT_Command(0x2C);

    // Send all 48 pixels in one SPI transaction
    SEL_TFT();

    HAL_GPIO_WritePin(
        TFT_DC_GPIO_Port,
        TFT_DC_Pin,
        GPIO_PIN_SET
    );

    HAL_SPI_Transmit(&hspi1, buffer, sizeof(buffer), 1000);

    UNSEL_TFT();
}

static void TFT_DrawString(uint16_t x, uint16_t y, const char *str,
                    uint16_t color, uint16_t bg)
{
    while (*str && x <= 122)
    {
        TFT_DrawChar(x, y, *str, color, bg);
        x += 6;
        str++;
    }
}

static void TFT_ShowFlashInfo(const FlashInfo *info)
{
    if (info == NULL)
        return;

    char buf[32];

    // Clear screen
    TFT_FillColor(TFT_BLACK);

    // Title
    TFT_DrawString(4, 4, "SPI FLASH MONITOR",
                   TFT_CYAN, TFT_BLACK);

    TFT_DrawString(4, 18, "W25Q128JV / SPI1",
                   TFT_GRAY, TFT_BLACK);

    // JEDEC ID
    if (info->has_id)
    {
        snprintf(buf, sizeof(buf), "ID:%02X %02X %02X",
                 info->id[0],
                 info->id[1],
                 info->id[2]);
    }
    else
    {
        snprintf(buf, sizeof(buf), "ID:N/A");
    }

    TFT_DrawString(4, 40, buf, TFT_WHITE, TFT_BLACK);

    // Status Register
    if (info->has_status)
    {
        snprintf(buf, sizeof(buf), "STATUS:%02X",
                 info->status);
    }
    else
    {
        snprintf(buf, sizeof(buf), "STATUS:N/A");
    }

    TFT_DrawString(4, 56, buf, TFT_GREEN, TFT_BLACK);

    // BUSY / WEL
    if (info->has_status)
    {
        snprintf(buf, sizeof(buf), "BUSY:%d WEL:%d",
                 info->status & 0x01,
                 (info->status >> 1) & 0x01);
    }
    else
    {
        snprintf(buf, sizeof(buf), "BUSY:N/A WEL:N/A");
    }

    TFT_DrawString(4, 72, buf, TFT_GREEN, TFT_BLACK);

    // Address
    if (info->has_address)
    {
        snprintf(buf, sizeof(buf), "ADDR:%06lX",
                 (unsigned long)(info->address & 0xFFFFFF));
    }
    else
    {
        snprintf(buf, sizeof(buf), "ADDR:N/A");
    }

    TFT_DrawString(4, 96, buf, TFT_YELLOW, TFT_BLACK);

    // Data
    TFT_DrawString(4, 112, "DATA:",
                   TFT_GRAY, TFT_BLACK);

    if (info->has_data)
    {
        snprintf(buf, sizeof(buf), "%02X %02X %02X %02X",
                 info->data[0],
                 info->data[1],
                 info->data[2],
                 info->data[3]);
    }
    else
    {
        snprintf(buf, sizeof(buf), "N/A");
    }

    TFT_DrawString(4, 124, buf, TFT_WHITE, TFT_BLACK);
}

static void TFT_ShowKeypad(char key)
{
    char line[20] = "KEY: NONE";
    if (key != '\0') {
        line[0] = 'K'; line[1] = 'E'; line[2] = 'Y'; line[3] = ':';
        line[4] = ' '; line[5] = key; line[6] = ' '; line[7] = ' ';
        line[8] = ' '; line[9] = '\0';
    }
    // Draw fixed-width text with background to erase the previous key.
    TFT_DrawString(4, 4, "KEYPAD TEST", TFT_CYAN, TFT_BLACK);
    TFT_DrawString(4, 40, "KEY: NONE   ", TFT_WHITE, TFT_BLACK);
    TFT_DrawString(4, 40, line, TFT_GREEN, TFT_BLACK);
}

static void Flash_RunDemo(void)
{
  FlashInfo flash = {0};

  // read ID
  SEL_FLASH();
  uint8_t command = 0x9F;
  HAL_StatusTypeDef commute_status = HAL_SPI_Transmit(&hspi1, &command, 1, 1000);
  if (commute_status == HAL_OK)
	  commute_status = HAL_SPI_Receive(&hspi1, flash.id, 3, 1000);
  UNSEL_FLASH();
  if (commute_status == HAL_OK) flash.has_id = 1;

  readFlashStatus(&flash);


  // read memory
  readMemory(&flash, 0x123400);
  TFT_ShowFlashInfo(&flash);
  HAL_Delay(15000);

  // write and read
  writeMemory(&flash, 0x123400, 0xAB);
  readMemory(&flash, 0x123400);
  TFT_ShowFlashInfo(&flash);
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
