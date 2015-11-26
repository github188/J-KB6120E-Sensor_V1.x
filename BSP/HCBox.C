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
#include "Pin.H"
#include "BSP.H"
/********************************** 功能说明 ***********************************
*	测量风扇转速
*******************************************************************************/
#define	fanCountListLen	(4u+(1u+2u))
static	uint16_t	fanCountList[fanCountListLen];
static	uint8_t		fanCountList_index = 0;

uint16_t	HCBoxFan_Circle_Read( void )
{
	return	TIM1->CNT;
}

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
	
	return	speed  * 30u;
}

uint16_t fanspeed;
static	uint16_t	fan_shut_delay = 0u;
void	HCBoxFan_Update( void )
{	
	
	//	定间隔记录转动圈数
 	fanCountList[ fanCountList_index] = HCBoxFan_Circle_Read();
	if ( ++fanCountList_index >= fanCountListLen )
	{
		fanCountList_index = 0u;
	}
	fanspeed = FanSpeed_fetch();
	//	风扇开关单稳态控制
	if ( --fan_shut_delay == 0u )
	{
		HCBoxFan_OutCmd( FALSE );
	}	
}

/********  (C) COPYRIGHT 2014 青岛金仕达电子科技有限公司  **** End Of File ****/


// #if 0

// // 使用TIMx的CH1的捕获功能，用DMA记录脉冲的周期.
// #define	CMR_Len	10
// static	uint16_t	CMRA[CMR_Len];

// void	CMR1( void )
// {
// 	DMA_Channel_TypeDef	* DMA1_Channelx = DMA1_Channel2;
// 	TIM_TypeDef * TIMx = TIM1;
// 	
// 	//	DMA1 channel1 configuration
// 	SET_BIT ( RCC->AHBENR,  RCC_AHBENR_DMA1EN );
// 	//	DMA模块禁能, 重新配置
// 	DMA1_Channelx->CCR  = 0u;
// 	DMA1_Channelx->CCR  = DMA_CCR2_PL_0						//	通道优先级：01 中等
// 						| DMA_CCR2_PSIZE_0					//	内存数据位：01 16位
// 						| DMA_CCR2_MSIZE_0					//	外设数据位：01 16位
// 						| DMA_CCR2_MINC						//	增量模式：内存增量
// 						| DMA_CCR2_CIRC						//	循环传输：使能循环
// 					//	| DMA_CCR6_DIR						//	传送方向：从外设读
// 						;
// 	DMA1_Channelx->CPAR  = (uint32_t) &TIM1->CCR1;			//	设置DMA外设地址
// 	DMA1_Channelx->CMAR  = (uint32_t) CMRA;					//	内存地址
// 	DMA1_Channelx->CNDTR = CMR_Len;							//	传输数量
// 	SET_BIT ( DMA1_Channelx->CCR, DMA_CCR1_EN );			//	使能DMA通道

// 	//	配置TIMx 进行输入捕获。
// 	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_TIM1EN );
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

// 	//	配置管脚：PA.12 浮空输入
// 	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
// 	MODIFY_REG( GPIOA->CRH, 0x000F0000u, 0x00040000u );
// }

// uint16_t	fetchSpeed( void )
// {	//	取 DMA 计数 或 内存地址指针，并连续向前增量两次。
// 	//	如果DMA计数 或 内存指针都不可用，取N次的差值，并丢弃最大值和最小值。
// 	
// 	/*	固定间隔1s记录风扇转动圈数到缓冲区，
// 	 *	依次计算增量并滤波的结果即风扇转速。
// 	 */
// 	DMA_Channel_TypeDef	* DMA1_Channelx = DMA1_Channel2;
// 	uint8_t 	ii, index;
// 	uint16_t	sum = 0u;
// //	uint16_t	max = 0u;
// //	uint16_t	min = 0xFFFFu;
// 	uint16_t	x0, x1, period;

// 	index = ( DMA1_Channelx->CMAR - ( uint32_t ) CMRA ) / sizeof( uint16_t);	//	内存地址
// 	if ( ++index >= CMR_Len ){  index = 0u; }
// 	if ( ++index >= CMR_Len ){  index = 0u; }
// 	
// 	x1 = CMRA[index];
// 	for ( ii = CMR_Len - 2u; ii != 0; --ii )
// 	{
// 		//	依次求增量得到速度
// 		x0 = x1;
// 		if ( ++index >= CMR_Len ){  index = 0u; }
// 		x1 = CMRA[index];
// 		period = (uint16_t)( x1 - x0 );
// 		//	对多个数据进行滤波
// //		if ( period > max ) {  max = period; }
// //		if ( period < min ) {  min = period; }
// //		sum += period;
// 	}
// 	period = sum / ( CMR_Len - 2u );
// //	period = (uint16_t)( sum - max - min ) / ( CMR_Len - (1u+2u));

// 	if ( period == 0u )
// 	{
// 		return	0xFFFFu;
// 	}
// 	else
// 	{	//	每分钟的计数周期 / 每个脉冲的计数时间 => 每分钟的转速
// 		return	(( 60u * 100000u ) / period );
// 	}
// }

// #endif
/*

****************/
