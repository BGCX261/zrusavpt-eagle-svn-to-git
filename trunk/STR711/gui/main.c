/******************** (C) COPYRIGHT 2007 STMicroelectronics ********************
* File Name          : main.c
* Author             : MCD Application Team
* Version            : V4.0
* Date               : 10/09/2007
* Description        : Main program body
********************************************************************************
 THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS WITH
 CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT
 OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "71x_lib.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static void delay(void)
{
	volatile int i, j;
	j = 0;

	while(j<3) {
		for(i=0; i<0xFFF;i++);
		j++;
	}
}

/*******************************************************************************
* Function Name  : main
* Description    : Main program
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void main(void) {

#ifdef DEBUG
    debug();
#endif

	// P1.8 as TTL output
	GPIO_Config( GPIO1, (1<<8), GPIO_OUT_PP );

  	while(1) {
  		GPIO_BitWrite( GPIO1, 8, !GPIO_BitRead( GPIO1, 8 ) );
   		delay();
    }
}

/******************* (C) COPYRIGHT 2007 STMicroelectronics *****END OF FILE****/
