#define MICROPY_HW_BOARD_NAME       		"01Studio Magellan"
#define MICROPY_HW_MCU_NAME         		"STM32F767IGT6"
#define	MICROPY_HW_FLASH_FS_LABEL			"MAGELLAN"

#define MICROPY_HW_ENABLE_RTC       (1)
#define MICROPY_HW_ENABLE_RNG       (1)
#define MICROPY_HW_ENABLE_ADC       (1)
#define MICROPY_HW_ENABLE_DAC       (1)
#define MICROPY_HW_ENABLE_USB       (1)
#define MICROPY_HW_ENABLE_SDCARD    (1)
#define MICROPY_HW_HAS_SWITCH       (1)
#define MICROPY_HW_HAS_FLASH        (1)
#define	MICROPY_HW_ENABLE_SERVO		(1) 

// #define MICROPY_PY_THREAD_GIL		(1)
// #define MICROPY_PY_THREAD			(1)

#define MICROPY_ENABLE_SDCARD_NIRQ  (1)

// The pyboard has a 32kHz crystal for the RTC
#define MICROPY_HW_RTC_USE_LSE      	(1)
#define MICROPY_HW_RTC_USE_US       	(0)
#define MICROPY_HW_RTC_USE_CALOUT   	(1)

#define MICROPY_HW_CLK_PLLM (6)
#define MICROPY_HW_CLK_PLLN (216)
#define MICROPY_HW_CLK_PLLP (RCC_PLLP_DIV2)
#define MICROPY_HW_CLK_PLLQ (9)
#define MICROPY_HW_FLASH_LATENCY (FLASH_LATENCY_7) // 210-216 MHz needs 7 wait states

#define MICROPY_HW_BOARD_MAGELLAM			(1)
#define MICROPY_PY_PICLIB					(1)
#define MICROPY_PY_HJPEG_DECODE				(1)
#define	MICROPY_ENABLE_JPEG_UTILS			(1)
#define MICROPY_ENABLE_NEOPIXEL				(1)
//TFTLCD
#define MICROPY_ENABLE_TFTLCD				(1)
#define MICROPY_HW_LTDC_LCD					(1)
#define	MICROPY_HW_LCD43M					(1)
#define	MICROPY_HW_LCD43R					(1)
#define	MICROPY_HW_LCD7R					(1)

//touch
#define	MICROPY_ENABLE_TOUCH				(1)
#define	MICROPY_ENABLE_IIC_DEV				(1)
#define	MICROPY_HW_GT1151					(1)
#define MICROPY_HW_FT54X6					(1)
#define MICROPY_HW_GT911					(1)
//gui

#define MICROPY_ENABLE_GUI					(1)
#define MICROPY_GUI_BUTTON					(1)
#define GUI_BTN_NUM_MAX						(50)
#define GUI_BTN_STR_LEN						(20)
//sensor
#define MICROPY_ENABLE_SENSOR				(1)
#define MICROPY_HW_OV2640					(1)
//audio
#define MICROPY_ENABLE_AUDIO					(1)
#define	MICROPY_HW_WM8978						(1)
#define	MICROPY_ENABLE_MP3						(1)
#define MICROPY_ENABLE_VIDEO					(1)

// UART1 config
#define MICROPY_HW_UART1_NAME				"UART1"    // on RX / TX
#define MICROPY_HW_UART1_TX  				(pin_B14)
#define MICROPY_HW_UART1_RX  				(pin_B15)
// UART config
#define MICROPY_HW_UART2_NAME				"UART2"    // on RX / TX
#define MICROPY_HW_UART2_TX					(pin_A2)
#define MICROPY_HW_UART2_RX					(pin_A3)
#define MICROPY_HW_UART_REPL				PYB_UART_3
#define MICROPY_HW_UART_REPL_BAUD			115200

#define MICROPY_HW_UART3_NAME				"UART3"    // on RX / TX
#define MICROPY_HW_UART3_TX					(pin_B10)
#define MICROPY_HW_UART3_RX					(pin_B11)

// I2C buses
#define MICROPY_HW_I2C1_NAME				"I2C1"
#define MICROPY_HW_I2C1_SCL					(pin_B8)
#define MICROPY_HW_I2C1_SDA					(pin_B9)

#define MICROPY_HW_I2C2_NAME				"I2C2"
#define MICROPY_HW_I2C2_SCL					(pin_H4)
#define MICROPY_HW_I2C2_SDA					(pin_H5)

// SPI buses
// #define MICROPY_HW_SPI3_NSS         (pin_A4)
// #define MICROPY_HW_SPI3_SCK         (pin_B3)
// #define MICROPY_HW_SPI3_MISO        (pin_B4)
// #define MICROPY_HW_SPI3_MOSI        (pin_B5)

