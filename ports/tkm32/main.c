/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "lib/oofatfs/ff.h"
#include "lib/littlefs/lfs1.h"
#include "lib/littlefs/lfs1_util.h"
#include "lib/littlefs/lfs2.h"
#include "lib/littlefs/lfs2_util.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "extmod/vfs_lfs.h"

#include "shared/runtime/interrupt_char.h"

#include "boardctrl.h"

#include "sdcard.h"
#include "qspi_fun.h"

#include "systick.h"
#include "pendsv.h"
#include "powerctrl.h"
#include "pybthread.h"
#include "gccollect.h"
#include "factoryreset.h"
#include "modmachine.h"
#include "softtimer.h"
#include "modnetwork.h"

#include "uart.h"
#include "timer.h"
#include "led.h"
#include "extint.h"
#include "usrsw.h"
#include "rtc.h"
#include "storage.h"
#include "i2c.h"

#if 1
#include "usb_lib.h"
#include "hw_config.h"
#include "usb_pwr.h"
#include "mass_mal.h"
#include "usb_process.h"
#endif
#include "usb.h" //CDC
#include "usb_cdc_port.h"

#include "lcd43g.h"

#if MICROPY_PY_THREAD
STATIC pyb_thread_t pyb_thread_main;
#endif

#if defined(MICROPY_HW_UART_REPL)
#ifndef MICROPY_HW_UART_REPL_RXBUF
#define MICROPY_HW_UART_REPL_RXBUF (260)
#endif
STATIC pyb_uart_obj_t pyb_uart_repl_obj;
STATIC uint8_t pyb_uart_repl_rxbuf[MICROPY_HW_UART_REPL_RXBUF];
#endif

STATIC int usb_mode = 0;//usb模式，默认为REPL，1为U盘模式
STATIC int usb_mode_last = 0;//usb模式，默认为REPL，1为U盘模式
STATIC bool usb_is_init = 0;//
STATIC bool sdcard_is_flag = 0;//
void flash_error(int n) {
    for (int i = 0; i < n; i++) {
        led_state(PYB_LED_RED, 0);
        led_state(PYB_LED_GREEN, 1);
        mp_hal_delay_ms(250);
        led_state(PYB_LED_RED, 1);
        led_state(PYB_LED_GREEN, 0);
        mp_hal_delay_ms(250);
    }
    led_state(PYB_LED_GREEN, 0);
}


void led_flash(pyb_led_t led , int n)
{
	for(int i = 0;i < n; i++)
	{
		led_state(led, 1);
		mp_hal_delay_ms(200);
		led_state(led, 0);
		mp_hal_delay_ms(200);
	}
}

void NORETURN __fatal_error(const char *msg) {
    for (volatile uint delay = 0; delay < 10000000; delay++) {
    }
    led_state(1, 0);
    led_state(2, 0);
    mp_hal_stdout_tx_strn("\nFATAL ERROR:\n", 14);
    mp_hal_stdout_tx_strn(msg, strlen(msg));
    for (uint i = 0;;) {
        led_toggle(((i++) & 3) + 1);
        for (volatile uint delay = 0; delay < 10000000; delay++) {
        }
        if (i >= 16) {
            // to conserve power
            __WFI();
        }
    }
}

void nlr_jump_fail(void *val) {
    printf("FATAL: uncaught exception %p\n", val);
    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(val));
    __fatal_error("");
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    (void)func;
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("");
}
#endif

STATIC mp_obj_t pyb_main(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	static const mp_arg_t allowed_args[] = {
			{ MP_QSTR_opt, MP_ARG_INT, {.u_int = 0} }
	};

	if (mp_obj_is_str(pos_args[0])) {
			MP_STATE_PORT(pyb_config_main) = pos_args[0];

			// parse args
			mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
			mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
			#if MICROPY_ENABLE_COMPILER
			MP_STATE_VM(mp_optimise_value) = args[0].u_int;
			#endif
	}
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_main_obj, 1, pyb_main);
//---------------------------------------------------------------------

