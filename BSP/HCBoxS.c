/**************** (C) COPYRIGHT 2014 青岛金仕达电子科技有限公司 ****************
* 文 件 名: HCBox.C
* 创 建 者: 董峰
* 描  述  : KB-6120E 恒温箱温度控制
* 最后修改: 2014年4月21日
*********************************** 修订记录 ***********************************
* 版  本: V1.1
* 修订人: 董峰
* 说  明: 增加显示接口，显示恒温箱状态
*******************************************************************************/

//#include	"BIOS.H"

#include "Pin.H"
#include "BSP.H"

struct	uHCBox
{
	uint8_t		SetMode;		//	设定的控制方式：禁止、加热、制冷、自动 四种方式
	FP32		SetTemp;		//	设定的控制温度：
	FP32		RunTemp;		//	实测的运行温度：
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
	fanCountList[ fanCountList_index] = HCBoxFan_Circle_Read();
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
	return	FanSpeed_fetch();
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
		if      ( HCBoxOutValue >  990 )
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


/********************************** 功能说明 ***********************************
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
static	BOOL ControlFlag = TRUE;
static	void	HCBox_Wait( void )
{
	//	设置自动模式即无法确定实际工作模式可暂时进入等待状态
	HCBoxValue_Update();
	HCBox_Output( 0.0f );	//	等待状态输出
}



/********************************** 功能说明 ***********************************
*  加热状态：加热方式工作
*******************************************************************************/

static	void	HCBox_Heat( void )  
{
	const	FP32	Kp = 0.0390625f;			//  5/128*2
	const	FP32	Ki = ( Kp / 160.0f );	//	160.0f 10.0f 
	const	FP32	Kd = ( Kp * 130.0f );	//	80.0f30

//	const	FP32	Kp = 0.2;
//	const	FP32	Ki = ( Kp / 100.0f );
//	const	FP32	Kd = ( Kp *  10.0f );

	FP32	TempRun, TempSet;
	static FP32	Ek_1, Ek = 0.0f;
	static FP32	Up = 0.0f, Ui = 0.0f, Ud = 0.0f;
	static FP32	Upid = 0.0f;

	if( EN_Heat )
	{
	
		HCBoxValue_Update();
		//	计算PID输出，输出量值归一化到[0.0 至+1.0]范围
		TempRun = HCBox.RunTemp;
		TempSet = HCBox.SetTemp - ((FP32)( usRegInputBuf[2] * 0.0625 - HCBox.SetTemp )) * 0.1f;//	跟环境温度有微小关系 
		Ek_1 = Ek;
		Ek = ( TempSet - TempRun );
		Up = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.3f ){  Ui = -0.3f; }
		if ( Ui > +0.3f ){  Ui = +0.3f; }//0.25
		Ud = ( Ud * 0.8f ) + (( Kd * ( Ek - Ek_1 )) * 0.2f );
		Upid = ( Up + Ui + Ud );
		if ( Upid <  0.0f ){  Upid = 0.0f; }
		if ( Upid > +1.0f ){  Upid = 1.0f; }

		if( ( HCBox.RunTemp - HCBox.SetTemp ) > 0 ) 
		{
			ControlFlag = FALSE;
			Upid = 0.0f;
		}		
		
		HCBox_Output( Upid );	//	加热状态输出（隐含循环定时功能）
	}
}
/********************************** 功能说明 ***********************************
*  制冷状态：制冷方式工作
*******************************************************************************/

static	void	HCBox_Cool( void )
{
	const	FP32	Kp = 0.1171875f;			//	15 / 128*2
	const	FP32	Ki = ( Kp / 240.0f );		//	240.0f10.0
	const	FP32	Kd = ( Kp * 80.0f );	//	80.0f3.0
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
		TempSet = HCBox.SetTemp - ((FP32)( usRegInputBuf[2] * 0.0625 - HCBox.SetTemp )) * 0.1f;//	跟环境温度有微小关系 ;
		Ek_1 = Ek;
		Ek  = ( TempSet - TempRun );
		Up  = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.30f ){  Ui = -0.30f; }
		if ( Ui > +0.30f ){  Ui = +0.30f; }	//	0.5
		Ud =( Ud * 0.8f ) + (( Kd * ( Ek - Ek_1 )) * 0.2f ); 
		Upid = Up + Ui + Ud;
		if ( Upid >  0.0f ){  Upid =  0.0f; }
		if ( Upid < -1.0f ){  Upid = -1.0f; }

		if( ( HCBox.RunTemp - HCBox.SetTemp ) < 0 ) 
		{
			ControlFlag = FALSE;
			Upid = 0.0f;
		}

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