// CAN buses
#define MICROPY_HW_CAN1_NAME				"CAN1"
#define MICROPY_HW_CAN1_TX					(pin_B9)
#define MICROPY_HW_CAN1_RX					(pin_B8)

// USRSW is pulled low. Pressing the button makes the input go high.
#define MICROPY_HW_USRSW_PIN				(pin_E3)
#define MICROPY_HW_USRSW_PULL				(GPIO_PULLUP)
#define MICROPY_HW_USRSW_EXTI_MODE			(GPIO_MODE_IT_FALLING)
#define MICROPY_HW_USRSW_PRESSED			(0)

// LEDs
#define MICROPY_HW_LED1						(pin_E2)
#define MICROPY_HW_LED2						(pin_B14)
#define MICROPY_HW_LED3						(pin_B15)
#define MICROPY_HW_LED4						(pin_D11)
#define MICROPY_HW_LED_ON(pin)				(mp_hal_pin_high(pin))
#define MICROPY_HW_LED_OFF(pin)				(mp_hal_pin_low(pin))

// USB config (CN13 - USB OTG FS)
#define MICROPY_HW_USB_FS					(1)
#define MICROPY_HW_USB_VBUS_DETECT_PIN		(pin_A9)
#define MICROPY_HW_USB_OTG_ID_PIN			(pin_A10)

// SD card detect switch
#define MICROPY_HW_SDCARD_DETECT_PIN        (pin_D7)
#define MICROPY_HW_SDCARD_DETECT_PULL       (GPIO_NOPULL)
#define MICROPY_HW_SDCARD_DETECT_PRESENT    (GPIO_PIN_RESET)

#define MICROPY_HW_ETH_MDC					(pin_C1)
#define MICROPY_HW_ETH_MDIO					(pin_A2)
#define MICROPY_HW_ETH_RMII_REF_CLK			(pin_A1)
#define MICROPY_HW_ETH_RMII_CRS_DV			(pin_A7)
#define MICROPY_HW_ETH_RMII_RXD0			(pin_C4)
#define MICROPY_HW_ETH_RMII_RXD1			(pin_C5)
#define MICROPY_HW_ETH_RMII_TX_EN			(pin_B11)
#define MICROPY_HW_ETH_RMII_TXD0			(pin_G13)
#define MICROPY_HW_ETH_RMII_TXD1			(pin_G14)

#if MICROPY_HW_HAS_FLASH
// QSPI Flash 
#define MICROPY_HW_ENABLE_INTERNAL_FLASH_STORAGE 			(0)
#define MICROPY_HW_SPIFLASH_ENABLE_CACHE					(1)
#define MICROPY_HW_SPIFLASH_SIZE_BITS   					(128 * 1024 * 1024)
#define MICROPY_HW_QSPIFLASH_SIZE_BITS_LOG2					(28)
#define MICROPY_HW_QSPIFLASH_CS         					(pin_B6)
#define MICROPY_HW_QSPIFLASH_SCK        					(pin_B2)
#define MICROPY_HW_QSPIFLASH_IO0        					(pin_F8)
#define MICROPY_HW_QSPIFLASH_IO1        					(pin_F9)
#define MICROPY_HW_QSPIFLASH_IO2        					(pin_F7)
#define MICROPY_HW_QSPIFLASH_IO3        					(pin_F6)

// block device config for SPI flash
extern const struct _mp_spiflash_config_t spiflash_config;
extern struct _spi_bdev_t spi_bdev;
#define MICROPY_HW_BDEV_IOCTL(op, arg) ( \
	(op) == BDEV_IOCTL_NUM_BLOCKS ? (MICROPY_HW_SPIFLASH_SIZE_BITS / 8 / FLASH_BLOCK_SIZE) : \
    (op) == BDEV_IOCTL_INIT ? spi_bdev_ioctl(&spi_bdev, (op), (uint32_t)&spiflash_config) : \
    spi_bdev_ioctl(&spi_bdev, (op), (arg)) \
)
#define MICROPY_HW_BDEV_READBLOCKS(dest, bl, n) spi_bdev_readblocks(&spi_bdev, (dest), (bl), (n))
#define MICROPY_HW_BDEV_WRITEBLOCKS(src, bl, n) spi_bdev_writeblocks(&spi_bdev, (src), (bl), (n))
#endif

// SDRAM
#define MICROPY_HW_SDRAM_SIZE  		(16384 *1024)  // 256 Mbit 

#define MICROPY_HW_SDRAM_STARTUP_TEST             (1)

#define MICROPY_HEAP_START              sdram_start()
#define MICROPY_HEAP_END                sdram_end()

#define HJPGD_FRAME_SIZE 			(480*800*2)
#define HJPGD_START_ADDR			(0xD0000000 +  MICROPY_HW_SDRAM_SIZE)

