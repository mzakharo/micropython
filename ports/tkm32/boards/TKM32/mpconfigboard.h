#define MICROPY_HW_BOARD_NAME		"01Studio Davinci"
#define MICROPY_HW_MCU_NAME			"TKM32F499"
#define	MICROPY_HW_FLASH_FS_LABEL	"DAVINCI"

#define MICROPY_HW_ENABLE_INTERNAL_FLASH_STORAGE (0)
#define MICROPY_HW_HAS_SWITCH       (1)
#define MICROPY_HW_HAS_FLASH        (1)
#define MICROPY_HW_ENABLE_RTC       (1)
#define MICROPY_HW_ENABLE_RNG       (1)
#define MICROPY_HW_ENABLE_SERVO     (0)

#define MICROPY_PY_NETWORK                  (1)
#define MICROPY_PY_USOCKET                  (1)
/***********net mod**************/
#define ESP8285_BUF_SIZE 40960
#define MICROPY_UART_NIC 1

// #define MICROPY_PY_THREAD_GIL					(1)
// #define MICROPY_PY_THREAD							(1)

// use external Qflash for storage
#define QFLASH_BASE_ADDR						(0x300000)
#define QFLASH_BLOCK_SIZE   	8     
#define MICROPY_HW_QFLASH_SIZE_BITS (1024 * 1024 * 128)
#define QFLASH_IOCTL_NUM_BLOCKS			((MICROPY_HW_QFLASH_SIZE_BITS-(QFLASH_BASE_ADDR * QFLASH_BLOCK_SIZE)) / QFLASH_BLOCK_SIZE / FLASH_BLOCK_SIZE)
//#define QFLASH_IOCTL_NUM_BLOCKS			((MICROPY_HW_QFLASH_SIZE_BITS-(QFLASH_BASE_ADDR * 8)) / 8 / FLASH_BLOCK_SIZE)

// LEDs
#define MICROPY_HW_LED1             (pin_C2) // red
#define MICROPY_HW_LED2             (pin_C3) // green
#define MICROPY_HW_LED3             (pin_C6) // orange
#define MICROPY_HW_LED4             (pin_C7) // blue
#define MICROPY_HW_LED_ON(pin)      (mp_hal_pin_high(pin))
#define MICROPY_HW_LED_OFF(pin)     (mp_hal_pin_low(pin))

// USRSW is pulled low. Pressing the button makes the input go high.
#define MICROPY_HW_USRSW_PIN        (pin_A0)
#define MICROPY_HW_USRSW_PULL       (MP_HAL_PIN_PULL_DOWN)
#define MICROPY_HW_USRSW_EXTI_MODE  (GPIO_MODE_IT_FALLING)//MP_HAL_PIN_MODE_IPUP
#define MICROPY_HW_USRSW_PRESSED    (1)

// USRSW is pulled low. Pressing the button makes the input go high.
#define MICROPY_START_MODE_PIN        (pin_A1)
#define MICROPY_START_MODE_PULL       (MP_HAL_PIN_PULL_DOWN)
#define MICROPY_START_MODE_EXTI_MODE  (GPIO_MODE_IT_FALLING)//MP_HAL_PIN_MODE_IPUP
#define MICROPY_START_MODE_PRESSED    (1)


// SD card detect switch
#define MICROPY_HW_ENABLE_SDCARD			(1)
#define MICROPY_HW_SDCARD_DETECT_PIN        (pin_A8)
#define MICROPY_HW_SDCARD_DETECT_PULL       (MP_HAL_PIN_PULL_NONE)
#define MICROPY_HW_SDCARD_DETECT_PRESENT    (0)

//#define MICROPY_HW_UART_REPL        PYB_UART_2
//#define MICROPY_HW_UART_REPL_BAUD   115200

#define MICROPY_HW_UART1_NAME   "UART1"    // on RX / TX
#define MICROPY_HW_UART1_TX     (pin_A9)
#define MICROPY_HW_UART1_RX     (pin_A10)

#define MICROPY_HW_UART2_NAME   "UART2"    // on RX / TX
#define MICROPY_HW_UART2_TX     (pin_A2)
#define MICROPY_HW_UART2_RX     (pin_A3)

#define MICROPY_HW_UART3_NAME   "UART3"    // on RX / TX
#define MICROPY_HW_UART3_TX     (pin_B10)
#define MICROPY_HW_UART3_RX     (pin_B11)

#define MICROPY_HW_UART4_NAME   "UART4"    // on RX / TX
#define MICROPY_HW_UART4_TX     (pin_D7)
#define MICROPY_HW_UART4_RX     (pin_D6)

#define MICROPY_HW_I2C1_NAME	"I2C1"
#define MICROPY_HW_I2C1_SCL		(pin_B2)
#define MICROPY_HW_I2C1_SDA		(pin_B0)

#define MICROPY_HW_I2C3_NAME	"I2C3"
#define MICROPY_HW_I2C3_SCL		(pin_C1)
#define MICROPY_HW_I2C3_SDA		(pin_C0)

#define MICROPY_HW_SPI1_NAME	"SPI1"
#define MICROPY_HW_SPI1_NSS		(pin_B3)
#define MICROPY_HW_SPI1_SCK		(pin_B2)
#define MICROPY_HW_SPI1_MISO	(pin_B1)
#define MICROPY_HW_SPI1_MOSI	(pin_B0)

#define MICROPY_HW_SPI2_NAME	"SPI2"
#define MICROPY_HW_SPI2_NSS		(pin_A4)
#define MICROPY_HW_SPI2_SCK		(pin_A5)
#define MICROPY_HW_SPI2_MISO	(pin_A6)
#define MICROPY_HW_SPI2_MOSI	(pin_A7)

#define MICROPY_HW_SPI4_NAME	"SPI4"
#define MICROPY_HW_SPI4_NSS		(pin_C0)
#define MICROPY_HW_SPI4_SCK		(pin_C1)
#define MICROPY_HW_SPI4_MISO	(pin_C2)
#define MICROPY_HW_SPI4_MOSI	(pin_C3)

#define	MICROPY_ENABLE_TOUCH			(1)
#define MICROPY_HW_FT54X6				(1)
#define MICROPY_HW_GT911				(1)

#define MICROPY_ENABLE_TFTLCD			(1)
#define	MICROPY_HW_LCD43G				(1)
#define	MICROPY_HW_LCD7R				(1)

#define MICROPY_PY_PICLIB				(1) //图片解码

#define MICROPY_ENABLE_GUI				(1) 	//GUI支持
#define MICROPY_GUI_BUTTON				(1) 	//按钮控件
#define GUI_BTN_NUM_MAX					(50) 	//最大支持按钮数量


#define MICROPY_HW_LTDC_DE			(pin_B4)
#define MICROPY_HW_LTDC_PCLK       	(pin_B5)
#define MICROPY_HW_LTDC_HSYNC       (pin_B6)
#define MICROPY_HW_LTDC_VSYNC       (pin_B7)

//#define MICROPY_HW_SPI_SDA     			(pin_B0)
#define MICROPY_HW_SPI_SDA			(pin_B10)
#define MICROPY_HW_SPI_CLK			(pin_B9)
#define MICROPY_HW_SPI_CS			(pin_B11)

#define MICROPY_HW_LTDC_BL			(pin_D8)
#define MICROPY_HW_LTDC_RST       	(pin_D6)

