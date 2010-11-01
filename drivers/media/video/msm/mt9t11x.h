
#ifndef MT9T11X_H
#define MT9T11X_H

#include <mach/board.h>
#include <mach/camera.h>

extern struct mt9t11x_reg_t mt9t111_regs;
extern struct mt9t11x_reg_t mt9t112_regs;

enum mt9t11x_width_t {
    WORD_LEN,
    BYTE_LEN
};

struct mt9t11x_i2c_reg_conf {
    unsigned short waddr;
    unsigned short wdata;
    enum mt9t11x_width_t width;
    unsigned short mdelay_time;
};

struct mt9t11x_reg_t {
    struct mt9t11x_i2c_reg_conf const *pll_tbl;
    uint16_t pll_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *clk_tbl;
    uint16_t clk_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *prevsnap_tbl;
    uint16_t prevsnap_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *wb_cloudy_tbl;
    uint16_t wb_cloudy_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *wb_daylight_tbl;
    uint16_t wb_daylight_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *wb_flourescant_tbl;
    uint16_t wb_flourescant_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *wb_incandescent_tbl;
    uint16_t wb_incandescent_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *wb_auto_tbl;
    uint16_t wb_auto_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *af_tbl;
    uint16_t af_tbl_sz;

    struct mt9t11x_i2c_reg_conf const **contrast_tbl;
    uint16_t const *contrast_tbl_sz;

    struct mt9t11x_i2c_reg_conf const **brightness_tbl;
    uint16_t const *brightness_tbl_sz;

    struct mt9t11x_i2c_reg_conf const **saturation_tbl;
    uint16_t const *saturation_tbl_sz;

    struct mt9t11x_i2c_reg_conf const **sharpness_tbl;
    uint16_t const *sharpness_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *lens_for_outdoor_tbl;
    uint16_t const lens_for_outdoor_tbl_sz;

    struct mt9t11x_i2c_reg_conf const *lens_for_indoor_tbl;
    uint16_t const lens_for_indoor_tbl_sz;
};

#endif 