// avoid inlining to avoid stack usage within main()
MP_NOINLINE STATIC bool init_flash_fs(uint reset_mode) {

    if (reset_mode == 3) {
        factory_reset_create_filesystem();
    }
    // Default block device to entire flash storage
    mp_obj_t bdev = MP_OBJ_FROM_PTR(&pyb_flash_obj);

    // Try to mount the flash on "/flash" and chdir to it for the boot-up directory.
    mp_obj_t mount_point = MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash);
    int ret = mp_vfs_mount_and_chdir_protected(bdev, mount_point);

    if (ret == -MP_ENODEV && bdev == MP_OBJ_FROM_PTR(&pyb_flash_obj) && reset_mode != 3) {
        // No filesystem, bdev is still the default (so didn't detect a possibly corrupt littlefs),
        // and didn't already create a filesystem, so try to create a fresh one now.
        ret = factory_reset_create_filesystem();
        if (ret == 0) {
            ret = mp_vfs_mount_and_chdir_protected(bdev, mount_point);
        }
    }
		//ret = mp_vfs_mount_and_chdir_protected(bdev, mount_point);
    if (ret != 0) {
        printf("MPY: can't mount flash\n");
        return false;
    }

    return true;
}

#if MICROPY_HW_SDCARD_MOUNT_AT_BOOT
STATIC bool init_sdcard_fs(void) {
	//mp_hal_pin_config(MICROPY_HW_DCMI_PWDN, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_UP, 0);

	//mp_hal_pin_high(MICROPY_HW_DCMI_PWDN); //sd mode

    bool first_part = true;
    for (int part_num = 1; part_num <= 4; ++part_num) {
        // create vfs object
        fs_user_mount_t *vfs_fat = m_new_obj_maybe(fs_user_mount_t);
        mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
        if (vfs == NULL || vfs_fat == NULL) {
            break;
        }
        vfs_fat->blockdev.flags = MP_BLOCKDEV_FLAG_FREE_OBJ;
        sdcard_init_vfs(vfs_fat, part_num);

        // try to mount the partition
        FRESULT res = f_mount(&vfs_fat->fatfs);

        if (res != FR_OK) {
            // couldn't mount
            m_del_obj(fs_user_mount_t, vfs_fat);
            m_del_obj(mp_vfs_mount_t, vfs);
        } else {
            // mounted via FatFs, now mount the SD partition in the VFS
            if (first_part) {
                // the first available partition is traditionally called "sd" for simplicity
                vfs->str = "/sd";
                vfs->len = 3;
            } else {
                // subsequent partitions are numbered by their index in the partition table
                if (part_num == 2) {
                    vfs->str = "/sd2";
                } else if (part_num == 3) {
                    vfs->str = "/sd3";
                } else {
                    vfs->str = "/sd4";
                }
                vfs->len = 4;
            }
            vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
            vfs->next = NULL;
            for (mp_vfs_mount_t **m = &MP_STATE_VM(vfs_mount_table);; m = &(*m)->next) {
                if (*m == NULL) {
                    *m = vfs;
                    break;
                }
            }

            {
                if (first_part) {
                    // use SD card as current directory
                    MP_STATE_PORT(vfs_cur) = vfs;
                }
            }
            first_part = false;
        }
    }
		
    if (first_part) {
        printf("MPY: can't mount SD card\n");
        return false;
    } else {
        return true;
    }
}
#endif

