

#include <string.h>
#include <stdio.h>
#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs_fat.h"

#include "sdcard.h"
#include "sdio_sdcard.h"
#include "pin.h"
#include "pin_static_af.h"
#include "bufhelper.h"
#include "irq.h"
#include "systick.h"

#if MICROPY_HW_ENABLE_SDCARD

// These are definitions for F4 MCUs so there is a common macro across all MCUs.
#define MICROPY_HW_SDIO1_CK (pin_D4)
#define MICROPY_HW_SDIO1_CMD (pin_D5)
#define MICROPY_HW_SDIO1_D0 (pin_D0)
#define MICROPY_HW_SDIO1_D1 (pin_D1)
#define MICROPY_HW_SDIO1_D2 (pin_D2)
#define MICROPY_HW_SDIO1_D3 (pin_D3)

#define PYB_SDMMC_FLAG_SD       (0x01)
#define PYB_SDMMC_FLAG_MMC      (0x02)
#define PYB_SDMMC_FLAG_ACTIVE   (0x04)

static uint8_t pyb_sdmmc_flags;

// TODO: I think that as an optimization, we can allocate these dynamically
//       if an sd card is detected. This will save approx 260 bytes of RAM
//       when no sdcard was being used.

void sdcard_init(void) {
    // Set SD/MMC to no mode and inactive
    pyb_sdmmc_flags = 0;

    #if defined(MICROPY_HW_SDCARD_DETECT_PIN)
    mp_hal_pin_config(MICROPY_HW_SDCARD_DETECT_PIN, MP_HAL_PIN_MODE_IN_FLOATING, MICROPY_HW_SDCARD_DETECT_PULL, 0);
    #endif
		mp_hal_pin_config(MICROPY_HW_SDIO1_D0, 	MP_HAL_PIN_MODE_ALT_OUTPUT,	MP_HAL_PIN_PULL_UP, 12);
		mp_hal_pin_config(MICROPY_HW_SDIO1_D1, 	MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, 12);
		mp_hal_pin_config(MICROPY_HW_SDIO1_D2, 	MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, 12);
		mp_hal_pin_config(MICROPY_HW_SDIO1_D3, 	MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, 12);
		mp_hal_pin_config(MICROPY_HW_SDIO1_CK, 	MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, 12);
		mp_hal_pin_config(MICROPY_HW_SDIO1_CMD, MP_HAL_PIN_MODE_ALT_OUTPUT, MP_HAL_PIN_PULL_UP, 12);

	{
		RCC->APB2ENR |= 0x1<<12; //open sdio2 clkgate
		
		
	//TEST_SDIOx->MMC_CARDSEL = 0xD9;   //enable module, enable mmcclk
	//TEST_SDIOx->MMC_CTRL    = 0x03|((0x1)<<3)|(1<<8);//4bit,low speed,1/16 divider,初始化时必须用低速	
	TEST_SDIOx->MMC_CARDSEL = 0xC0;   //enable module, enable mmcclk		
	TEST_SDIOx->MMC_CTRL    = 0x03|((0x7)<<3)|(1<<8);//4bit,low speed,1/16 divider,初始化时必须用低速	
	TEST_SDIOx->MMC_CRCCTL   = 0xC0; 

	}

}

bool sdcard_is_present(void) {
    #if defined(MICROPY_HW_SDCARD_DETECT_PIN)
    return GPIO_ReadInputDataBit(MICROPY_HW_SDCARD_DETECT_PIN->gpio, MICROPY_HW_SDCARD_DETECT_PIN->pin_mask) == MICROPY_HW_SDCARD_DETECT_PRESENT;
    #else
    return false;
    #endif
}

#if MICROPY_HW_ENABLE_SDCARD
STATIC SD_Error sdmmc_init_sd(void) {
    // SD device interface configuration

    // init the SD interface, with retry if it's not ready yet
    SD_Error status;
    for (int retry = 10; (status = SD_Init()) != SD_OK; retry--) {
        if (retry == 0) {
            return status;
        }
        mp_hal_delay_ms(10);
    }

    return SD_OK;
}
#endif

