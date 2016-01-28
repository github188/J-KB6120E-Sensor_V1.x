/**************** (C) COPYRIGHT 2014 青岛金仕达电子科技有限公司 ****************
* 文 件 名: HCBox.C
* 创 建 者: 董峰
* 描	述	: KB-6120E 恒温箱温度控制
* 最后修改: 2015年11月28日
*********************************** 修订记录 ***********************************/
// Header:		(C) COPYRIGHT 2014 青岛金仕达电子科技有限公司
// File Name: HCBox.C
// Author:		...
// Date:			2015年11月28日

#include "Pin.H"
#include "BSP.H"
// #include "math.h"
struct	uHCBox
{
	uint8_t		SetMode;		//	设定的控制方式：禁止、加热、制冷、自动 四种方式
	FP32		SetTemp;			//	设定的控制温度：
	FP32		RunTemp;			//	实测的运行温度：
	uint16_t	OutValue;		//	控制信号输出值[0,+2000]，>1000表示加热，<1000表示制冷。
}HCBox;

enum
{	//	恒温箱控制温度的方式
	MD_Shut,
	MD_Heat,
	MD_Cool,
	MD_Auto
};


/********************************** 功能说明 ***********************************
*	测量风扇转速
*******************************************************************************/
#define	fanCountListLen	(4u+(1u+2u))
static	uint16_t	fanCountList[fanCountListLen];
static	uint8_t		fanCountList_index = 0;

uint16_t	FanSpeed_fetch( void )
{
	/*	固定间隔1s记录风扇转动圈数到缓冲区，
	 *	依次计算增量并滤波的结果即风扇转速。
	 */
	uint8_t 	ii, index = fanCountList_index;
	uint16_t	sum = 0u;
	uint16_t	max = 0u;
	uint16_t	min = 0xFFFFu;
	uint16_t	x0, x1, speed;

	x1 = fanCountList[index];
	for ( ii = fanCountListLen - 1u; ii != 0; --ii )
	{
		//	依次求增量得到速度
		x0 = x1;
		if ( ++index >= fanCountListLen ){  index = 0u; }
		x1 = fanCountList[index];
		speed = ( x1 - x0 );
		//	对多个数据进行滤波
		if ( speed > max ) {  max = speed; }
		if ( speed < min ) {  min = speed; }
		sum += speed;
	}

	speed = (uint16_t)( sum - max - min ) / ( fanCountListLen - (1u+2u));
	
	return	speed  * 60u;
}


uint16_t	HCBoxFan_Circle_Read( void )
{
	return	TIM1->CNT;		//不停的计数
}


static	uint16_t	FanSpeed;
uint16_t	volatile  fan_shut_delay;
void	HCBoxFan_Update( void )
{	//	定间隔记录转动圈数
	fanCountList[fanCountList_index] = HCBoxFan_Circle_Read();
	if ( ++fanCountList_index >= fanCountListLen )
	{
		fanCountList_index = 0u;
	}
	FanSpeed = FanSpeed_fetch();
	//	风扇开关单稳态控制
	if ( fan_shut_delay > 0u )
		fan_shut_delay --;
	else
		HCBoxFan_OutCmd( FALSE );
}



void	set_HCBoxTemp( FP32 TempSet, uint8_t ModeSet )
{
	HCBox.SetTemp = TempSet;
	HCBox.SetMode = ModeSet;
}



FP32	get_HCBoxTemp( void )
{
	return	usRegInputBuf[5] * 0.0625;
}



uint16_t	get_HCBoxOutput( void )
{
	return	HCBox.OutValue;
}



uint16_t	get_HCBoxFanSpeed( void )
{
	return	FanSpeed;
}

/********************************** 功能说明 ***********************************
*  输出控制（隐含循环定时功能）
*******************************************************************************/

