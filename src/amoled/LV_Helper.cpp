/**
 * @file      LV_Helper.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-04-20
 *
 */

#if AMOLED_S3

#include <Arduino.h>
#include "amoled/LV_Helper.h"


#if LV_VERSION_CHECK(9,0,0)
#error "Currently not supported 9.x"
#endif

static void lvgl_port_update_callback(lv_disp_drv_t *drv);
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t  indev_drv;

/* Display flushing */
static void disp_flush( lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    static_cast<LilyGo_Display *>(disp_drv->user_data)->pushColors(area->x1, area->y1, w, h, (uint16_t *)color_p);
    lv_disp_flush_ready( disp_drv );
}

/*Read the touchpad*/
static void touchpad_read( lv_indev_drv_t *indev_driver, lv_indev_data_t *data )
{
    static int16_t x, y;
    uint8_t touched =   static_cast<LilyGo_Display *>(indev_driver->user_data)->getPoint(&x, &y, 1);
    if ( touched ) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
        return;
    }
    data->state = LV_INDEV_STATE_REL;
}

#ifndef BOARD_HAS_PSRAM
#error "Please turn on PSRAM to OPI !"
#else
static lv_color_t *buf = NULL;
#endif

#if LV_USE_LOG
void lv_log_print_g_cb(const char *buf)
{
    Serial.println(buf);
    Serial.flush();
}
#endif

void beginLvglHelper(LilyGo_Display &board, bool debug)
{

    lv_init();

#if LV_USE_LOG
    if (debug) {
        lv_log_register_print_cb(lv_log_print_g_cb);
    }
#endif

    size_t lv_buffer_size = board.width() * board.height() * sizeof(lv_color_t);
    buf = (lv_color_t *)ps_malloc(lv_buffer_size);
    assert(buf);

    Serial.printf("AMOLED: Allocated %zu bytes for lvgl buffer: %p\n", (size_t)lv_buffer_size, buf);

    lv_disp_draw_buf_init( &draw_buf, buf, NULL, board.width() * board.height());

    /*Initialize the display*/
    lv_disp_drv_init( &disp_drv );
    /* display resolution */
    disp_drv.hor_res = board.width();
    disp_drv.ver_res = board.height();
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;
    disp_drv.user_data = &board;
    lv_disp_drv_register( &disp_drv );

    if (board.hasTouch()) {
        lv_indev_drv_init( &indev_drv );
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = touchpad_read;
        indev_drv.user_data = &board;
        lv_indev_drv_register( &indev_drv );
    }
}

#endif
