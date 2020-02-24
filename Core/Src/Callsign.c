/* SPDX-License-Identifier: BSD-3-Clause
 * Author: Johannes Eigner, DH4NES
 *
 * This module is responsible for generating the call sign.
 *
 * This module will use Timer1 and Timer2!
 * TODO change timer 3 and 4. Do the repetition conter in sw.
 */

/* Includes ------------------------------------------------------------------*/
#include "Callsign.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables (constants)----------------------------------------------*/

// Table with cw symbol definitions. Only supporting A-Z and 0-9.
static const char cwTable[36][6] =
{
    "-----",    //0
    ".----",    //1
    "..---",    //2
    "...--",    //3
    "....-",    //4
    ".....",    //5
    "-....",    //6
    "--...",    //7
    "---..",    //8
    "----.",    //9
    ".-",       //A
    "-...",     //B
    "-.-.",     //C
    "-..",      //D
    ".",        //E
    "..-.",     //F
    "--.",      //G
    "....",     //H
    "..",       //I
    ".---",     //J
    "-.-",      //K
    ".-..",     //L
    "--",       //M
    "-.",       //N
    "---",      //O
    ".--.",     //P
    "--.-",     //Q
    ".-.",      //R
    "...",      //S
    "-",        //T
    "..-",      //U
    "...-",     //V
    ".--",      //W
    "-..-",     //X
    "-.--",     //Y
    "--.."      //Z
};

/* Private variables ---------------------------------------------------------*/

// Definition auto generated in main.c
extern TIM_HandleTypeDef htim1;

/* cw call index
 * needed for output generation
 */
static int cwIndex;

// #### Configuration ####
/* Calculated call
 * CALL123 LO12CA34
 * Call + Separator + Locator + End + \0
 * Call sign max 7 Char
 * Locator max 8 Char
 * CW max 5/char +1 space
 * => Max length = 7*6+1+8*6+2 = 93
 *
 * Definition of characters:
 * '.' Dot
 * '-' Dash
 * ' ' Character separator
 * '_' Word separator
 * 'E' End of transmission
 */
static char cwCall[93] = "";

/** GPIO Port, where x can be A..G depending on device used */
static GPIO_TypeDef *port;

/** GPIO Pin number */
static uint16_t pin;

/** Pin state for TX */
static GPIO_PinState txOn;

/** Pin state for no TX */
static GPIO_PinState txOff;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/**
  * @brief Finds index on cwTable for given ASCII character
  *
  * @param c: ASCII character
  *
  * @retval -1 on error, else the corresponding cwTable index
  */
int getCwTableIndex(char c)
{
    int i = -1;
    // check if it is a ASCII number
    if (c <= '9' )
    {
        i = c - '0';
    }
    // check if it is a ASCII capital alphabetic character
    else if (c <= 'Z')
    {
        i = c + 10 - 'A';  //Character start at index 10
    }
    // check if it is a ASCII lower case alphabetic character
    else if (c <= 'z')
    {
        i = c + 10 - 'a';  //Character start at index 10
    }
    if (i < 0 || i > 35)
        return -1;      //not a supported character
    else
        return i;
}

/**
  * @brief Append string in to string out
  *
  * @param  in:  String we want to add to the out string
  *
  * @param  out: The string to which we will append the in string
  *
  * @retval Number of characters appended
  */
int appendCwString(char *out, const char *in)
{
    int i = 0;
    while (*in != '\0')
    {
        // limit to 5 chars
        if (i++ > 5)
            return i-1;
        // copy char
        *++out = *in++;
    }
    return i;
}

/* Exported functions --------------------------------------------------------*/

int callInit(CallCfg *cfg)
{
    // set configuration parameters
    // TODO check port and pin from configuration
    port = cfg->GPIOx;
    pin = cfg->GPIO_Pin;

    if (GPIO_PIN_SET == cfg->TxActive)
    {
        txOn  = GPIO_PIN_SET;
        txOff = GPIO_PIN_RESET;
    }
    else
    {
        txOn  = GPIO_PIN_RESET;
        txOff = GPIO_PIN_SET;
    }

    // Calculate cw from call and locator
    int i = 0;
    int o = 0;
    // convert call sign
    while (cfg->callSign[i] != '\0')
    {
        // get cw from table
        int cti = getCwTableIndex(cfg->callSign[i]);
        if ( cti < 0 )
            return 1;   //not a supported character
        // append cw to table
        o += appendCwString( &cwCall[o], &cwTable[cti][0]);
        // append space
        cwCall[++o] = ' ';
        i++;
    }
    // in case a locator is given => change space to word separator
    if (cfg->locator[0] != '\0')
    {
        cwCall[o] = '_';
    }
    // convert locator
    i = 0;
    while (cfg->locator[i] != '\0')
    {
        // get cw from table
        int cti = getCwTableIndex(cfg->locator[i]);
        if ( cti < 0 )
            return 2;   //not a supported character
        // append cw to table
        o += appendCwString( &cwCall[o], &cwTable[cti][0]);
        // append space
        cwCall[++o] = ' ';
        i++;
    }
    // add end marking
    cwCall[o] = 'E';
    cwCall[++o] = '\0';

    // switch tx off as long we get no start
    HAL_GPIO_WritePin(port, pin, txOff);
    return 0;
}

void callStart(void)
{
    ;
}

void callStop(void)
{
    ;
}

void callBeginSign(void)
{
    TIM_TypeDef *TIMx = TIM1;

    // reset CW index
    cwIndex = 0;
    // set repetition counter
    // add 6 off cycles => total 7 cycles
    TIMx->RCR = 5;
    // switch of tx
    HAL_GPIO_WritePin(port, pin, txOff);
    // start timer
    HAL_TIM_Base_Start_IT(&htim1);
}

void callNextStep(void)
{
    /* cwIndex gets increased on each call. Will switch tx off on even numbers
     * and tx on on odd numbers. Through this we will have always one cycle off
     * before the sign.
     */
    TIM_TypeDef *TIMx = TIM1;

    //check if space needs to be send
    if (1 == cwIndex % 2)
    {
        // transmit a sign
        switch (cwCall[cwIndex/2])
        {
            case '.':
                // dot => one cycle on
                HAL_GPIO_WritePin(port, pin, txOn);
                TIMx->RCR = 0;
                break;
            case '-':
                // dash => three cycles on
                HAL_GPIO_WritePin(port, pin, txOn);
                TIMx->RCR = 2;
                break;
            case ' ':
                // character separator, add 1 off cycles => total 3 cycles off
                //HAL_GPIO_WritePin(port, pin, txOff);
                TIMx->RCR = 0;
                break;
            case '_':
                // word separator, add 5 off cycles => total 7 cycles off
                //HAL_GPIO_WritePin(port, pin, txOff);
                TIMx->RCR = 4;
                break;
            case 'E':
                // end of cw id, add 5 off cycles => total 7 cycles off
                //HAL_GPIO_WritePin(port, pin, txOff);
                TIMx->RCR = 4;
                break;
            case '\0':
                // end of call sign
                // switch TX on
                HAL_GPIO_WritePin(port, pin, txOn);
                // stop the timer
                HAL_TIM_Base_Stop(&htim1);
                break;
            default:
                // something went wrong => stop cw
                cwIndex = 0;
                HAL_TIM_Base_Stop(&htim1);
                break;
        }
    }
    else
    {
        // separator between dot or dash => one cycle off
        HAL_GPIO_WritePin(port, pin, txOff);
        TIMx->RCR = 0;
    }
    cwIndex++;
}