#include "math.h"
/********************************** 功能说明 ***********************************
*	恒温箱温度控制
*******************************************************************************/
static	uint32_t HCBoxCount = 0;

void	HCBoxControl( void )
{   
	FP32	EK;

	if( HCBoxFlag )
	{
	
		HCBoxFlag = FALSE;
		HCBoxFan_Update();			//	测量风扇转速
		HCBoxValue_Update();		//	实时参数读取;
		EK = HCBox.RunTemp - HCBox.SetTemp;

		if( !ControlFlag )
		{	
			EN_Cool = FALSE;
			EN_Heat = FALSE;
			if( fabs( EK ) > 2 )
				HCBoxCount ++;
			else
				HCBoxCount = 0;				
			if( HCBoxCount >= 60 * 30 )
				ControlFlag = TRUE;
		}
		else
		{
			HCBoxCount = 0;
			if( EK >  2 )
			{
				EN_Cool = TRUE;
				EN_Heat = FALSE;
			}		
			if( EK < -2 )
			{
				EN_Cool = FALSE;
				EN_Heat = TRUE;
			}
			
			switch ( HCBox.SetMode )
			{
			case MD_Auto:
				if( EN_Cool )
					HCBox_Cool();
				if( EN_Heat )
					HCBox_Heat();
				break;
			case MD_Cool:	EN_Cool = TRUE; EN_Heat = FALSE;	HCBox_Cool();	break;
			case MD_Heat:	EN_Heat = TRUE; EN_Cool = FALSE;	HCBox_Heat();	break;
			default:
			case MD_Shut:	EN_Heat = EN_Cool = FALSE;				HCBox_Wait();	break;							
			}			
		}
	}	
}








/********  (C) COPYRIGHT 2014 青岛金仕达电子科技有限公司  **** End Of File ****/





// 			//	如果温度偏差超过2'C且维持一段时间（30min）, 切换工作方式
// 			if ( Ek > -1.5f )
// 			{
// 				shutcount_Cool = 0u;
// 			}
// 			else if ( shutcount_Cool < ( 60u * 1 ))//30u
// 			{
// 				++shutcount_Cool;
// 			}
// 			else
// 			{
// 				EN_Cool = FALSE;
// 				EN_Heat = TRUE;
// 			}







































#if 0

// 使用TIMx的CH1的捕获功能，用DMA记录脉冲的周期.
#define	CMR_Len	10
static	uint16_t	CMRA[CMR_Len];

// void	CMR1( void )
// {
// 	DMA_Channel_TypeDef	* DMA1_Channelx = DMA1_Channel6;
// 	TIM_TypeDef * TIMx = TIM16;
// 	
// 	//	DMA1 channel1 configuration
// 	SET_BIT ( RCC->AHBENR,  RCC_AHBENR_DMA1EN );
// 	//	DMA模块禁能, 重新配置
// 	DMA1_Channelx->CCR  = 0u;
// 	DMA1_Channelx->CCR  = DMA_CCR6_PL_0						//	通道优先级：01 中等
// 						| DMA_CCR6_PSIZE_0					//	内存数据位：01 16位
// 						| DMA_CCR6_MSIZE_0					//	外设数据位：01 16位
// 						| DMA_CCR6_MINC						//	增量模式：内存增量
// 						| DMA_CCR6_CIRC						//	循环传输：使能循环
// 					//	| DMA_CCR6_DIR						//	传送方向：从外设读
// 						;
// 	DMA1_Channelx->CPAR  = (uint32_t) &TIM16->CCR1;			//	设置DMA外设地址
// 	DMA1_Channelx->CMAR  = (uint32_t) CMRA;					//	内存地址
// 	DMA1_Channelx->CNDTR = CMR_Len;							//	传输数量
// 	SET_BIT ( DMA1_Channelx->CCR, DMA_CCR1_EN );			//	使能DMA通道