void	HCBox_Output( FP32 OutValue )
{
	static	volatile	uint16_t	HCBoxOutValue = 0;
	//	更新输出状态
	HCBox.OutValue = OutValue * 1000 + 1000;
	
	if      ( HCBox.OutValue < 1000 )
	{
		HCBoxOutValue = ( 1000 - HCBox.OutValue  );
		//	关闭加热
		HCBoxHeat_OutCmd( 0 );
		//	开启制冷
		if      ( HCBoxOutValue > 990 )
		{
			HCBoxCool_OutCmd( 1000 );
		}
		else if ( HCBoxOutValue < 10  )
		{
			HCBoxCool_OutCmd( 0 );
		}
		else
		{	
			HCBoxCool_OutCmd( HCBoxOutValue );
		}
	}
	else if ( HCBox.OutValue > 1000 )
	{
		HCBoxOutValue = HCBox.OutValue - 1000;		
		HCBoxCool_OutCmd( 0 );//	关闭制冷
		//	开启加热
		if      ( HCBoxOutValue > 990 )
		{		
			HCBoxHeat_OutCmd( 1000 );		
		}
		else if ( HCBoxOutValue < 10 )
		{                                                
			HCBoxHeat_OutCmd( 0 );
		}
		else
		{
			HCBoxHeat_OutCmd( HCBoxOutValue );
		}
	}
	else
	{
		//	关闭加热
		HCBoxHeat_OutCmd( 0 );
		//	关闭制冷
		HCBoxCool_OutCmd( 0 );
	}

}


/********************************* *功能说明* **********************************
*	加热器恒温箱共用一个温度信号，两者不能同时使用。
*******************************************************************************/
extern	uint16_t	iRetry;
void	HCBoxValue_Update( void )
{
	if ( iRetry >= 30 )
		HCBox_Output( 0.0f );	//	注意与等待状态的输出保持一致
	set_HCBoxTemp( usRegHoldingBuf[5] * 0.0625f, usRegHoldingBuf[6] );
	HCBox.RunTemp = get_HCBoxTemp();
}

/********************************** 功能说明 ***********************************
*  等待状态，禁止加热、制冷
*******************************************************************************/
volatile	BOOL	EN_Cool = TRUE;
volatile	BOOL	EN_Heat = TRUE;
static		BOOL 	ControlFlag = TRUE;
static	void	HCBox_Wait( void )
{
	//	设置自动模式即无法确定实际工作模式可暂时进入等待状态
	HCBoxValue_Update();
	HCBox_Output( 0.0f );	//	等待状态输出
}

#include "math.h"
// struct	uPID_Parament Heat;
// struct	uPID_Parament Cool;
/********************************** 功能说明 ***********************************
*  加热状态：加热方式工作
*******************************************************************************/

static	void	HCBox_Heat( void )  
{
	FP32	Kp = 10.0f / 128.0f;
	FP32	Ki = ( Kp / 240.0f );
	FP32	Kd = ( Kp * 75.0f );

	FP32	TempRun, TempSet;
	static FP32	Ek_1, Ek = 0.0f;
	static FP32	Up = 0.0f, Ui = 0.0f, Ud = 0.0f;
	static FP32	Upid = 0.0f;

	if( EN_Heat )
	{
		HCBoxValue_Update();
		//	计算PID输出，输出量值归一化到[0.0 至+1.0]范围
		TempRun = HCBox.RunTemp;
		TempSet = HCBox.SetTemp;// - ((FP32)( usRegInputBuf[2] * 0.0625 - HCBox.SetTemp )) * 0.01f;//	跟环境温度有微小关系 
		Ek_1 = Ek;
		Ek = ( TempSet - TempRun );
		Up = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.3f ){  Ui = -0.3f; }
		if ( Ui > +0.3f ){  Ui = +0.3f; }//0.25
		if( ( HCBox.RunTemp - HCBox.SetTemp ) >= 0 ) 
			Ud = ( Ud * 0.9f ) + (( Kd * ( - fabs(Ek - Ek_1) )) * 0.1f );
		else
			Ud = ( Ud * 0.8f ) + (( Kd * (Ek - Ek_1)) * 0.2f );
		Upid = ( Up + Ui + Ud );
		if ( Upid <  0.0f ){  Upid = 0.0f; }
		if ( Upid > +1.0f ){  Upid = 1.0f; }
		
		
		HCBox_Output( Upid );	//	加热状态输出（隐含循环定时功能）
	}
}
/********************************** 功能说明 ***********************************
*  制冷状态：制冷方式工作
*******************************************************************************/

