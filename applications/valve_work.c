/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-11-27     Rick       the first version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "led.h"
#include "flashwork.h"
#include "water_work.h"

#define DBG_TAG "valve"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

uint8_t valve_status = 0;
uint8_t lock_status = 0;
uint8_t valve_valid = 1;
uint8_t valve_left_check_result = 0;    //0:normal,1:warning
uint8_t valve_right_check_result = 0;    //0:normal,1:warning

#define VALVE_STATUS_CLOSE   0
#define VALVE_STATUS_OPEN    1

rt_timer_t valve_delay_close_timer = RT_NULL;
rt_timer_t valve_detect_once_timer = RT_NULL;
rt_timer_t valve_left_turn_back_timer = RT_NULL;
rt_timer_t valve_right_turn_back_timer = RT_NULL;
rt_timer_t valve_left_turn_check_timer = RT_NULL;
rt_timer_t valve_right_turn_check_timer = RT_NULL;

extern enum Device_Status DeviceStatus;
extern WariningEvent ValveLeftFailEvent;
extern WariningEvent ValveRightFailEvent;
extern WariningEvent SlaverSensorLeakEvent;

uint8_t mb_valve_control_state[2] = {0};//control
uint8_t mb_valve_check_state[2] = {0};//check

void valve_single_control(uint8_t id,uint8_t level)
{
    if(id == 0)
    {
        mb_valve_control_state[0] = level;
        rt_pin_write(MOTO_LEFT_CONTROL_PIN,!level);
    }
    else
    {
        mb_valve_control_state[1] = level;
        rt_pin_write(MOTO_RIGHT_CONTROL_PIN,!level);
    }
    LOG_I("valve_single_control,ID %d,value %d",id,level);
}

void valve_turn_control(int dir)
{
    if(dir < 0)
    {
        valve_status = VALVE_STATUS_CLOSE;
        mb_valve_control_state[0] = 0;
        rt_pin_write(MOTO_LEFT_CONTROL_PIN,PIN_HIGH);
        mb_valve_control_state[1] = 0;
        rt_pin_write(MOTO_RIGHT_CONTROL_PIN,PIN_HIGH);
    }
    else
    {
        valve_status = VALVE_STATUS_OPEN;
        mb_valve_control_state[0] = 1;
        rt_pin_write(MOTO_LEFT_CONTROL_PIN,PIN_LOW);
        mb_valve_control_state[1] = 1;
        rt_pin_write(MOTO_RIGHT_CONTROL_PIN,PIN_LOW);
    }
}

rt_err_t valve_open(void)
{
    if(lock_status == 1 || valve_valid == 0)
    {
        led_valve_fail();
        return RT_ERROR;
    }

    DeviceStatus = ValveOpen;
    led_valve_on();
    beep_once();
    valve_turn_control(1);

    rt_timer_stop(valve_left_turn_back_timer);
    rt_timer_stop(valve_right_turn_back_timer);
    rt_timer_stop(valve_left_turn_check_timer);
    rt_timer_stop(valve_right_turn_check_timer);
    rt_timer_stop(valve_delay_close_timer);
    rt_timer_start(valve_detect_once_timer);

    return RT_EOK;
}

rt_err_t valve_close(void)
{
    if(valve_valid == 0)
    {
        led_valve_fail();
        valve_turn_control(-1);
        return RT_ERROR;
    }

    DeviceStatus = ValveClose;
    led_valve_off();
    beep_key_down();
    valve_turn_control(-1);

    rt_timer_stop(valve_left_turn_back_timer);
    rt_timer_stop(valve_right_turn_back_timer);
    rt_timer_stop(valve_left_turn_check_timer);
    rt_timer_stop(valve_right_turn_check_timer);
    rt_timer_stop(valve_delay_close_timer);
    rt_timer_stop(valve_detect_once_timer);

    return RT_EOK;
}

void valve_lock(void)
{
    if(lock_status == 0)
    {
        lock_status = 1;
        flash_set_key("valve_lock",1);
    }
}

void valve_unlock(void)
{
    if(lock_status == 1)
    {
        lock_status = 0;
        flash_set_key("valve_lock",0);
    }
}

uint8_t get_valve_lock(void)
{
    return lock_status;
}

uint8_t get_valve_status(void)
{
    return valve_status;
}

void valve_check_start(void)
{
    if(valve_status == VALVE_STATUS_CLOSE)
    {
        LOG_D("valve_check fail,because valve is close");
        return;
    }

    if(rt_pin_read(MOTO_LEFT_FULLY_OPEN_PIN) == 0)
    {
        rt_timer_stop(valve_left_turn_back_timer);
        rt_timer_stop(valve_left_turn_check_timer);
        valve_left_check_result = 0;
        mb_valve_check_state[0] = 1;
        rt_pin_write(MOTO_LEFT_CONTROL_PIN,PIN_HIGH);
        rt_timer_start(valve_left_turn_back_timer);
        LOG_D("valve_left_turn_check start");
    }

    if(rt_pin_read(MOTO_RIGHT_FULLY_OPEN_PIN) == 0)
    {
        rt_timer_stop(valve_right_turn_back_timer);
        rt_timer_stop(valve_right_turn_check_timer);
        valve_right_check_result = 0;
        mb_valve_check_state[1] = 1;
        rt_pin_write(MOTO_RIGHT_CONTROL_PIN,PIN_HIGH);
        rt_timer_start(valve_right_turn_back_timer);
        LOG_D("valve_right_turn_check start");
    }
}

void valve_detect_once_timer_callback(void *parameter)
{
    valve_check_start();
}

