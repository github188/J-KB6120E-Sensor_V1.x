#include "BSP.H"
#include "Pin.H"

//lint	--e{750}	Info 750: local macro 'XX' not referenced

///////////////////////////////////////////////////////////////////
// Communications Register (RS:000)
///////////////////////////////////////////////////////////////////
#define DRDY			0x80U			// status of the DRDY
#define RS_0		((uint8_t)0x00)		// (0x00U<<4)	Communications Register, 8 Bits
#define RS_1		((uint8_t)0x10)		// (0x01U<<4)	Setup Register, 8 Bits
#define RS_2		((uint8_t)0x20)		// (0x02U<<4)	Clock Register, 8 Bits
#define RS_3		((uint8_t)0x30)		// (0x03U<<4)	Data Register, 16 Bits
#define RS_4		((uint8_t)0x40)		// (0x04U<<4)	Test Register, 8 Bits
#define RS_5		((uint8_t)0x50)		// (0x05U<<4)	No Operation
#define RS_6		((uint8_t)0x60)		// (0x06U<<4)	Offset Register, 24 Bits
#define RS_7		((uint8_t)0x70)		// (0x07U<<4)	Gain Register, 24 Bits
#define READ			0x08U			// Read/Write Select
#define STBY 			0x04U			// Standby
#define CH_0		((uint8_t)0x00)		// 7705: AIN1(+) AIN1(-), 7706: AIN1 COMMON
#define CH_1		((uint8_t)0x01)		// 7705: AIN2(+) AIN2(-), 7706: AIN2 COMMON
#define CH_2		((uint8_t)0x02)		// 7705: AIN1(-) AIN1(-), 7706: COMMON COMMON
#define CH_3		((uint8_t)0x03)		// 7705: AIN1(-) AIN2(-), 7706: AIN3 COMMON
///////////////////////////////////////////////////////////////////
// Setup Register (RS:001)
///////////////////////////////////////////////////////////////////
#define FSYNC		((uint8_t)0x01)		// Filter Synchronization
#define BUF			((uint8_t)0x02)		// Buffer Control
#define	B_U			((uint8_t)0x04)		// 0: Bipolar Operation, 1: Unipolar Operation
#define BIPOLAR 		0x00u
#define UNIPOLAR		0x04u
#define MD_0		((uint8_t)0x00)		// Normal Mode
#define MD_1		((uint8_t)0x40)		// Self-Calibration
#define MD_2		((uint8_t)0x80)		// Zero-Scale System Calibration
#define MD_3		((uint8_t)0xC0)		// Full-Scale System Calibration
#define G_0			((uint8_t)0x00)		// Gain = 1		(0x00u<<3)
#define G_1			((uint8_t)0x08)		// Gain = 2		(0x01u<<3)
#define G_2			((uint8_t)0x10)		// Gain = 4		(0x02u<<3)
#define G_3			((uint8_t)0x18)		// Gain = 8		(0x03u<<3)
#define G_4			((uint8_t)0x20)		// Gain = 16	(0x04u<<3)
#define G_5			((uint8_t)0x28)		// Gain = 32	(0x05u<<3)
#define G_6			((uint8_t)0x30)		// Gain = 64	(0x06u<<3)
#define G_7			((uint8_t)0x38)		// Gain = 128	(0x07u<<3)
///////////////////////////////////////////////////////////////////
// Clock Register (RS:010)
///////////////////////////////////////////////////////////////////
#define CLKDIS		((uint8_t)0x10)		// Master Clock Disable Bit
#define CLKDIV		((uint8_t)0x08)		// Clock Divider Bit
#define CLK			((uint8_t)0x04)		// Clock Bit.
#define FS_0_0		((uint8_t)0x00)		// Code = 391, 19.98@1MHz
#define FS_0_1		((uint8_t)0x01)		// Code = 312, 25.04@1MHz
#define FS_0_2		((uint8_t)0x02)		// Code =  78, 100.2@1MHz
#define FS_0_3		((uint8_t)0x03)		// Code =  39, 200.3@1MHz
#define FS_1_0		((uint8_t)0x04)		// Code = 384, 50.0 @2.4576MHz
#define FS_1_1		((uint8_t)0x05)		// Code = 320, 60.0 @2.4576MHz
#define FS_1_2		((uint8_t)0x06)		// Code =  77, 249.4@2.4576MHz
#define FS_1_3		((uint8_t)0x07)		// Code =  38, 505.3@2.4576MHz
///////////////////////////////////////////////////////////////////
#define	AD7705_Shift_In()		(uint8_t)bus_SPI1xShift(0xFFu)
#define	AD7705_Shift_Out(_cout)	( void )bus_SPI1xShift(_cout)

