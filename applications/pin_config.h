/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-08-06     Rick       the first version
 */

#ifndef APPLICATIONS_PIN_CONFIG_H_
#define APPLICATIONS_PIN_CONFIG_H_
/*
 * RF
 */
#define RF_SW1_PIN                      6
#define RF_SW2_PIN                      7
#define TCXO_PWR_PIN                    16

/*
 * MOTO
 */
#define MOTO_LEFT_FULLY_OPEN_PIN        28
#define MOTO_LEFT_FULLY_CLOSE_PIN       11
#define MOTO_LEFT_CONTROL_PIN           18
#define MOTO_RIGHT_FULLY_OPEN_PIN       13
#define MOTO_RIGHT_FULLY_CLOSE_PIN      14
#define MOTO_RIGHT_CONTROL_PIN          12

/*
 * KEY
 */
#define KEY_ON_PIN                      15
#define KEY_OFF_PIN                     45

/*
 * BUZZER
 */
#define BEEP_PIN                        23

/*
 * DRY_CONTACT
 */
#define DRY_CONTACT_VALVE_INPUT_PIN     4
#define DRY_CONTACT_VALVE_STATUS_PIN    1
#define DRY_CONTACT_LEAK_ALARM_PIN      0
#define DRY_CONTACT_TECH_ALARM_PIN      24
#define DRY_CONTACT_SPARE_PIN           3


#endif /* APPLICATIONS_PIN_CONFIG_H_ */