bool sdcard_power_on(void) {
    if (pyb_sdmmc_flags & PYB_SDMMC_FLAG_ACTIVE) {
        return true;
    }

    HAL_StatusTypeDef status = HAL_ERROR;
    switch (pyb_sdmmc_flags) {
        #if MICROPY_HW_ENABLE_SDCARD
        case PYB_SDMMC_FLAG_SD:
            if (sdcard_is_present()) {
                status = sdmmc_init_sd();
            }
            break;
        #endif
    }

    if (status == HAL_OK) {
        pyb_sdmmc_flags |= PYB_SDMMC_FLAG_ACTIVE;
        return true;
    } else {
        return false;
    }
}

void sdcard_power_off(void) {
    switch (pyb_sdmmc_flags) {
        #if MICROPY_HW_ENABLE_SDCARD
        case PYB_SDMMC_FLAG_ACTIVE | PYB_SDMMC_FLAG_SD:
            SD_PowerOFF();
            break;
        #endif
    }
    pyb_sdmmc_flags &= ~PYB_SDMMC_FLAG_ACTIVE;
}

uint64_t sdcard_get_capacity_in_bytes(void) {
    switch (pyb_sdmmc_flags) {
        #if MICROPY_HW_ENABLE_SDCARD
        case PYB_SDMMC_FLAG_ACTIVE | PYB_SDMMC_FLAG_SD: {
						SD_GetCardInfo(&SDCardInfo);
            return (uint64_t)SDCardInfo.CardCapacity;
        }
        #endif
        default:
            return 0;
    }
}

STATIC HAL_StatusTypeDef sdcard_wait_finished(uint32_t timeout) {

    uint32_t start = HAL_GetTick();
    // Wait for SD card to complete the operation
    for (;;) {
        uint32_t state;
        state = SD_GetState();
        if (state == SD_CARD_TRANSFER) {
            return HAL_OK;
        }
        if (!(state == SD_CARD_SENDING || state == SD_CARD_RECEIVING || state == SD_CARD_PROGRAMMING)) {
           // return HAL_ERROR;
        }
        if (HAL_GetTick() - start >= timeout) {
            return HAL_TIMEOUT;
        }
        mp_hal_delay_us(1);
    }

    return HAL_OK;
}

mp_uint_t sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks) {
    // check that SD card is initialised
    if (!(pyb_sdmmc_flags & PYB_SDMMC_FLAG_ACTIVE)) {
        return HAL_ERROR;
    }
    uint8_t err = HAL_OK;
    // check that dest pointer is aligned on a 4-byte boundary
    uint8_t *orig_dest = NULL;
    uint32_t saved_word;
    if (((uint32_t)dest & 3) != 0) {

        orig_dest = dest;
        dest = (uint8_t *)((uint32_t)dest & ~3);
        saved_word = *(uint32_t *)dest;
    }

	uint32_t quer_sta = query_irq();
	uint32_t basepri = 0;
	if (quer_sta == IRQ_STATE_ENABLED) {
			basepri = raise_irq_pri(IRQ_PRI_OTG_FS);
		}
		err = SD_ReadDisk(dest, block_num, num_blocks);
		if (err == HAL_OK) {
				err = sdcard_wait_finished(60000);
		}
		if (quer_sta == IRQ_STATE_ENABLED){
			restore_irq_pri(basepri);
		}

    if (orig_dest != NULL) {
        // move the read data to the non-aligned position, and restore the initial bytes
        memmove(orig_dest, dest, num_blocks * SDCARD_BLOCK_SIZE);
        memcpy(dest, &saved_word, orig_dest - dest);
    }

    return err;
}

