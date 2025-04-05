#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "i2cdev.h"
#include "mpu6050.h"
#include "hmc5883l.h"
#include "spl06.h"
#include "sensors_mpu6050_spl06.h"
#include "usec_time.h"
#include "config.h"

#include "py/mphal.h"
#include "py/obj.h"
#include "esp_log.h"
#include "filter.h"
#include "imu.h"
#include "stabilizer_types.h"
#include "sensfusion6.h"

static const char* TAG = "mpu6050_spl06";

#define SENSORS_ENABLE_PRESSURE_SPL06
#define SENSORS_ENABLE_MAG_HM5883L

#define MAG_GAUSS_PER_LSB 12000//666.7f

#define ESP_INTR_FLAG_DEFAULT 0
#define SENSORS_NBR_OF_BIAS_SAMPLES		1024	/* 计算方差的采样样本个数 */

#define SENSORS_GYRO_FS_CFG MPU6050_GYRO_FS_2000
#define SENSORS_ACCEL_FS_CFG MPU6050_ACCEL_FS_16

// Buffer length for MPU6050 slave reads
#define GPIO_INTA_MPU6050_IO CONFIG_MPU_PIN_INT
#define SENSORS_MPU6050_BUFF_LEN 14

#if defined(SENSORS_ENABLE_MAG_HM5883L)
#define SENSORS_MAG_STATUS_LEN		1
#define SENSORS_MAG_DATA_LEN		6
#define SENSORS_MAG_BUFF_LEN (SENSORS_MAG_STATUS_LEN + SENSORS_MAG_DATA_LEN)
#else
#define SENSORS_MAG_BUFF_LEN 0
#endif


// MPU6050主机模式读取数据 缓冲区长度
#if defined(SENSORS_ENABLE_PRESSURE_MS5611)
#define SENSORS_BARO_BUFF_S_P_LEN	MS5611_D1D2_SIZE
#define SENSORS_BARO_BUFF_T_LEN		MS5611_D1D2_SIZE

#elif defined(SENSORS_ENABLE_PRESSURE_BMP280) || defined(SENSORS_ENABLE_PRESSURE_SPL06)
#define SENSORS_BARO_STATUS_LEN		1
#define SENSORS_BARO_DATA_LEN		6
#define SENSORS_BARO_BUFF_S_P_LEN	SENSORS_BARO_STATUS_LEN
#define SENSORS_BARO_BUFF_T_LEN		SENSORS_BARO_DATA_LEN
#else
#define SENSORS_BARO_BUFF_S_P_LEN	1
#define SENSORS_BARO_BUFF_T_LEN		1
#endif

#define SENSORS_BARO_BUFF_LEN (SENSORS_BARO_BUFF_S_P_LEN + SENSORS_BARO_BUFF_T_LEN)

#define SENSORS_GYRO_FS_CFG MPU6050_GYRO_FS_2000
#define SENSORS_DEG_PER_LSB_CFG MPU6050_DEG_PER_LSB_2000

#define SENSORS_ACCEL_FS_CFG MPU6050_ACCEL_FS_16
#define SENSORS_G_PER_LSB_CFG MPU6050_G_PER_LSB_16

#define SENSORS_BIAS_SAMPLES 1000
#define SENSORS_ACC_SCALE_SAMPLES 200 /* 加速计采样个数 */
#define SENSORS_GYRO_BIAS_CALCULATE_STDDEV

#define GYRO_NBR_OF_AXES 3
#define GYRO_MIN_BIAS_TIMEOUT_MS M2T(1 * 1000)
// Number of samples used in variance calculation. Changing this effects the threshold
#define SENSORS_NBR_OF_BIAS_SAMPLES 1024 /* 计算方差的采样样本个数 */
// Variance threshold to take zero bias for gyro
#define GYRO_VARIANCE_BASE 5000  /* 陀螺仪零偏方差阈值 */

#define GYRO_VARIANCE_THRESHOLD_X (GYRO_VARIANCE_BASE)
#define GYRO_VARIANCE_THRESHOLD_Y (GYRO_VARIANCE_BASE)
#define GYRO_VARIANCE_THRESHOLD_Z (GYRO_VARIANCE_BASE)
#define ESP_INTR_FLAG_DEFAULT 0

#define SENSORS_VARIANCE_MAN_TEST_TIMEOUT   M2T(1000) // Timeout in ms
#define SENSORS_MAN_TEST_LEVEL_MAX          5.0f      // Max degrees off

// This buffer needs to hold data from all sensors
static uint8_t buffer[SENSORS_MPU6050_BUFF_LEN + SENSORS_MAG_BUFF_LEN + SENSORS_BARO_BUFF_LEN] = {0};

// Low Pass filtering
#define GYRO_LPF_CUTOFF_FREQ 80
#define ACCEL_LPF_CUTOFF_FREQ 30
static lpf2pData accLpf[3];
static lpf2pData gyroLpf[3];

static xSemaphoreHandle sensorsDataReady;
static xSemaphoreHandle dataReady;