// Timing configuration for 200MHz/2=100MHz (10ns)
#define MICROPY_HW_SDRAM_CLOCK_PERIOD       2
#define MICROPY_HW_SDRAM_CAS_LATENCY        2
#define MICROPY_HW_SDRAM_FREQUENCY          (100000) // 100 MHz
#define MICROPY_HW_SDRAM_TIMING_TMRD        (2)
#define MICROPY_HW_SDRAM_TIMING_TXSR        (7)
#define MICROPY_HW_SDRAM_TIMING_TRAS        (4)
#define MICROPY_HW_SDRAM_TIMING_TRC         (7)
#define MICROPY_HW_SDRAM_TIMING_TWR         (2)
#define MICROPY_HW_SDRAM_TIMING_TRP         (2)
#define MICROPY_HW_SDRAM_TIMING_TRCD        (2)

// 16-bit SDRAM
#define MICROPY_HW_SDRAM_ROW_BITS_NUM       13
#define MICROPY_HW_SDRAM_MEM_BUS_WIDTH      16
#define MICROPY_HW_SDRAM_REFRESH_CYCLES     8192

#define MICROPY_HW_SDRAM_COLUMN_BITS_NUM    9
#define MICROPY_HW_SDRAM_INTERN_BANKS_NUM   4
#define MICROPY_HW_SDRAM_RPIPE_DELAY        0
#define MICROPY_HW_SDRAM_RBURST             (1)
#define MICROPY_HW_SDRAM_WRITE_PROTECTION   (0)
#define MICROPY_HW_SDRAM_AUTOREFRESH_NUM    (8)
#define MICROPY_HW_SDRAM_BURST_LENGTH       1
#define MICROPY_HW_SDRAM_REFRESH_RATE       (64) // ms

#define MICROPY_HW_FMC_SDCKE1   (pin_H7)
#define MICROPY_HW_FMC_SDNE1    (pin_H6)
#define MICROPY_HW_FMC_SDCLK    (pin_G8)
#define MICROPY_HW_FMC_SDNCAS   (pin_G15)
#define MICROPY_HW_FMC_SDNRAS   (pin_F11)
#define MICROPY_HW_FMC_SDNWE    (pin_C0)
#define MICROPY_HW_FMC_BA0      (pin_G4)
#define MICROPY_HW_FMC_BA1      (pin_G5)
#define MICROPY_HW_FMC_NBL0     (pin_E0)
#define MICROPY_HW_FMC_NBL1     (pin_E1)
#define MICROPY_HW_FMC_A0       (pin_F0)
#define MICROPY_HW_FMC_A1       (pin_F1)
#define MICROPY_HW_FMC_A2       (pin_F2)
#define MICROPY_HW_FMC_A3       (pin_F3)
#define MICROPY_HW_FMC_A4       (pin_F4)
#define MICROPY_HW_FMC_A5       (pin_F5)
#define MICROPY_HW_FMC_A6       (pin_F12)
#define MICROPY_HW_FMC_A7       (pin_F13)
#define MICROPY_HW_FMC_A8       (pin_F14)
#define MICROPY_HW_FMC_A9       (pin_F15)
#define MICROPY_HW_FMC_A10      (pin_G0)
#define MICROPY_HW_FMC_A11      (pin_G1)
#define MICROPY_HW_FMC_A12      (pin_G2)
#define MICROPY_HW_FMC_D0       (pin_D14)
#define MICROPY_HW_FMC_D1       (pin_D15)
#define MICROPY_HW_FMC_D2       (pin_D0)
#define MICROPY_HW_FMC_D3       (pin_D1)
#define MICROPY_HW_FMC_D4       (pin_E7)
#define MICROPY_HW_FMC_D5       (pin_E8)
#define MICROPY_HW_FMC_D6       (pin_E9)
#define MICROPY_HW_FMC_D7       (pin_E10)
#define MICROPY_HW_FMC_D8       (pin_E11)
#define MICROPY_HW_FMC_D9       (pin_E12)
#define MICROPY_HW_FMC_D10      (pin_E13)
#define MICROPY_HW_FMC_D11      (pin_E14)
#define MICROPY_HW_FMC_D12      (pin_E15)
#define MICROPY_HW_FMC_D13      (pin_D8)
#define MICROPY_HW_FMC_D14      (pin_D9)
#define MICROPY_HW_FMC_D15      (pin_D10)

#if MICROPY_HW_LCD43M

#define LCD43M_REG	(*(volatile uint16_t *)(0x6807FFFE))
#define LCD43M_RAM	(*(volatile uint16_t *)(0x68080000))

