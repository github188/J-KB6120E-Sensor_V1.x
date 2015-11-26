#include "BSP.H"
#include <math.h>
#include <string.h>

#define	Read_BitN(_b_buf, _n)	(_b_buf[(_n)/8] & ( 1 << (( _n ) % 8 )))
#define	Set_BitN(_b_buf, _n)	(_b_buf[(_n)/8] |= ( 1 << (( _n ) % 8 )))
#define	Clean_BitN(_b_buf, _n)	(_b_buf[(_n)/8] &= ~( 1 << (( _n ) % 8 )))

extern	uint16_t	usRegInputBuf[];
extern	uint16_t	usRegHoldingBuf[];
extern	uint8_t		ucRegDiscBuf[];
extern	uint8_t		ucRegCoilsBuf[];

///////////////////////////////////////////////////
//	根据温度自动控制风扇的启停。
///////////////////////////////////////////////////
static	void	Fan2_Exec( void )
{
	static	bool	OutState = false;
	
	FP32	Temp = get_NTC2_Temp();
	
	//	高于40℃开，低于40℃关。
	if ( Temp > 40.5f ){  OutState = true;  }
	if ( Temp < 39.5f ){  OutState = false; }
	
	Fan2_OutCmd( OutState );
}

///////////////////////////////////////////////////
//	加热器温度控制
///////////////////////////////////////////////////
static	void	Heater_Exec( void )
{
	static	bool	OutState = false;
	static	uint16_t	iRetry = 0u;
	int16_t	Temp16S;
	int16_t	Temp_Set;
	bool	Heater_EN;

	if ( DS18B20_2_Read( &Temp16S ))
	{
		iRetry = 0u;

		usRegInputBuf[8]  = Temp16S;

		Temp_Set = (int16_t)usRegHoldingBuf[8];			//	设定温度

		if (( Temp_Set > ( 55 * 16 )) || ( Temp_Set < ( 15 * 16 )))
		{
			Heater_EN = false;	//	设定温度不合理，自动禁止加热器工作。
		}
		else
		{
			Heater_EN = Read_BitN( ucRegCoilsBuf, 8 );	//	允许温控
		}

		if ( Heater_EN )
		{	
			//	根据温度自动控制加热器的加热。
			OutState = ( Temp16S < Temp_Set );
		}
		else
		{
			OutState = false;	//	禁止加热器工作。
		}
	}
	else
	{
		if ( iRetry < 30u )
		{
			++iRetry;
		}
		else
		{
			OutState = false;	//	读取温度传感器失败，禁止加热器工作（输出复位）。
		}
	}
	
	Heater_OutCmd( OutState );
	usRegInputBuf[9] = OutState ? 1000u : 0u;
}	



///////////////////////////////////////////////////
//	时间片段0，空余时间尽量快的执行
///////////////////////////////////////////////////
static	void	Slice0_Exec( void )
{
	bool	Protect_EN;
	bool	Output_EN;
	
	Protect_EN = MonitorTickTimeout();	//	监视定时器超时，保护性的关闭所有输出
	//	粉尘泵
// 	Output_EN = Read_BitN( ucRegCoilsBuf, 15 );	// 允许输出?
// 	if ( ! Protect_EN && Output_EN )
// 	{
// 		Fan1_OutCmd( true );
// 		MotorPower_OutCmd( true );
// 		MotorPWM_SetOutput( usRegHoldingBuf[15] );
// 	}
// 	else
// 	{
// 		MotorPWM_SetOutput( 0u );
// 		MotorPower_OutCmd( false );
// 		Fan1_OutCmd( false );
// 	}

	//	时均C
	Output_EN = Read_BitN( ucRegCoilsBuf, 30 );	// 允许输出?
	if ( ! Protect_EN && Output_EN )
	{
		PWM1_SetOutput( usRegHoldingBuf[30] );
	}
	else
	{
		PWM1_SetOutput( 0u );
	}

	//	时均D
	Output_EN = Read_BitN( ucRegCoilsBuf, 35 );	// 允许输出?
	if ( ! Protect_EN && Output_EN )
	{
		PWM2_SetOutput( usRegHoldingBuf[35] );
	}
	else
	{
		PWM2_SetOutput( 0u );
	}
}

///////////////////////////////////////////////////
//	时间片段1，AD7705 切换到 CH0 时执行
///////////////////////////////////////////////////
static	void	Slice1_Exec( void )
{
	int16_t	Temp16S;

	if ( DS18B20_1_Read( &Temp16S ))
	{
		usRegInputBuf[2]  = Temp16S;	//	环境温度
	}

	if ( DS18B20_3_Read( &Temp16S ))
	{	
// 		usRegInputBuf[17] = Temp16S;	//	计前温度（粉尘）
		usRegInputBuf[32] = Temp16S;	//	计前温度（时均C）
		usRegInputBuf[37] = Temp16S;	//	计前温度（时均D）	
	}

	Heater_Exec();		//	加热器温度更新，加热器温度控制

	//	调试
	usRegInputBuf[22] = rint ( 16.0f * get_NTC1_Temp());	//	NTC1 温度
	usRegInputBuf[27] = rint ( 16.0f * get_NTC2_Temp());	//	NTC2 温度
}

///////////////////////////////////////////////////
//	时间片段1，AD7705 切换到 CH2 时执行
///////////////////////////////////////////////////
static	void	Slice2_Exec( void )
{
	usRegInputBuf[3] = rint ( 16.0f * get_NTC1_Temp());	//	电机温度

	Fan2_Exec();			//	电源温度更新，电源散热风扇控制
	
}