void tkm32_main(uint32_t reset_mode) {

    // Enable 8-byte stack alignment for IRQ handlers, in accord with EABI
    SCB->CCR |= SCB_CCR_STKALIGN_Msk;

    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);

    // SysTick is needed by HAL_RCC_ClockConfig (called in SystemClock_Config)
    HAL_InitTick(TICK_INT_PRIORITY);

		#if defined(MICROPY_BOARD_EARLY_INIT)
		MICROPY_BOARD_EARLY_INIT();
		#endif
		
		led_init();
		
		#if MICROPY_PY_THREAD
    pyb_thread_init(&pyb_thread_main);
    #endif
    pendsv_init();
		
    #if MICROPY_HW_HAS_SWITCH
    switch_init0();
    #endif
		usb_mode = update_user_mode();
		
    //machine_init();
    #if MICROPY_HW_ENABLE_RTC
    rtc_init(); //RTC init
    #endif

    #if MICROPY_PY_PYB_LEGACY && MICROPY_HW_ENABLE_HW_I2C
    //i2c_init0();
    #endif
    #if MICROPY_HW_ENABLE_SDCARD
    sdcard_init();
		sdcard_is_flag = sdcard_is_present();
    #endif
    storage_init();
		
	  #if defined(MICROPY_HW_UART_REPL)
    // Set up a UART REPL using a statically allocated object
    pyb_uart_repl_obj.base.type = &pyb_uart_type;
    pyb_uart_repl_obj.uart_id = MICROPY_HW_UART_REPL;
    pyb_uart_repl_obj.is_static = true;
    pyb_uart_repl_obj.timeout = 0;
    pyb_uart_repl_obj.timeout_char = 2;
    uart_init(&pyb_uart_repl_obj, MICROPY_HW_UART_REPL_BAUD, UART_WordLength_8b, UART_Parity_No, UART_StopBits_1, 0);
    uart_set_rxbuf(&pyb_uart_repl_obj, sizeof(pyb_uart_repl_rxbuf), pyb_uart_repl_rxbuf);
    //uart_attach_to_repl(&pyb_uart_repl_obj, true);
		uart_attach_to_repl(&pyb_uart_repl_obj, 0);
    MP_STATE_PORT(pyb_uart_obj_all)[MICROPY_HW_UART_REPL - 1] = &pyb_uart_repl_obj;
    #endif

    boardctrl_state_t state;
    state.reset_mode = reset_mode;
    state.log_soft_reset = false;

		MICROPY_BOARD_BEFORE_SOFT_RESET_LOOP(&state);

soft_reset:

		MICROPY_BOARD_TOP_SOFT_RESET_LOOP(&state);
		
    // Python threading init
    #if MICROPY_PY_THREAD
    mp_thread_init();
    #endif

    // Stack limit should be less than real stack size, so we have a chance
    // to recover from limit hit.  (Limit is measured in bytes.)
    // Note: stack control relies on main thread being initialised above
    mp_stack_set_top(&_estack);
    mp_stack_set_limit((char *)&_estack - (char *)&_sstack - 1024);

    // GC init
    gc_init(MICROPY_HEAP_START, MICROPY_HEAP_END);

    #if MICROPY_ENABLE_PYSTACK
    static mp_obj_t pystack[384];
    mp_pystack_init(pystack, &pystack[384]);
    #endif

    // MicroPython init
    mp_init();
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

    #if defined(MICROPY_HW_UART_REPL)
    MP_STATE_PORT(pyb_stdio_uart) = &pyb_uart_repl_obj;
    #else
    MP_STATE_PORT(pyb_stdio_uart) = NULL;
    #endif

    readline_init0();
    pin_init0();
    extint_init0();
    timer_init0();

/********************************************************************************************************/
		bool mounted_flash = false;
/********************************************************************************************************/

    // Initialise the local flash filesystem.
    // Create it if needed, mount in on /flash, and set it as current dir.
    //bool mounted_flash = false;
    mounted_flash = init_flash_fs(state.reset_mode);

    bool mounted_sdcard = false;
    #if MICROPY_HW_SDCARD_MOUNT_AT_BOOT
    // if an SD card is present then mount it on /sd/
    if (sdcard_is_present()) {
        // if there is a file in the flash called "SKIPSD", then we don't mount the SD card
        if (!mounted_flash || mp_vfs_import_stat("SKIPSD") == MP_IMPORT_STAT_NO_EXIST) {
            mounted_sdcard = init_sdcard_fs();
        }
    }
    #endif

    // set sys.path based on mounted filesystems (/sd is first so it can override /flash)
    if (mounted_sdcard) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_sd_slash_lib));
    }
    if (mounted_flash) {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));
    }
