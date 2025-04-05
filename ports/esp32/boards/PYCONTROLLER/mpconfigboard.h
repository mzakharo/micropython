#define MICROPY_HW_BOARD_NAME "01Studio PYCONTROLLER-ESP32S3"
#define MICROPY_HW_MCU_NAME "ESP32S3"

#define MICROPY_PY_MACHINE_DAC			(0)

#define MICROPY_HW_I2C0_SCL				(9)
#define MICROPY_HW_I2C0_SDA				(8)

#define MICROPY_ENABLE_TFTLCD			(1)

#define MICROPY_STRING_SIZE_24			(1)
#define MICROPY_STRING_SIZE_32			(1)
#define MICROPY_STRING_SIZE_48			(1)

#define MICROPY_HW_LCD15				(1)

#define MICROPY_PY_PICLIB				(1)

#define LCD_PIN_RST						(42)
#define LCD_PIN_DC						(38)
#define LCD_PIN_CS						(39)
#define LCD_PIN_CLK						(40)
#define LCD_PIN_MISO					(-1)
#define LCD_PIN_MOSI					(41)

#define MICROPY_ENABLE_CONTROLLER		(1)
#define MICROPY_HW_GAMEPAD				(1)

#define MICROPY_ENABLE_GAME				(1)
#define MICROPY_HW_NESEMU				(1)
#define MICROPY_ENABLE_PSXCONTROLLER	(1)

#define PSX_GPIO_UP						(10)
#define PSX_GPIO_DOWN					(11)
#define PSX_GPIO_LEFT					(12)
#define PSX_GPIO_RIGHT					(13)
#define PSX_GPIO_X						(14)
#define PSX_GPIO_Y						(15)
#define PSX_GPIO_A						(16)
#define PSX_GPIO_B						(21)
#define PSX_GPIO_BACK					(1)
#define PSX_GPIO_START					(0)
#define PSX_GPIO_A_OK					(6)
#define PSX_GPIO_B_OK					(9)

#define PSX_UP_PRESSED					(0)
#define PSX_DOWN_PRESSED				(0)
#define PSX_LEFT_PRESSED				(0)
#define PSX_RIGHT_PRESSED				(0)
#define PSX_X_PRESSED					(0)
#define PSX_Y_PRESSED					(0)
#define PSX_A_PRESSED					(0)
#define PSX_B_PRESSED					(0)
#define PSX_BACK_PRESSED				(0)
#define PSX_START_PRESSED				(0)
#define PSX_AOK_PRESSED					(0)
#define PSX_BOK_PRESSED					(0)

#define PSX_ADCL_LEFTRIGHT_CH		ADC1_CHANNEL_3
#define PSX_ADCL_UPDOWN_CH			ADC1_CHANNEL_4

#define PSX_ADCR_LEFTRIGHT_CH		ADC1_CHANNEL_6
#define PSX_ADCR_UPDOWN_CH			ADC1_CHANNEL_7

#define PSX_JOY_UP_VALUE				(600*4)
#define PSX_JOY_DOWN_VALUE				(200*4)
#define PSX_JOY_LEFT_VALUE				(200*4)
#define PSX_JOY_RIGHT_VALUE				(600*4)