#define MICROPY_HW_LCD43M_BL    (pin_B5)
#define MICROPY_HW_LCD_NE3    	(pin_G10)
#define MICROPY_HW_LCD_NOE      (pin_D4)
#define MICROPY_HW_LCD_NWE      (pin_D5)
#define MICROPY_HW_MCULCD_CS	MICROPY_HW_LCD_NE3

#define MICROPY_HW_LCD_A18       (pin_D13)

#define MICROPY_HW_LCD_D0       (pin_D14)
#define MICROPY_HW_LCD_D1       (pin_D15)
#define MICROPY_HW_LCD_D2       (pin_D0)
#define MICROPY_HW_LCD_D3       (pin_D1)
#define MICROPY_HW_LCD_D4       (pin_E7)
#define MICROPY_HW_LCD_D5       (pin_E8)
#define MICROPY_HW_LCD_D6       (pin_E9)
#define MICROPY_HW_LCD_D7       (pin_E10)
#define MICROPY_HW_LCD_D8       (pin_E11)
#define MICROPY_HW_LCD_D9       (pin_E12)
#define MICROPY_HW_LCD_D10      (pin_E13)
#define MICROPY_HW_LCD_D11      (pin_E14)
#define MICROPY_HW_LCD_D12      (pin_E15)
#define MICROPY_HW_LCD_D13      (pin_D8)
#define MICROPY_HW_LCD_D14      (pin_D9)
#define MICROPY_HW_LCD_D15      (pin_D10)
#endif

#if MICROPY_HW_LCD43R
#define MICROPY_RGB_SPI_SDA     (pin_D13)  //RS
#define MICROPY_RGB_SPI_CLK     (pin_D5)  //WR
#define MICROPY_RGB_SPI_CS      (pin_G10)
#endif

#if MICROPY_HW_LTDC_LCD
#define MICROPY_HW_LTDC_BL      (pin_B5)
#define MICROPY_HW_LTDC_DE      (pin_F10)
#define MICROPY_HW_LTDC_PCLK    (pin_G7)
#define MICROPY_HW_LTDC_HSYNC   (pin_I10)
#define MICROPY_HW_LTDC_VSYNC   (pin_I9)
#define MICROPY_HW_LCD_R0      	(pin_H2)
#define MICROPY_HW_LCD_R1      	(pin_H3)
#define MICROPY_HW_LCD_R2      	(pin_H8)
#define MICROPY_HW_LCD_R3      	(pin_H9)
#define MICROPY_HW_LCD_R4      	(pin_H10)
#define MICROPY_HW_LCD_R5      	(pin_H11)
#define MICROPY_HW_LCD_R6      	(pin_H12)
#define MICROPY_HW_LCD_R7      	(pin_G6)
#define MICROPY_HW_LCD_G0      	(pin_E5)
#define MICROPY_HW_LCD_G1      	(pin_E6)
#define MICROPY_HW_LCD_G2      	(pin_H13)
#define MICROPY_HW_LCD_G3      	(pin_H14)
#define MICROPY_HW_LCD_G4      	(pin_H15)
#define MICROPY_HW_LCD_G5      	(pin_I0)
#define MICROPY_HW_LCD_G6      	(pin_I1)
#define MICROPY_HW_LCD_G7      	(pin_I2)
#define MICROPY_HW_LCD_B0      	(pin_E4)
#define MICROPY_HW_LCD_B1      	(pin_G12)
#define MICROPY_HW_LCD_B2      	(pin_D6)
#define MICROPY_HW_LCD_B3      	(pin_G11)
#define MICROPY_HW_LCD_B4      	(pin_I4)
#define MICROPY_HW_LCD_B5      	(pin_I5)
#define MICROPY_HW_LCD_B6      	(pin_I6)
#define MICROPY_HW_LCD_B7      	(pin_I7)

#endif

#if MICROPY_HW_OV2640
#define MICROPY_HW_DCMI_RESE	(pin_I8)
#define MICROPY_HW_DCMI_PWDN	(pin_G9)
#define MICROPY_HW_DCMI_HSYNC	(pin_A4)
#define MICROPY_HW_DCMI_PIXCK	(pin_A6)
#define MICROPY_HW_DCMI_VSYNC	(pin_B7)
#define MICROPY_HW_DCMI_XCLK	(pin_A8)
#define MICROPY_HW_DCMI_D0		(pin_C6)
#define MICROPY_HW_DCMI_D1		(pin_C7)
#define MICROPY_HW_DCMI_D2		(pin_C8)
#define MICROPY_HW_DCMI_D3		(pin_C9)
#define MICROPY_HW_DCMI_D4		(pin_C11)
#define MICROPY_HW_DCMI_D5		(pin_D3)
#define MICROPY_HW_DCMI_D6		(pin_B8)
#define MICROPY_HW_DCMI_D7		(pin_B9)

#endif




