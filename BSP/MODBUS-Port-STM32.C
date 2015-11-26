#include "BSP.H"
#include "Pin.H"

#include "mb.h"
#include "mbport.h"
#include "mbutils.h"

#define	MB_USART	USART1
#define	MB_TIMER	TIM4

#include "portEvent.c"
#include "portSerial.c"
#include "portTimer.c"

/* ----------------------- Defines ------------------------------------------*/
#define REG_INPUT_START 	1
#define REG_INPUT_NREGS	 	40
USHORT	usRegInputBuf[REG_INPUT_NREGS];

#define	REG_HOLDING_START	1
#define	REG_HOLDING_NREGS	40
USHORT	usRegHoldingBuf[REG_HOLDING_NREGS];

#define	REG_DISC_START		1
#define	REG_DISC_SIZE		40
UCHAR	ucRegDiscBuf[(REG_DISC_SIZE+7U)/8U];

#define	REG_COILS_START		1
#define	REG_COILS_SIZE		40
UCHAR	ucRegCoilsBuf[(REG_COILS_SIZE+7U)/8U];

#define	REG_NVRAM_START		1001
#define	REG_NVRAM_NREGS		1000

/* ----------------------- Static variables ---------------------------------*/

//	读数字寄存器 功能码0x04
eMBErrorCode
eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int16_t			iRegIndex;

    if( ( usAddress >= REG_INPUT_START )
        && ( usAddress + usNRegs <= REG_INPUT_START + REG_INPUT_NREGS ) )
    {
        iRegIndex = (int16_t)( usAddress - REG_INPUT_START );
        while( usNRegs > 0 )
        {
            *pucRegBuffer++ = (uint8_t)( usRegInputBuf[iRegIndex] >> 8 );
            *pucRegBuffer++ = (uint8_t)( usRegInputBuf[iRegIndex] & 0xFF );
            iRegIndex++;
            usNRegs--;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }

    return eStatus;
}

// 寄存器的读写函数 支持的命令为读 0x03 和写0x06
eMBErrorCode
eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int16_t			iRegIndex;

    if( ( usAddress >= REG_HOLDING_START ) && ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
	{	//	访问的地址空间是RAM映像区
        iRegIndex = (int16_t)( usAddress - REG_HOLDING_START );
        switch ( eMode )
        {
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
 				*pucRegBuffer++ = (uint8_t)( usRegHoldingBuf[iRegIndex] >> 8 );
				*pucRegBuffer++ = (uint8_t)( usRegHoldingBuf[iRegIndex] & 0xFF );
				iRegIndex++;
                usNRegs--;
            }
            break;

        case MB_REG_WRITE:
            while( usNRegs > 0 )
            {
				usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
				usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
				iRegIndex++;
                usNRegs--;
            }
        }
    }
   else if( ( usAddress >= REG_NVRAM_START ) && ( usAddress + usNRegs <= REG_NVRAM_START + REG_NVRAM_NREGS ) )
	{	//	访问的地址空间是 NVRAM 区
        iRegIndex = ( int )( usAddress - REG_NVRAM_START );
        switch ( eMode )
        {
        case MB_REG_READ:
//             while( usNRegs > 0 )
//             {
//  				*pucRegBuffer++ = ( unsigned char )( usRegHoldingBuf[iRegIndex] >> 8 );
// 				*pucRegBuffer++ = ( unsigned char )( usRegHoldingBuf[iRegIndex] & 0xFF );
// 				iRegIndex++;
//                 usNRegs--;
//             }
			NVRAM_Load( iRegIndex * 2u, pucRegBuffer, usNRegs * 2u );
            break;

        case MB_REG_WRITE:
			NVRAM_Save( iRegIndex * 2u, pucRegBuffer, usNRegs * 2u );
// 			while( usNRegs > 0 )
//             {
// 				usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
// 				usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
// 				iRegIndex++;
//                 usNRegs--;
//             }
        }
	}
     else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

