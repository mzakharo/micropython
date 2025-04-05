#define MICROPY_HW_BOARD_NAME               "01Studio pyWiFi-ESP32-S3P AI"
#define MICROPY_HW_MCU_NAME                 "ESP32S3"

#define MICROPY_PY_MACHINE_DAC              (0)

#define MICROPY_HW_I2C0_SCL                 (9)
#define MICROPY_HW_I2C0_SDA                 (8)

#define MICROPY_PY_PICLIB				(1)

#define MICROPY_ENABLE_TFTLCD			(1)
#define MICROPY_STRING_SIZE_24			(1)
#define MICROPY_STRING_SIZE_32			(1)
#define MICROPY_STRING_SIZE_48			(1)

#define MICROPY_HW_LCD32				(1)	
#define MICROPY_HW_LCD15				(1)	
#define MICROPY_HW_LCD18				(1)	

#define LCD_PIN_DC						(48)
#define LCD_PIN_RST						(38)
#define LCD_PIN_CS						(39)
#define LCD_PIN_CLK						(40)
#define LCD_PIN_MISO					(41)
#define LCD_PIN_MOSI					(42)

#define	MICROPY_ENABLE_TOUCH			(1)
#define	MICROPY_HW_XPT2046				(1)

#define XPT_PIN_IRQ						(47)
#define XPT_PIN_CS						(3)

#define MICROPY_ENABLE_GUI				(1)
#define MICROPY_GUI_BUTTON				(1)
#define GUI_BTN_NUM_MAX					(20)
#define GUI_BTN_STR_LEN					(20)

//sensor
#define MICROPY_ENABLE_SENSOR			(1)
#define MICROPY_HW_OV2640				(1)

#define MICROPY_HW_ESPAI				(1)
#define MICROPY_ENABLE_FACE_DETECTION	(1)
#define MICROPY_ENABLE_CAT_DETECTION	(1)
#define MICROPY_ENABLE_COLOR_DETECTION	(1)
#define MICROPY_ENABLE_CODE_RECOGNITION	(1)
#define MICROPY_ENABLE_MOTION_DETECTION	(1)
#define MICROPY_ENABLE_FACE_RECOGNITION (1)

//WEB stream
#define MICROPY_ENABLE_STREAM			(1) 
//CAM
#define CAM_PIN_XCLK					(10)
#define CAM_PIN_SIOD					(21)
#define CAM_PIN_SIOC					(14)

#define CAM_PIN_D7						(11)
#define CAM_PIN_D6						(9)
#define CAM_PIN_D5						(8)
#define CAM_PIN_D4						(16)
#define CAM_PIN_D3						(6)
#define CAM_PIN_D2						(4)
#define CAM_PIN_D1						(5)
#define CAM_PIN_D0						(17)
#define CAM_PIN_VSYNC					(13)
#define CAM_PIN_HREF					(12)
#define CAM_PIN_PCLK					(18)

#ifndef CONFIG_OV2640_SUPPORT
#define CONFIG_OV2640_SUPPORT			(1)
#endif
#define CONFIG_SCCB_HARDWARE_I2C_PORT1	(1)
//usb cam
#define MICROPY_ENABLE_ESP_USB			(1)
#define MICROPY_HW_USB_CAM				(1)