// 	//	配置TIMx 进行输入捕获。
// 	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_TIM16EN );
// 	TIMx->CR1   = 0u;
// 	TIMx->CR2   = 0u;
// 	TIMx->CCER  = 0u;
// 	TIMx->CCMR1 = 0u;
// 	TIMx->CCMR2 = 0u;
// 	//	TIMx 时基初始化: 输入时钟频率24MHz，分频成1MHz的输入。
// 	//	时基决定可以测量的最低速度与最高速度。
// 	TIMx->PSC = 240u - 1;	//	10us @ 24MHz
// 	TIMx->ARR = 0xFFFFu;
// 	TIMx->EGR = TIM_EGR_UG;
// 	
// 	TIMx->CCMR1 = TIM_CCMR1_CC1S_0					//	CC1S  : 01b   IC1 映射到IT1上。
// 				| TIM_CCMR1_IC1F_1|TIM_CCMR1_IC1F_0	//	IC1F  : 0011b 配置输入滤波器，8个定时器时钟周期滤波
// 				| TIM_CCMR1_IC2PSC_1				//	IC1PSC: 01b   配置输入分频，每隔2次事件发生一次捕获
// 				;
// 	TIMx->CCER  = TIM_CCER_CC1E						//	允许 CCR1 执行捕获
// 				| TIM_CCER_CC1P						//	负边沿CCR1捕获信号周期。
// 				;
// 	TIMx->DIER  = TIM_DIER_CC1DE;

// 	TIMx->CR1   = TIM_CR1_CEN;						//	使能定时器

// 	//	配置管脚：PA.6 浮空输入
// 	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
// 	MODIFY_REG( GPIOA->CRL, 0x0F000000u, 0x04000000u );
// }

uint16_t	fetchSpeed( void )
{	//	取 DMA 计数 或 内存地址指针，并连续向前增量两次。
	//	如果DMA计数 或 内存指针都不可用，取N次的差值，并丢弃最大值和最小值。
	
	/*	固定间隔1s记录风扇转动圈数到缓冲区，
	 *	依次计算增量并滤波的结果即风扇转速。
	 */
	DMA_Channel_TypeDef	* DMA1_Channelx = DMA1_Channel6;
	uint8_t 	ii, index;
	uint16_t	sum = 0u;
//	uint16_t	max = 0u;
//	uint16_t	min = 0xFFFFu;
	uint16_t	x0, x1, period;

	index = ( DMA1_Channelx->CMAR - ( uint32_t ) CMRA ) / sizeof( uint16_t);	//	内存地址
	if ( ++index >= CMR_Len ){  index = 0u; }
	if ( ++index >= CMR_Len ){  index = 0u; }
	
	x1 = CMRA[index];
	for ( ii = CMR_Len - 2u; ii != 0; --ii )
	{
		//	依次求增量得到速度
		x0 = x1;
		if ( ++index >= CMR_Len ){  index = 0u; }
		x1 = CMRA[index];
		period = (uint16_t)( x1 - x0 );
		//	对多个数据进行滤波
//		if ( period > max ) {  max = period; }
//		if ( period < min ) {  min = period; }
//		sum += period;
	}
	period = sum / ( CMR_Len - 2u );
//	period = (uint16_t)( sum - max - min ) / ( CMR_Len - (1u+2u));

	if ( period == 0u )
	{
		return	0xFFFFu;
	}
	else
	{	//	每分钟的计数周期 / 每个脉冲的计数时间 => 每分钟的转速
		return	(( 60u * 100000u ) / period );
	}
}

#endif
// iCount;


// _isr_t( void )

// {
// 	static uint8_t iPWM =0;
// 	iCount ++;
// 	if ( (t1 - t0) > 1000 )
// 	//	t0 += 1000;
// 	(t0 = t1;
// 	{
// 		
// 	}
// 	
// 	
// 	++iPWM;
// 	if (iPWM>= 100 )
// 	{
// 		iPWM = 0;
// 	}
// 	
// 	Heat = iPWM>out
// 	
// }
// void heat( void )
// {
// 	static t0;// = iCount;
// 	t1 = iCount;
// 	if ( (t1 - t0) > 1000 )
// 	//	t0 += 1000;
// 	(t0 = t1;
// 		
// 	{
// 		pid = ???
// 		out = pid;		
// 	}
// 	
// 	
// }

