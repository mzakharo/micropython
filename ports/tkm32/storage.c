/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2018 Damien P. George
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

#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "extmod/vfs_fat.h"

#include "systick.h"
#include "led.h"
#include "storage.h"
#include "irq.h"
#include "qspi_fun.h"
#include "stdio.h"


//#if MICROPY_HW_ENABLE_STORAGE
#if 1
static bool storage_is_initialised = false;
void storage_init(void) {
	    if (!storage_is_initialised) {
        //storage_is_initialised = true;
				qspi_bdev_ioctl(BDEV_IOCTL_INIT, 0);
			}

}

//获取块大小
uint32_t storage_get_block_size(void) {
	
    return QFLASH_BLOCK_SIZE;
}
//获取块总数量
uint32_t storage_get_block_count(void) {
	
    return QFLASH_IOCTL_NUM_BLOCKS;
}

void storage_flush(void) {
  qspi_bdev_ioctl(BDEV_IOCTL_SYNC, 0);
}
//block_num 扇区，num_blocks 扇区数
int storage_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {

	//if(!num_blocks) return 1; // error
	led_state(PYB_LED_RED, 1); // indicate a dirty cache with LED on
	
	#if 1
	for(;num_blocks > 0; num_blocks--){										  
		w25qxx_read(dest, QFLASH_BASE_ADDR + (block_num*FLASH_BLOCK_SIZE),FLASH_BLOCK_SIZE); 
		block_num++;
		dest+=FLASH_BLOCK_SIZE;	
	}
	#else

	for(;num_blocks > 0; num_blocks--){										  

		QspiFlashRead(QFLASH_BASE_ADDR + (block_num*FLASH_BLOCK_SIZE),dest,FLASH_BLOCK_SIZE);
		block_num++;
		dest+=FLASH_BLOCK_SIZE;	
	}
	#endif
	led_state(PYB_LED_RED, 0);
	
	return 0; // success
}

int storage_write_blocks(uint8_t *src, uint32_t block_num, uint32_t num_blocks) {

//	if(!num_blocks) return 1; // error

	led_state(PYB_LED_RED, 1); // indicate a dirty cache with LED on

	#if 1
	for(;num_blocks > 0; num_blocks--){										  
		w25qxx_write(src, QFLASH_BASE_ADDR + (block_num*FLASH_BLOCK_SIZE),FLASH_BLOCK_SIZE); 
		block_num++;
		src+=FLASH_BLOCK_SIZE;	
	}
	#else
		
	for(;num_blocks > 0; num_blocks--){										  
		QFLASH_Check_FunProgram(QFLASH_BASE_ADDR + (block_num*FLASH_BLOCK_SIZE),src,FLASH_BLOCK_SIZE);
		block_num++;
		src+=FLASH_BLOCK_SIZE;	
	}
	
	#endif

	led_state(PYB_LED_RED, 0); 

	return 0; // success
}

/******************************************************************************/
// MicroPython bindings
typedef struct _pyb_flash_obj_t {
    mp_obj_base_t base;
} pyb_flash_obj_t;

// This Flash object represents the entire available flash, with emulated partition table at start
const pyb_flash_obj_t pyb_flash_obj = {
    { &pyb_flash_type }
};

STATIC void pyb_flash_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
		mp_printf(print, "<Flash>");
}

STATIC mp_obj_t pyb_flash_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return MP_OBJ_FROM_PTR(&pyb_flash_obj);
}

STATIC mp_obj_t pyb_flash_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {

		mp_buffer_info_t bufinfo;
		mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);

		mp_uint_t ret = storage_read_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / FLASH_BLOCK_SIZE);

		return MP_OBJ_NEW_SMALL_INT(ret);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_readblocks_obj, pyb_flash_readblocks);

STATIC mp_obj_t pyb_flash_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
	mp_buffer_info_t bufinfo;
	mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);

	mp_uint_t ret = storage_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / FLASH_BLOCK_SIZE);

	return MP_OBJ_NEW_SMALL_INT(ret);

}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_writeblocks_obj, pyb_flash_writeblocks);

STATIC mp_obj_t pyb_flash_ioctl(mp_obj_t self_in, mp_obj_t cmd_in, mp_obj_t arg_in) {

    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
			case MP_BLOCKDEV_IOCTL_INIT: 
				storage_init();
				
				return MP_OBJ_NEW_SMALL_INT(0);
						
			case MP_BLOCKDEV_IOCTL_DEINIT: 
				storage_flush();

				return MP_OBJ_NEW_SMALL_INT(0);	// TODO properly

			case MP_BLOCKDEV_IOCTL_SYNC:        
				storage_flush();

				return MP_OBJ_NEW_SMALL_INT(0);
			case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:		
	
			return MP_OBJ_NEW_SMALL_INT(QFLASH_IOCTL_NUM_BLOCKS);
			
			case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:  

			return MP_OBJ_NEW_SMALL_INT(FLASH_BLOCK_SIZE);
			
			case MP_BLOCKDEV_IOCTL_BLOCK_ERASE: {
					int ret = 0;
				//	ret = qspi_bdev_ioctl(BDEV_IOCTL_BLOCK_ERASE, 0);

					return MP_OBJ_NEW_SMALL_INT(ret);
			}
			
			default: 
			return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_flash_ioctl_obj, pyb_flash_ioctl);

STATIC const mp_rom_map_elem_t pyb_flash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&pyb_flash_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&pyb_flash_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&pyb_flash_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_flash_locals_dict, pyb_flash_locals_dict_table);

const mp_obj_type_t pyb_flash_type = {
    { &mp_type_type },
    .name = MP_QSTR_Flash,
    .print = pyb_flash_print,
    .make_new = pyb_flash_make_new,
    .locals_dict = (mp_obj_dict_t *)&pyb_flash_locals_dict,
};

void pyb_flash_init_vfs(fs_user_mount_t *vfs) {
    vfs->base.type = &mp_fat_vfs_type;
    vfs->blockdev.flags |= MP_BLOCKDEV_FLAG_NATIVE | MP_BLOCKDEV_FLAG_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    vfs->fatfs.part = 0; // flash filesystem lives on first partition
    vfs->blockdev.readblocks[0] = MP_OBJ_FROM_PTR(&pyb_flash_readblocks_obj);
    vfs->blockdev.readblocks[1] = MP_OBJ_FROM_PTR(&pyb_flash_obj);
    vfs->blockdev.readblocks[2] = MP_OBJ_FROM_PTR(storage_read_blocks); // native version
    vfs->blockdev.writeblocks[0] = MP_OBJ_FROM_PTR(&pyb_flash_writeblocks_obj);
    vfs->blockdev.writeblocks[1] = MP_OBJ_FROM_PTR(&pyb_flash_obj);
    vfs->blockdev.writeblocks[2] = MP_OBJ_FROM_PTR(storage_write_blocks); // native version
    vfs->blockdev.u.ioctl[0] = MP_OBJ_FROM_PTR(&pyb_flash_ioctl_obj);
    vfs->blockdev.u.ioctl[1] = MP_OBJ_FROM_PTR(&pyb_flash_obj);
}

#endif















