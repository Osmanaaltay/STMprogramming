#include "main.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ASSERT_CS()   (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET))
#define DEASSERT_CS() (HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET))

typedef struct {
    uint8_t lsb_temp;
    uint8_t msb_temp;
} tc72_t;

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

void floatToStr(float val, char data[])
{
    if (val < 0) {
        *data = '-';
        data++;
        val *= -1;
    }
    int intVal = val * 100;
    data[5] = (intVal % 10) + '0';
    intVal /= 10;
    data[4] = (intVal % 10) + '0';
    data[3] = '.';
    intVal /= 10;
    data[2] = (intVal % 10) + '0';
    intVal /= 10;
    data[1] = (intVal % 10) + '0';
    intVal /= 10;
    data[0] = (intVal % 10) + '0';
    data[6] = '\0'; // Null-terminate the string
}

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_SPI1_Init(void);
void MX_USART1_UART_Init(void);

uint8_t readRegister(uint8_t reg);

int main(void)
{
    tc72_t tc72;
    uint8_t TxBuf[10];
    uint8_t RxBuf[10];

    char str[20];
    char str2[20] = "temperature = ";
    float temp;
    int x;

    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();

    while (1)
    {
        // Set TC72 to continuous read mode
        TxBuf[0] = 0x80;
        TxBuf[1] = 0x01|0x80;

        ASSERT_CS();
        HAL_SPI_Transmit(&hspi1, TxBuf, 4, 100);
        DEASSERT_CS();

        // Read and print control register
        uint8_t controlReg = readRegister(0x03);
        char debugStr[30];
        sprintf(debugStr, "Control Register: 0x%02X\n\r", controlReg);
        HAL_UART_Transmit(&huart1, (uint8_t*)debugStr, strlen(debugStr), 100);

        // Read LSB bits (0x01 register)
        TxBuf[0] = 0x01;
        temp = 0.0f;

        ASSERT_CS();
        HAL_SPI_TransmitReceive(&hspi1, TxBuf, RxBuf, 4, 100);
        DEASSERT_CS();

        // Print LSB register
        sprintf(debugStr, "LSB Register: 0x%02X\n\r", RxBuf[1]);
        HAL_UART_Transmit(&huart1, (uint8_t*)debugStr, strlen(debugStr), 100);

        if (RxBuf[1] & (1 << 6)) {
            temp += 0.5f;
        } else if (RxBuf[1] & (1 << 7)) {
            temp += 0.25f;
        }

        // Read MSB bits (0x02 register)
        TxBuf[0] = 0x02;
        ASSERT_CS();
        HAL_SPI_TransmitReceive(&hspi1, TxBuf, RxBuf, 3, 100);
        DEASSERT_CS();

        // Print MSB register
        sprintf(debugStr, "MSB Register: 0x%02X\n\r", RxBuf[1]);
        HAL_UART_Transmit(&huart1, (uint8_t*)debugStr, strlen(debugStr), 100);

        tc72.msb_temp = RxBuf[1];
        tc72.lsb_temp = RxBuf[2];

        int16_t Temp_reg = (tc72.msb_temp << 8) | tc72.lsb_temp;
        Temp_reg >>= 7;
        x = ((signed char)(Temp_reg));
        temp += x;

        floatToStr(temp, str);

        HAL_UART_Transmit(&huart1, (uint8_t*)str2, strlen(str2), 100);
        HAL_UART_Transmit(&huart1, (uint8_t*)str, strlen(str), 100);
        HAL_UART_Transmit(&huart1, (uint8_t*)"\n\r", 2, 100);
        HAL_Delay(1000);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
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
void MX_SPI1_Init(void)
{
    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
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

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin : CS_Pin */
    GPIO_InitStruct.Pin = CS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CS_GPIO_Port, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
}

uint8_t readRegister(uint8_t reg)
{
    uint8_t TxBuf[1] = {reg};
    uint8_t RxBuf[1];

    ASSERT_CS();
    if (HAL_SPI_TransmitReceive(&hspi1, TxBuf, RxBuf, 1, 100) != HAL_OK) {
        DEASSERT_CS();
        return 0xFF; // Error value
    }
    DEASSERT_CS();

    return RxBuf[0];
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
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */
