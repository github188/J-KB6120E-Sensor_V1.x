/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portserial.c,v 1.1 2006/08/22 21:35:13 wolti Exp $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRxEnable enable serial receive interrupts.
     * If xTxENable enable transmitter empty interrupts.
     */
	
	if ( xRxEnable )
	{
		SET_BIT( MB_USART->CR1, USART_CR1_RXNEIE );
	}
	else
	{
		CLEAR_BIT( MB_USART->CR1, USART_CR1_RXNEIE );
	}
	
	if ( xTxEnable )
	{
		//	允许发送，将总线切换到发送状态
		MB_485_Direct_Transmit();
		SET_BIT( MB_USART->CR1, USART_CR1_TXEIE );
	}
	else
	{
		CLEAR_BIT( MB_USART->CR1, USART_CR1_TXEIE );
		//	禁止发送，通过发送结束中断切换总线方向
		SET_BIT( MB_USART->CR1, USART_CR1_TCIE );
	}
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
	( void )ucPORT;

	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_USART1EN );
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	//	MODIFY_REG( AFIO->MAPR, AFIO_MAPR_USART3_REMAP, AFIO_MAPR_USART3_REMAP_NOREMAP );
	//	PB.2  USART3 DE, 3：(0011B)通用推挽输出模式50MHz
	//	PC.11 USART3 Rx, 4：(0100B)浮空输入模式(复位后的状态)
	//	PC.10 USART3 Tx, B：(1011B)复用功能推挽输出模式50MHz
	// 	MODIFY_REG( GPIOB->CRL, 0x00000F00u, 0x00000300u );
	// 	MODIFY_REG( GPIOB->CRH, 0x0000FF00u, 0x00004B00u );
	// 	MODIFY_REG( GPIOB->CRL, 0x00000F00u, 0x00000300u );
 	MODIFY_REG( GPIOA->CRH, 0x0000FF0u, 0x00004F0u );
	MB_485_Direct_Receive();
	MB_USART->CR1 = 0u;
	MB_USART->CR2 = 0u;
	MB_USART->CR3 = 0u;
	MB_USART->CR1 = USART_CR1_RE | USART_CR1_TE;
	MB_USART->BRR = ( SystemCoreClock / ulBaudRate );

	switch ( ucDataBits )
	{
	case 7:
		CLEAR_BIT( MB_USART->CR1, USART_CR1_M );
		break;
	case 8:
		SET_BIT( MB_USART->CR1, USART_CR1_M );
		break;
	}

	switch ( eParity )
	{
	case MB_PAR_NONE:		/*!< No parity. */
		CLEAR_BIT( MB_USART->CR1, USART_CR1_PCE );
		break;
	case MB_PAR_ODD:    	/*!< Odd parity. */
		SET_BIT( MB_USART->CR1, USART_CR1_PS );
		SET_BIT( MB_USART->CR1, USART_CR1_PCE );
		break;		
	case MB_PAR_EVEN:   	/*!< Even parity. */
		CLEAR_BIT( MB_USART->CR1, USART_CR1_PS );
		SET_BIT( MB_USART->CR1, USART_CR1_PCE );
		break;		
	}

	SET_BIT( MB_USART->CR1, USART_CR1_UE );		// Enable MB_USART

	NVIC_EnableIRQ( USART1_IRQn );

    return	TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
    /* Put a byte in the UARTs transmit buffer. This function is called
     * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
     * called. */
	MB_USART->DR = ucByte;

    return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */

	* pucByte = MB_USART->DR;	

    return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}
