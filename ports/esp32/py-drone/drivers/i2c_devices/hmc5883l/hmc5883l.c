// I2Cdev library collection - HMC5883L I2C device class
// Based on Honeywell HMC5883L datasheet, 10/2010 (Form #900405 Rev B)
// 6/12/2012 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/i2cdevlib

/* ============================================
 I2Cdev device library code is placed under the MIT license
 Copyright (c) 2012 Jeff Rowberg
 Adapted to Crazyflie FW by Bitcraze

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ===============================================
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"
#include "i2cdev.h"
#include "mpu6050.h"
#include "hmc5883l.h"
#include "py/obj.h"
#include "esp_log.h"
#define TAG "HMC5883L"

static uint8_t devAddr;
static uint8_t buffer[6];
static uint8_t mode;
static I2C_Dev *I2Cx;
static bool isInit;

/** Power on and prepare for general usage.
 * This will prepare the magnetometer with default settings, ready for single-
 * use mode (very low power requirements). Default settings include 8-sample
 * averaging, 15 Hz data output rate, normal measurement bias, a,d 1090 gain (in
 * terms of LSB/Gauss). Be sure to adjust any settings you need specifically
 * after initialization, especially the gain settings if you happen to be seeing
 * a lot of -4096 values (see the datasheet for mor information).
 */
void hmc5883lInit(I2C_Dev *i2cPort)
{
    if (isInit) {
        return;
    }

    I2Cx = i2cPort;
    devAddr = HMC5883L_ADDRESS;

	i2cdevWriteByte(I2Cx, devAddr, QMC5883L_RA_CONFIG_2,0x00);
	vTaskDelay(100 / portTICK_PERIOD_MS);

	i2cdevWriteByte(I2Cx, devAddr, QMC5883L_RA_MODE,0x01);
	i2cdevWriteByte(I2Cx, devAddr, QMC5883L_RA_CONFIG_2,0x40);
    isInit = true;
}
void hmc5883lDeInit(void)
{
	isInit = false;
}
/** Verify the I2C connection.
 * Make sure the device is connected and responds as expected.
 * @return True if connection is valid, false otherwise
 */
bool hmc5883lTestConnection()
{
	uint8_t read_id = 0;

    if (i2cdevReadByte(I2Cx, devAddr, QMC5883L_CHIP_ID, &read_id)) {
		ESP_LOGI(TAG,"MC5883 ID IS: 0x%X\n",read_id);
        return (read_id == HMC5883L_DEFAULT_ADDRESS);
    }
    return false;
}

/** Do a self test.
 * @return True if self test passed, false otherwise
 */
bool hmc5883lSelfTest()
{
    bool testStatus = true;
    return testStatus;
}

/** Evaluate the values from a HMC8335L self test.
 * @param min The min limit of the self test
 * @param max The max limit of the self test
 * @param value The value to compare with.
 * @param string A pointer to a string describing the value.
 * @return True if self test within min - max limit, false otherwise
 */
bool hmc5883lEvaluateSelfTest(int16_t min, int16_t max, int16_t value, char *string)
{
    if (value < min || value > max) {
        ESP_LOGI(TAG,"Self test %s [FAIL]. low: %d, high: %d, measured: %d\n",
                     string, min, max, value);
        return false;
    }

    return true;
}

/** Get data output rate value.
 * The Table below shows all selectable output rates in continuous measurement
 * mode. All three channels shall be measured within a given output rate. Other
 * output rates with maximum rate of 160 Hz can be achieved by monitoring DRDY
 * interrupt pin in single measurement mode.
 *
 * Value | Typical Data Output Rate (Hz)
 * ------+------------------------------
 * 0     | 0.75
 * 1     | 1.5
 * 2     | 3
 * 3     | 7.5
 * 4     | 15 (Default)
 * 5     | 30
 * 6     | 75
 * 7     | Not used
 *
 * @return Current rate of data output to registers
 * @see HMC5883L_RATE_15
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_RATE_BIT
 * @see HMC5883L_CRA_RATE_LENGTH
 */
uint8_t hmc5883lGetDataRate()
{
    i2cdevReadBits(I2Cx, devAddr, HMC5883L_RA_CONFIG_A, HMC5883L_CRA_RATE_BIT, HMC5883L_CRA_RATE_LENGTH, buffer);
    return buffer[0];
}
/** Set data output rate value.
 * @param rate Rate of data output to registers
 * @see getDataRate()
 * @see HMC5883L_RATE_15
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_RATE_BIT
 * @see HMC5883L_CRA_RATE_LENGTH
 */
