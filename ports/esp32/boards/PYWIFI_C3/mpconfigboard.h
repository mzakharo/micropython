#define MICROPY_HW_BOARD_NAME "01Studio pyWiFi-ESP32-C3"
#define MICROPY_HW_MCU_NAME "ESP32C3"

#define MICROPY_HW_I2C0_SCL (GPIO_NUM_4)
#define MICROPY_HW_I2C0_SDA (GPIO_NUM_5)

#define MICROPY_HW_SPI1_SCK (14)
#define MICROPY_HW_SPI1_MOSI (13)
#define MICROPY_HW_SPI1_MISO (12)

#define MICROPY_HW_SPI2_SCK (18)
#define MICROPY_HW_SPI2_MOSI (21)
#define MICROPY_HW_SPI2_MISO (19)

#define MICROPY_HW_ENABLE_SDCARD	(0)
#define MICROPY_PY_MACHINE_DAC		(0)
#define MICROPY_PY_MACHINE_I2S		(0)

#define MICROPY_ENABLE_TFTLCD		(1)
#define MICROPY_HW_LCD15			(1)

#define LCD_PIN_DC					(18)
#define LCD_PIN_RST					(19)
#define LCD_PIN_CS					(6)
#define LCD_PIN_CLK					(0)
#define LCD_PIN_MISO				(7)
#define LCD_PIN_MOSI				(1)

#define MICROPY_STRING_SIZE_24		(1)
#define MICROPY_STRING_SIZE_32		(1)

#define MICROPY_STRING_SIZE_48		(1)
#define MICROPY_HW_LCD32			(1)	
#define MICROPY_HW_LCD18			(1)	

#define MICROPY_PY_PICLIB			(1)

#define	MICROPY_ENABLE_TOUCH		(1)
#define	MICROPY_HW_XPT2046			(1)

#define XPT_PIN_IRQ					(9)
#define XPT_PIN_CS					(2)

#define MICROPY_ENABLE_GUI			(1)
#define MICROPY_GUI_BUTTON			(1)
#define GUI_BTN_NUM_MAX				(15)
#define GUI_BTN_STR_LEN				(20)


