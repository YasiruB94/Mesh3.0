/**
 ******************************************************************************
 *	Copyright (c) 2024 CencePower Inc
 ******************************************************************************
 * @file	: status_led.h
 * @author	: Yasiru Benaragama
 * @date	: 16 Feb 2024
 * @brief	: functions used in controlling the lights of the GW with the 
 * LED driver
 ******************************************************************************
 *
 ******************************************************************************
 */

#if defined(IPNODE) || defined(GATEWAY_ETH) || defined(GATEWAY_SIM7080)
#ifndef STATUS_LED_H
#define STATUS_LED_H
#include "includes/SpacrGateway_commands.h"
#ifdef GATEWAY_ETH
#include "gw_includes/I2C_comm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/timers.h"
#include <math.h>
#endif


#define PCA9685_RESPONSE_TIMEOUT_GENERIC    10

#ifdef USING_OLD_GW
#define LED_I2C_ADDRESS                     0x40    /*!< lave address for PCA9685 */
#else
#define LED_I2C_ADDRESS                     0x47    /*!< lave address for PCA9685 */
#endif

#define ACK_CHECK_EN                        0x1     /*!< I2C master will check ack from slave */
#define ACK_CHECK_DIS                       0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL                             0x0     /*!< I2C ack value */
#define NACK_VAL                            0x1     /*!< I2C nack value */

#define MODE1                               0x00    /*!< Mode register 1 */
#define MODE2                               0x01    /*!< Mode register 2 */
#define LED0_ON_L                           0x6     /*!< LED0 output and brightness control byte 0 */
#define LED0_ON_H                           0x7     /*!< LED0 output and brightness control byte 1 */
#define LED0_OFF_L                          0x8     /*!< LED0 output and brightness control byte 2 */
#define LED0_OFF_H                          0x9     /*!< LED0 output and brightness control byte 3 */
#define LED_MULTIPLYER                      4       /*!< For the other 15 channels */
#define ALLLED_ON_L                         0xFA    /*!< load all the LEDn_ON registers, byte 0 (turn 0-7 channels on) */
#define ALLLED_ON_H                         0xFB    /*!< load all the LEDn_ON registers, byte 1 (turn 8-15 channels on) */
#define ALLLED_OFF_L                        0xFC    /*!< load all the LEDn_OFF registers, byte 0 (turn 0-7 channels off) */
#define ALLLED_OFF_H                        0xFD    /*!< load all the LEDn_OFF registers, byte 1 (turn 8-15 channels off) */
#define PRE_SCALE                           0xFE    /*!< prescaler for output frequency */
#define CLOCK_FREQ                          25000000.0  /*!< 25MHz default osc clock */

#define LED_RED_1                           2U
#define LED_RED_2                           6U
#define LED_RED_3                           8U
#define LED_GREEN_1                         0U
#define LED_GREEN_2                         4U
#define LED_GREEN_3                         9U
#define LED_BLUE_1                          1U
#define LED_BLUE_2                          5U
#define LED_BLUE_3                          10U

#define LED_MAX_VAL                         4096U
#define LED_RED_MAX_VAL                     2500U
#define LED_GREEN_MAX_VAL                   1800U
#define LED_BLUE_MAX_VAL                    2500U

#define LED_MAX_STAGES                      8
#define LED_CHANGE_MOMENTARY_DURATION       300
#define LED_CHANGE_RAPID_DURATION           50
#define LED_CHANGE_EXTENDED_DURATION        5000

typedef enum CNGW_LED_Command
{
    CNGW_LED_CMD_No_Action  = 0,
    CNGW_LED_CMD_ALL_OFF,
    CNGW_LED_CMD_FW_UPDATE_PRE_PREPARATION,
    CNGW_LED_CMD_FW_UPDATE,
    CNGW_LED_CMD_CONN_PENDING,
    CNGW_LED_CMD_IDLE,
    CNGW_LED_CMD_BUSY,
    CNGW_LED_CMD_ERROR,
    CNGW_LED_CMD_CONN_STAGE_01,
    CNGW_LED_CMD_CONN_STAGE_02,

} __attribute__((packed)) CNGW_LED_Command;

typedef enum CNGW_LED_Type
{
    CNGW_LED_NONE = 0,
    CNGW_LED_GEN,
    CNGW_LED_CN,
    CNGW_LED_COMM,
    CNGW_LED_ALL,

} __attribute__((packed)) CNGW_LED_Type;

typedef struct GW_LED_Param_t
{
    uint8_t             LED_RED_ADDR;
    uint8_t             LED_GREEN_ADDR;
    uint8_t             LED_BLUE_ADDR;
    CNGW_LED_Command    previous_command;
    CNGW_LED_Command    current_command;
    bool                RED[LED_MAX_STAGES];
    bool                GREEN[LED_MAX_STAGES];
    bool                BLUE[LED_MAX_STAGES];
    bool                RED_P;
    bool                GREEN_P;
    bool                BLUE_P;
    bool                momentary_task_busy;

} __attribute__((packed)) GW_LED_Param_t;


typedef struct GW_LED_t
{
    GW_LED_Param_t      LED_ARRAY[LED_MAX_STAGES];
    uint16_t            delay_between_stages;
    uint8_t             current_state;

} __attribute__((packed)) GW_LED_t;



esp_err_t init_status_LEDs();
esp_err_t PCA9685_Reset(void);
esp_err_t PCA9685_Generic_write_i2c_register_two_words(uint8_t regaddr, uint16_t valueOn, uint16_t valueOff);
esp_err_t PCA9685_Generic_write_i2c_register_word(uint8_t regaddr, uint16_t value);
esp_err_t PCA9685_Generic_write_i2c_register(uint8_t regaddr, uint8_t value);
esp_err_t PCA9685_Generic_read_two_i2c_register(uint8_t regaddr, uint8_t* valueA, uint8_t* valueB);
esp_err_t PCA9685_Generic_read_i2c_register_word(uint8_t regaddr, uint16_t* value);
esp_err_t PCA9685_setFrequency(uint16_t freq);
esp_err_t PCA9685_Set_PWM(uint8_t num, uint16_t on, uint16_t off);
esp_err_t PCA9685_Turn_all_LEDs_off(void);
esp_err_t LED_change_task_momentarily(CNGW_LED_Command command, CNGW_LED_Type target, uint16_t delay);
void LED_Main_Loop_task(void *pvParameters);
esp_err_t LED_assign_task(CNGW_LED_Command command, CNGW_LED_Type target);

#endif
#endif