void valve_delay_close_timer_callback(void *parameter)
{
    if(DeviceStatus == ValveClose || DeviceStatus == ValveOpen || DeviceStatus == SlaverSensorLost)
    {
        valve_lock();
        valve_close();
        gateway_control_master_control(0);
    }
}

void valve_delay_control(uint8_t value)
{
    if(value)
    {
        rt_timer_start(valve_delay_close_timer);
    }
    else
    {
        rt_timer_stop(valve_delay_close_timer);
    }
}

void valve_dry_contact_control(uint8_t level)
{
    if(DeviceStatus == ValveClose || DeviceStatus == ValveOpen || DeviceStatus == SlaverSensorLost)
    {
        if(level)
        {
            if(valve_close() == RT_EOK)//valve close success
            {
                gateway_control_master_control(0);
            }
        }
        else
        {
            if(valve_open() == RT_EOK)//valve open success
            {
                gateway_control_master_control(1);
            }
        }
    }
}

void valve_left_turn_back_timer_callback(void *parameter)
{
    if(rt_pin_read(MOTO_LEFT_FULLY_OPEN_PIN) == 1)
    {
        rt_pin_write(MOTO_LEFT_CONTROL_PIN,PIN_LOW);
        mb_valve_control_state[0] = 1;
        rt_timer_start(valve_left_turn_check_timer);
        LOG_D("valve_left_turn_check stage 1 success");
    }
    else
    {
        valve_valid = 0;
        valve_left_check_result = 1;
        mb_valve_check_state[0] = 0;
        warning_enable(ValveLeftFailEvent);
        LOG_E("valve_left_turn_check stage 1 fail");
    }
}

void valve_right_turn_back_timer_callback(void *parameter)
{
    if(rt_pin_read(MOTO_RIGHT_FULLY_OPEN_PIN) == 1)
    {
        rt_pin_write(MOTO_RIGHT_CONTROL_PIN,PIN_LOW);
        mb_valve_control_state[1] = 1;
        rt_timer_start(valve_right_turn_check_timer);
        LOG_D("valve_right_turn_check stage 1 success");
    }
    else
    {
        valve_valid = 0;
        valve_right_check_result = 1;
        mb_valve_check_state[1] = 0;
        warning_enable(ValveRightFailEvent);
        LOG_E("valve_right_turn_check stage 1 fail");
    }
}

void valve_left_turn_check_timer_callback(void *parameter)
{
    mb_valve_check_state[0] = 0;
    if(rt_pin_read(MOTO_LEFT_FULLY_OPEN_PIN) == 0)
    {
        if(valve_right_check_result == 0)
        {
            valve_valid = 1;
            valvefail_warning_disable();
        }
        valve_left_check_result = 0;
        gateway_warning_master_valve_check(1);
        LOG_D("valve_left_turn_check stage 2 success");
    }
    else
    {
        valve_valid = 0;
        valve_left_check_result = 1;
        warning_enable(ValveLeftFailEvent);
        LOG_E("valve_left_turn_check stage 2 fail");
    }
}

void valve_right_turn_check_timer_callback(void *parameter)
{
    mb_valve_check_state[1] = 0;
    if(rt_pin_read(MOTO_RIGHT_FULLY_OPEN_PIN) == 0)
    {
        if(valve_left_check_result == 0)
        {
            valve_valid = 1;
            valvefail_warning_disable();
        }
        valve_right_check_result = 0;
        gateway_warning_master_valve_check(2);
        LOG_D("valve_right_turn_check stage 2 success");
    }
    else
    {
        valve_valid = 0;
        valve_right_check_result = 1;
        warning_enable(ValveRightFailEvent);
        LOG_E("valve_right_turn_check stage 2 fail");
    }
}

void valve_init(void)
{
    lock_status = flash_get_key("valve_lock");

    rt_pin_mode(MOTO_LEFT_CONTROL_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_LEFT_FULLY_OPEN_PIN,PIN_MODE_INPUT);
    rt_pin_mode(MOTO_LEFT_FULLY_CLOSE_PIN,PIN_MODE_INPUT);
    rt_pin_mode(MOTO_RIGHT_CONTROL_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(MOTO_RIGHT_FULLY_OPEN_PIN,PIN_MODE_INPUT);
    rt_pin_mode(MOTO_RIGHT_FULLY_CLOSE_PIN,PIN_MODE_INPUT);
    rt_pin_write(MOTO_LEFT_CONTROL_PIN,PIN_HIGH);
    rt_pin_write(MOTO_RIGHT_CONTROL_PIN,PIN_HIGH);

    valve_detect_once_timer  = rt_timer_create("valve_detect", valve_detect_once_timer_callback, RT_NULL, 60*1000*5, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    valve_delay_close_timer = rt_timer_create("valve_delay_close_timer", valve_delay_close_timer_callback, RT_NULL, 4*60*60*1000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    valve_left_turn_back_timer = rt_timer_create("left_turn_back", valve_left_turn_back_timer_callback, RT_NULL, 3000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    valve_right_turn_back_timer = rt_timer_create("right_turn_back", valve_right_turn_back_timer_callback, RT_NULL, 3000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    valve_left_turn_check_timer = rt_timer_create("left_turn_check", valve_left_turn_check_timer_callback, RT_NULL, 9000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);
    valve_right_turn_check_timer = rt_timer_create("right_turn_check", valve_right_turn_check_timer_callback, RT_NULL, 10000, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);

    if(aq_device_waterleak_find())
    {
        warning_enable(SlaverSensorLeakEvent);
    }
}
