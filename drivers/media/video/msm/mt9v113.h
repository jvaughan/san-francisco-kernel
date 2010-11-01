

#ifndef MT9V113_H
#define MT9V113_H

#include <mach/board.h>
#include <mach/camera.h>

extern struct mt9v113_reg_t mt9v113_regs;

enum mt9v113_width_t {
    WORD_LEN,
    BYTE_LEN
};

struct mt9v113_i2c_reg_conf {
    unsigned short waddr;
    unsigned short wdata;
    enum mt9v113_width_t width;
    unsigned short mdelay_time;
};

struct mt9v113_reg_t {
    struct mt9v113_i2c_reg_conf const *prev_snap_reg_settings;
    uint16_t prev_snap_reg_settings_size;
    struct mt9v113_i2c_reg_conf const *noise_reduction_reg_settings;
    uint16_t noise_reduction_reg_settings_size;
    struct mt9v113_i2c_reg_conf const *plltbl;
    uint16_t plltbl_size;
    struct mt9v113_i2c_reg_conf const *stbl;
    uint16_t stbl_size;
    struct mt9v113_i2c_reg_conf const *rftbl;
    uint16_t rftbl_size;
};

#endif 
