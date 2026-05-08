/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Teste ST7735S 1.8" 128x160 via SPI3 - Blackpill STM32F411
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"

#include <stdint.h>

/* Private define ------------------------------------------------------------*/
#define APP_START_ADDRESS 0x08004000U

/* =========================
 * PINAGEM LCD ST7735S
 * ========================= */

#define LCD_CS_PORT    GPIOB
#define LCD_CS_PIN     GPIO_PIN_4

#define LCD_DC_PORT    GPIOB
#define LCD_DC_PIN     GPIO_PIN_6

#define LCD_RST_PORT   GPIOB
#define LCD_RST_PIN    GPIO_PIN_7

/* =========================
 * DIMENSÕES DO DISPLAY
 * ========================= */

#define ST7735_WIDTH   128
#define ST7735_HEIGHT  160

/* =========================
 * COMANDOS ST7735
 * ========================= */

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_DISPON  0x29
#define ST7735_INVON   0x21
#define ST7735_INVOFF  0x20

/* =========================
 * CORES RGB565
 * ========================= */

#define COLOR_BLACK    0x0000
#define COLOR_WHITE    0xFFFF
#define COLOR_RED      0xF800
#define COLOR_GREEN    0x07E0
#define COLOR_BLUE     0x001F
#define COLOR_YELLOW   0xFFE0
#define COLOR_CYAN     0x07FF
#define COLOR_MAGENTA  0xF81F

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

static void lcd_init(void);
static void lcd_reset(void);
static void lcd_write_command(uint8_t cmd);
static void lcd_write_data(uint8_t data);
static void lcd_write_data_buffer(uint8_t *data, uint16_t len);
static void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
static void lcd_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
static void lcd_draw_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
static void lcd_fill_screen(uint16_t color);

static void lcd_cs_low(void);
static void lcd_cs_high(void);
static void lcd_dc_command(void);
static void lcd_dc_data(void);

