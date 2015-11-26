/**************** (C) COPYRIGHT 2015 青岛金仕达电子科技有限公司 ****************
* 文 件 名: SENSOR.C
* 创 建 者: 董峰
* 描  述  : 读取传感器读数
*         : 
* 最后修改: 2015年7月10日
*********************************** 修订记录 ***********************************
* 版  本: 
* 修订人: 
*******************************************************************************/

#include "BSP.H"
#include <math.h>

static	FP32	_CV_NTC10K( uint16_t ADx )
{
#define	B	3380.f
#define	K	273.15f
#define	T0	25.0f
#define	ADC_Max	4096.0f

	FP32	Tx;
	
	if ( ADx <= 100u )
	{
		Tx = -50.0f;	//	-48.2
	}
	else if ( ADx >= 4000u )
	{
		Tx = 180.0f;	//	171.2
	}
	else
	{
		FP32	RxDivR0 = (FP32)(ADx ) / (FP32)( 4096u - ADx);//2015.11.16--6120A新板子
		Tx = B / ( logf( RxDivR0 ) + ( B / ( K + T0 ))) - K;
	}
	
	return	Tx;
}




uint16_t	get_VDD_Volt( void )
{
	uint16_t	ADCRef = STM32_ADC1_Readout( 0u );
	
	if ( 0u == ADCRef )
	{
		return	0xFFFFu;
	}

	return	1200u * 4096u / ADCRef;
}

int16_t	get_CPU_Temp( void )
{
	uint16_t	ADCRef, ADCResult;
	int32_t		Volt;
	int16_t		Temp;
	
	ADCRef    = STM32_ADC1_Readout( 0u );
	ADCResult = STM32_ADC1_Readout( 1u );

	/**
		为保留温度转换前的运算精度，结果扩大160倍
	*/
	if ( 0u != ADCRef )
	{
		Volt = 10u * 16u * ( ADCResult * 1200u ) / ADCRef;
	}
	else
	{
		Volt = 10u * 16u * ( ADCResult * 3300u ) / 4096u;
	}

	/**
		Temp = 25.0f + ( VoltT25 - Volt ) / VoltDT1;
		VoltT25 = 1430mV
		VoltDT1 = 4.3mV/℃
		运算结果扩大16倍以表达1/16℃的分辨率。
	*/
	Temp = ( 25 * 16 ) + ((( 1430 * 10 * 16 ) - Volt ) / ( 43 ));
	if ( Temp >= ( 125 * 16 ))
	{
		return	( 125 * 16 );
	}
	
	if ( Temp <= ( -55 * 16 ))
	{
		return	( -55 * 16 );
	}
	
	return	Temp;	
}

FP32	get_NTC1_Temp( void )
{
	uint16_t	ADCResult = STM32_ADC1_Readout( 2u );

	return	_CV_NTC10K( ADCResult );
}

FP32	get_NTC2_Temp( void )
{
	uint16_t	ADCResult = STM32_ADC1_Readout( 3u );

	return	_CV_NTC10K( ADCResult );
}

/********  (C) COPYRIGHT 2015 青岛金仕达电子科技有限公司  **** End Of File ****/
