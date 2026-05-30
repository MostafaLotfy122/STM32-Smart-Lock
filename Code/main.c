/* USER CODE BEGIN Header */
/**
  * @file           : main.c
  * @brief          : Smart Lock - Anti-Bounce & Hardware Mapping Build
  * @author         : Shaden Ibrahim
  **************************
  */
/* USER CODE END Header */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include "i2c-lcd.h"
#include "rc522.h"

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart1;
SPI_HandleTypeDef hspi1;

uint8_t rx_data;
uint8_t bt_unlocked_flag = 0;

char password[5] = "1234";
char entered_pass[5];
int key_count = 0;

uint8_t str[16];
uint8_t MyCard[4] = {0x59, 0xAE, 0x0B, 0x7B};

GPIO_TypeDef* Row_Ports[] = {GPIOB, GPIOB, GPIOB, GPIOB};
uint16_t Row_Pins[] = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};
GPIO_TypeDef* Col_Ports[] = {GPIOA, GPIOA, GPIOA, GPIOA};
uint16_t Col_Pins[] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11};

// MAPPING: Swapped Rows 3 & 4 based on your hardware behavior
char keys[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'*','0','#','D'},
    {'7','8','9','C'}
};

/* Prototypes */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
void Buzzer_Beep(int duration);
void delay_us(uint16_t us);
void Servo_Write(int angle);
void Access_Granted_Sequence(void);
void Access_Denied_Sequence(void);
void Change_Password_Sequence(void);
char Keypad_Scan(void);

/* --- Logic Helpers --- */

void delay_us(uint16_t us) {
    volatile uint32_t count = us * 10;
    while (count--) { __NOP(); }
}

void Buzzer_Beep(int duration) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);
    HAL_Delay(duration);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
}

void Servo_Write(int angle) {
    uint16_t pulse_us = (angle == 0) ? 500 : 2500;
    for(int i = 0; i < 50; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
        delay_us(pulse_us);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
        delay_us(20000 - pulse_us);
    }
}

void Access_Granted_Sequence(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET);
    Buzzer_Beep(200);
    lcd_clear();
    lcd_send_string("ACCESS GRANTED");
    Servo_Write(180);
    HAL_Delay(2000);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
    lcd_clear();
    lcd_send_string("LOCKING...");
    Servo_Write(0);
    lcd_clear();
}

void Access_Denied_Sequence(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    lcd_clear();
    lcd_send_string("WRONG ENTRY");
    Buzzer_Beep(500);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    lcd_clear();
}

void Change_Password_Sequence(void) {
    char temp_pass[5] = {0};
    int count = 0;
    lcd_clear();
    lcd_send_string("Old Password:");
    while(count < 4) {
        char k = Keypad_Scan();
        if (k >= '0' && k <= '9') {
            temp_pass[count++] = k;
            lcd_put_cur(1, count-1);
            lcd_send_data('*');
        }
    }
    temp_pass[4] = '\0';
    if (strcmp(temp_pass, password) == 0) {
        lcd_clear();
        lcd_send_string("New Password:");
        count = 0;
        memset(temp_pass, 0, 5);
        while(count < 4) {
            char k = Keypad_Scan();
            if (k >= '0' && k <= '9') {
                temp_pass[count++] = k;
                lcd_put_cur(1, count-1);
                lcd_send_data('*');
            }
        }
        strcpy(password, temp_pass);
        lcd_clear();
        lcd_send_string("CHANGED!");
        Buzzer_Beep(200);
        HAL_Delay(1000);
    } else {
        lcd_clear();
        lcd_send_string("WRONG OLD PASS");
        Buzzer_Beep(600);
        HAL_Delay(1000);
    }
    lcd_clear();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rx_data == 'A') bt_unlocked_flag = 1;
        HAL_UART_Receive_IT(huart, &rx_data, 1);
    }
}

/* --- REFINED KEYPAD SCAN WITH ANTI-BOUNCE --- */
char Keypad_Scan(void) {
    for (int r = 0; r < 4; r++) {
        for (int i = 0; i < 4; i++) HAL_GPIO_WritePin(Row_Ports[i], Row_Pins[i], GPIO_PIN_SET);
        HAL_GPIO_WritePin(Row_Ports[r], Row_Pins[r], GPIO_PIN_RESET);

        for (int c = 0; c < 4; c++) {
            if (HAL_GPIO_ReadPin(Col_Ports[c], Col_Pins[c]) == GPIO_PIN_RESET) {
                HAL_Delay(30); // INITIAL DEBOUNCE
                if (HAL_GPIO_ReadPin(Col_Ports[c], Col_Pins[c]) == GPIO_PIN_RESET) {
                    while(HAL_GPIO_ReadPin(Col_Ports[c], Col_Pins[c]) == GPIO_PIN_RESET); // WAIT FOR RELEASE
                    HAL_Delay(50); // RELEASE DEBOUNCE
                    return keys[r][c];
                }
            }
        }
    }
    return 0;
}

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  lcd_init();
  RC522_Init();
  HAL_UART_Receive_IT(&huart1, &rx_data, 1);
  Servo_Write(0);
  lcd_clear();

  while (1) {
    if (bt_unlocked_flag == 1) { Access_Granted_Sequence(); bt_unlocked_flag = 0; }
    if (RC522_Request(PICC_REQIDL, str) == MI_OK) {
        if (RC522_Anticoll(str) == MI_OK) {
            if (memcmp(str, MyCard, 4) == 0) Access_Granted_Sequence();
            else Access_Denied_Sequence();
        }
    }

    char key = Keypad_Scan();
    if (key != 0) {
        if (key == '#') {
            Change_Password_Sequence();
            key_count = 0;
            memset(entered_pass, 0, 5);
        }
        else if (key >= '0' && key <= '9') {
            if (key_count < 4) {
                entered_pass[key_count++] = key;
                lcd_put_cur(1, key_count-1);
                lcd_send_data('*');

                if (key_count == 4) {
                    entered_pass[4] = '\0';
                    if (strcmp(entered_pass, password) == 0) Access_Granted_Sequence();
                    else Access_Denied_Sequence();
                    key_count = 0;
                    memset(entered_pass, 0, 5);
                }
            }
        }
    }

    if (key_count == 0) {
        lcd_put_cur(0, 0);
        lcd_send_string("Enter Password:");
    }
    HAL_Delay(20);
  }
}

/* --- Standard Initializations --- */

void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

static void MX_I2C1_Init(void) {
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  HAL_I2C_Init(&hi2c1);
}

static void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.Mode = UART_MODE_TX_RX;
  HAL_UART_Init(&huart1);
}

static void MX_SPI1_Init(void) {
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  HAL_SPI_Init(&hspi1);
}

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
