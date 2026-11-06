/********************************************************************************
	* Copyright (C), 2021 -2026, 01studio Tech. Co., Ltd.https://www.01studio.cc/
	* File Name				:	modgb.c
	* Author				:	Folktale
	* Version				:	v1.0
	* date					:	2026/11/06
	* Description			:	Walnut-CGB GameBoy / GameBoy Color emulator support
******************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "py/objstr.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "lcd_spibus.h"
#include "lib/oofatfs/ff.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#define STATIC static

#if MICROPY_HW_GBEMU

#include "ST7789.h"
#include "modmachine.h"
#include "psxcontroller.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_task.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

#if MICROPY_ENABLE_TFTLCD
#include "modtftlcd.h"
#endif

#if MICROPY_HW_USB_CDC
#include "shared/tinyusb/mp_usbd.h"
#include "shared/tinyusb/mp_usbd_cdc.h"
#endif

#include "global.h"

#define ENABLE_SOUND 0
#define ENABLE_LCD 1
struct gb_s;
#include "walnut_cgb.h"
#include "extras/sgb.h"

#define GB_DISPLAY_WIDTH  240
#define GB_DISPLAY_HEIGHT 240
#define GB_LINE_COUNT     20

typedef struct _gbemu_obj_t {
    mp_obj_base_t base;
} gbemu_obj_t;

struct gb_priv_t {
    uint8_t *rom;
    size_t rom_size;
    uint8_t *cart_ram;
    size_t save_size;
    uint16_t selected_palette[12];
    uint16_t (*fb)[LCD_WIDTH];
};

static struct gb_s *s_gb = NULL;
static struct gb_priv_t s_priv;
static uint16_t *s_lcd_line_buf = NULL;
static QueueHandle_t s_gb_vidQueue = NULL;
static TaskHandle_t s_gb_vidTaskHandle = NULL;

static inline uint16_t rgb888_to_rgb565_gb(uint32_t rgb) {
    return (uint16_t)(
        ((rgb >> 8) & 0xF800) |
        ((rgb >> 5) & 0x07E0) |
        ((rgb >> 3) & 0x001F)
    );
}

static inline uint16_t rgb555_to_rgb565_gb(uint16_t c555) {
    uint16_t r5 = (c555 >> 10) & 0x1F;
    uint16_t g5 = (c555 >> 5) & 0x1F;
    uint16_t b5 = c555 & 0x1F;
    uint16_t g6 = (g5 << 1) | (g5 >> 4);
    return (r5 << 11) | (g6 << 5) | b5;
}

static void sgb_palette_to_rgb565(uint32_t palette_index, uint16_t *dst_rgb565) {
    const uint32_t *src = &default32sgb_palettes[palette_index * 12];
    for (unsigned i = 0; i < 12; i++) {
        dst_rgb565[i] = rgb888_to_rgb565_gb(src[i]);
    }
}

static void auto_assign_palette(struct gb_priv_t *priv, uint8_t game_checksum) {
    switch (game_checksum) {
        case 0x71:
        case 0xFF:
            sgb_palette_to_rgb565(0, priv->selected_palette);
            break;
        case 0x15:
        case 0xDB:
        case 0x95:
            sgb_palette_to_rgb565(16, priv->selected_palette);
            break;
        case 0x19: {
            const uint16_t palette[12] = {
                rgb555_to_rgb565_gb(0x7FFF), rgb555_to_rgb565_gb(0x7E10), rgb555_to_rgb565_gb(0x48E7), rgb555_to_rgb565_gb(0x0000),
                rgb555_to_rgb565_gb(0x7FFF), rgb555_to_rgb565_gb(0x7E10), rgb555_to_rgb565_gb(0x48E7), rgb555_to_rgb565_gb(0x0000),
                rgb555_to_rgb565_gb(0x7FFF), rgb555_to_rgb565_gb(0x7E60), rgb555_to_rgb565_gb(0x7C00), rgb555_to_rgb565_gb(0x0000)
            };
            memcpy(priv->selected_palette, palette, sizeof(palette));
            break;
        }
        case 0x61:
        case 0x45:
        case 0xD8:
            sgb_palette_to_rgb565(SGB_PALETTE.POKEMON_BLUE, priv->selected_palette);
            break;
        case 0x14:
            sgb_palette_to_rgb565(SGB_PALETTE.POKEMON_RED, priv->selected_palette);
            break;
        case 0x8B:
            sgb_palette_to_rgb565(7, priv->selected_palette);
            break;
        case 0x27:
        case 0x49:
        case 0x5C:
        case 0xB3:
            sgb_palette_to_rgb565(2, priv->selected_palette);
            break;
        case 0x18:
        case 0x6A:
        case 0x4B:
        case 0x6B:
        case 0x86:
            sgb_palette_to_rgb565(SGB_PALETTE.DONKEY_KONG_LAND_1, priv->selected_palette);
            break;
        case 0x70:
            sgb_palette_to_rgb565(SGB_PALETTE.ZELDA_LINKS_AWAKENING, priv->selected_palette);
            break;
        case 0x01:
        case 0x10:
        case 0x29:
        case 0x52:
        case 0x5D:
        case 0x68:
        case 0x6D:
        case 0xF6:
            sgb_palette_to_rgb565(36, priv->selected_palette);
            break;
        default: {
            const uint32_t default_pal[12] = {
                0x7B8210, 0x5A7942, 0x39594A, 0x294139,
                0x7B8210, 0x5A7942, 0x39594A, 0x294139,
                0x7B8210, 0x5A7942, 0x39594A, 0x294139
            };
            for (int i = 0; i < 12; i++) {
                priv->selected_palette[i] = rgb888_to_rgb565_gb(default_pal[i]);
            }
            break;
        }
    }
}

static uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr) {
    const struct gb_priv_t * const p = (const struct gb_priv_t *)gb->direct.priv;
    return p->rom[addr];
}

static uint16_t gb_rom_read_16bit(struct gb_s *gb, const uint_fast32_t addr) {
    const uint8_t *src = &((const struct gb_priv_t *)gb->direct.priv)->rom[addr];
    if ((uintptr_t)src & 1) {
        return ((uint16_t)src[0]) | ((uint16_t)src[1] << 8);
    }
    return *(const uint16_t *)src;
}

static uint32_t gb_rom_read_32bit(struct gb_s *gb, const uint_fast32_t addr) {
    const uint8_t *src = &((const struct gb_priv_t *)gb->direct.priv)->rom[addr];
    if ((uintptr_t)src & 3) {
        return ((uint32_t)src[0]) |
               ((uint32_t)src[1] << 8) |
               ((uint32_t)src[2] << 16) |
               ((uint32_t)src[3] << 24);
    }
    return *(const uint32_t *)src;
}

static uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr) {
    const struct gb_priv_t * const p = (const struct gb_priv_t *)gb->direct.priv;
    if (p->cart_ram && addr < p->save_size) {
        return p->cart_ram[addr];
    }
    return 0xFF;
}

static void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val) {
    const struct gb_priv_t * const p = (const struct gb_priv_t *)gb->direct.priv;
    if (p->cart_ram && addr < p->save_size) {
        p->cart_ram[addr] = val;
    }
}

static void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr) {
    const char *gb_err_str[GB_INVALID_MAX] = {
        "UNKNOWN",
        "INVALID OPCODE",
        "INVALID READ",
        "INVALID WRITE",
        "HALT FOREVER"
    };
    printf("Walnut-CGB Error: %s at 0x%04X\r\n", gb_err < GB_INVALID_MAX ? gb_err_str[gb_err] : "UNKNOWN", addr);
}

#if ENABLE_LCD
static void gb_lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line) {
    struct gb_priv_t *priv = (struct gb_priv_t *)gb->direct.priv;
    uint16_t (*fb565)[LCD_WIDTH] = priv->fb;
    if (!fb565 || line >= LCD_HEIGHT) {
        return;
    }

    if (gb->cgb.cgbMode) {
        for (unsigned int x = 0; x < LCD_WIDTH; x++) {
            fb565[line][x] = gb->cgb.fixPalette[pixels[x]];
        }
    }
#if WALNUT_GB_12_COLOUR
    else {
        for (unsigned int x = 0; x < LCD_WIDTH; x++) {
            fb565[line][x] = priv->selected_palette[((pixels[x] & 18) >> 1) | (pixels[x] & 3)];
        }
    }
#else
    else {
        for (unsigned int x = 0; x < LCD_WIDTH; x++) {
            fb565[line][x] = priv->selected_palette[pixels[x] & 3];
        }
    }
#endif
}
#endif

static void gb_lcd_write_frame(const uint16_t (*fb)[LCD_WIDTH]) {
    if (s_lcd_line_buf == NULL) {
        return;
    }

    if (fb == NULL) {
        for (int i = 0; i < GB_DISPLAY_WIDTH * GB_LINE_COUNT; i++) {
            s_lcd_line_buf[i] = 0x0000;
        }
        for (int y = 0; y < GB_DISPLAY_HEIGHT; y += GB_LINE_COUNT) {
            #if (MICROPY_HW_LCD15 & MICROPY_ENABLE_TFTLCD)
            st7789_Full(0, y, GB_DISPLAY_WIDTH, GB_LINE_COUNT, s_lcd_line_buf);
            #endif
        }
        return;
    }

    // Scale 160x144 to 240x240
    for (int y = 0; y < GB_DISPLAY_HEIGHT; y += GB_LINE_COUNT) {
        for (int i = 0; i < GB_LINE_COUNT; ++i) {
            int out_y = y + i;
            if (out_y >= GB_DISPLAY_HEIGHT) {
                break;
            }
            int src_y = (out_y * LCD_HEIGHT) / GB_DISPLAY_HEIGHT;
            const uint16_t *src_row = fb[src_y];
            uint16_t *dst_row = &s_lcd_line_buf[i * GB_DISPLAY_WIDTH];

            for (int x = 0; x < GB_DISPLAY_WIDTH; x++) {
                int src_x = (x * LCD_WIDTH) / GB_DISPLAY_WIDTH;
                dst_row[x] = src_row[src_x];
            }
        }
        #if (MICROPY_HW_LCD15 & MICROPY_ENABLE_TFTLCD)
        st7789_Full(0, y, GB_DISPLAY_WIDTH, GB_LINE_COUNT, s_lcd_line_buf);
        #endif
    }
}

static void gb_videoTask(void *arg) {
    uint16_t (*fb)[LCD_WIDTH] = NULL;
    while (1) {
        if (xQueueReceive(s_gb_vidQueue, &fb, portMAX_DELAY) == pdTRUE) {
            gb_lcd_write_frame((const uint16_t (*)[LCD_WIDTH])fb);
        }
    }
}

static void gb_make_save_filename(const char *rom_filename, char *save_filename, size_t max_len) {
    strncpy(save_filename, rom_filename, max_len - 1);
    save_filename[max_len - 1] = '\0';
    char *dot = strrchr(save_filename, '.');
    if (dot != NULL && (size_t)(dot - save_filename) + 4 < max_len) {
        strcpy(dot, ".sav");
    } else if (strlen(save_filename) + 4 < max_len) {
        strcat(save_filename, ".sav");
    }
}

static void gb_load_sram(const char *rom_filename, struct gb_priv_t *priv) {
    if (priv->save_size == 0 || priv->cart_ram == NULL) {
        return;
    }
    char save_fn[256];
    gb_make_save_filename(rom_filename, save_fn, sizeof(save_fn));

    if (mp_vfs_import_stat(save_fn) != MP_IMPORT_STAT_FILE) {
        return;
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_obj_t args[2] = {
            mp_obj_new_str(save_fn, strlen(save_fn)),
            MP_OBJ_NEW_QSTR(MP_QSTR_rb),
        };
        mp_obj_t fp = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);
        mp_stream_posix_read(fp, priv->cart_ram, priv->save_size);
        mp_stream_close(fp);
        nlr_pop();
    } else {
        // Ignore error if save file cannot be read
    }
}

static uint8_t *gb_load_rom_file(const char *filename, size_t *out_size) {
    mp_obj_t args[2] = {
        mp_obj_new_str(filename, strlen(filename)),
        MP_OBJ_NEW_QSTR(MP_QSTR_rb),
    };
    mp_obj_t fp = mp_vfs_open(MP_ARRAY_SIZE(args), &args[0], (mp_map_t *)&mp_const_empty_map);

    off_t file_size = mp_stream_posix_lseek(fp, 0, MP_SEEK_END);
    mp_stream_posix_lseek(fp, 0, MP_SEEK_SET);

    if (file_size <= 0) {
        mp_stream_close(fp);
        return NULL;
    }

    uint8_t *rom = (uint8_t *)heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (rom == NULL) {
        rom = (uint8_t *)m_malloc(file_size);
    }
    if (rom == NULL) {
        mp_stream_close(fp);
        return NULL;
    }

    size_t total_read = 0;
    while (total_read < (size_t)file_size) {
        size_t to_read = (size_t)file_size - total_read;
        if (to_read > 16384) {
            to_read = 16384;
        }
        ssize_t r = mp_stream_posix_read(fp, rom + total_read, to_read);
        if (r <= 0) {
            break;
        }
        total_read += r;
    }

    mp_stream_close(fp);

    if (total_read != (size_t)file_size) {
        heap_caps_free(rom);
        return NULL;
    }

    *out_size = (size_t)file_size;
    return rom;
}

/*
 ________________________________________________________________________________________________________
|  15 |   14  |   13  |   12   | 11| 10| 9 | 8 |   7  |   6   |  5  |   4   |   3   |   2    |  1  |  0  |
|-----|-------|-------|--------|---|---|---|---|------|-------|-----|-------|-------|--------|-----|-----|
| UP1 | DOWN1 | LEFT1 | RIGHT1 | A | B | X | Y | BACK | START | UP2 | DOWN2 | LEFT2 | RIGHT2 | TDB | TDB |
|-----|-------|-------|--------|---|---|---|---|------|-------|-----|-------|-------|--------|-----|-----|
*/
static void gb_update_input(struct gb_s *gb) {
    int b = psxReadInput();

    uint8_t joypad = 0xFF;

    if (!(b & (1 << 15))) joypad &= ~JOYPAD_UP;
    if (!(b & (1 << 14))) joypad &= ~JOYPAD_DOWN;
    if (!(b & (1 << 13))) joypad &= ~JOYPAD_LEFT;
    if (!(b & (1 << 12))) joypad &= ~JOYPAD_RIGHT;
    if (!(b & (1 << 11))) joypad &= ~JOYPAD_A;
    if (!(b & (1 << 10))) joypad &= ~JOYPAD_B;
    if (!(b & (1 << 7)))  joypad &= ~JOYPAD_SELECT;
    if (!(b & (1 << 6)))  joypad &= ~JOYPAD_START;

    gb->direct.joypad = joypad;
}