void hmc5883lSetDataRate(uint8_t rate)
{
    i2cdevWriteBits(I2Cx, devAddr, HMC5883L_RA_CONFIG_A, HMC5883L_CRA_RATE_BIT, HMC5883L_CRA_RATE_LENGTH, rate);
}
/** Get measurement bias value.
 * @return Current bias value (0-2 for normal/positive/negative respectively)
 * @see HMC5883L_BIAS_NORMAL
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_BIAS_BIT
 * @see HMC5883L_CRA_BIAS_LENGTH
 */
uint8_t hmc5883lGetMeasurementBias()
{
    i2cdevReadBits(I2Cx, devAddr, HMC5883L_RA_CONFIG_A, HMC5883L_CRA_BIAS_BIT, HMC5883L_CRA_BIAS_LENGTH, buffer);
    return buffer[0];
}
/** Set measurement bias value.
 * @param bias New bias value (0-2 for normal/positive/negative respectively)
 * @see HMC5883L_BIAS_NORMAL
 * @see HMC5883L_RA_CONFIG_A
 * @see HMC5883L_CRA_BIAS_BIT
 * @see HMC5883L_CRA_BIAS_LENGTH
 */
void hmc5883lSetMeasurementBias(uint8_t bias)
{
    i2cdevWriteBits(I2Cx, devAddr, HMC5883L_RA_CONFIG_A, HMC5883L_CRA_BIAS_BIT, HMC5883L_CRA_BIAS_LENGTH, bias);
}



// MODE register

/** Get measurement mode.
 * In continuous-measurement mode, the device continuously performs measurements
 * and places the result in the data register. RDY goes high when new data is
 * placed in all three registers. After a power-on or a write to the mode or
 * configuration register, the first measurement set is available from all three
 * data output registers after a period of 2/fDO and subsequent measurements are
 * available at a frequency of fDO, where fDO is the frequency of data output.
 *
 * When single-measurement mode (default) is selected, device performs a single
 * measurement, sets RDY high and returned to idle mode. Mode register returns
 * to idle mode bit values. The measurement remains in the data output register
 * and RDY remains high until the data output register is read or another
 * measurement is performed.
 *
 * @return Current measurement mode
 * @see HMC5883L_MODE_CONTINUOUS
 * @see HMC5883L_MODE_SINGLE
 * @see HMC5883L_MODE_IDLE
 * @see HMC5883L_RA_MODE
 * @see HMC5883L_MODEREG_BIT
 * @see HMC5883L_MODEREG_LENGTH
 */
uint8_t hmc5883lGetMode()
{
    i2cdevReadBits(I2Cx, devAddr, HMC5883L_RA_MODE, HMC5883L_MODEREG_BIT, HMC5883L_MODEREG_LENGTH, buffer);
    return buffer[0];
}
/** Set measurement mode.
 * @param newMode New measurement mode
 * @see getMode()
 * @see HMC5883L_MODE_CONTINUOUS
 * @see HMC5883L_MODE_SINGLE
 * @see HMC5883L_MODE_IDLE
 * @see HMC5883L_RA_MODE
 * @see HMC5883L_MODEREG_BIT
 * @see HMC5883L_MODEREG_LENGTH
 */
void hmc5883lSetMode(uint8_t newMode)
{
    // use this method to guarantee that bits 7-2 are set to zero, which is a
    // requirement specified in the datasheet; it's actually more efficient than
    // using the I2Cdev.writeBits method
	uint8_t tempMode = (QMC5883L_OUTPUT_10HZ | QMC5883L_OUTPUT_2G | QMC5883L_SAMPLE_128);
	
    i2cdevWriteByte(I2Cx, devAddr, QMC5883L_RA_CONFIG_1, newMode | tempMode);
	
    mode = newMode;// track to tell if we have to clear bit 7 after a read
}

// DATA* registers

/** Get 3-axis heading measurements.
 * In the event the ADC reading overflows or underflows for the given channel,
 * or if there is a math overflow during the bias measurement, this data
 * register will contain the value -4096. This register value will clear when
 * after the next valid measurement is made. Note that this method automatically
 * clears the appropriate bit in the MODE register if Single mode is active.
 * @param x 16-bit signed integer container for X-axis heading
 * @param y 16-bit signed integer container for Y-axis heading
 * @param z 16-bit signed integer container for Z-axis heading
 * @see HMC5883L_RA_DATAX_H
 */