///////////////////////////////////////////////////
//	转换/读取 AD7705 通道 CH0
///////////////////////////////////////////////////
void	Update_CH0(void )
{
	uint32_t	Sum7705_CH0[CS7705_Max];
	uint8_t		isExist7705[CS7705_Max];
	uint16_t	i, cnt;

	for ( i = 0u; i < CS7705_Max; ++i )
	{
		isExist7705[i] = Convert7705 ((enum enumCS7705)i, 0u );
		Sum7705_CH0[i] = 0ul;
	}

	///////////////////////////////////////////////////
	Slice1_Exec();		//	时间片段1
	///////////////////////////////////////////////////

	for ( cnt = 0u; cnt < 8u; ++cnt )
	{
		///////////////////////////////////////////////////
		Slice0_Exec();	//	时间片段0，空余时间尽量快的执行
		///////////////////////////////////////////////////

		for ( i = 0u; i < CS7705_Max; ++i )
		{
			if ( isExist7705[i] )
			{
				Sum7705_CH0[i] += Readout7705 ((enum enumCS7705)i, 0u );
			}
		}
	}
	
	//	通道 CH0 的结果连续多次采样的平均值
	for ( i = 0u; i < CS7705_Max; ++i )
	{
		if ( isExist7705[i] )
		{
			uint16_t mean = Sum7705_CH0[i] / cnt;
			usRegInputBuf[25 + ( i * 5 )] = mean;	//	孔板差压
		}
	}
}

///////////////////////////////////////////////////
//	转换/读取 AD7705 通道 CH1
///////////////////////////////////////////////////
void	Update_CH1( void )
{
	static	uint32_t	Sum7705_CH1[CS7705_Max][4];
	uint8_t		isExist7705[CS7705_Max];
	uint16_t	i, cnt;

	for ( i = 0u; i < CS7705_Max; ++i )
	{
		isExist7705[i] = Convert7705 ((enum enumCS7705)i, 1u );
	}
	
	///////////////////////////////////////////////////
	//	时间片段2
	///////////////////////////////////////////////////
	Slice2_Exec();

	for ( cnt = 0u; cnt < 2u; ++cnt )
	{
		///////////////////////////////////////////////////
		Slice0_Exec();	//	时间片段0，空余时间尽量快的执行
		///////////////////////////////////////////////////
		
		for ( i = 0u; i < CS7705_Max; ++i )
		{
			if ( isExist7705[i] )
			{
				uint16_t	mean;

				Sum7705_CH1[i][0] = Sum7705_CH1[i][1];
				Sum7705_CH1[i][1] = Sum7705_CH1[i][2];
				Sum7705_CH1[i][2] = Sum7705_CH1[i][3];
				Sum7705_CH1[i][3] = Readout7705 ((enum enumCS7705)i, 1u );
				
				mean = ((uint32_t)Sum7705_CH1[i][0] + Sum7705_CH1[i][1] + Sum7705_CH1[i][2] + Sum7705_CH1[i][3] ) / 4u;

				usRegInputBuf[26+ ( i * 5 )] = mean;	//	计前压力
			}
		}
	}
}

int32_t	main( void )
{
	int16_t		Temp16S;
	uint16_t	i;
	
	//	板上器件初始化
	BIOS_Init( );
	DS18B20_1_Read( &Temp16S );		//	读18B20, 跳过 0x0550
	DS18B20_2_Read( &Temp16S );
	DS18B20_3_Read( &Temp16S );
	Initialize7705( );

	//	初始化通信缓冲区
	for ( i = 0; i < 40u; ++i )
	{
		usRegHoldingBuf[i] = 0u;
		usRegInputBuf[i]   = 0u;
	}

	for ( i = 0; i < (( 40u + 7 ) / 8 ); ++i )
	{
		ucRegCoilsBuf[i] = 0u;
		ucRegDiscBuf[i] = 0u;
	}
	
	// Set_BitN( ucRegDiscBuf, 1 );	//	大气压力 存在
	Set_BitN( ucRegDiscBuf, 2 );	//	环境温度 存在
	Set_BitN( ucRegDiscBuf, 3 );	//	电机温度 存在
	Set_BitN( ucRegDiscBuf, 5 );	//	恒温箱 存在
// 	Set_BitN( ucRegDiscBuf, 8 );	//	加热器 存在
// 	Set_BitN( ucRegDiscBuf, 10 );	//	大气泵 存在
// 	Set_BitN( ucRegDiscBuf, 15 );	//	粉尘泵 本组信号 存在
// 	Set_BitN( ucRegDiscBuf, 16 );	//	       计前压力 有效
// 	Set_BitN( ucRegDiscBuf, 17 );	//	       计前温度 有效
	Set_BitN( ucRegDiscBuf, 30 );	//	时均C  本组信号 存在
	Set_BitN( ucRegDiscBuf, 31 );	//	       计前压力 有效
	Set_BitN( ucRegDiscBuf, 32 );	//	       计前温度 有效
	Set_BitN( ucRegDiscBuf, 35 );	//	时均D  本组信号 存在
	Set_BitN( ucRegDiscBuf, 36 );	//	       计前压力 有效
	Set_BitN( ucRegDiscBuf, 37 );	//	       计前温度 有效
	
	//	初始化MODBUS协议栈
	MODBUS_Init( 1 );
	
	//	看门狗配置
	//	InitWDT();
	for(;;)
	{
		//	活动计数器，表示系统工作正常。
		++usRegInputBuf[0];

		Update_CH0( );

		Update_CH1( );

		//	看门狗控制
		//	ClearWDT();
	}
}

/********  (C) COPYRIGHT 2015 青岛金仕达电子科技有限公司  **** End Of File ****/