static void error_blink(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /*
   * Necessário porque o app está em 0x08004000,
   * depois do bootloader WeAct.
   */
  SCB->VTOR = APP_START_ADDRESS;
  __DSB();
  __ISB();

  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_SPI3_Init();

  /*
   * Estado inicial seguro.
   */
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);

  HAL_Delay(100);

  lcd_init();

  /*
   * Tela preta para validar que o display respondeu.
   */
  lcd_fill_screen(COLOR_BLACK);

  /*
   * Um pixel vermelho no centro aproximado.
   */


  /*
   * Bloco vermelho 9x9 no centro.
   * Isso facilita enxergar que funcionou.
   */
  //lcd_draw_block(60, 76, 9, 9, COLOR_RED);

  /*
   * Alguns pixels/blocos extras para testar cores.
   */
  lcd_draw_block(5, 5, 10, 10, COLOR_GREEN);
  lcd_draw_block(20, 5, 10, 10, COLOR_BLUE);
  lcd_draw_block(35, 5, 10, 10, COLOR_YELLOW);


  HAL_Delay(5000);

  int y = 0;
  while (1)
  {

	  lcd_draw_pixel(64, y, COLOR_RED);
	  y++;

	  if(y>160){
		  y = 0;
	  }
	  HAL_Delay(200);
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

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /*
   * Blackpill com cristal externo de 25 MHz.
   *
   * HSE  = 25 MHz
   * PLLM = 25  -> 1 MHz
   * PLLN = 192 -> 192 MHz
   * PLLP = 2   -> SYSCLK 96 MHz
   * PLLQ = 4   -> 48 MHz
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 96 MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    // 48 MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    // 96 MHz

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* =========================
 * LCD - CONTROLE DE PINOS
 * ========================= */

static void lcd_cs_low(void)
{
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
}

static void lcd_cs_high(void)
{
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static void lcd_dc_command(void)
{
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
}

static void lcd_dc_data(void)
{
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
}

/* =========================
 * LCD - ESCRITA SPI
 * ========================= */

static void lcd_write_command(uint8_t cmd)
{
  lcd_dc_command();
  lcd_cs_low();

  if (HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    lcd_cs_high();
    Error_Handler();
  }

  lcd_cs_high();
}

static void lcd_write_data(uint8_t data)
{
  lcd_dc_data();
  lcd_cs_low();

  if (HAL_SPI_Transmit(&hspi3, &data, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    lcd_cs_high();
    Error_Handler();
  }

  lcd_cs_high();
}

static void lcd_write_data_buffer(uint8_t *data, uint16_t len)
{
  lcd_dc_data();
  lcd_cs_low();

  if (HAL_SPI_Transmit(&hspi3, data, len, HAL_MAX_DELAY) != HAL_OK)
  {
    lcd_cs_high();
    Error_Handler();
  }

  lcd_cs_high();
}

/* =========================
 * LCD - INICIALIZAÇÃO
 * ========================= */

static void lcd_reset(void)
{
  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(10);

  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(50);

  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(120);
}

static void lcd_init(void)
{
  lcd_cs_high();

  lcd_reset();

  /*
   * Software reset.
   */
  lcd_write_command(ST7735_SWRESET);
  HAL_Delay(150);

  /*
   * Sai do modo sleep.
   */
  lcd_write_command(ST7735_SLPOUT);
  HAL_Delay(150);

  /*
   * Pixel format:
   * 0x05 = 16 bits por pixel, RGB565.
   */
  lcd_write_command(ST7735_COLMOD);
  lcd_write_data(0x05);
  HAL_Delay(10);

  /*
   * MADCTL controla orientação e ordem RGB/BGR.
   *
   * 0x00 = orientação básica.
   * Se a tela ficar invertida/girada ou cores trocadas,
   * ajustamos esse valor depois.
   */
  lcd_write_command(ST7735_MADCTL);
  lcd_write_data(0x00);

  /*
   * Liga o display.
   */
  lcd_write_command(ST7735_DISPON);
  HAL_Delay(150);
}

/* =========================
 * LCD - DESENHO
 * ========================= */

static void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  /*
   * Define intervalo de colunas.
   */
  lcd_write_command(ST7735_CASET);

  uint8_t col_data[4] = {
      0x00, x0,
      0x00, x1
  };

  lcd_write_data_buffer(col_data, 4);

  /*
   * Define intervalo de linhas.
   */
  lcd_write_command(ST7735_RASET);

  uint8_t row_data[4] = {
      0x00, y0,
      0x00, y1
  };

  lcd_write_data_buffer(row_data, 4);

  /*
   * Próximos bytes enviados serão dados de pixels.
   */
  lcd_write_command(ST7735_RAMWR);
}

static void lcd_draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
  if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
  {
    return;
  }

  lcd_set_address_window(x, y, x, y);

  uint8_t color_data[2] = {
      (uint8_t)(color >> 8),
      (uint8_t)(color & 0xFF)
  };

  lcd_write_data_buffer(color_data, 2);
}

static void lcd_draw_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
  for (uint8_t yy = y; yy < (uint8_t)(y + h); yy++)
  {
    for (uint8_t xx = x; xx < (uint8_t)(x + w); xx++)
    {
      lcd_draw_pixel(xx, yy, color);
    }
  }
}

static void lcd_fill_screen(uint16_t color)
{
  lcd_set_address_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);

  uint8_t color_data[2] = {
      (uint8_t)(color >> 8),
      (uint8_t)(color & 0xFF)
  };

  lcd_dc_data();
  lcd_cs_low();

  for (uint32_t i = 0; i < (uint32_t)ST7735_WIDTH * ST7735_HEIGHT; i++)
  {
    if (HAL_SPI_Transmit(&hspi3, color_data, 2, HAL_MAX_DELAY) != HAL_OK)
    {
      lcd_cs_high();
      Error_Handler();
    }
  }

  lcd_cs_high();
}

/* =========================
 * ERRO
 * ========================= */

static void error_blink(void)
{
  /*
   * Se você não configurou PC13 como saída nesse projeto,
   * essa função não será visível. Não é obrigatório para o LCD.
   */
  while (1)
  {
    HAL_Delay(100);
  }
}

void Error_Handler(void)
{
  __disable_irq();
  error_blink();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