static	void	HCBox_Cool( void )
{
	FP32	Kp = 25.0f / 128.0f ; 
	FP32	Ki = ( Kp / 180.0f );
	FP32	Kd = ( Kp * 60.0f ) ;
	static FP32	Ek_1, Ek = 0.0f;
	static FP32	Up = 0.0f, Ui = 0.0f, Ud = 0.0f;
	static FP32	Upid = 0.0f;
	FP32	 TempRun, TempSet;
  
  
	//	计算PID输出，输出量值归一化到[-1.0至 0.0]范围
	
	if( EN_Cool )
	{
		HCBoxValue_Update();		//	实时读取温度;  if ( 失败 ) 转入待机状态
		//	计算PID输出，输出量值归一化到[-1.0至 0.0]范围
		TempRun = HCBox.RunTemp;
		TempSet = HCBox.SetTemp;// - ((FP32)( usRegInputBuf[2] * 0.0625 - HCBox.SetTemp )) * 0.01f;//	跟环境温度有微小关系 ;
		Ek_1 = Ek;
		Ek  = ( TempSet - TempRun );
		Up  = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.30f ){  Ui = -0.30f; }
		if ( Ui > +0.30f ){  Ui = +0.30f; }	//	0.5
		if( ( HCBox.RunTemp - HCBox.SetTemp ) <= 0 ) 
			Ud =( Ud * 0.9f ) + (( Kd * fabs( Ek - Ek_1 )) * 0.1f ); 
		else
			Ud =( Ud * 0.8f ) + (( Kd * ( Ek - Ek_1 )) * 0.2f ); 
		Upid = Up + Ui + Ud;
		if ( Upid >  0.0f ){  Upid =  0.0f; }
		if ( Upid < -1.0f ){  Upid = -1.0f; }
		//	风扇输出控制（制冷方式下开启风扇，暂不调速－2014年1月15日）
		if ( Upid < 0.0f )
		{
			fan_shut_delay = 60u;
			HCBoxFan_OutCmd( TRUE );
		}
		//	输出
		if ( FanSpeed < 100u )	//	风扇不转，禁止制冷片工作
		{													
				HCBox_Output( 0.0f );	//	注意与等待状态的输出保持一致
		}
		else
		{
			HCBox_Output( Upid );	//	制冷状态输出（隐含循环定时功能）
		}
	}

}



/********************************** 功能说明 ***********************************
*	恒温箱温度控制
*******************************************************************************/
static	uint32_t HCBoxCount = 0;

void	HCBoxControl( void )
{   
/**/	FP32	EK;

	if( HCBoxFlag )
	{
	
		HCBoxFlag = FALSE;
		HCBoxFan_Update();			//	测量风扇转速
		HCBoxValue_Update();		//	实时参数读取;
		EK = HCBox.RunTemp - HCBox.SetTemp;
	
		switch ( HCBox.SetMode )
		{
		case MD_Auto:
			if( !ControlFlag )
			{	
				EN_Cool = FALSE;
				EN_Heat = FALSE;
				if( fabs( EK ) >= 1 )
					HCBoxCount ++;
				else
					HCBoxCount = 0;				
				if(( HCBoxCount >= 60 * 10 ) || ( fabs( EK ) >= 5 ))	//	温差大于1℃且小于5℃超过10分钟或者10分钟内温度变化太快（温差大于5℃）
				{
					ControlFlag = TRUE;
				}
			}
			else
			{  
				HCBoxCount = 0;

				if( EK >=  1 )
				{
					if( EN_Cool == FALSE )
						ControlFlag = FALSE;

					EN_Cool = TRUE;
					EN_Heat = FALSE;
				}

				if( EK <= -1 )
				{
					if( EN_Heat == FALSE )
						ControlFlag = FALSE;

					EN_Cool = FALSE;
					EN_Heat = TRUE;
				}
			}			
			break;
		case MD_Cool:	
			if(  ( HCBox.RunTemp - HCBox.SetTemp ) <= -1 ) 
			{
				ControlFlag = FALSE;
			}
			else
			{
				ControlFlag = TRUE;
				EN_Cool = TRUE; 
				EN_Heat = FALSE;	
			}
			break;
		case MD_Heat:	
			if( ( HCBox.RunTemp - HCBox.SetTemp ) >= 1 ) 
			{
				ControlFlag = FALSE;
			}
			else
			{
				ControlFlag = TRUE;
				EN_Heat = TRUE; 
				EN_Cool = FALSE;	
			}
			break;
		default:
		case MD_Shut:	
			EN_Heat = 
			EN_Cool = FALSE;				
			HCBox_Wait();	
			break;							
		}
		if( ControlFlag )
		{
			if( EN_Heat )
				HCBox_Heat();
			if( EN_Cool )
				HCBox_Cool();
		}
		else
		{	
			HCBox_Wait();
		}
	}

}


