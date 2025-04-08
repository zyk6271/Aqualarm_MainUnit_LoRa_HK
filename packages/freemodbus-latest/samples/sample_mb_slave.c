/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-06-21     flybreak     first version
 */

#include <rtthread.h>
#include <rtdevice.h>

#include "mb.h"
#include "user_mb_app.h"
#include "water_work.h"
#include "pin_config.h"

#ifdef PKG_MODBUS_SLAVE_SAMPLE
#define SLAVE_ADDR      MB_SAMPLE_SLAVE_ADDR
#define PORT_NUM        MB_SLAVE_USING_PORT_NUM
#define PORT_BAUDRATE   MB_SLAVE_USING_PORT_BAUDRATE
#else
#define SLAVE_ADDR      0x01
#define PORT_NUM        2
#define PORT_BAUDRATE   115200
#endif

#define PORT_PARITY     MB_PAR_NONE

#define MB_POLL_THREAD_PRIORITY  10
#define MB_SEND_THREAD_PRIORITY  RT_THREAD_PRIORITY_MAX - 1

#define MB_POLL_CYCLE_MS 20

extern UCHAR  ucSDiscInBuf[S_DISCRETE_INPUT_NDISCRETES/8];
extern USHORT usSRegHoldBuf[S_REG_HOLDING_NREGS];
extern USHORT usSRegInBuf[S_REG_INPUT_NREGS];
extern UCHAR  ucSCoilBuf[S_COIL_NCOILS/8];

#define BIT_GET(x, bit) ((x & (1 << bit)) >> bit)    /* 获取第bit位 */
#define BIT_CLEAR(x, bit) (x &= ~(1 << bit))         /* 清零第bit位 */
#define BIT_SET(x, bit) (x |= (1 << bit))            /* 置位第bit位 */
#define WRITE_BIT(number,pos,value) (value > 0)?(BIT_SET(number,pos)):(BIT_CLEAR(number,pos))

extern uint8_t mb_valve_control_state[2];
extern uint8_t mb_valve_check_state[2];
extern uint8_t dry_contact_input_state[8];
extern uint8_t dry_contact_output_state[8];

void mb_valve_data_copy(void)
{
    WRITE_BIT(ucSDiscInBuf[2], 0, mb_valve_control_state[0]);
    WRITE_BIT(ucSDiscInBuf[2], 1, mb_valve_control_state[1]);

    WRITE_BIT(ucSDiscInBuf[3], 0, rt_pin_read(MOTO_LEFT_FULLY_OPEN_PIN));
    WRITE_BIT(ucSDiscInBuf[3], 1, rt_pin_read(MOTO_RIGHT_FULLY_OPEN_PIN));
    WRITE_BIT(ucSDiscInBuf[4], 0, rt_pin_read(MOTO_LEFT_FULLY_CLOSE_PIN));
    WRITE_BIT(ucSDiscInBuf[4], 1, rt_pin_read(MOTO_RIGHT_FULLY_CLOSE_PIN));
}

void mb_input_dry_contact_data_copy(void)
{
    WRITE_BIT(ucSDiscInBuf[0], 0, dry_contact_input_state[0]);
}

void mb_output_dry_contact_data_copy(void)
{
    for(uint8_t i = 0; i < 3; i++)
    {
        WRITE_BIT(ucSDiscInBuf[1], i, dry_contact_output_state[i]);
    }
}

void mb_device_status_data_copy(void)
{
    usSRegHoldBuf[0] = device_status_get();
}

void mb_adc_value_data_copy(void)
{
    usSRegInBuf[0] = 0;
}

void mb_coils_data_copy(void)
{
    WRITE_BIT(ucSCoilBuf[0], 0, get_valve_status());
}

void mb_holding_data_copy(void)
{
    usSRegHoldBuf[0] = warning_status_get();
}

void mb_valve_single_control_write_callback(void)
{
    if(BIT_GET(ucSCoilBuf[0],0))
    {
        key_on_click_handle();
    }
    else
    {
        key_off_click_handle();
    }
}

void mb_holding_write_callback(void)
{
    key_off_click_handle();
}

static void mb_slave_poll(void *parameter)
{
    eMBInit(MB_RTU, SLAVE_ADDR, PORT_NUM, PORT_BAUDRATE, PORT_PARITY);
    eMBEnable();
    while (1)
    {
        mb_valve_data_copy();
        mb_input_dry_contact_data_copy();
        mb_output_dry_contact_data_copy();
        mb_adc_value_data_copy();
        mb_coils_data_copy();
        mb_holding_data_copy();
        eMBPoll();
        rt_thread_mdelay(MB_POLL_CYCLE_MS);
    }
}

void mb_slave_start(void)
{
    rt_thread_t tid1 = RT_NULL;

    tid1 = rt_thread_create("md_s_poll", mb_slave_poll, "RTU", 1024, MB_POLL_THREAD_PRIORITY, 10);
    rt_thread_startup(tid1);
}
