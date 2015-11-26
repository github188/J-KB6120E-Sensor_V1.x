#include "BSP.H"
#include <string.h>
extern	void	MODBUS_Init( uint8_t MBAddress );
extern	uint16_t	usRegInputBuf[];
extern	uint16_t	usRegHoldingBuf[];
extern	uint8_t		ucRegDiscBuf[];
extern	uint8_t		ucRegCoilsBuf[];

#define	_EE_SLAVE_BASE	2500u
#define	_EE_IMAGE_BASE	2048u
#define	_EE_IMAGE_LEN	100u

uint16_t	get_AD7705( uint8_t	SelectBPGA );
uint16_t	get_AD7705( uint8_t	SelectBPGA )
{
	uint32_t	Sum = 0u;

	Sum += Readout_AD7705( SelectBPGA );
	Sum += Readout_AD7705( SelectBPGA );
	Sum += Readout_AD7705( SelectBPGA );
	Sum += Readout_AD7705( SelectBPGA );

	return	(uint16_t)( Sum / 4u );
	
// 	delay( 80u );
// 	
// 	return	get_CPU_Temp();
}

////////////////////////////////////////////////////////////////////////////////
#define	ArrayLen	128u
volatile uint16_t	Array[ArrayLen];
uint16_t	ArrayIndex = 0u;

void	FilterArrayIn( uint16_t InValue )
{
	if ( ++ArrayIndex >= ArrayLen )
	{
		ArrayIndex = 0u;
	}
	Array[ArrayIndex] = InValue;
}

void	FilterArrayOut( uint16_t OutResult[] )
{
	const	static	uint16_t	NSEL[] =
	{
		2, 		//	取最后 0.16 秒的均值
		4,  	//	取最后 0.32 秒的均值
		8,  	//	取最后 0.64 秒的均值
		12, 	//	取最后 0.96 秒的均值
		25, 	//	取最后 2.00 秒的均值
		60, 	//	取最后 4.80 秒的均值
		125 	//	取最后 10.0 秒的均值
	};

	uint16_t	Index = ArrayIndex;
	uint16_t	Select = 0u;
	uint16_t	Count = 0u;
	uint32_t	Sum = 0u;
	
	for ( Select = 0u; Select < sizeof( NSEL) / sizeof(*NSEL); ++Select )
	{
		uint8_t	FilterLen = NSEL[Select];
		
		do {
			Sum += Array[Index];
			if ( 0u == Index )
			{
				Index = ArrayLen;
			}
			--Index;
		} while ( ++Count < FilterLen );
	
		OutResult[Select] = Sum / Count;
	}
}

////////////////////////////////////////////////////////////////////////////////

