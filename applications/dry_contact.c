/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-01-16     Rick       the first version
 */
#include "rtthread.h"
#include "rtdevice.h"
#include "pin_config.h"
#include "water_work.h"

#define DBG_TAG "dry"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

int dry_contact_valve_input_level = -1;
rt_timer_t dry_contact_valve_input_timer = RT_NULL;

uint8_t dry_contact_input_state[8] = {0};
uint8_t dry_contact_output_state[8] = {0};

void dry_contact_tech_alarm_set(uint8_t level)
{
    rt_pin_write(DRY_CONTACT_TECH_ALARM_PIN, !level);
    dry_contact_output_state[2] = level;
    led_dry_contact_set(3,level);
}

void dry_contact_leak_alarm_set(uint8_t level)
{
    rt_pin_write(DRY_CONTACT_LEAK_ALARM_PIN, !level);
    dry_contact_output_state[1] = level;
    led_dry_contact_set(4,level);
}

void dry_contact_valve_status_set(uint8_t level)
{
    rt_pin_write(DRY_CONTACT_VALVE_STATUS_PIN, !level);
    dry_contact_output_state[0] = level;
    led_dry_contact_set(5,level);
}

void dry_contact_polling_timer_callback(void *parameter)
{
    uint8_t level = rt_pin_read(DRY_CONTACT_VALVE_INPUT_PIN);
    led_dry_contact_set(6,!level);
    if(level != dry_contact_valve_input_level)
    {
        dry_contact_valve_input_level = level;
        dry_contact_input_state[0] = !level;
        valve_dry_contact_control(level);
    }

    switch(warning_status_get())
    {
    case ValveLeftFail:
    case ValveRightFail:
    case SlaverLowPower:
    case SlaverSensorLost:
    case SlaverUltraLowPower:
    case SlaverOffline:
        dry_contact_tech_alarm_set(1);
        dry_contact_leak_alarm_set(0);
        break;
    case SlaverSensorLeak:
        dry_contact_tech_alarm_set(0);
        dry_contact_leak_alarm_set(1);
        break;
    default:
        dry_contact_tech_alarm_set(0);
        dry_contact_leak_alarm_set(0);
        break;
    }

    dry_contact_valve_status_set(get_valve_status());
}

void dry_contact_init(void)
{
    rt_pin_mode(DRY_CONTACT_VALVE_INPUT_PIN,PIN_MODE_INPUT);
    rt_pin_mode(DRY_CONTACT_VALVE_STATUS_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(DRY_CONTACT_LEAK_ALARM_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(DRY_CONTACT_TECH_ALARM_PIN,PIN_MODE_OUTPUT);
    rt_pin_mode(DRY_CONTACT_SPARE_PIN,PIN_MODE_OUTPUT);

    rt_pin_write(DRY_CONTACT_VALVE_STATUS_PIN, PIN_HIGH);
    rt_pin_write(DRY_CONTACT_LEAK_ALARM_PIN, PIN_HIGH);
    rt_pin_write(DRY_CONTACT_TECH_ALARM_PIN, PIN_HIGH);
    rt_pin_write(DRY_CONTACT_SPARE_PIN, PIN_HIGH);

    dry_contact_valve_input_timer  = rt_timer_create("valve_input", dry_contact_polling_timer_callback, \
                                    RT_NULL, 200, RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(dry_contact_valve_input_timer);
}