//	读/写开关寄存器  0x01  0x05
eMBErrorCode
eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNCoils, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int16_t			iNCoils = (int16_t)usNCoils;
    uint16_t		usBitOffset;

    /* Check if we have registers mapped at this block. */
    if( ( usAddress >= REG_COILS_START ) &&
        ( usAddress + usNCoils <= REG_COILS_START + REG_COILS_SIZE ) )
    {
        usBitOffset = (uint16_t)( usAddress - REG_COILS_START );
        switch ( eMode )
        {
                /* Read current values and pass to protocol stack. */
            case MB_REG_READ:
                while( iNCoils > 0 )
                {
                    *pucRegBuffer++ = xMBUtilGetBits( ucRegCoilsBuf, usBitOffset, (uint8_t)( iNCoils > 8 ? 8 : iNCoils ));
                    iNCoils -= 8;
                    usBitOffset += 8;
                }
                break;

                /* Update current register values. */
            case MB_REG_WRITE:
                while( iNCoils > 0 )
                {
                    xMBUtilSetBits( ucRegCoilsBuf, usBitOffset, (uint8_t)( iNCoils > 8 ? 8 : iNCoils ), *pucRegBuffer++ );
                    iNCoils -= 8;
                }
                break;
        }

    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}

//	读开关寄存器 0x02
eMBErrorCode
eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int16_t			iNDiscrete = ( int16_t )usNDiscrete;
    uint16_t		usBitOffset;

    /* Check if we have registers mapped at this block. */
    if( ( usAddress >= REG_DISC_START ) &&
        ( usAddress + usNDiscrete <= REG_DISC_START + REG_DISC_SIZE ) )
    {
        usBitOffset = ( uint16_t )( usAddress - REG_DISC_START );
        while( iNDiscrete > 0 )
        {
            *pucRegBuffer++ = xMBUtilGetBits( ucRegDiscBuf, usBitOffset, (uint8_t)( iNDiscrete > 8 ? 8 : iNDiscrete ) );
            iNDiscrete -= 8;
            usBitOffset += 8;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}


//	初始化MODBUS
void	MODBUS_Init( uint8_t MBAddress )
{
	eMBInit( MB_RTU, MBAddress, 1, 9600, MB_PAR_EVEN );	//	初始化 FreeModbus 为RTU模式 从机地址为1 Uart1 9600 无校验
	eMBEnable();										//	启动   FreeModbus
}

//	PendSV中断
__irq
void
PendSV_Handler ( void )
{
	eMBPoll();
}

//	串口中断
__irq
void
USART1_IRQHandler( void )
{
	if ( READ_BIT( MB_USART->CR1, USART_CR1_RXNEIE ))
	{
		if ( READ_BIT( MB_USART->SR, USART_SR_RXNE ))
		{
			//	接收数据处理
			prvvUARTRxISR();
		}
	}

	if ( READ_BIT( MB_USART->CR1, USART_CR1_TXEIE ))
	{
		if ( READ_BIT( MB_USART->SR, USART_SR_TXE ))
		{
			//	发送数据处理
			prvvUARTTxReadyISR();
		}
	}

	//	发送完成，将总线切换到接收状态
	if ( READ_BIT( MB_USART->CR1, USART_CR1_TCIE ))
	{
		if ( READ_BIT( MB_USART->SR, USART_SR_TC ))
		{
			CLEAR_BIT( MB_USART->CR1, USART_CR1_TCIE );
			MB_485_Direct_Receive();
		}
	}
}

//	定时器中断
__irq
void
TIM4_IRQHandler( void )
{
	if ( READ_BIT( MB_TIMER->SR, TIM_SR_UIF ))
	{
		/* Clear TIM4 Capture Compare1 interrupt pending bit*/
		CLEAR_BIT( MB_TIMER->SR, TIM_SR_UIF );
		prvvTIMERExpiredISR( );
	}
}

/********  (C) COPYRIGHT 2012 青岛金仕达电子科技有限公司  **** End Of File ****/