void hmc5883lGetHeading(int16_t *x, int16_t *y, int16_t *z)
{
    i2cdevReadReg8(I2Cx, devAddr, HMC5883L_RA_DATAX_H, 6, buffer);

    if (mode == HMC5883L_MODE_SINGLE) {
        i2cdevWriteByte(I2Cx, devAddr, HMC5883L_RA_MODE, HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1));
    }

    *x = (((int16_t)buffer[0]) << 8) | buffer[1];
    *y = (((int16_t)buffer[4]) << 8) | buffer[5];
    *z = (((int16_t)buffer[2]) << 8) | buffer[3];
}
/** Get X-axis heading measurement.
 * @return 16-bit signed integer with X-axis heading
 * @see HMC5883L_RA_DATAX_H
 */
int16_t hmc5883lGetHeadingX()
{
    // each axis read requires that ALL axis registers be read, even if only
    // one is used; this was not done ineffiently in the code by accident
    i2cdevReadReg8(I2Cx, devAddr, HMC5883L_RA_DATAX_H, 6, buffer);

    if (mode == HMC5883L_MODE_SINGLE) {
        i2cdevWriteByte(I2Cx, devAddr, HMC5883L_RA_MODE, HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1));
    }

    return (((int16_t)buffer[0]) << 8) | buffer[1];
}
/** Get Y-axis heading measurement.
 * @return 16-bit signed integer with Y-axis heading
 * @see HMC5883L_RA_DATAY_H
 */
int16_t hmc5883lGetHeadingY()
{
    // each axis read requires that ALL axis registers be read, even if only
    // one is used; this was not done ineffiently in the code by accident
    i2cdevReadReg8(I2Cx, devAddr, HMC5883L_RA_DATAX_H, 6, buffer);

    if (mode == HMC5883L_MODE_SINGLE) {
        i2cdevWriteByte(I2Cx, devAddr, HMC5883L_RA_MODE, HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1));
    }

    return (((int16_t)buffer[4]) << 8) | buffer[5];
}
/** Get Z-axis heading measurement.
 * @return 16-bit signed integer with Z-axis heading
 * @see HMC5883L_RA_DATAZ_H
 */
int16_t hmc5883lGetHeadingZ()
{
    // each axis read requires that ALL axis registers be read, even if only
    // one is used; this was not done ineffiently in the code by accident
    i2cdevReadReg8(I2Cx, devAddr, HMC5883L_RA_DATAX_H, 6, buffer);

    if (mode == HMC5883L_MODE_SINGLE) {
        i2cdevWriteByte(I2Cx, devAddr, HMC5883L_RA_MODE, HMC5883L_MODE_SINGLE << (HMC5883L_MODEREG_BIT - HMC5883L_MODEREG_LENGTH + 1));
    }

    return (((int16_t)buffer[2]) << 8) | buffer[3];
}

// STATUS register

/** Get data output register lock status.
 * This bit is set when this some but not all for of the six data output
 * registers have been read. When this bit is set, the six data output registers
 * are locked and any new data will not be placed in these register until one of
 * three conditions are met: one, all six bytes have been read or the mode
 * changed, two, the mode is changed, or three, the measurement configuration is
 * changed.
 * @return Data output register lock status
 * @see HMC5883L_RA_STATUS
 * @see HMC5883L_STATUS_LOCK_BIT
 */
bool hmc5883lGetLockStatus()
{
    i2cdevReadBit(I2Cx, devAddr, HMC5883L_RA_STATUS, HMC5883L_STATUS_LOCK_BIT, buffer);
    return buffer[0];
}
/** Get data ready status.
 * This bit is set when data is written to all six data registers, and cleared
 * when the device initiates a write to the data output registers and after one
 * or more of the data output registers are written to. When RDY bit is clear it
 * shall remain cleared for 250 us. DRDY pin can be used as an alternative to
 * the status register for monitoring the device for measurement data.
 * @return Data ready status
 * @see HMC5883L_RA_STATUS
 * @see HMC5883L_STATUS_READY_BIT
 */
bool hmc5883lGetReadyStatus()
{
    i2cdevReadBit(I2Cx, devAddr, HMC5883L_RA_STATUS, HMC5883L_STATUS_READY_BIT, buffer);
    return buffer[0];
}

