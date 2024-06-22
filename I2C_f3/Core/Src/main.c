#include "main.h"

#define SI7021_ADDRESS 0x40-1
#define SI7021_MEASURE_TEMP_NO_HOLD 0xE3
#define SI7021_MEASURE_HUM_NO_HOLD 0xE5
#define led_GPIO_Port GPIOA
#define led_Pin GPIO_PIN_5

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;

char uartBuf[50];

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
int32_t readTemperature(void);
int32_t readHumidity(void);
void sendUARTData(int32_t temp, int32_t hum);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();

  while (1)
  {
      int32_t temperature = readTemperature();
      int32_t humidity = readHumidity();
      sendUARTData(temperature, humidity);
      HAL_Delay(1000);
      HAL_GPIO_TogglePin(led_GPIO_Port, led_Pin);
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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

static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

void sendUARTData(int32_t temp, int32_t hum)
{
    sprintf(uartBuf, "Temp: %ld.%02ld C, Humidity: %ld.%02ld%%\r\n",
            temp / 100, temp % 100, hum / 100, hum % 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)uartBuf, strlen(uartBuf), 10);
}

int32_t readTemperature(void)
{
    uint8_t tempCmd[1] = {SI7021_MEASURE_TEMP_NO_HOLD};
    uint8_t i2cBuf[2];
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Transmit(&hi2c1, SI7021_ADDRESS << 1, tempCmd, 1, 10);
    if (ret != HAL_OK)
    {
        sprintf(uartBuf, "Temp Transmit Error\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uartBuf, strlen(uartBuf), 10);
        return -1;
    }

    HAL_Delay(50);

    ret = HAL_I2C_Master_Receive(&hi2c1, SI7021_ADDRESS << 1, i2cBuf, 2, 10);
    if (ret != HAL_OK)
    {
        sprintf(uartBuf, "Temp Receive Error\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uartBuf, strlen(uartBuf), 10);
        return -1;
    }

    uint16_t rawTemperature = (i2cBuf[0] << 8) | i2cBuf[1];
    int32_t temperature = ((17572 * rawTemperature) / 65536) - 4685;

    return temperature;
}

int32_t readHumidity(void)
{
    uint8_t humCmd[1] = {SI7021_MEASURE_HUM_NO_HOLD};
    uint8_t i2cBuf[2];
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Transmit(&hi2c1, SI7021_ADDRESS << 1, humCmd, 1, 10);
    if (ret != HAL_OK)
    {
        sprintf(uartBuf, "Hum Transmit Error\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uartBuf, strlen(uartBuf), 10);
        return -1;
    }

    HAL_Delay(50);

    ret = HAL_I2C_Master_Receive(&hi2c1, SI7021_ADDRESS << 1, i2cBuf, 2, 10);
    if (ret != HAL_OK)
    {
        sprintf(uartBuf, "Hum Receive Error\r\n");
        HAL_UART_Transmit(&huart1, (uint8_t*)uartBuf, strlen(uartBuf), 10);
        return -1;
    }

    uint16_t rawHumidity = (i2cBuf[0] << 8) | i2cBuf[1];
    int32_t humidity = ((12500 * rawHumidity) / 65536) - 600;

    return humidity;
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(led_GPIO_Port, led_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = led_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(led_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
