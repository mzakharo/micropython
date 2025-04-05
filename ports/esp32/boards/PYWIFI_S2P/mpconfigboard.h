#define MICROPY_HW_BOARD_NAME               "01Studio pyWiFi-ESP32-S2P"
#define MICROPY_HW_MCU_NAME                 "ESP32S2"

#define MICROPY_PY_BLUETOOTH			(0)
#define MICROPY_HW_ENABLE_SDCARD		(0)
#define MICROPY_PY_MACHINE_I2S			(0)

#define MICROPY_PY_PICLIB				(1)

#define MICROPY_ENABLE_TFTLCD			(1)
#define MICROPY_STRING_SIZE_24			(1)
#define MICROPY_STRING_SIZE_32			(1)
#define MICROPY_STRING_SIZE_48			(1)

#define MICROPY_HW_LCD32				(1)	
#define MICROPY_HW_LCD15				(1)	
#define MICROPY_HW_LCD18				(1)	

#define LCD_PIN_DC						(35)
#define LCD_PIN_RST						(36)
#define LCD_PIN_CS						(37)
#define LCD_PIN_CLK						(38)
#define LCD_PIN_MISO					(39)
#define LCD_PIN_MOSI					(40)

#define	MICROPY_ENABLE_TOUCH			(1)
#define	MICROPY_HW_XPT2046				(1)

#define XPT_PIN_IRQ						(33)
#define XPT_PIN_CS						(34)

#define MICROPY_ENABLE_GUI				(1)
#define MICROPY_GUI_BUTTON				(1)
#define GUI_BTN_NUM_MAX					(20)
#define GUI_BTN_STR_LEN					(20)
//sensor
#define MICROPY_ENABLE_SENSOR			(1)
#define MICROPY_HW_OV2640				(1)
//WEB stream
#define MICROPY_ENABLE_STREAM			(1) 

//CAM
#define CAM_PIN_XCLK					(14)
#define CAM_PIN_SIOD					(21)
#define CAM_PIN_SIOC					(18)

#define CAM_PIN_D7						(15)
#define CAM_PIN_D6						(13)
#define CAM_PIN_D5						(12)
#define CAM_PIN_D4						(10)
#define CAM_PIN_D3						(8)
#define CAM_PIN_D2						(5)
#define CAM_PIN_D1						(7)
#define CAM_PIN_D0						(9)
#define CAM_PIN_VSYNC					(17)
#define CAM_PIN_HREF					(16)
#define CAM_PIN_PCLK					(11)

// #ifndef CONFIG_OV2640_SUPPORT
// #define CONFIG_OV2640_SUPPORT			(1)
// #endif

#define CONFIG_OV2640_SUPPORT			(1)
#define CONFIG_SCCB_HARDWARE_I2C_PORT1	(1)
#define CONFIG_CAMERA_CORE0				(1)
#define CONFIG_CAMERA_DMA_BUFFER_SIZE_MAX	8192

//usb cam
#define MICROPY_ENABLE_ESP_USB			(1)
#define MICROPY_HW_USB_CAM				(1)