static xQueueHandle accelerometerDataQueue;
static xQueueHandle gyroDataQueue;
static xQueueHandle magnetometerDataQueue;
static xQueueHandle barometerDataQueue;

static volatile uint64_t imuIntTimestamp;
static sensorData_t sensorData;
static bool isInit = false;

static bool isBarometerPresent = false;
static bool isMagnetometerPresent = false;
static uint8_t isprintf = 0;
typedef struct {
    Axis3f bias;
    Axis3f variance;//
    Axis3f mean;//
    bool isBiasValueFound;
    bool isBufferFilled;
    Axis3i16 *bufHead;
    Axis3i16 buffer[SENSORS_NBR_OF_BIAS_SAMPLES];
} BiasObj;

static Axis3i16 gyroRaw;
static Axis3i16 accelRaw;
static Axis3i16 magRaw;

static float TempRaw;
static float PressureRaw;
static float AslRaw;

static BiasObj gyroBiasRunning;
static Axis3f gyroBias;
#if defined(SENSORS_GYRO_BIAS_CALCULATE_STDDEV) && defined(GYRO_BIAS_LIGHT_WEIGHT)
static Axis3f gyroBiasStdDev;
#endif

static bool gyroBiasFound = false;
static float accScaleSum = 0;
static float accScale = 1;

// Pre-calculated values for accelerometer alignment
float cosPitch;
float sinPitch;
float cosRoll;
float sinRoll;


void debugpeintf(const char *str)
{
	#if CONFIG_USB_ENABLED
	if(isprintf){
		uint16_t len = strlen(str);
		mp_hal_stdout_tx_strn(str,len);
	}
	#endif
}

/*从队列读取陀螺数据*/
bool sensorsReadGyro(Axis3f *gyro)
{
    return (pdTRUE == xQueueReceive(gyroDataQueue, gyro, 0));
}

/*从队列读取加速计数据*/
bool sensorsReadAcc(Axis3f *acc)
{
    return (pdTRUE == xQueueReceive(accelerometerDataQueue, acc, 0));
}

/*从队列读取磁力计数据*/
bool sensorsReadMag(Axis3f *mag)
{
    return (pdTRUE == xQueueReceive(magnetometerDataQueue, mag, 0));
}

/*从队列读取气压数据*/
bool sensorsReadBaro(baro_t *baro)
{
	return (pdTRUE == xQueueReceive(barometerDataQueue, baro, 0));
}

/*获取传感器数据*/
void sensorsAcquire(sensorData_t *sensors, const uint32_t tick)	
{
    sensorsReadGyro(&sensors->gyro);
    sensorsReadAcc(&sensors->acc);
    sensorsReadMag(&sensors->mag);
    sensorsReadBaro(&sensors->baro);
    sensors->interruptTimestamp = sensorData.interruptTimestamp;
}