mp_uint_t sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks) {
    // check that SD card is initialised
    if (!(pyb_sdmmc_flags & PYB_SDMMC_FLAG_ACTIVE)) {
        return HAL_ERROR;
    }

    uint8_t err = HAL_OK;

    // check that src pointer is aligned on a 4-byte boundary
    if (((uint32_t)src & 3) != 0) {
        // pointer is not aligned, so allocate a temporary block to do the write
        uint8_t *src_aligned = m_new_maybe(uint8_t, SDCARD_BLOCK_SIZE);
        if (src_aligned == NULL) {
            return HAL_ERROR;
        }
        for (size_t i = 0; i < num_blocks; ++i) {
            memcpy(src_aligned, src + i * SDCARD_BLOCK_SIZE, SDCARD_BLOCK_SIZE);
            err = sdcard_write_blocks(src_aligned, block_num + i, 1);
            if (err != HAL_OK) {
                break;
            }
        }
        m_del(uint8_t, src_aligned, SDCARD_BLOCK_SIZE);
        return err;
    }

    err = SD_WriteDisk((uint8_t *)src, block_num, num_blocks);
    if (err == HAL_OK) {
        err = sdcard_wait_finished(600000);
    }
    return err;
}

/******************************************************************************/
// MicroPython bindings
//
// Expose the SD card or MMC as an object with the block protocol.

// There are singleton SDCard/MMCard objects
#if MICROPY_HW_ENABLE_SDCARD
const mp_obj_base_t pyb_sdcard_obj = {&pyb_sdcard_type};
#endif

#if MICROPY_HW_ENABLE_SDCARD
STATIC mp_obj_t pyb_sdcard_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    pyb_sdmmc_flags |= PYB_SDMMC_FLAG_SD;

    // return singleton object
    return MP_OBJ_FROM_PTR(&pyb_sdcard_obj);
}
#endif

STATIC mp_obj_t sd_present(mp_obj_t self) {
    return mp_obj_new_bool(sdcard_is_present());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sd_present_obj, sd_present);

STATIC mp_obj_t sd_power(mp_obj_t self, mp_obj_t state) {
    bool result;
    if (mp_obj_is_true(state)) {
        result = sdcard_power_on();
    } else {
        sdcard_power_off();
        result = true;
    }
    return mp_obj_new_bool(result);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sd_power_obj, sd_power);

STATIC mp_obj_t sd_info(mp_obj_t self) {
    if (!(pyb_sdmmc_flags & PYB_SDMMC_FLAG_ACTIVE)) {
        return mp_const_none;
    }
    uint32_t card_type;
		card_type = SDCardInfo.CardType;
    // cardinfo.SD_csd and cardinfo.SD_cid have lots of info but we don't use them
    mp_obj_t tuple[3] = {
        mp_obj_new_int_from_ull((uint64_t)SDCardInfo.CardCapacity),
        mp_obj_new_int_from_uint(SDCardInfo.CardBlockSize),
        mp_obj_new_int(card_type),
    };
    return mp_obj_new_tuple(3, tuple);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(sd_info_obj, sd_info);

// now obsolete, kept for backwards compatibility
STATIC mp_obj_t sd_read(mp_obj_t self, mp_obj_t block_num) {
    uint8_t *dest = m_new(uint8_t, SDCARD_BLOCK_SIZE);
    mp_uint_t ret = sdcard_read_blocks(dest, mp_obj_get_int(block_num), 1);

    if (ret != 0) {
        m_del(uint8_t, dest, SDCARD_BLOCK_SIZE);
        mp_raise_msg_varg(&mp_type_Exception, MP_ERROR_TEXT("sdcard_read_blocks failed [%u]"), ret);
    }

    return mp_obj_new_bytearray_by_ref(SDCARD_BLOCK_SIZE, dest);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(sd_read_obj, sd_read);

// now obsolete, kept for backwards compatibility
STATIC mp_obj_t sd_write(mp_obj_t self, mp_obj_t block_num, mp_obj_t data) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);
    if (bufinfo.len % SDCARD_BLOCK_SIZE != 0) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("writes must be a multiple of %d bytes"), SDCARD_BLOCK_SIZE);
    }

    mp_uint_t ret = sdcard_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);

    if (ret != 0) {
        mp_raise_msg_varg(&mp_type_Exception, MP_ERROR_TEXT("sdcard_write_blocks failed [%u]"), ret);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(sd_write_obj, sd_write);

STATIC mp_obj_t pyb_sdcard_readblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);
    mp_uint_t ret = sdcard_read_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_readblocks_obj, pyb_sdcard_readblocks);