int32_t	main( void )
{
	uint16_t	Result7705[8];
	uint16_t	Config7705;
	uint8_t		MBAddress;
	uint16_t	i;
	
	//	板上器件初始化
	BIOS_Init();
	
	for ( i = 0; i < 10u; ++i )
	{
		usRegHoldingBuf[i] = 0u;
	}

	for ( i = 0; i < 40u; ++i )
	{
		usRegInputBuf[i] = 0u;
	}
	
//	for ( i = 0; i < ArrayLen - 1; ++i )
//	{
//		FilterArrayIn( 0u );
//	}
	
//	get_Unique96(( void * ) &usRegInputBuf[13] );

	Initialize_AD7705();

	//	读取内部EEPROM
//	Vload( _EE_IMAGE_BASE, &usRegHoldingBuf[10], _EE_IMAGE_LEN );
//	Vload( _EE_SLAVE_BASE, &MBAddress, sizeof( MBAddress ));
	Vload( _EE_IMAGE_BASE, &usRegHoldingBuf[10], _EE_IMAGE_LEN );
	Vload( _EE_SLAVE_BASE, &MBAddress, sizeof( MBAddress ));
	if (( MBAddress < 1u ) || ( MBAddress > 247u ))
	{
		MBAddress = 1u;
	}

	//	初始化MODBUS协议栈
	MODBUS_Init( 1 );	//	MODBUS_Init( MBAddress );
	
	//	看门狗配置
	for(;;)
	{
		//	看门狗控制


		//	活动计数器，表示从机工作正常
		++usRegInputBuf[0];
		delay(500);
		
		++usRegInputBuf[15];
		++usRegInputBuf[16];
		++usRegInputBuf[17];
		
	}
//	
//		++usRegInputBuf[10];
//		++usRegInputBuf[11];
//		++usRegInputBuf[12];
//		++usRegInputBuf[13];
//		//	取得 CPU 温度读数
//		usRegInputBuf[1] = get_CPU_Temp();

//		//	测量 供电电源 电压
//		usRegInputBuf[2] = get_Bat_Volt();

//		//	测量 高压反馈 电压
//		usRegInputBuf[3] = get_HV_Volt();

//		//	取得AD7705的读数
//		Config7705 = usRegHoldingBuf[2];
//		Result7705[0] = get_AD7705( Config7705 & 0x00FFu ); //	决定循环时间的主要因素，理论值为80ms
//		FilterArrayIn( Result7705[0] );
//		FilterArrayOut( Result7705 + 1u );

//		usRegInputBuf[4] = Result7705[( Config7705 >> 8  ) % 8u];
//		usRegInputBuf[5] = Result7705[( Config7705 >> 12 ) % 8u];
//		usRegInputBuf[ 6] = Result7705[1];
//		usRegInputBuf[ 7] = Result7705[2];
//		usRegInputBuf[ 8] = Result7705[3];
//		usRegInputBuf[ 9] = Result7705[4];
//		usRegInputBuf[10] = Result7705[5];
//		usRegInputBuf[11] = Result7705[6];
//		usRegInputBuf[12] = Result7705[7];

//		//	控制自检LED
//		if ( 0u != usRegHoldingBuf[0] )
//		{
//			PWM1_OutputSet( usRegHoldingBuf[0] );	//	输出PWM
//		}
//		else
//		{
//			PWM1_OutputSet( 0u );					//	关闭PWM
//		}

//		//	控制高压
//		if ( 0u != usRegHoldingBuf[1] )
//		{	
//			DAC1_OutputSet( usRegHoldingBuf[1] );
//			HVPower_OutCmd( TRUE );					//	打开高压
//		}
//		else
//		{
//			DAC1_OutputSet( 0u );
//			HVPower_OutCmd( FALSE );				//	关闭高压
//		}

//		//	处理数据控制命令
//		if ( 0u != usRegHoldingBuf[9] )
//		{
//			if ( 0u == memcmp( &usRegHoldingBuf[3], &usRegInputBuf[13], 12 ))
//			{
//				uint16_t	DataControlWord = usRegHoldingBuf[9];
//				uint8_t	HiByte, LoByte;
//				
//				HiByte = DataControlWord >> 8;
//				LoByte = DataControlWord & 0xFFu;
//			
//				if ( (uint8_t)( ~HiByte ) == LoByte )
//				{
//					switch ( LoByte )
//					{
//					case 0x00u:
//					case 0xF8u:
//					case 0xF9u:
//					case 0xFAu:
//					case 0xFBu:
//					case 0xFCu:	break;

//					default:	//	1u - 247u 修改从机地址命令
//						MBAddress = LoByte;
//						Vsave( _EE_SLAVE_BASE, &MBAddress, sizeof( MBAddress ));
//						break;
//					case 0xFDu:	//	Write Disable
//						usRegInputBuf[19] = FALSE;
//						NVRAM_Write_Disable();
//						break;
//					case 0xFEu:	//	Write Enable
//						usRegInputBuf[19] = TRUE;
//						NVRAM_Write_Enable();
//						break;
//					case 0xFFu:	//	保存内部EEPROM映像
//						Vsave( _EE_IMAGE_BASE, &usRegHoldingBuf[10], _EE_IMAGE_LEN );
//						break;
//					}
//					//	请求处理完成，撤销请求标志，复位密码区
//					memset( &usRegHoldingBuf[3], 0u, 12 + 2 );
//				}
//			}
//		}
//	}
}