//中断
static void IRAM_ATTR sensors_inta_isr_handler(void *arg)
{

    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    imuIntTimestamp = usecTimestamp(); //This function returns the number of microseconds since esp_timer was initialized
    xSemaphoreGiveFromISR(sensorsDataReady, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static void sensorsInterruptInit(void)
{

    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
    //bit mask of the pins
    io_conf.pin_bit_mask = (1ULL << MICROPY_MPU_PIN_IRQ);
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    sensorsDataReady = xSemaphoreCreateBinary();
    dataReady = xSemaphoreCreateBinary();
    gpio_config(&io_conf);
    //install gpio isr service
    //portDISABLE_INTERRUPTS();
    gpio_set_intr_type(MICROPY_MPU_PIN_IRQ, GPIO_INTR_POSEDGE);
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(MICROPY_MPU_PIN_IRQ, sensors_inta_isr_handler, (void *)MICROPY_MPU_PIN_IRQ);
    //portENABLE_INTERRUPTS();

    //   FSYNC "shall not be floating, must be set high or low by the MCU"
}

static void sensorsDeviceInit(void)
{
	isMagnetometerPresent = false;
	isBarometerPresent = false;
lab1:
    i2cdevInit(I2C0_DEV);
	
    mpu6050Init(I2C0_DEV);

    if (mpu6050TestConnection() == true) {
        ESP_LOGI(TAG,"MPU6050 I2C connection [OK].\n");
		debugpeintf("MPU6050 I2C connection [OK]\r\n");
    } else {
        ESP_LOGE(TAG,"MPU6050 I2C connection [FAIL].\n");
		debugpeintf("MPU6050 I2C connection [FALL]\r\n");
		while(1){
			vTaskDelay(100 / portTICK_PERIOD_MS);
			if (mpu6050TestConnection() == true){
				ESP_LOGE(TAG,"MPU6050 I2C connection [OK].\n");
				debugpeintf("MPU6050 I2C connection [OK]\r\n");
				break;
			}
		}
    }

	ESP_LOGE(TAG,"start Reset MPU6050\n");
	debugpeintf("start Reset MPU6050\r\n");
    mpu6050Reset();
    vTaskDelay(50 / portTICK_PERIOD_MS);
    // Activate mpu6050
    mpu6050SetSleepEnabled(false);
    // Delay until registers are reset
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // Set x-axis gyro as clock source
    mpu6050SetClockSource(MPU6050_CLOCK_PLL_XGYRO);
    // Delay until clock is set and stable
    vTaskDelay(200 / portTICK_PERIOD_MS);
    // 使温度传感器
    mpu6050SetTempSensorEnabled(true);
    // 禁止中断
    mpu6050SetIntEnabled(false);
    // 将MAG和BARO连接到主I2C总线
    mpu6050SetI2CBypassEnabled(true);
    // 设置陀螺满量程
    mpu6050SetFullScaleGyroRange(SENSORS_GYRO_FS_CFG);
    // 设置加速度计满量程
    mpu6050SetFullScaleAccelRange(SENSORS_ACCEL_FS_CFG);

    // Set digital low-pass bandwidth for gyro and acc
    // board ESP32_S2_DRONE_V1_2 has more vibrations, bandwidth should be lower

    // To low DLPF bandwidth might cause instability and decrease agility
    // but it works well for handling vibrations and unbalanced propellers
    // Set output rate (1): 1000 / (1 + 0) = 1000Hz
    mpu6050SetRate(0);
    mpu6050SetDLPFMode(MPU6050_DLPF_BW_42);
    // 初始化加速计和陀螺二阶低通滤波
    for (uint8_t i = 0; i < 3; i++) {
        lpf2pInit(&gyroLpf[i], 1000, GYRO_LPF_CUTOFF_FREQ);
        lpf2pInit(&accLpf[i], 1000, ACCEL_LPF_CUTOFF_FREQ);
    }
#ifdef SENSORS_ENABLE_MAG_HM5883L
    hmc5883lInit(I2C0_DEV);

    if (hmc5883lTestConnection() == true) {
        isMagnetometerPresent = true;
        hmc5883lSetMode(QMC5883L_MODE_CONTINUOUS); // 16bit 100Hz
        ESP_LOGI(TAG,"hmc5883l I2C connection [OK].\n");
		debugpeintf("hmc5883l I2C connection [OK]\r\n");
    } else {
        ESP_LOGE(TAG,"hmc5883l I2C connection [FAIL].\n");
		debugpeintf("hmc5883l I2C connection [FAIL]\r\n");
    }

#endif
    if (SPL06Init(I2C0_DEV)) {
        isBarometerPresent = true;
        ESP_LOGI(TAG,"SPL06 I2C connection [OK].\n");
		debugpeintf("SPL06 I2C connection [OK]\r\n");
    } else {
        //TODO: Should sensor test fail hard if no connection
       ESP_LOGE(TAG,"SPL06 I2C connection [FAIL].\n");
	   debugpeintf("SPL06 I2C connection [FAIL]\r\n");
	   goto lab1;
    }
}

static void sensorsSetupSlaveRead(void)
{
    // Now begin to set up the slaves
#ifdef SENSORS_MPU6050_DLPF_256HZ
    // As noted in registersheet 4.4: "Data should be sampled at or above sample rate;
    // SMPLRT_DIV is only used for 1kHz internal sampling." Slowest update rate is then 500Hz.
    mpu6050SetSlave4MasterDelay(15); // read slaves at 500Hz = (8000Hz / (1 + 15))
#else
    mpu6050SetSlave4MasterDelay(9); // read slaves at 100Hz = (500Hz / (1 + 4))
#endif

    mpu6050SetI2CBypassEnabled(false);
    mpu6050SetWaitForExternalSensorEnabled(true);     // the slave data isn't so important for the state estimation
    mpu6050SetInterruptMode(0);                       // active high
    mpu6050SetInterruptDrive(0);                      // push pull
    mpu6050SetInterruptLatch(0);                      // latched until clear
    mpu6050SetInterruptLatchClear(1);                 // cleared on any register read
    mpu6050SetSlaveReadWriteTransitionEnabled(false); // Send a stop at the end of a slave read
    mpu6050SetMasterClockSpeed(13);                   // Set i2c speed to 400kHz

#ifdef SENSORS_ENABLE_MAG_HM5883L

    if (isMagnetometerPresent) {
        // Set registers for mpu6050 master to read from
        mpu6050SetSlaveAddress(0, 0x80 | HMC5883L_ADDRESS); 
        mpu6050SetSlaveRegister(0, QMC5883L_RA_STATUS);    
        mpu6050SetSlaveDataLength(0, SENSORS_MAG_STATUS_LEN);
        mpu6050SetSlaveDelayEnabled(0, true);
        mpu6050SetSlaveEnabled(0, true);
		
		mpu6050SetSlaveAddress(1, 0x80 | HMC5883L_ADDRESS);
        mpu6050SetSlaveRegister(1, QMC5883L_RA_DATAX_L);  
        mpu6050SetSlaveDataLength(1, SENSORS_MAG_DATA_LEN);
        mpu6050SetSlaveDelayEnabled(1, true);
        mpu6050SetSlaveEnabled(1, true);
		
        ESP_LOGI(TAG,"mpu6050SetSlaveAddress HMC5883L done \n");
    }

#endif

    if (isBarometerPresent) {
        // Configure the LPS25H as a slave and enable read
        // Setting up two reads works for LPS25H fifo avg filter as well as the
        // auto inc wraps back to LPS25H_PRESS_OUT_L after LPS25H_PRESS_OUT_H is read.
        mpu6050SetSlaveAddress(2, 0x80 | SPL06_I2C_ADDR);
        mpu6050SetSlaveRegister(2, SPL06_MODE_CFG_REG);
        mpu6050SetSlaveDataLength(2, SENSORS_BARO_STATUS_LEN);
        mpu6050SetSlaveDelayEnabled(2, true);
        mpu6050SetSlaveEnabled(2, true);

        mpu6050SetSlaveAddress(3, 0x80 | SPL06_I2C_ADDR); //temperature
        mpu6050SetSlaveRegister(3, SPL06_PRESSURE_MSB_REG);
        mpu6050SetSlaveDataLength(3, SENSORS_BARO_DATA_LEN);
        mpu6050SetSlaveDelayEnabled(3, true);
        mpu6050SetSlaveEnabled(3, true);
		ESP_LOGI(TAG,"mpu6050SetSlaveAddress SPL06 done \n");
    }

    // Enable sensors after configuration
    mpu6050SetI2CMasterModeEnabled(true);

    mpu6050SetIntDataReadyEnabled(true);

    ESP_LOGI(TAG,"sensorsSetupSlaveRead done \n");
}

/*传感器偏置初始化*/
static void sensorsBiasObjInit(BiasObj *bias)
{
    bias->isBufferFilled = false;
    bias->bufHead = bias->buffer;
}

/*计算方差和平均值*/
static void sensorsCalculateVarianceAndMean(BiasObj *bias, Axis3f *varOut, Axis3f *meanOut)
{
    uint32_t i;
    int64_t sum[GYRO_NBR_OF_AXES] = {0};
    int64_t sumSq[GYRO_NBR_OF_AXES] = {0};

    for (i = 0; i < SENSORS_NBR_OF_BIAS_SAMPLES; i++) {
        sum[0] += bias->buffer[i].x;
        sum[1] += bias->buffer[i].y;
        sum[2] += bias->buffer[i].z;
        sumSq[0] += bias->buffer[i].x * bias->buffer[i].x;
        sumSq[1] += bias->buffer[i].y * bias->buffer[i].y;
        sumSq[2] += bias->buffer[i].z * bias->buffer[i].z;
    }

    varOut->x = (sumSq[0] - ((int64_t)sum[0] * sum[0]) / SENSORS_NBR_OF_BIAS_SAMPLES);
    varOut->y = (sumSq[1] - ((int64_t)sum[1] * sum[1]) / SENSORS_NBR_OF_BIAS_SAMPLES);
    varOut->z = (sumSq[2] - ((int64_t)sum[2] * sum[2]) / SENSORS_NBR_OF_BIAS_SAMPLES);

    meanOut->x = (float)sum[0] / SENSORS_NBR_OF_BIAS_SAMPLES;
    meanOut->y = (float)sum[1] / SENSORS_NBR_OF_BIAS_SAMPLES;
    meanOut->z = (float)sum[2] / SENSORS_NBR_OF_BIAS_SAMPLES;
}

/**
 * 往方差缓冲区（循环缓冲区）添加一个新值，缓冲区满后，替换旧的的值
 */
static void sensorsAddBiasValue(BiasObj *bias, int16_t x, int16_t y, int16_t z)
{
    bias->bufHead->x = x;
    bias->bufHead->y = y;
    bias->bufHead->z = z;
    bias->bufHead++;

    if (bias->bufHead >= &bias->buffer[SENSORS_NBR_OF_BIAS_SAMPLES]) {
        bias->bufHead = bias->buffer;
        bias->isBufferFilled = true;
    }
}


// static char peintf_buf[100] = {0};
void setPrintf(uint8_t set)
{
	isprintf = set;
}
static Axis3f readvariance;

void readBiasVlue(Axis3f *variance)
{
	*variance = readvariance;
}
/**
 * Checks if the variances is below the predefined thresholds.
 * The bias value should have been added before calling this.
 * @param bias  The bias object
 */
 /*传感器查找偏置值*/
static bool sensorsFindBiasValue(BiasObj *bias)
{
    static int32_t varianceSampleTime;
    bool foundBias = false;

    if (bias->isBufferFilled) {
        sensorsCalculateVarianceAndMean(bias, &bias->variance, &bias->mean);

        if (bias->variance.x < GYRO_VARIANCE_THRESHOLD_X &&
			bias->variance.y < GYRO_VARIANCE_THRESHOLD_Y &&
			bias->variance.z < GYRO_VARIANCE_THRESHOLD_Z &&
			
			(varianceSampleTime + GYRO_MIN_BIAS_TIMEOUT_MS < xTaskGetTickCount())) {
				
            varianceSampleTime = xTaskGetTickCount();
            bias->bias.x = bias->mean.x;
            bias->bias.y = bias->mean.y;
            bias->bias.z = bias->mean.z;
            foundBias = true;
            bias->isBiasValueFound = true;
			
			isprintf = 0;
        }
		readvariance.x = bias->variance.x;
		readvariance.y = bias->variance.y;
		readvariance.z = bias->variance.z;
		//printf("x:%0.2f,y:%0.2f,z:%0.2f\r\n",bias->variance.x,bias->variance.y,bias->variance.z);
		
		ESP_LOGE(TAG,"x:%0.2f,y:%0.2f,z:%0.2f\r\n",bias->variance.x,bias->variance.y,bias->variance.z);
		
		// if(isprintf)
		// {
			// memset(peintf_buf, '\0', 100);
			// sprintf(peintf_buf,"x:%0.2f,y:%0.2f,z:%0.2f\r\n",bias->variance.x,bias->variance.y,bias->variance.z);
			// debugpeintf(peintf_buf);
		// }
    }

    return foundBias;
}


#ifdef GYRO_BIAS_LIGHT_WEIGHT
//计算陀螺方差
/**
 * Calculates the bias out of the first SENSORS_BIAS_SAMPLES gathered. Requires no buffer
 * but needs platform to be stable during startup.
 */
static bool processGyroBiasNoBuffer(int16_t gx, int16_t gy, int16_t gz, Axis3f *gyroBiasOut)
{
    static uint32_t gyroBiasSampleCount = 0;
    static bool gyroBiasNoBuffFound = false;
    static Axis3i64 gyroBiasSampleSum;
    static Axis3i64 gyroBiasSampleSumSquares;

    if (!gyroBiasNoBuffFound) {
        // If the gyro has not yet been calibrated:
        // Add the current sample to the running mean and variance
        gyroBiasSampleSum.x += gx;
        gyroBiasSampleSum.y += gy;
        gyroBiasSampleSum.z += gz;
#ifdef SENSORS_GYRO_BIAS_CALCULATE_STDDEV
        gyroBiasSampleSumSquares.x += gx * gx;
        gyroBiasSampleSumSquares.y += gy * gy;
        gyroBiasSampleSumSquares.z += gz * gz;
#endif
        gyroBiasSampleCount += 1;

        // If we then have enough samples, calculate the mean and standard deviation
        if (gyroBiasSampleCount == SENSORS_BIAS_SAMPLES) {
            gyroBiasOut->x = (float)(gyroBiasSampleSum.x) / SENSORS_BIAS_SAMPLES;
            gyroBiasOut->y = (float)(gyroBiasSampleSum.y) / SENSORS_BIAS_SAMPLES;
            gyroBiasOut->z = (float)(gyroBiasSampleSum.z) / SENSORS_BIAS_SAMPLES;

#ifdef SENSORS_GYRO_BIAS_CALCULATE_STDDEV
            gyroBiasStdDev.x = sqrtf((float)(gyroBiasSampleSumSquares.x) / SENSORS_BIAS_SAMPLES - (gyroBiasOut->x * gyroBiasOut->x));
            gyroBiasStdDev.y = sqrtf((float)(gyroBiasSampleSumSquares.y) / SENSORS_BIAS_SAMPLES - (gyroBiasOut->y * gyroBiasOut->y));
            gyroBiasStdDev.z = sqrtf((float)(gyroBiasSampleSumSquares.z) / SENSORS_BIAS_SAMPLES - (gyroBiasOut->z * gyroBiasOut->z));
#endif
            gyroBiasNoBuffFound = true;
        }
    }

    return gyroBiasNoBuffFound;
}
#else
/**
 * Calculates the bias first when the gyro variance is below threshold. Requires a buffer
 * but calibrates platform first when it is stable.
 */
static bool processGyroBias(int16_t gx, int16_t gy, int16_t gz, Axis3f *gyroBiasOut)
{
    sensorsAddBiasValue(&gyroBiasRunning, gx, gy, gz);

    if (!gyroBiasRunning.isBiasValueFound) {
        sensorsFindBiasValue(&gyroBiasRunning);

        // if (gyroBiasRunning.isBiasValueFound) {
            // soundSetEffect(SND_CALIB);
            // ledseqRun(&seq_calibrated);
            // DEBUG_PRINTI("isBiasValueFound!");
        // }
    }

    gyroBiasOut->x = gyroBiasRunning.bias.x;
    gyroBiasOut->y = gyroBiasRunning.bias.y;
    gyroBiasOut->z = gyroBiasRunning.bias.z;

    return gyroBiasRunning.isBiasValueFound;
}
#endif

/**
 * Calculates accelerometer scale out of SENSORS_ACC_SCALE_SAMPLES samples. Should be called when
 * platform is stable.
 */
 //根据样本计算重力加速度缩放因子
static bool processAccScale(int16_t ax, int16_t ay, int16_t az)
{
    static bool accBiasFound = false;
    static uint32_t accScaleSumCount = 0;

    if (!accBiasFound) {
        accScaleSum += sqrtf(powf(ax * SENSORS_G_PER_LSB_CFG, 2) + powf(ay * SENSORS_G_PER_LSB_CFG, 2) + powf(az * SENSORS_G_PER_LSB_CFG, 2));
        accScaleSumCount++;

        if (accScaleSumCount == SENSORS_ACC_SCALE_SAMPLES) {
            accScale = accScaleSum / SENSORS_ACC_SCALE_SAMPLES;
            accBiasFound = true;
        }
    }

    return accBiasFound;
}

/**
 * Compensate for a miss-aligned accelerometer. It uses the trim
 * data gathered from the UI and written in the config-block to
 * rotate the accelerometer to be aligned with gravity.
 */
 #if 0
static void sensorsAccAlignToGravity(Axis3f *in, Axis3f *out)
{
    Axis3f rx;
    Axis3f ry;

    // Rotate around x-axis
    rx.x = in->x;
    rx.y = in->y * cosRoll - in->z * sinRoll;
    rx.z = in->y * sinRoll + in->z * cosRoll;

    // Rotate around y-axis
    ry.x = rx.x * cosPitch - rx.z * sinPitch;
    ry.y = rx.y;
    ry.z = -rx.x * sinPitch + rx.z * cosPitch;

    out->x = ry.x;
    out->y = ry.y;
    out->z = ry.z;
}
#endif
static void applyAxis3fLpf(lpf2pData *data, Axis3f *in)
{
    for (uint8_t i = 0; i < 3; i++) {
        in->axis[i] = lpf2pApply(&data[i], in->axis[i]);
    }
}

/*处理加速计和陀螺仪数据*/
void processAccGyroMeasurements(const uint8_t *buffer)
{
    /*  Note the ordering to correct the rotated 90ยบ IMU coordinate system */

    Axis3f accScaled;

#ifdef CONFIG_TARGET_ESPLANE_V1
    /* sensors step 2.1 read from buffer */
    accelRaw.x = (((int16_t)buffer[0]) << 8) | buffer[1];
    accelRaw.y = (((int16_t)buffer[2]) << 8) | buffer[3];
    accelRaw.z = (((int16_t)buffer[4]) << 8) | buffer[5];
    gyroRaw.x = (((int16_t)buffer[8]) << 8) | buffer[9];
    gyroRaw.y = (((int16_t)buffer[10]) << 8) | buffer[11];
    gyroRaw.z = (((int16_t)buffer[12]) << 8) | buffer[13];
#else
    /* sensors step 2.1 read from buffer */
    accelRaw.y = (((int16_t)buffer[0]) << 8) | buffer[1];
    accelRaw.x = (((int16_t)buffer[2]) << 8) | buffer[3];
    accelRaw.z = (((int16_t)buffer[4]) << 8) | buffer[5];
    gyroRaw.y = (((int16_t)buffer[8]) << 8) | buffer[9];
    gyroRaw.x = (((int16_t)buffer[10]) << 8) | buffer[11];
    gyroRaw.z = (((int16_t)buffer[12]) << 8) | buffer[13];
#endif

#ifdef GYRO_BIAS_LIGHT_WEIGHT
    gyroBiasFound = processGyroBiasNoBuffer(gyroRaw.x, gyroRaw.y, gyroRaw.z, &gyroBias);
#else
    /* sensors step 2.2 Calculates the gyro bias first when the  variance is below threshold */
    gyroBiasFound = processGyroBias(gyroRaw.x, gyroRaw.y, gyroRaw.z, &gyroBias);
#endif

    /*sensors step 2.3 Calculates the acc scale when platform is steady */
    if (gyroBiasFound) {
        processAccScale(accelRaw.x, accelRaw.y, accelRaw.z);
    }

    /* sensors step 2.4 convert  digtal value to physical angle */
#ifdef CONFIG_TARGET_ESPLANE_V1
    sensorData.gyro.x = (gyroRaw.x - gyroBias.x) * SENSORS_DEG_PER_LSB_CFG;
#else
    sensorData.gyro.x = -(gyroRaw.x - gyroBias.x) * SENSORS_DEG_PER_LSB_CFG;
#endif

    sensorData.gyro.y = (gyroRaw.y - gyroBias.y) * SENSORS_DEG_PER_LSB_CFG;/*单位 °/s */
    sensorData.gyro.z = (gyroRaw.z - gyroBias.z) * SENSORS_DEG_PER_LSB_CFG;
    /* sensors step 2.5 low pass filter */
    applyAxis3fLpf((lpf2pData *)(&gyroLpf), &sensorData.gyro);

#ifdef CONFIG_TARGET_ESPLANE_V1
    accScaled.x = (accelRaw.x) * SENSORS_G_PER_LSB_CFG / accScale; /*单位 g(9.8m/s^2)*/
#else
    accScaled.x = -(accelRaw.x) * SENSORS_G_PER_LSB_CFG / accScale;   
#endif

    accScaled.y = (accelRaw.y) * SENSORS_G_PER_LSB_CFG / accScale;
    accScaled.z = (accelRaw.z) * SENSORS_G_PER_LSB_CFG / accScale;

    /* sensors step 2.6 Compensate for a miss-aligned accelerometer. */
	#if 0
    sensorsAccAlignToGravity(&accScaled, &sensorData.acc);
	#else
	sensorData.acc.x = accScaled.x;	/*单位 g(9.8m/s^2)*/
	sensorData.acc.y = accScaled.y;	/*重力加速度缩放因子accScale 根据样本计算得出*/
	sensorData.acc.z = accScaled.z;
	#endif
	
    applyAxis3fLpf((lpf2pData *)(&accLpf), &sensorData.acc);
}
/*上位机获取读取原始数据*/
void getSensorRawData(Axis3i16* acc, Axis3i16* gyro, Axis3i16*mag )
{
	*acc = accelRaw;
	*gyro = gyroRaw;
	*mag = magRaw;
}

void getPressureRawData(float* temp, float* press, float*asl )
{
	*temp = TempRaw;
	*press = PressureRaw;
	*asl = AslRaw;
}

/*处理磁力计数据*/
void processMagnetometerMeasurements(const uint8_t *buffer)
{
	#ifdef SENSORS_ENABLE_MAG_HM5883L
    //TODO: replace it to hmc5883l
    if (buffer[0] & QMC5883L_STATUS_DRDY_BIT) {
        int16_t headingx = (((int16_t)buffer[2]) << 8) | buffer[1];
        int16_t headingy = (((int16_t)buffer[4]) << 8) | buffer[3];
        int16_t headingz = (((int16_t)buffer[6]) << 8) | buffer[5];

        sensorData.mag.x = (float)headingx / MAG_GAUSS_PER_LSB; //to gauss
        sensorData.mag.y = (float)headingy / MAG_GAUSS_PER_LSB;
        sensorData.mag.z = (float)headingz / MAG_GAUSS_PER_LSB;
		
		magRaw.x = headingx;/*用于上传到上位机*/
		magRaw.y = headingy;
		magRaw.z = headingz;
		
    }
	#endif
}

/*处理气压计数据*/
void processBarometerMeasurements(const uint8_t *buffer)
{
	static float temp;
	static float pressure;

	// Check if there is a new data update
	if(!isBarometerPresent){
		return;
	}

	int32_t Pressure = (int32_t)buffer[1]<<16 | (int32_t)buffer[2]<<8 | (int32_t)buffer[3];
	Pressure = (Pressure & 0x800000) ? (0xFF000000 | Pressure) : Pressure;

	int32_t rawTemp = (int32_t)buffer[4]<<16 | (int32_t)buffer[5]<<8 | (int32_t)buffer[6];
	rawTemp = (rawTemp & 0x800000) ? (0xFF000000 | rawTemp) : rawTemp;

	temp = spl0601_get_temperature(rawTemp);
	pressure = spl0601_get_pressure(Pressure, rawTemp);
	sensorData.baro.pressure = pressure / 100.0f;
	sensorData.baro.temperature = (float)temp; /*单位度*/
	sensorData.baro.asl = SPL06PressureToAltitude(sensorData.baro.pressure) * 100.f; //cm
	//ESP_LOGI(TAG,"pressure:%f,temperature:%f,asl:%lf",sensorData.baro.pressure,sensorData.baro.temperature,sensorData.baro.asl);
	
	TempRaw = sensorData.baro.temperature;
	PressureRaw = sensorData.baro.pressure;
	AslRaw = sensorData.baro.asl;
}

static void sensorsTask(void *param)
{
    //TODO:
    // systemWaitStart();
    vTaskDelay(M2T(200));

    sensorsSetupSlaveRead(); //
    ESP_LOGI(TAG,"xTaskCreate sensorsTask SetupSlave done\n");

    while (1) {

        /* mpu6050 interrupt trigger: data is ready to be read */
        if (pdTRUE == xSemaphoreTake(sensorsDataReady, portMAX_DELAY)) {
            sensorData.interruptTimestamp = imuIntTimestamp;

            /* sensors step 1-read data from I2C */
            uint8_t dataLen = (uint8_t)(SENSORS_MPU6050_BUFF_LEN +
                                        (isMagnetometerPresent ? SENSORS_MAG_BUFF_LEN : 0) +
                                        (isBarometerPresent ? SENSORS_BARO_BUFF_LEN : 0));
            i2cdevReadReg8(I2C0_DEV, MPU6050_ADDRESS_AD0_LOW, MPU6050_RA_ACCEL_XOUT_H, dataLen, buffer);

            /* sensors step 2-process the respective data */
            processAccGyroMeasurements(&(buffer[0]));

            if (isMagnetometerPresent) {
                processMagnetometerMeasurements(&(buffer[SENSORS_MPU6050_BUFF_LEN]));
            }

            if (isBarometerPresent) {
                processBarometerMeasurements(&(buffer[isMagnetometerPresent ? SENSORS_MPU6050_BUFF_LEN + SENSORS_MAG_BUFF_LEN : SENSORS_MPU6050_BUFF_LEN]));
            }
			
			vTaskSuspendAll();
            /* sensors step 3- queue sensors data  on the output queues */
            xQueueOverwrite(accelerometerDataQueue, &sensorData.acc);
            xQueueOverwrite(gyroDataQueue, &sensorData.gyro);

            if (isMagnetometerPresent) {
                xQueueOverwrite(magnetometerDataQueue, &sensorData.mag);
            }

            if (isBarometerPresent) {
                xQueueOverwrite(barometerDataQueue, &sensorData.baro);
            }
			xTaskResumeAll();
            /* sensors step 4- Unlock stabilizer task */
            xSemaphoreGive(dataReady);
        }
    }
}
TaskHandle_t sensors_handle = NULL;
static void sensorsTaskInit(void)
{
	/*创建传感器数据队列*/
	accelerometerDataQueue = xQueueCreate(1, sizeof(Axis3f));
	gyroDataQueue = xQueueCreate(1, sizeof(Axis3f));
	magnetometerDataQueue = xQueueCreate(1, sizeof(Axis3f));
	barometerDataQueue = xQueueCreate(1, sizeof(baro_t));

	xTaskCreate(sensorsTask, SENSORS_TASK_NAME, SENSORS_TASK_STACKSIZE, NULL, SENSORS_TASK_PRI, &sensors_handle);			/*创建传感器处理任务*/
  // STATIC_MEM_TASK_CREATE(sensorsTask, sensorsTask, SENSORS_TASK_NAME, NULL, SENSORS_TASK_PRI);
	ESP_LOGI(TAG,"xTaskCreate sensorsTask \r\n");
}
/*传感器数据校准*/
bool sensorsAreCalibrated(void)	
{
	return gyroBiasFound;
}

//传感器测试
bool sensorsMpu6050Test(void)
{
    bool testStatus = false;
    Axis3i16 g;
    Axis3i16 a;
    Axis3f acc; // Accelerometer axis data in mG
    float pitch, roll;
    uint32_t startTick = xTaskGetTickCount();

    testStatus = mpu6050SelfTest();

    if (testStatus) {
        sensorsBiasObjInit(&gyroBiasRunning);

        while (xTaskGetTickCount() - startTick < SENSORS_VARIANCE_MAN_TEST_TIMEOUT) {
            mpu6050GetMotion6(&a.y, &a.x, &a.z, &g.y, &g.x, &g.z);

            if (processGyroBias(g.x, g.y, g.z, &gyroBias)) {
                gyroBiasFound = true;
                ESP_LOGI(TAG,"Gyro variance test [OK]\n");
                break;
            }
        }

        if (gyroBiasFound) {
            acc.x = (a.x) * SENSORS_G_PER_LSB_CFG;
            acc.y = (a.y) * SENSORS_G_PER_LSB_CFG;
            acc.z = (a.z) * SENSORS_G_PER_LSB_CFG;

            // Calculate pitch and roll based on accelerometer. Board must be level
            pitch = tanf(-acc.x / (sqrtf(acc.y * acc.y + acc.z * acc.z))) * 180 / (float)M_PI;
            roll = tanf(acc.y / acc.z) * 180 / (float)M_PI;

            if ((fabsf(roll) < SENSORS_MAN_TEST_LEVEL_MAX) && (fabsf(pitch) < SENSORS_MAN_TEST_LEVEL_MAX)) {
                ESP_LOGI(TAG,"Acc level test [OK]\n");
                testStatus = true;
            } else {
                ESP_LOGI(TAG,"Acc level test Roll:%0.2f, Pitch:%0.2f [FAIL]\n", (double)roll, (double)pitch);
                testStatus = false;
            }
        } else {
            ESP_LOGI(TAG,"Gyro variance test [FAIL]\n");
            testStatus = false;
        }
    }

    return testStatus;
}


//初始化传感器
void sensorsMpu6050Spl06Init(void)
{
    if (isInit) {
        return;
    }

    sensorsBiasObjInit(&gyroBiasRunning);
    sensorsDeviceInit();  //mpu初始化
	
	//sensorsMpu6050Test();////

    sensorsInterruptInit();
    sensorsTaskInit(); //传感器任务
    isInit = true;
}
void sensorsI2CdevDeInit(void)
{
	i2cDrvDeInit(I2C0_DEV);
	i2cDrvDeInit(I2C1_DEV);
}

void sensorsMpu6050Spl06DeInit(void)
{
	if(!isInit) return;
	gyroBiasFound = false;
	
	// hmc5883lDeInit();
	mpu6050DeInit();
	setCalibrated(false);
	// SPL06DeInit();
	
	gpio_isr_handler_remove(MICROPY_MPU_PIN_IRQ);
	gpio_uninstall_isr_service();
	gpio_pad_select_gpio(MICROPY_MPU_PIN_IRQ);
	gpio_matrix_out(MICROPY_MPU_PIN_IRQ, SIG_GPIO_OUT_IDX, false, false);
	gpio_set_direction(MICROPY_MPU_PIN_IRQ, GPIO_MODE_INPUT);
	
	if( sensors_handle != NULL )
	{
		vTaskDelete( sensors_handle );
	}
	
	vSemaphoreDelete(dataReady);
	vSemaphoreDelete(sensorsDataReady);

	vQueueDelete(accelerometerDataQueue);
	vQueueDelete(gyroDataQueue);
	vQueueDelete(magnetometerDataQueue);
	vQueueDelete(barometerDataQueue);

	isInit = false;
}