STATIC mp_obj_t pyb_sdcard_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);
    mp_uint_t ret = sdcard_write_blocks(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / SDCARD_BLOCK_SIZE);
    return mp_obj_new_bool(ret == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_writeblocks_obj, pyb_sdcard_writeblocks);

STATIC mp_obj_t pyb_sdcard_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in) {
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch (cmd) {
        case MP_BLOCKDEV_IOCTL_INIT:
            if (!sdcard_power_on()) {
                return MP_OBJ_NEW_SMALL_INT(-1); // error
            }
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case MP_BLOCKDEV_IOCTL_DEINIT:
            sdcard_power_off();
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case MP_BLOCKDEV_IOCTL_SYNC:
            // nothing to do
            return MP_OBJ_NEW_SMALL_INT(0); // success

        case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
            return MP_OBJ_NEW_SMALL_INT(sdcard_get_capacity_in_bytes() / SDCARD_BLOCK_SIZE);

        case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
            return MP_OBJ_NEW_SMALL_INT(SDCARD_BLOCK_SIZE);

        default: // unknown command
            return MP_OBJ_NEW_SMALL_INT(-1); // error
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_sdcard_ioctl_obj, pyb_sdcard_ioctl);

STATIC const mp_rom_map_elem_t pyb_sdcard_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_present), MP_ROM_PTR(&sd_present_obj) },
    { MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&sd_power_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&sd_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&sd_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&sd_write_obj) },
    // block device protocol
    { MP_ROM_QSTR(MP_QSTR_readblocks), MP_ROM_PTR(&pyb_sdcard_readblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeblocks), MP_ROM_PTR(&pyb_sdcard_writeblocks_obj) },
    { MP_ROM_QSTR(MP_QSTR_ioctl), MP_ROM_PTR(&pyb_sdcard_ioctl_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_sdcard_locals_dict, pyb_sdcard_locals_dict_table);

#if MICROPY_HW_ENABLE_SDCARD
const mp_obj_type_t pyb_sdcard_type = {
    { &mp_type_type },
    .name = MP_QSTR_SDCard,
    .make_new = pyb_sdcard_make_new,
    .locals_dict = (mp_obj_dict_t *)&pyb_sdcard_locals_dict,
};
#endif

void sdcard_init_vfs(fs_user_mount_t *vfs, int part) {
    pyb_sdmmc_flags = (pyb_sdmmc_flags & PYB_SDMMC_FLAG_ACTIVE) | PYB_SDMMC_FLAG_SD; // force SD mode
    vfs->base.type = &mp_fat_vfs_type;
    vfs->blockdev.flags |= MP_BLOCKDEV_FLAG_NATIVE | MP_BLOCKDEV_FLAG_HAVE_IOCTL;
    vfs->fatfs.drv = vfs;
    #if MICROPY_FATFS_MULTI_PARTITION
    vfs->fatfs.part = part;
    #endif
    vfs->blockdev.readblocks[0] = MP_OBJ_FROM_PTR(&pyb_sdcard_readblocks_obj);
    vfs->blockdev.readblocks[1] = MP_OBJ_FROM_PTR(&pyb_sdcard_obj);
    vfs->blockdev.readblocks[2] = MP_OBJ_FROM_PTR(sdcard_read_blocks); // native version
    vfs->blockdev.writeblocks[0] = MP_OBJ_FROM_PTR(&pyb_sdcard_writeblocks_obj);
    vfs->blockdev.writeblocks[1] = MP_OBJ_FROM_PTR(&pyb_sdcard_obj);
    vfs->blockdev.writeblocks[2] = MP_OBJ_FROM_PTR(sdcard_write_blocks); // native version
    vfs->blockdev.u.ioctl[0] = MP_OBJ_FROM_PTR(&pyb_sdcard_ioctl_obj);
    vfs->blockdev.u.ioctl[1] = MP_OBJ_FROM_PTR(&pyb_sdcard_obj);
}

#endif // MICROPY_HW_ENABLE_SDCARD || MICROPY_HW_ENABLE_MMCARD