//================================================================================================
#if 1
	if(!usb_is_init)
	{
		USB_IO_Config();
		USB_Interrupts_Config();//配置USB中断
		#if 0
		if(usb_mode && sdcard_is_flag){
		#else
		if(usb_mode){
		#endif
			MAL_Init(0);
			USB_Init();//USB初始化
		}else{
			USB_CDC_Init();
		}	
		usb_is_init = 1;
	}
	
	if(usb_mode_last != usb_mode)
	{
		#if 0
		if(usb_mode && sdcard_is_flag){
		#else
		if(usb_mode){
		#endif
			MAL_Init(0);
			USB_Init();//USB初始化
		#if 0
		}else if(!usb_mode){
		#else
		}else{
		#endif
			USB_CDC_Init();
		}	
	}
	//if(!usb_mode || !sdcard_is_flag){
	if(!usb_mode){
		usb_vcp_init0();
	}

	usb_mode_last = usb_mode;
#endif
//================================================================================================
    // reset config variables; they should be set by boot.py
    MP_STATE_PORT(pyb_config_main) = MP_OBJ_NULL;

    // Run boot.py (or whatever else a board configures at this stage).
    if (MICROPY_BOARD_RUN_BOOT_PY(&state) == BOARDCTRL_GOTO_SOFT_RESET_EXIT) {
        goto soft_reset_exit;
    }

    // Now we initialise sub-systems that need configuration from boot.py,
    // or whose initialisation can be safely deferred until after running
    // boot.py.

    #if MICROPY_HW_HAS_MMA7660
    // MMA accel: init and reset
    accel_init();
    #endif

    #if MICROPY_HW_ENABLE_SERVO
    servo_init();
    #endif

    #if MICROPY_PY_NETWORK
    mod_network_init();
    #endif

    // At this point everything is fully configured and initialised.

    // Run main.py (or whatever else a board configures at this stage).
    if (MICROPY_BOARD_RUN_MAIN_PY(&state) == BOARDCTRL_GOTO_SOFT_RESET_EXIT) {
        goto soft_reset_exit;
    }
//----------------------------------------------------------------------
	#if MICROPY_ENABLE_COMPILER
	// Main script is finished, so now go into REPL mode.
	// The REPL mode can change, or it can request a soft reset.
	for (;;) {
			if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
					if (pyexec_raw_repl() != 0) {
							break;
					}
			} else {
					if (pyexec_friendly_repl() != 0) {
							break;
					}
			}
	}
	#endif

soft_reset_exit:

    // soft reset
		MICROPY_BOARD_START_SOFT_RESET(&state);
		
		#if MICROPY_HW_ENABLE_STORAGE
    if (state.log_soft_reset) {
        mp_printf(&mp_plat_print, "MPY: sync filesystems\n");
    }
    storage_flush();
    #endif
		
    if (state.log_soft_reset) {
        mp_printf(&mp_plat_print, "MPY: soft reboot\n");
    }
		
    #if MICROPY_PY_BLUETOOTH
    mp_bluetooth_deinit();
    #endif
    #if MICROPY_PY_NETWORK
    mod_network_deinit();
    #endif
    soft_timer_deinit();
    timer_deinit();
    uart_deinit_all();
    #if MICROPY_HW_ENABLE_CAN
    can_deinit_all();
    #endif
    machine_deinit();

    #if MICROPY_PY_THREAD
    pyb_thread_deinit();
    #endif
		
		MICROPY_BOARD_END_SOFT_RESET(&state);

    gc_sweep_all();

    goto soft_reset;
}

void USB_IRQHandler(void)
{
	IRQ_ENTER(USB_IRQn)
	#if 0
	if(usb_mode && sdcard_is_flag){
	#else
	if(usb_mode){
	#endif
		 USB_Istr();
	}else{
		USB_CDC_IRQHandler();
	}
	IRQ_EXIT(USB_IRQn);
}

