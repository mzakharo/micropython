
#ifndef MICROPY_INCLUDED_TKM32_SDCARD_H
#define MICROPY_INCLUDED_TKM32_SDCARD_H

// this is a fixed size and should not be changed
#define SDCARD_BLOCK_SIZE (512)

extern void sdcard_init(void);
extern bool sdcard_is_present(void);
extern bool sdcard_power_on(void);
extern void sdcard_power_off(void);
extern uint64_t sdcard_get_capacity_in_bytes(void);

// these return 0 on success, non-zero on error
extern mp_uint_t sdcard_read_blocks(uint8_t *dest, uint32_t block_num, uint32_t num_blocks);
extern mp_uint_t sdcard_write_blocks(const uint8_t *src, uint32_t block_num, uint32_t num_blocks);

extern const struct _mp_obj_type_t pyb_sdcard_type;

extern const struct _mp_obj_base_t pyb_sdcard_obj;

struct _fs_user_mount_t;
void sdcard_init_vfs(struct _fs_user_mount_t *vfs, int part);

#endif // MICROPY_INCLUDED_TKM32_SDCARD_H
