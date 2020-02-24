/* SPDX-License-Identifier: BSD-3-Clause
 * Author: Johannes Eigner, DH4NES
 *
 * This module is responsible for generating the call sign.
 *
 * This module will use Timer1 and Timer2!
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef INC_CALLSIGN_H_
#define INC_CALLSIGN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Private includes ----------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    char          callSign[8]; //!< Call sign as string
    char          locator[9];  //!< Locator as string, leave empty to disable
    GPIO_TypeDef  *GPIOx;      /*!< GPIO Port, where x can be A..G depending on
                                    device used */
    uint16_t      GPIO_Pin;    /*!< GPIO Pin number. Should be a GPIO_PIN_x
                                    where x can be 0..15. */
    GPIO_PinState TxActive;    /*!< Pin state for active TX. Can be:
                                    GPIO_PIN_SET   (TX on Pin high)
                                    GPIO_PIN_RESET (TX on Pin low) */
    // TODO Add CW Speed and cycle time
} CallCfg;

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Initializes the call sign generator.
  *
  * @param  configuration: Configuration struct.
  *
  * @retval 0: on Success
  * TODO define error codes
  */
int callInit(CallCfg *cfg);

/**
  * @brief  Starts the continuous call sign generation.
  *
  * @retval None
  */
void callStart(void);

/**
  * @brief  Stops the continuous call sign generation.
  *
  * @retval None
  */
void callStop(void);

/**
  * @brief  First cycle of call sign output
  *
  * @note   This function gets called from timer interrupts. Do not use this
  *         function by your own.
  *
  * @retval None
  */
void callBeginSign(void);

/**
  * @brief  Start next cycle of call sign output
  *
  * @note   This function gets called from timer interrupts. Do not use this
  *         function by your own.
  *
  * @retval None
  */
void callNextStep(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_CALLSIGN_H_ */