void app_gb_start(const char *filename) {
    memset(&s_priv, 0, sizeof(s_priv));

    s_priv.rom = gb_load_rom_file(filename, &s_priv.rom_size);
    if (s_priv.rom == NULL) {
        mp_raise_ValueError(MP_ERROR_TEXT("Could not allocate or read GB ROM image"));
    }

    if (s_gb == NULL) {
        s_gb = (struct gb_s *)heap_caps_malloc(sizeof(struct gb_s), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (s_gb == NULL) {
            s_gb = (struct gb_s *)m_malloc(sizeof(struct gb_s));
        }
    }
    if (s_gb == NULL) {
        heap_caps_free(s_priv.rom);
        s_priv.rom = NULL;
        mp_raise_ValueError(MP_ERROR_TEXT("Could not allocate GB emulator context"));
    }
    memset(s_gb, 0, sizeof(struct gb_s));

    s_priv.fb = heap_caps_malloc(LCD_HEIGHT * LCD_WIDTH * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_priv.fb == NULL) {
        s_priv.fb = m_malloc(LCD_HEIGHT * LCD_WIDTH * sizeof(uint16_t));
    }
    if (s_priv.fb == NULL) {
        heap_caps_free(s_priv.rom);
        s_priv.rom = NULL;
        mp_raise_ValueError(MP_ERROR_TEXT("Could not allocate GB framebuffer"));
    }
    memset(s_priv.fb, 0, LCD_HEIGHT * LCD_WIDTH * sizeof(uint16_t));

    if (s_lcd_line_buf == NULL) {
        s_lcd_line_buf = (uint16_t *)heap_caps_malloc(GB_DISPLAY_WIDTH * GB_LINE_COUNT * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    }

    enum gb_init_error_e gb_ret = gb_init(
        s_gb,
        &gb_rom_read,
        &gb_rom_read_16bit,
        &gb_rom_read_32bit,
        &gb_cart_ram_read,
        &gb_cart_ram_write,
        &gb_error,
        &s_priv
    );

    if (gb_ret != GB_INIT_NO_ERROR) {
        heap_caps_free(s_priv.fb);
        s_priv.fb = NULL;
        heap_caps_free(s_priv.rom);
        s_priv.rom = NULL;
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid or unsupported GB ROM"));
    }

    if (gb_get_save_size_s(s_gb, &s_priv.save_size) < 0) {
        s_priv.save_size = 0;
    }

    if (s_priv.save_size > 0) {
        s_priv.cart_ram = (uint8_t *)heap_caps_malloc(s_priv.save_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (s_priv.cart_ram == NULL) {
            s_priv.cart_ram = (uint8_t *)m_malloc(s_priv.save_size);
        }
        if (s_priv.cart_ram != NULL) {
            memset(s_priv.cart_ram, 0xFF, s_priv.save_size);
            gb_load_sram(filename, &s_priv);
        }
    }

    auto_assign_palette(&s_priv, gb_colour_hash(s_gb));

#if ENABLE_LCD
    gb_init_lcd(s_gb, &gb_lcd_draw_line);
    s_gb->direct.interlace = false;
    s_gb->direct.frame_skip = false;
#endif

    gb_lcd_write_frame(NULL);

    if (s_gb_vidQueue == NULL) {
        s_gb_vidQueue = xQueueCreate(1, sizeof(uint16_t (*)[LCD_WIDTH]));
    }
    if (s_gb_vidTaskHandle == NULL) {
        xTaskCreatePinnedToCore(&gb_videoTask, "gbVideoTask", 2048, NULL, 5, &s_gb_vidTaskHandle, 1);
    }

    psxcontrollerInit();

    const uint32_t target_speed_us = (uint32_t)(1000000.0 / VERTICAL_SYNC);

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        while (1) {
            struct timeval timecheck;
            gettimeofday(&timecheck, NULL);
            uint32_t start_us = (uint32_t)timecheck.tv_sec * 1000000 + (uint32_t)timecheck.tv_usec;

            #if MICROPY_HW_USB_CDC
            mp_usbd_cdc_poll_interfaces(0);
            #endif
            mp_handle_pending(true);

            gb_update_input(s_gb);

            gb_run_frame_dualfetch(s_gb);

            uint16_t (*fb_ptr)[LCD_WIDTH] = s_priv.fb;
            xQueueSend(s_gb_vidQueue, &fb_ptr, 0);

            gettimeofday(&timecheck, NULL);
            uint32_t end_us = (uint32_t)timecheck.tv_sec * 1000000 + (uint32_t)timecheck.tv_usec;
            int32_t delay_us = (int32_t)target_speed_us - (int32_t)(end_us - start_us);
            if (delay_us > 1000) {
                vTaskDelay((delay_us / 1000) / portTICK_PERIOD_MS);
            } else {
                vTaskDelay(1);
            }
        }
        nlr_pop();
    } else {
        // Clean up allocated resources on KeyboardInterrupt (Ctrl-C) or exception
        if (s_priv.cart_ram != NULL) {
            heap_caps_free(s_priv.cart_ram);
            s_priv.cart_ram = NULL;
        }
        if (s_priv.fb != NULL) {
            heap_caps_free(s_priv.fb);
            s_priv.fb = NULL;
        }
        if (s_priv.rom != NULL) {
            heap_caps_free(s_priv.rom);
            s_priv.rom = NULL;
        }
        MP_STATE_MEM(gc_auto_collect_enabled) = 1;
        nlr_jump(nlr.ret_val);
    }
}

//===================================================================================================================
STATIC mp_obj_t gbemu_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t gbemu_allowed_args[] = {
        { MP_QSTR_file,    MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_reserve, MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool = true} },
    };

    uint8_t arg_num = MP_ARRAY_SIZE(gbemu_allowed_args);
    mp_arg_val_t args[arg_num];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, arg_num, gbemu_allowed_args, args);

    if (args[0].u_obj != MP_OBJ_NULL) {
        mp_buffer_info_t bufinfo;
        if (mp_obj_is_int(args[0].u_obj)) {
            mp_raise_ValueError(MP_ERROR_TEXT("gb parameter error"));
        } else {
            mp_get_buffer_raise(args[0].u_obj, &bufinfo, MP_BUFFER_READ);

            const char *file_path = (const char *)bufinfo.buf;
            const char *ftype = mp_obj_str_get_str(file_type(file_path));
            if (strncmp(ftype, "gb", 2) == 0 || strncmp(ftype, "GB", 2) == 0 ||
                strncmp(ftype, "gbc", 3) == 0 || strncmp(ftype, "GBC", 3) == 0) {
                MP_STATE_MEM(gc_auto_collect_enabled) = 0;
                app_gb_start(file_path);
            } else {
                mp_raise_ValueError(MP_ERROR_TEXT("input gb file type error"));
                return mp_const_none;
            }
        }
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("gb parameter is empty"));
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(gbemu_start_obj, 1, gbemu_start);

//----------------------------------------------------------------------------------
STATIC mp_obj_t gbemu_deinit(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(gbemu_deinit_obj, 0, gbemu_deinit);

//----------------------------------------------------------------------------------
STATIC mp_obj_t gbemu_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, MP_OBJ_FUN_ARGS_MAX, true);

    gbemu_obj_t *gbemu_obj = m_new_obj(gbemu_obj_t);
    gbemu_obj->base.type = type;

    return MP_OBJ_FROM_PTR(gbemu_obj);
}

/******************************************************************************/
STATIC const mp_rom_map_elem_t gbemu_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_game_gb) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&gbemu_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_start), MP_ROM_PTR(&gbemu_start_obj) },
};
STATIC MP_DEFINE_CONST_DICT(gbemu_locals_dict, gbemu_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    game_gbemu_type,
    MP_QSTR_GB,
    MP_TYPE_FLAG_NONE,
    make_new, gbemu_make_new,
    locals_dict, &gbemu_locals_dict
);

#endif
