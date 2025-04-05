#define MICROPY_HW_BOARD_NAME "01Studio pyWiFi-ESP32P"
#define MICROPY_HW_MCU_NAME "ESP32"

#define MICROPY_ENABLE_TFTLCD			(1)
#define MICROPY_STRING_SIZE_24			(1)
#define MICROPY_STRING_SIZE_32			(1)
#define MICROPY_STRING_SIZE_48			(1)
#define MICROPY_HW_LCD32				(1)	
#define MICROPY_HW_LCD18				(1)	
#define MICROPY_HW_LCD15				(1)

#define MICROPY_PY_PICLIB				(1)
#define LCD_PIN_DC						(21)
#define LCD_PIN_RST						(33)
#define LCD_PIN_CS						(15)
#define LCD_PIN_CLK						(14)
#define LCD_PIN_MISO					(12)
#define LCD_PIN_MOSI					(13)

#define	MICROPY_ENABLE_TOUCH			(1)
#define	MICROPY_HW_XPT2046				(1)

#define XPT_PIN_IRQ    	0
#define XPT_PIN_CS    	2

#define MICROPY_ENABLE_GUI				(1)
#define MICROPY_GUI_BUTTON				(1)
#define GUI_BTN_NUM_MAX					(15)
#define GUI_BTN_STR_LEN					(20)