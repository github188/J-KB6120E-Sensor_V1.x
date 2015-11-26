#include "Pin.H"
#include "BSP.H"
/********************************** 功能说明 ***********************************
*	测量风扇转速
*******************************************************************************/
#define	fanCountListLen	( 4u+(1u+2u) )
static	uint16_t	fanCountList[fanCountListLen];
static	uint8_t		fanCountList_index = 0;

uint16_t	HCBoxFan_Circle_Read( void )
{
	return	TIM1->CNT;		//不停的计数
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


extern  uint16_t	volatile  fan_shut_delay;
uint16_t	HCBoxFan_Update( void )
{	
	static uint16_t fanspeed;
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
	return fanspeed;
}