///////////////////////////////////////////////////////////////////
// AD7705 访问控制接口
///////////////////////////////////////////////////////////////////
static	const	uint8_t	Cfg7705[CS7705_Max][2] =
{
    {
        BIPOLAR + G_5 + BUF,	// C1H0: pf
        BIPOLAR + G_4 + BUF,	// C1H1: pr
    },
    {
        BIPOLAR + G_5 + BUF,	// C2H0: pf
        BIPOLAR + G_4 + BUF,	// C2H1: pr
    },
    {
        BIPOLAR + G_5 + BUF,	// C3H0: pf
        BIPOLAR + G_4 + BUF,	// C3H1: pr
    }
};

/**
 *	对指定的通道进行转换。回读配置供主程序判断7705是否工作正常。
 */
static	uint8_t	_Convert7705( uint8_t mode_set, uint8_t xs )
{
	uint8_t	mode_readback;

    AD7705_Shift_Out( xs + RS_1 );			// 切换到指定的采样通道
    AD7705_Shift_Out( mode_set );			// 以指定的配置进行采样

    AD7705_Shift_Out( xs + RS_2 );			// Clock Register, 8 Bits
    AD7705_Shift_Out( CLKDIV + FS_1_0 );	// 50Hz @ 4.9152MHz

    AD7705_Shift_Out( xs + RS_4 );			// Test Register, 8 Bits
    AD7705_Shift_Out( 0x00u );				// Default: 0x00

    AD7705_Shift_Out( xs + RS_3 + READ );	//	读取数据以复位 DRDY#
    (void)AD7705_Shift_In();
    (void)AD7705_Shift_In();

    AD7705_Shift_Out( xs + RS_1 + READ );	//	回读配置
    mode_readback = AD7705_Shift_In();
	
	return	mode_readback;
}

bool	Convert7705( enum enumCS7705 cs, uint8_t xs )
{
	const uint8_t mode_set = MD_0 + Cfg7705[cs][xs];
	uint8_t	mode_readback;

	AD7705_Select( cs );
	mode_readback = _Convert7705( mode_set, xs );
	AD7705_Select( CS7705_none );
	
	if ( mode_set == mode_readback )
	{
		return	true;	//	工作正常
	}
	else
	{
		return	false;	//	工作有问题，没焊?!
	}
}

uint16_t	Readout7705( enum enumCS7705 cs, uint8_t xs )
{
    uint16_t	Result = 0u;
    uint8_t		iRetry;

    for ( iRetry = 20u; iRetry != 0u; --iRetry )
    {
		uint8_t	ReadyState;

		AD7705_Select( cs );
        AD7705_Shift_Out( RS_0 + READ + xs );
        ReadyState = AD7705_Shift_In();
        AD7705_Select( CS7705_none );			//	DeSelect All

        if ( 0x00u == ( ReadyState & DRDY ))
        {	//	is DRDY# ?
			uint8_t	ResultH, ResultL;

			AD7705_Select( cs );
            AD7705_Shift_Out( RS_3 + READ + xs );	//	Read.
            ResultH = AD7705_Shift_In();
            ResultL = AD7705_Shift_In();
            Result = ( ResultL + ( ResultH * 0x100u ));
			AD7705_Select( CS7705_none );			//	DeSelect All
			
			break;	//	done.
        }

        delay( 10u );
    }

    return	Result;
}

void	Initialize7705( void )
{
	uint8_t	i;

	for ( i = 0u; i < CS7705_Max; ++i )
	{
		AD7705_Select((enum enumCS7705) i );

		// Synchronization.
		AD7705_Shift_Out( 0xFFu );
		AD7705_Shift_Out( 0xFFu );
		AD7705_Shift_Out( 0xFFu );
		AD7705_Shift_Out( 0xFFu );
		AD7705_Shift_Out( 0xFFu );
		AD7705_Shift_Out( 0xFFu );
		AD7705_Shift_Out( 0xFFu );
		
		// 启动指定通道的自校准转换
		_Convert7705( MD_1 + Cfg7705[i][1], 1u );

		AD7705_Select( CS7705_none );
	}

	delay( 200u );

	for ( i = 0u; i < CS7705_Max; ++i )
	{
		AD7705_Select((enum enumCS7705) i );

		// 启动指定通道的自校准转换
		_Convert7705( MD_1 + Cfg7705[i][0], 0u );

		AD7705_Select( CS7705_none );
	}

	delay( 200u );
}

/////////////////////////////////////////////////// End of File /////
