/**
* @file main
*
*/

/*********************
*      INCLUDES
*********************/
#include <stdlib.h>
#include <Windows.h>
#include <SDL.h>
#include "lvgl/lvgl.h"
#include "lv_drivers/display/monitor.h"
#include "lv_drivers/indev/mouse.h"
#include "lv_drivers/indev/keyboard.h"
#include "lv_examples/lv_examples.h"

#include "my_watch.h"
#include "gui.h"

/*********************
*      DEFINES
*********************/

/**********************
*      TYPEDEFS
**********************/

/**********************
*  STATIC PROTOTYPES
**********************/
static void hal_init(void);
static int tick_thread(void *data);

/**********************
*  STATIC VARIABLES
**********************/
static lv_indev_t * kb_indev;

static DWORD sleep_ms = 10; // default

/**********************
*      MACROS
**********************/

/**********************
*   GLOBAL FUNCTIONS
**********************/

void update_time(bool update_date)
{
    SYSTEMTIME time;
    GetLocalTime(&time);
    updateTime(time.wHour, time.wMinute, time.wSecond);
    if (update_date)
        updateDate(time.wYear, time.wMonth, time.wDay, time.wDayOfWeek);
}

uint16_t get_bat_charging(void)
{
    return 1; // not implemented
}

uint16_t get_bat_level(void)
{
    return 100; // not implemented
}

uint32_t get_free_mem(void)
{
    return 0; // not implemented
}

void play_sound(void)
{
    // not implemented
}

void get_accel(lv_point_t *dir)
{
    if (!dir)
        return;

    // simulated
    DWORD curr_ms = GetTickCount();
    dir->x = 50 * sin(0.003*curr_ms);
    dir->y = 50 * cos(0.005*curr_ms);
}

void ntp_sync_time(void)
{
  // not implemented	
}

void set_normal_speed(void)
{
    sleep_ms = 50;
}

void set_high_speed(void)
{
    sleep_ms = 5;
}


int main(int argc, char** argv)
{
    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL for LittlevGL*/
    hal_init();

    /*
     * Demos, benchmarks, and tests.
     *
     * Uncomment any one (and only one) of the functions below to run that
     * item.
     */

    if (1)
    {
        setupGui();
        updateBatteryLevel();
        updateStepCounter(123);
        update_time(true);

        lv_disp_trig_activity(NULL);
    }
    else
        my_watch();

    //lv_demo_widgets();
    //lv_demo_benchmark();
    //lv_demo_keypad_encoder();
    //lv_demo_printer();
    //lv_demo_stress();
    //lv_ex_get_started_1();
    //lv_ex_get_started_2();
    //lv_ex_get_started_3();

    //lv_ex_style_1();
    //lv_ex_style_2();
    //lv_ex_style_3();
    //lv_ex_style_4();
    //lv_ex_style_5();
    //lv_ex_style_6();
    //lv_ex_style_7();
    //lv_ex_style_8();
    //lv_ex_style_9();
    //lv_ex_style_10();
    //lv_ex_style_11();

    /*
     * There are many examples of individual widgets found under the
     * lv_examples/src/lv_ex_widgets directory.  Here are a few sample test
     * functions.  Look in that directory to find all the rest.
     */
    //lv_ex_arc_1();
    //lv_ex_cpicker_1();
    //lv_ex_gauge_1();
    //lv_ex_img_1();
    //lv_ex_tileview_1();

    Uint32 start_ms = SDL_GetTicks();
    while (1) 
    {
        /* Periodically call the lv_task handler.
        * It could be done in a timer interrupt or an OS task too.*/
        lv_task_handler();
        Sleep(sleep_ms);       // depends on activity

        Uint32 curr_ms = SDL_GetTicks();
        Uint32 elapsed;
        if (curr_ms >= start_ms)
        {
            elapsed = curr_ms - start_ms;
        }
        else
        {
            elapsed = UINT32_MAX - start_ms + 1;
            elapsed += curr_ms;
        }
        lv_tick_inc(elapsed);
        start_ms = curr_ms;
    }

    return 0;
}

/**********************
*   STATIC FUNCTIONS
**********************/

/**
* A task to measure the elapsed time for LittlevGL
* @param data unused
* @return never return
*/
static int tick_thread(void *data)
{
    while (1) {
        lv_tick_inc(5);
        SDL_Delay(5);   /*Sleep for 1 millisecond*/
    }

    return 0;
}

static Uint32 my_tick_timer(Uint32 interval, void *param)
{
    lv_tick_inc(5);
    return 5; // next call
}

/**
* Initialize the Hardware Abstraction Layer (HAL) for the Littlev graphics library
*/
static void hal_init(void)
{
    /* Add a display
    * Use the 'monitor' driver which creates window on PC's monitor to simulate a display*/
    monitor_init();

    static lv_disp_buf_t disp_buf1;
    static lv_color_t buf1_1[LV_HOR_RES_MAX * 120];
    lv_disp_buf_init(&disp_buf1, buf1_1, NULL, LV_HOR_RES_MAX * 120);

    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.buffer = &disp_buf1;
    disp_drv.flush_cb = monitor_flush;
    lv_disp_drv_register(&disp_drv);

    /* Add the mouse (or touchpad) as input device
    * Use the 'mouse' driver which reads the PC's mouse*/
    mouse_init();
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);          /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read;         /*This function will be called periodically (by the library) to get the mouse position and state*/
    lv_indev_drv_register(&indev_drv);

    /* If the PC keyboard driver is enabled in`lv_drv_conf.h`
    * add this as an input device. It might be used in some examples. */
#if USE_KEYBOARD
    lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = keyboard_read;
    kb_indev = lv_indev_drv_register(&kb_drv);
#endif

    /* Tick init.
    * You have to call 'lv_tick_inc()' in every milliseconds
    * Create an SDL thread to do this*/
    //SDL_CreateThread(tick_thread, "tick", NULL);
    //SDL_AddTimer(5, my_tick_timer, NULL);
}

