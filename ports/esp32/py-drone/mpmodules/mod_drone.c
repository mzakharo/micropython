/********************************************************************************
	* Copyright (C), 2022 -2023, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	mod_drone.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2022/4/21
	* Description			:	
******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"

#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "py/objstr.h"
#include "py/objlist.h"

#include "py/mperrno.h" // used by mp_is_nonblocking_error
#include "py/nlr.h"
#include "py/gc.h"

#include "mod_drone.h"
#include "i2cdev.h"
#include "mpu6050.h"

#include "commander.h"
#include "system_int.h"
#include "stabilizer.h"

#include "state_estimator.h"
#include "pm_esplane.h"
#include "ledseq.h"
#include "sensors_mpu6050_spl06.h"

static bool isOffse = 0;
static ctrlVal_t wifiCtrl;

static float limit(float value,float min, float max)
{
	if(value > max)
	{
		value = max;
	}
	else if(value < min)
	{
		value = min;
	}
	return value;
}

//===================================================================================================================
typedef struct _drone_obj_t {
  mp_obj_base_t base;
} drone_obj_t;

STATIC bool is_init = 0;

//====================================================================================================================

// STATIC mp_obj_t drone_deinit(mp_obj_t self_in) {
	// systemDeInit();
	// is_init = false;
	// return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_1(drone_deinit_obj, drone_deinit);

//==============================================================================================================
STATIC mp_obj_t drone_stop(mp_obj_t self_in) {

	setCommanderEmerStop(true);
	setCommanderKeyFlight(false);
	setCommanderKeyland(false);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(drone_stop_obj, drone_stop);
//==============================================================================================================
STATIC mp_obj_t drone_landing(mp_obj_t self_in) {

	setCommanderKeyFlight(false);
	setCommanderKeyland(true);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(drone_landing_obj, drone_landing);
//==============================================================================================================
STATIC mp_obj_t drone_take_off(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t drone_take_args[] = {
		{ MP_QSTR_distance,	MP_ARG_INT, {.u_int = 80} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(drone_take_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drone_take_args), drone_take_args, args);

	setLandingDis((float)args[0].u_int);

	if(getCommanderKeyFlight() != true )
	{
		if(!isOffse){
			attitude_t attitude;
			getAttitudeData(&attitude);

			wifiCtrl.trimPitch = (attitude.pitch*0.3);
			wifiCtrl.trimRoll = (attitude.roll*0.95);
			
			isOffse = 1;
		}
		setCommanderCtrlMode(1);
		setCommanderKeyFlight(true);
		setCommanderKeyland(false);
	}

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(drone_take_off_obj, 0, drone_take_off);
//==============================================================================================================
STATIC mp_obj_t drone_trim(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t drone_take_args[] = {
		{ MP_QSTR_rol,	MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_pit,	MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(drone_take_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drone_take_args), drone_take_args, args);
	
	float rol = ((float)args[0].u_int)/100.0f;
	float pit = ((float)args[0].u_int)/100.0f;

	wifiCtrl.trimRoll += limit(rol,-10.0, 10.0);
	wifiCtrl.trimPitch += limit(pit,-10.0, 10.0);
	
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(drone_trim_obj, 0, drone_trim);
//==============================================================================================================

static uint16_t lastThrust;
STATIC mp_obj_t drone_control(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    static const mp_arg_t drone_take_args[] = {
		{ MP_QSTR_rol,	MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_pit,	MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_yaw,	MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_thr,	MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(drone_take_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(drone_take_args), drone_take_args, args);

	float fTemp = 0.0f;
	
	fTemp = ((float)(args[0].u_int)/10.0f);
	wifiCtrl.roll = limit(fTemp,-10, 10);
	
	// wifiCtrl.roll += wifiCtrl.trimRoll;
	
	fTemp = ((float)(args[1].u_int)/10.0f);
	wifiCtrl.pitch = limit(fTemp,-10, 10);
	
	// wifiCtrl.pitch -= wifiCtrl.trimPitch;
	
	fTemp = ((float)(args[2].u_int*2));
	wifiCtrl.yaw = limit(fTemp,-200, 200);
	
	fTemp = ((float)(args[3].u_int + 100)/2.0f);
	fTemp = limit(fTemp,3, 100);
	wifiCtrl.thrust = (uint16_t)(fTemp* 655.35f);

	if((wifiCtrl.thrust ==32768) && lastThrust<10000)/*手动飞切换到定高*/
	{
		setCommanderCtrlMode(1);
		setCommanderKeyFlight(false);
		setCommanderKeyland(false);
	}
	else if(wifiCtrl.thrust==0 && lastThrust>256)/*定高切换成手动飞*/
	{
		setCommanderCtrlMode(0);
		wifiCtrl.thrust = 0;
	}
	lastThrust = wifiCtrl.thrust;
	
	flightCtrldataCache(WIFI, wifiCtrl);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(drone_control_obj, 1, drone_control);
//==============================================================================================================

STATIC mp_obj_t read_states(mp_obj_t self_in)
{
	mp_obj_t tuple[9];
	
	attitude_t attitude;
	getAttitudeData(&attitude);
	uint16_t bat = (uint16_t)(pmMeasureExtBatteryVoltage()*100.0f);
	int32_t FusedHeight =(int32_t) (getFusedHeight());

	tuple[0] = mp_obj_new_int(attitude.roll*100); 
	tuple[1] = mp_obj_new_int(attitude.pitch*100);
	tuple[2] = mp_obj_new_int(attitude.yaw*100);
	tuple[3] = mp_obj_new_int(wifiCtrl.roll*100);
	tuple[4] = mp_obj_new_int(wifiCtrl.pitch*100);
	tuple[5] = mp_obj_new_int(wifiCtrl.yaw*100);
	tuple[6] = mp_obj_new_int(((wifiCtrl.thrust/655.35)+0.5));
	tuple[7] = mp_obj_new_int(bat);
	tuple[8] = mp_obj_new_int(FusedHeight);
	
	return mp_obj_new_tuple(9, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(read_states_obj, read_states);

//----------------------------------------------------------------------------------
STATIC mp_obj_t read_accelerometer(mp_obj_t self_in)
{
	Axis3i16 acc,gyro,mag;
	mp_obj_t tuple[6];
	getSensorRawData(&acc, &gyro,&mag);
	
	tuple[0] = mp_obj_new_int(gyro.y); 
	tuple[1] = mp_obj_new_int(gyro.z);
	tuple[2] = mp_obj_new_int(gyro.z);
	tuple[3] = mp_obj_new_int(acc.y);
	tuple[4] = mp_obj_new_int(acc.x);
	tuple[5] = mp_obj_new_int(acc.z);

	return mp_obj_new_tuple(6, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(read_accelerometer_obj, read_accelerometer);

//----------------------------------------------------------------------------------
STATIC mp_obj_t read_compass(mp_obj_t self_in)
{
	Axis3i16 acc,gyro,mag;
	mp_obj_t tuple[3];
	getSensorRawData(&acc, &gyro,&mag);

	tuple[0] = mp_obj_new_int(mag.x); 
	tuple[1] = mp_obj_new_int(mag.y);
	tuple[2] = mp_obj_new_int(mag.z);

	return mp_obj_new_tuple(3, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(read_compass_obj, read_compass);

STATIC mp_obj_t read_calibrated(mp_obj_t self_in)
{
	return mp_obj_new_int(sensorsAreCalibrated());
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(read_calibrated_obj, read_calibrated);
//----------------------------------------------------------------------------------
STATIC mp_obj_t read_air_pressure(mp_obj_t self_in)
{
	float temp,press,asl;
	mp_obj_t tuple[2];
	getPressureRawData(&temp, &press, &asl );
	
	tuple[0] = mp_obj_new_int(press*100);
	tuple[1] = mp_obj_new_int((int16_t)(temp*100)); 

	return mp_obj_new_tuple(2, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(read_air_pressure_obj, read_air_pressure);
//----------------------------------------------------------------------------------
STATIC mp_obj_t read_cal_data(mp_obj_t self_in)
{
	Axis3f variance;
	mp_obj_t tuple[3];
	readBiasVlue(&variance);

	tuple[0] = mp_obj_new_int(variance.x);
	tuple[1] = mp_obj_new_int(variance.y);
	tuple[2] = mp_obj_new_int(variance.z);

	return mp_obj_new_tuple(3, tuple);
}STATIC MP_DEFINE_CONST_FUN_OBJ_1(read_cal_data_obj, read_cal_data);

//----------------------------------------------------------------------------------
static void InitDrone(void)
{
	static uint16_t lastThrust;
	
	if(is_init ) return;
	systemInit();

	wifiCtrl.roll   = 0;	/*roll: ±9.5 ±19.2 ±31.7*/
	wifiCtrl.pitch  = 0;	/*pitch:±9.5 ±19.2 ±31.7*/
	wifiCtrl.yaw    = 0;	/*yaw : ±203.2*/				
	wifiCtrl.thrust = 32768;					/*thrust :0~63356*/

	if(wifiCtrl.thrust==32768 && lastThrust<10000)/*手动飞切换到定高*/
	{
		setCommanderCtrlMode(1);
		setCommanderKeyFlight(false);
		setCommanderKeyland(false);
	}
	else if(wifiCtrl.thrust==0 && lastThrust>256)/*定高切换成手动飞*/
	{
		setCommanderCtrlMode(0);
		wifiCtrl.thrust = 0;
	}
	lastThrust = wifiCtrl.thrust;

	flightCtrldataCache(WIFI, wifiCtrl);
}
STATIC mp_obj_t drone_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_flightmode, MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_debug, MP_ARG_INT, {.u_int = 0} },
	};

	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
	mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	
	if(args[0].u_int == 1){
		setCommanderFlightmode(0);
	}else{
		setCommanderFlightmode(1);
	}
	
	setPrintf(args[1].u_int);

	InitDrone();

	drone_obj_t *drone_type;
	drone_type = m_new_obj(drone_obj_t);
	drone_type->base.type = &drone_drone_type;
	is_init = true;
	return MP_OBJ_FROM_PTR(drone_type);
}
/******************************************************************************/
STATIC const mp_rom_map_elem_t drone_locals_dict_table[] = {
	{ MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_DRONE) },
	//{ MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&drone_deinit_obj) },
	{ MP_ROM_QSTR(MP_QSTR_stop), MP_ROM_PTR(&drone_stop_obj) },
	{ MP_ROM_QSTR(MP_QSTR_landing), MP_ROM_PTR(&drone_landing_obj) },
	{ MP_ROM_QSTR(MP_QSTR_take_off), MP_ROM_PTR(&drone_take_off_obj) },
	{ MP_ROM_QSTR(MP_QSTR_control), MP_ROM_PTR(&drone_control_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_states), MP_ROM_PTR(&read_states_obj) },
	{ MP_ROM_QSTR(MP_QSTR_trim), MP_ROM_PTR(&drone_trim_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_accelerometer), MP_ROM_PTR(&read_accelerometer_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_compass), MP_ROM_PTR(&read_compass_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_air_pressure), MP_ROM_PTR(&read_air_pressure_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_calibrated), MP_ROM_PTR(&read_calibrated_obj) },
	{ MP_ROM_QSTR(MP_QSTR_read_cal_data), MP_ROM_PTR(&read_cal_data_obj) },

};
STATIC MP_DEFINE_CONST_DICT(drone_drone_locals_dict,drone_locals_dict_table);

const mp_obj_type_t drone_drone_type = {
    { &mp_type_type },
    .name = MP_QSTR_drone,
    .make_new = drone_make_new,
    .locals_dict = (mp_obj_dict_t *)&drone_drone_locals_dict,
};

