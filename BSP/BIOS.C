#include "BSP.H"
#include "Pin.H"

#include "system_stm32f10x.c"

#define	PinBB( _Port, _Num )	(*(__IO int32_t *)(PERIPH_BB_BASE + ((uint32_t)&(_Port) - PERIPH_BASE) * 32u + (_Num) * 4u ))

#pragma O3
void	delay_us ( uint32_t us )
{
	while ( us-- )
	{
		__nop(); __nop(); __nop(); __nop(); __nop();
 		__nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop();
		__nop(); __nop(); __nop(); __nop(); __nop();
	}
}

void	delay( uint16_t ms )
{
	while ( ms-- )
	{
		delay_us( 999u );
	}
}


/**
 *	访问 OW 总线
 */

#define	_OW_Pin_1_Input()		(PinBB( GPIOB->IDR, 13U ))
#define	_OW_Pin_1_Output(_b)	(PinBB( GPIOB->ODR, 13U )=(_b))

#define	_OW_Pin_2_Input()		(PinBB( GPIOA->IDR, 8U ))
#define	_OW_Pin_2_Output(_b)	(PinBB( GPIOA->ODR, 8U )=(_b))

#define	_OW_Pin_3_Input()		(PinBB( GPIOA->IDR, 11U ))
#define	_OW_Pin_3_Output(_b)	(PinBB( GPIOA->ODR, 11U )=(_b))

#define	_OW_Pin_4_Input()		(PinBB( GPIOA->IDR, 15U ))
#define	_OW_Pin_4_Output(_b)	(PinBB( GPIOA->ODR, 15U )=(_b))

__svc_indirect(0)	BOOL	_SVCCall_Reset( BOOL (*)( void ));
__svc_indirect(0)	BOOL	_SVCCall_Slot ( BOOL (*)( BOOL ), BOOL Slot );

BOOL	OW_1_Init( void )
{
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPBEN );
	MODIFY_REG( GPIOB->CRH, 0x00F00000u, 0x00700000u );

	_OW_Pin_1_Output(1);
	delay_us( 5 );
	return	_OW_Pin_1_Input();
}

static	BOOL	_OW_1_Reset( void )
{
	BOOL	acknowledge;
	// 	DINT();
	delay_us( 1 );						// 延时G(0us)
	_OW_Pin_1_Output(0);				// 把总线拉为低电平
	delay_us( 480U );					// 延时H(480us)
	_OW_Pin_1_Output(1);				// 释放总线		
	delay_us( 70U );					// 延时I(70us)
	acknowledge = ! _OW_Pin_1_Input();	// 主机对总线采样, 0 表示总线上有应答, 1 表示无应答；
	// 	EINT();
	delay_us( 410U );					// 延时J(410us)
	return	acknowledge;
}

static	BOOL	_OW_1_Slot( BOOL bitOut )
{
	BOOL	bitIn;
	// 	DINT();
	_OW_Pin_1_Output(0);				// 将总线拉低启动一次时隙
	delay_us( 6U );						// 延时A(6us)
	_OW_Pin_1_Output(bitOut);			// 输出要写入总线的位
	delay_us( 9U );						// 延时E(9us)
	bitIn = _OW_Pin_1_Input();			// 主机对总线采样, 以读取从机上的数据
	delay_us( 45U );					// 延时C(60us)-A-E == 45us
	_OW_Pin_1_Output(1);				// 时隙开始后的60us总线浮空
	// 	EINT();
	delay_us( 10U );					// 延时D(10us), 等待总线恢复

	return	bitIn;
}

BOOL	OW_2_Init( void )
{
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	MODIFY_REG( GPIOA->CRH, 0x0000000Fu, 0x00000007u );

	_OW_Pin_2_Output(1);
	delay_us( 5 );
	return	_OW_Pin_2_Input();
}

static	BOOL	_OW_2_Reset( void )
{
	BOOL	acknowledge;
	// 	DINT();
	delay_us( 1 );						// 延时G(0us)
	_OW_Pin_2_Output(0);				// 把总线拉为低电平
	delay_us( 480U );					// 延时H(480us)
	_OW_Pin_2_Output(1);				// 释放总线		
	delay_us( 70U );					// 延时I(70us)
	acknowledge = ! _OW_Pin_2_Input();	// 主机对总线采样, 0 表示总线上有应答, 1 表示无应答；
	// 	EINT();
	delay_us( 410U );					// 延时J(410us)
	return	acknowledge;
}

static	BOOL	_OW_2_Slot( BOOL bitOut )
{
	BOOL	bitIn;
	// 	DINT();
	_OW_Pin_2_Output(0);				// 将总线拉低启动一次时隙
	delay_us( 6U );						// 延时A(6us)
	_OW_Pin_2_Output(bitOut);			// 输出要写入总线的位
	delay_us( 9U );						// 延时E(9us)
	bitIn = _OW_Pin_2_Input();			// 主机对总线采样, 以读取从机上的数据
	delay_us( 45U );					// 延时C(60us)-A-E == 45us
	_OW_Pin_2_Output(1);				// 时隙开始后的60us总线浮空
	// 	EINT();
	delay_us( 10U );					// 延时D(10us), 等待总线恢复

	return	bitIn;
}

BOOL	OW_3_Init( void )
{
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	MODIFY_REG( GPIOA->CRH, 0x0000F000u, 0x00007000u );
	_OW_Pin_3_Output(1);
	delay_us( 5 );
	return	_OW_Pin_3_Input();
}

static	BOOL	_OW_3_Reset( void )
{
	BOOL	acknowledge;
	// 	DINT();
	delay_us( 1 );						// 延时G(0us)
	_OW_Pin_3_Output(0);				// 把总线拉为低电平
	delay_us( 480U );					// 延时H(480us)
	_OW_Pin_3_Output(1);				// 释放总线		
	delay_us( 70U );					// 延时I(70us)
	acknowledge = ! _OW_Pin_3_Input();	// 主机对总线采样, 0 表示总线上有应答, 1 表示无应答；
	// 	EINT();
	delay_us( 410U );					// 延时J(410us)
	return	acknowledge;
}

static	BOOL	_OW_3_Slot( BOOL bitOut )
{
	BOOL	bitIn;
	// 	DINT();
	_OW_Pin_3_Output(0);				// 将总线拉低启动一次时隙
	delay_us( 6U );						// 延时A(6us)
	_OW_Pin_3_Output(bitOut);			// 输出要写入总线的位
	delay_us( 9U );						// 延时E(9us)
	bitIn = _OW_Pin_3_Input();			// 主机对总线采样, 以读取从机上的数据
	delay_us( 45U );					// 延时C(60us)-A-E == 45us
	_OW_Pin_3_Output(1);				// 时隙开始后的60us总线浮空
	// 	EINT();
	delay_us( 10U );					// 延时D(10us), 等待总线恢复

	return	bitIn;
}

BOOL	OW_4_Init( void )
{
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	MODIFY_REG( GPIOA->CRH, 0xF0000000u, 0x70000000u );

	_OW_Pin_4_Output(1);
	delay_us( 5 );
	return	_OW_Pin_4_Input();
}

static	BOOL	_OW_4_Reset( void )
{
	BOOL	acknowledge;
	// 	DINT();
	delay_us( 1 );						// 延时G(0us)
	_OW_Pin_4_Output(0);				// 把总线拉为低电平
	delay_us( 480U );					// 延时H(480us)
	_OW_Pin_4_Output(1);				// 释放总线		
	delay_us( 70U );					// 延时I(70us)
	acknowledge = ! _OW_Pin_4_Input();	// 主机对总线采样, 0 表示总线上有应答, 1 表示无应答；
	// 	EINT();
	delay_us( 410U );					// 延时J(410us)
	return	acknowledge;
}

static	BOOL	_OW_4_Slot( BOOL bitOut )
{
	BOOL	bitIn;
	// 	DINT();
	_OW_Pin_4_Output(0);				// 将总线拉低启动一次时隙
	delay_us( 6U );						// 延时A(6us)
	_OW_Pin_4_Output(bitOut);			// 输出要写入总线的位
	delay_us( 9U );						// 延时E(9us)
	bitIn = _OW_Pin_4_Input();			// 主机对总线采样, 以读取从机上的数据
	delay_us( 45U );					// 延时C(60us)-A-E == 45us
	_OW_Pin_4_Output(1);				// 时隙开始后的60us总线浮空
	// 	EINT();
	delay_us( 10U );					// 延时D(10us), 等待总线恢复

	return	bitIn;
}
BOOL	OW_1_Reset( void )
{
	return	_SVCCall_Reset( _OW_1_Reset );
}

BOOL	OW_1_Slot( BOOL bitOut )
{
	return	_SVCCall_Slot ( _OW_1_Slot, bitOut );
}

BOOL	OW_2_Reset( void )
{
	return	_SVCCall_Reset( _OW_2_Reset );
}

BOOL	OW_2_Slot( BOOL bitOut )
{
	return	_SVCCall_Slot ( _OW_2_Slot, bitOut );
}

BOOL	OW_3_Reset( void )
{
	return	_SVCCall_Reset( _OW_3_Reset );
}

BOOL	OW_3_Slot( BOOL bitOut )
{
	return	_SVCCall_Slot ( _OW_3_Slot, bitOut );
}

BOOL	OW_4_Reset( void )
{
	return	_SVCCall_Reset( _OW_4_Reset );
}

BOOL	OW_4_Slot( BOOL bitOut )
{
	return	_SVCCall_Slot ( _OW_4_Slot, bitOut );
}
/**
 *	访问I2C总线
 */
#define	Pin_I2C_SCL_In		PinBB( GPIOB->IDR, 6U )
#define	Pin_I2C_SCL_Out		PinBB( GPIOB->ODR, 6U )
#define	Pin_I2C_SDA_In		PinBB( GPIOB->IDR, 7U )
#define	Pin_I2C_SDA_Out		PinBB( GPIOB->ODR, 7U )


BOOL	bus_i2c_start ( uint8_t Address8Bit, enum I2C_DirectSet DirectSet )
{
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPBEN );
 	MODIFY_REG( GPIOB->CRL, 0xFF000000u, 0x77000000u );

	//	Verify bus available.
	Pin_I2C_SDA_Out = 1;
	Pin_I2C_SCL_Out = 1;
	delay_us( 10 );
	if( ! Pin_I2C_SDA_In ){	return	FALSE;	}
	if( ! Pin_I2C_SCL_In ){	return	FALSE;	}

	Pin_I2C_SDA_Out = 0;
	delay_us( 1 );
	Pin_I2C_SCL_Out = 0;

	if ( I2C_Write == DirectSet )
	{
		return	bus_i2c_shout( Address8Bit & 0xFEu );
	}
	else
	{
		return	bus_i2c_shout( Address8Bit | 0x01u );
	}
}

void	bus_i2c_stop ( void )
{
	Pin_I2C_SDA_Out = 0;
	delay_us( 1 );
	Pin_I2C_SCL_Out = 1;
	delay_us( 1 );
	Pin_I2C_SDA_Out = 1;
	delay_us( 1 );
}

BOOL	bus_i2c_shout ( uint8_t cOutByte )
{
	BOOL	AcknowlegeState;
	uint8_t	i;

	for( i = 8U; i != 0U; --i )
	{
		if ( cOutByte & 0x80u )
		{
			Pin_I2C_SDA_Out = 1;
		}
		else
		{
			Pin_I2C_SDA_Out = 0;
		}
		cOutByte <<= 1;

		delay_us( 1 );
		Pin_I2C_SCL_Out = 1;

		delay_us( 1 );
		Pin_I2C_SCL_Out = 0;
	}
	
	Pin_I2C_SDA_Out = 1;
	delay_us( 1 );
	Pin_I2C_SCL_Out = 1;
	delay_us( 1 );
 	AcknowlegeState	= Pin_I2C_SDA_In;
	Pin_I2C_SCL_Out = 0;

	if ( I2C_ACK != AcknowlegeState )
	{
		return	FALSE;
	}
	else
	{
		return	TRUE;
	}
}

uint8_t	bus_i2c_shin( enum I2C_AcknowlegeSet AcknowlegeSet )
{
	uint8_t		cInByte = 0U;
	uint8_t		i;

	Pin_I2C_SDA_Out = 1;		// make SDA an input
	for( i = 8U; i != 0U; --i )
	{
		delay_us( 1 );
		Pin_I2C_SCL_Out = 1;

		delay_us( 1 );
		cInByte <<= 1;
		if ( Pin_I2C_SDA_In )
		{
			cInByte |= 0x01u;
		}
		else 
		{
			cInByte &= 0xFEu;
		}

		Pin_I2C_SCL_Out = 0;
	}

	if ( I2C_ACK == AcknowlegeSet )
	{
		Pin_I2C_SDA_Out = 0;
	}
	else
	{
		Pin_I2C_SDA_Out = 1;
	}
	delay_us( 1 );
	Pin_I2C_SCL_Out = 1;
	delay_us( 1 );
	Pin_I2C_SCL_Out = 0;

	return	cInByte;
}


/**
 *	访问SPI总线
 */
void	bus_SPI1xPortInit( void )
{
	/* Initialize and enable the SSP Interface module. */
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN );
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_AFIOEN | RCC_APB2ENR_SPI1EN );
	MODIFY_REG( AFIO->MAPR, AFIO_MAPR_SWJ_CFG, AFIO_MAPR_SWJ_CFG_JTAGDISABLE );
	SET_BIT( AFIO->MAPR, AFIO_MAPR_SPI1_REMAP );

	/* SPI1_SCK, SPI1_MISO, SPI1_MOSI are SPI pins. */
 	MODIFY_REG( GPIOB->CRL, 0x00FFF000u, 0x00B4B000u );

	/* Enable SPI in Master Mode, CPOL=1, CPHA=1. */
	SPI1->CR1 = SPI_CR1_SSI	| SPI_CR1_SSM	| SPI_CR1_SPE	| SPI_CR1_BR |
				SPI_CR1_MSTR | SPI_CR1_CPHA | SPI_CR1_CPOL;
	SPI1->CR2 = 0x0000u;
}

uint8_t bus_SPI1xShift( uint8_t OutByte )
{
	uint8_t	inByte;
	
	SPI1->DR = OutByte;	 
	while ( ! ( SPI1->SR & SPI_SR_RXNE )){}
	inByte = SPI1->DR;

	return	inByte;
}

/**
 *	访问 STM32 ADC
 */
volatile	uint16_t	ADCResultBuf[10][4];

static	void	STM32_ADC_Init( void )
{
	//	DMA1 channel1 configuration
	SET_BIT( RCC->AHBENR,	RCC_AHBENR_DMA1EN );
	//	DMA模块禁能, 重新配置
	DMA1_Channel1->CCR	 = 0u;
	DMA1_Channel1->CCR	 = DMA_CCR1_PL | DMA_CCR1_MINC | DMA_CCR1_PSIZE_0 | DMA_CCR1_MSIZE_0 | DMA_CCR1_CIRC;
	//	设置DMA外设地址, 内存地址, 传输数量
	DMA1_Channel1->CPAR	= (uint32_t)&ADC1->DR;				
	DMA1_Channel1->CMAR	= (uint32_t)ADCResultBuf;			
	DMA1_Channel1->CNDTR = ( sizeof(ADCResultBuf)/sizeof(ADCResultBuf[0][0]));
	SET_BIT( DMA1_Channel1->CCR, DMA_CCR1_EN );				//	DMA通道使能

	//	配置 GPIO
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	MODIFY_REG( GPIOA->CRL, 0x00FF0000u, 0x00000000u );

	//	配置 ADC1
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_ADC1EN );
	MODIFY_REG( RCC->CFGR , RCC_CFGR_ADCPRE, RCC_CFGR_ADCPRE_DIV6 );

	// 配置控制寄存器
	ADC1->CR1 = ADC_CR1_SCAN;
	ADC1->CR2 = ADC_CR2_ADON | ADC_CR2_TSVREFE | ADC_CR2_CONT | ADC_CR2_DMA;

	//	配置通道采样时间（通道编号从0开始）
	#define	ADC1_SMP_1p5_C		0	// 1.5 cycles
	#define	ADC1_SMP_7p5_C		1	// 7.5 cycles
	#define	ADC1_SMP_13p5_C		2	// 13.5 cycles
	#define	ADC1_SMP_28p5_C		3	// 28.5 cycles
	#define	ADC1_SMP_41p5_C		4	// 41.5 cycles
	#define	ADC1_SMP_55p5_C		5	// 55.5 cycles
	#define	ADC1_SMP_71p5_C		6	// 71.5 cycles
	#define	ADC1_SMP_239p5_C	7	// 239.5 cycles

	#define	ADC1_SMPR1_BASE		10	// R1 起始编号
	#define	ADC1_SMPR2_BASE		0	// R2 起始编号
	#define	ADC1_SMPR_WIDTH		3	// 每组设置3位

	ADC1->SMPR2 =	(( ADC1_SMP_239p5_C ) << ( ADC1_SMPR_WIDTH * (	0 - ADC1_SMPR2_BASE )))	//	通道0
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * (	1 - ADC1_SMPR2_BASE )))	//	通道1
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * (	2 - ADC1_SMPR2_BASE )))	//	通道2
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * (	3 - ADC1_SMPR2_BASE )))	//	通道3
				|	(( ADC1_SMP_239p5_C ) << ( ADC1_SMPR_WIDTH * (	4 - ADC1_SMPR2_BASE )))	//	通道4
				|	(( ADC1_SMP_239p5_C ) << ( ADC1_SMPR_WIDTH * (	5 - ADC1_SMPR2_BASE )))	//	通道5
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * (	6 - ADC1_SMPR2_BASE )))	//	通道6
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * (	7 - ADC1_SMPR2_BASE )))	//	通道7
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * (	8 - ADC1_SMPR2_BASE )))	//	通道8
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * (	9 - ADC1_SMPR2_BASE )))	//	通道9
				;
	ADC1->SMPR1 =	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * ( 10 - ADC1_SMPR1_BASE )))	//	通道10
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * ( 11 - ADC1_SMPR1_BASE )))	//	通道11
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * ( 12 - ADC1_SMPR1_BASE )))	//	通道12
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * ( 13 - ADC1_SMPR1_BASE )))	//	通道13
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * ( 14 - ADC1_SMPR1_BASE )))	//	通道14
				|	(( ADC1_SMP_1p5_C	 ) << ( ADC1_SMPR_WIDTH * ( 15 - ADC1_SMPR1_BASE )))	//	通道15
				|	(( ADC1_SMP_41p5_C	) << ( ADC1_SMPR_WIDTH * ( 16 - ADC1_SMPR1_BASE )))	//	通道16(内部温度)
				|	(( ADC1_SMP_41p5_C	) << ( ADC1_SMPR_WIDTH * ( 17 - ADC1_SMPR1_BASE )))	//	通道17(内部基准)
				;

	//	配置规则转换序列（序列编号从1开始）
	#define	ADC1_SQR1_BASE		13// R1 起始编号
	#define	ADC1_SQR2_BASE		7	// R2 起始编号
	#define	ADC1_SQR3_BASE		1	// R3 起始编号
	#define	ADC1_SQR_WIDTH		5	// 每组设置5位

	ADC1->SQR3	=	(( 17 )	<< ( ADC1_SQR_WIDTH * (	1 - ADC1_SQR3_BASE )))	//	序列1
				|	(( 16 )	<< ( ADC1_SQR_WIDTH * (	2 - ADC1_SQR3_BASE )))	//	序列2
				|	((	4 ) << ( ADC1_SQR_WIDTH * (	3 - ADC1_SQR3_BASE )))	//	序列3
				|	((	5 ) << ( ADC1_SQR_WIDTH * (	4 - ADC1_SQR3_BASE )))	//	序列4
				|	((	0 ) << ( ADC1_SQR_WIDTH * (	5 - ADC1_SQR3_BASE )))	//	序列5
				|	((	0 ) << ( ADC1_SQR_WIDTH * (	6 - ADC1_SQR3_BASE )))	//	序列6
				;
	ADC1->SQR2	=	((	0 ) << ( ADC1_SQR_WIDTH * (	7 - ADC1_SQR2_BASE )))	//	序列7
				|	((	0 ) << ( ADC1_SQR_WIDTH * (	8 - ADC1_SQR2_BASE )))	//	序列8
				|	((	0 ) << ( ADC1_SQR_WIDTH * (	9 - ADC1_SQR2_BASE )))	//	序列9
				|	((	0 ) << ( ADC1_SQR_WIDTH * ( 10 - ADC1_SQR2_BASE )))	//	序列10
				|	((	0 ) << ( ADC1_SQR_WIDTH * ( 11 - ADC1_SQR2_BASE )))	//	序列11
				|	((	0 ) << ( ADC1_SQR_WIDTH * ( 12 - ADC1_SQR2_BASE )))	//	序列12
				;
	ADC1->SQR1	=	((	0 ) << ( ADC1_SQR_WIDTH * ( 13 - ADC1_SQR1_BASE )))	//	序列13
				|	((	0 ) << ( ADC1_SQR_WIDTH * ( 14 - ADC1_SQR1_BASE )))	//	序列14
				|	((	0 ) << ( ADC1_SQR_WIDTH * ( 15 - ADC1_SQR1_BASE )))	//	序列15
				|	((	0 ) << ( ADC1_SQR_WIDTH * ( 16 - ADC1_SQR1_BASE )))	//	序列16
				|	((4 -1) << ( ADC1_SQR_WIDTH * ( 17 - ADC1_SQR1_BASE )))	//	长度(0表示1次)
				;

	delay_us( 1u );
	SET_BIT( ADC1->CR2, ADC_CR2_ADON );							//	Enable ADC1
	SET_BIT( ADC1->CR2, ADC_CR2_RSTCAL );						//	Reset calibration register
	while ( READ_BIT( ADC1->CR2, ADC_CR2_RSTCAL ));				//	Check the end of ADC1 reset calibration register
	SET_BIT( ADC1->CR2, ADC_CR2_CAL );							//	Start ADC1 calibration
	while ( READ_BIT( ADC1->CR2, ADC_CR2_CAL ));				//	Check the end of ADC1 calibration
	SET_BIT( ADC1->CR2, ADC_CR2_SWSTART | ADC_CR2_EXTTRIG );		//	Start ADC1 Software Conversion
}

uint16_t	STM32_ADC1_Readout( uint8_t Channel )
{
	uint16_t	sum;
	uint16_t	i;
	
	sum = 0u;
	for ( i = 0u; i < sizeof(ADCResultBuf)/sizeof(ADCResultBuf[0]); ++i )
	{
		sum += ADCResultBuf[i][Channel];
	}

	return	sum / (sizeof(ADCResultBuf)/sizeof(ADCResultBuf[0]));
}

/********************************** 功能说明 ***********************************
*	使用 TIM1 计数 ETR 脉冲，测量风扇转动圈数。
*******************************************************************************/
static	void	TIM1_Init( void )
{
	TIM_TypeDef	* TIMx = TIM1;

	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_TIM1EN );

	TIMx->SMCR	= TIM_SMCR_ETP 
				| TIM_SMCR_ETF_3
				| TIM_SMCR_ETPS_0
				;
	GPIOA->CRH&=0XFFF0FFFF; //PA12 清除之前设置   
	GPIOA->CRH|=0X00080000; //PA12 上拉下拉输入    
// 	GPIOA->ODR&=0XEFFF;	
	/* Enable the TIM Counter */
	SET_BIT( TIMx->SMCR, TIM_SMCR_ECE );
 	SET_BIT( TIMx->CR1, TIM_CR1_CEN );

	// 	//	使用 TI2
	//  	CLEAR_BIT( TIMx->CCER,  TIM_CCER_CC2E );
	//  	MODIFY_REG( TIMx->CCMR1, TIM_CCMR1_CC2S | TIM_CCMR1_IC2F, TIM_CCMR1_CC2S_0 );
	//  	SET_BIT(   TIMx->CCER,  TIM_CCER_CC2P | TIM_CCER_CC2E );

	// 	/* Select the Trigger source */
	// 	MODIFY_REG( TIMx->SMCR, TIM_SMCR_TS, TIM_SMCR_TS_2 | TIM_SMCR_TS_1 );	//	TIM_TIxExternalCLK1Source_TI2
	// 	/* Select the External clock mode1 */
	// 	SET_BIT( TIMx->SMCR, TIM_SMCR_SMS );	//	TIM_SlaveMode_External1
}
//}

/**
 *	访问 PWM (TIM2)
 */
void	MotorPWM_SetOutput( uint16_t OutValue );	//	PA1(TIM2_CH2,高电平有效)
void	PWM1_SetOutput( uint16_t OutValue );		//	PA6(TIM3_CH1,低电平有效)
void	PWM2_SetOutput( uint16_t OutValue );		//	PA7(TIM3_CH2,低电平有效)

#define	TIM2_OutputMax	20000u
#define	TIM3_OutputMax	1000u
#define	TIM15_OutputMax	1000u
static	void	TIM2_Configure( void )
{
	TIM_TypeDef	* TIMx = TIM2;

	SET_BIT( RCC->APB1ENR, RCC_APB1ENR_TIM2EN );

	//	时基初始化: 输入时钟频率2.4MHz
	TIMx->CR1	= 0u;
	TIMx->PSC	= 0u;
	TIMx->ARR	= TIM2_OutputMax	- 1u;
	TIMx->EGR	= TIM_EGR_UG;
	TIMx->CR1	= TIM_CR1_CEN;

	//	输出配置
	TIMx->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1;
	TIMx->CCER	= TIM_CCER_CC1E;

	//	配置端口
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	MODIFY_REG( GPIOA->CRL, 0x0000000Fu, 0x0000000Bu );
}

static	void	TIM3_Configure( void )
{
	TIM_TypeDef	* TIMx = TIM3;

	SET_BIT( RCC->APB1ENR, RCC_APB1ENR_TIM3EN );
	//	时基初始化: 输入时钟频率2.4MHz
	TIMx->CR1	= 0u;
	TIMx->PSC	= 0u;
	TIMx->ARR	= TIM3_OutputMax	- 1u;
	TIMx->EGR	= TIM_EGR_UG;
	TIMx->CR1	= TIM_CR1_CEN;

	//	输出配置
	TIMx->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1
				| TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
	TIMx->CCMR2 = TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1
				| TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1;
	TIMx->CCER	= TIM_CCER_CC1E | TIM_CCER_CC1P
				| TIM_CCER_CC2E | TIM_CCER_CC2P |
								TIM_CCER_CC3E | TIM_CCER_CC3P
				| TIM_CCER_CC4E | TIM_CCER_CC4P;


	//	配置端口
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	MODIFY_REG( GPIOA->CRL, 0xFF000000u, 0xBB000000u );
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPBEN );
	MODIFY_REG( GPIOB->CRL, 0x000000FFu, 0x000000BBu );
}
void	TIM15_Configure( void )
{
	TIM_TypeDef	* TIMx = TIM15;

	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_TIM15EN );
	//	时基初始化: 输入时钟频率1KHz
	TIMx->CR1	= 0u;
	TIMx->PSC	= 24000 - 1u;	//	恒温箱温度控制 频率放低
	TIMx->ARR	= TIM15_OutputMax	- 1u;	//	999u;
	TIMx->EGR	= TIM_EGR_UG;
	TIMx->CR1	= TIM_CR1_CEN;

	//	输出配置
	TIMx->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1
				| TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
	TIMx->CCER	= TIM_CCER_CC1E// | TIM_CCER_CC1P
				| TIM_CCER_CC2E// | TIM_CCER_CC2P
				;

	//	配置端口
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_AFIOEN );
	SET_BIT( AFIO->MAPR2, AFIO_MAPR2_TIM15_REMAP );//							AFIO_MAPR_SWJ_CFG, AFIO_MAPR_SWJ_CFG_JTAGDISABLE );
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPBEN );
	MODIFY_REG( GPIOB->CRH, 0xFF000000u, 0xBB000000u );
	
	SET_BIT( TIMx->BDTR, TIM_BDTR_MOE );
}

void	MotorPWM_SetOutput( uint16_t OutValue )	//TSP
{
	TIM2->CCR1 = (uint32_t)OutValue * TIM2_OutputMax / 27648u;
}

void	PWM1_SetOutput( uint16_t OutValue )//A
{
	TIM3->CCR3 = (uint32_t)OutValue * TIM3_OutputMax / 27648u;
}

void	PWM2_SetOutput( uint16_t OutValue )//B
{
	TIM3->CCR4 = (uint32_t)OutValue * TIM3_OutputMax / 27648u;
}

void	PWM3_SetOutput( uint16_t OutValue )//C
{
	TIM3->CCR1 = (uint32_t)OutValue * TIM3_OutputMax / 27648u;
}

void	PWM4_SetOutput( uint16_t OutValue )//D
{
	TIM3->CCR2 = (uint32_t)OutValue * TIM3_OutputMax / 27648u;
}

void	HCBoxHeat_OutCmd( uint16_t OutValue )			//	PB14(高电平有效)
{	//	PB.14
	TIM15->CCR1 = (uint32_t)OutValue;
}


void	HCBoxCool_OutCmd( uint16_t OutValue )		//	PB15(高电平有效)
{	//	PB.15
	TIM15->CCR2 = (uint32_t)OutValue;
}


/**
 *	访问 GPIO
 */
//	控制485总线方向
void	MB_485_Direct_Transmit( void )
{
//	GPIOB->BSRR = ( 1 << 2 );
	MonitorTickReset();
}

void	MB_485_Direct_Receive( void )
{
//	GPIOB->BRR	= ( 1 << 2 );
}

void	Select7705( uint8_t C_SEL )
{
	//	Port Configure
	SET_BIT ( RCC->APB2ENR, RCC_APB2ENR_IOPBEN );
	MODIFY_REG ( GPIOB->CRH, 0x000FFFFFu, 0x00033333u );

	switch ( C_SEL )
	{
	case CS7705_1:	GPIOB->BSRR =(GPIO_BSRR_BR8)| GPIO_BSRR_BS12 | GPIO_BSRR_BS11 | GPIO_BSRR_BS9 | GPIO_BSRR_BS10;		break;	//	对应位复位		粉尘
	case CS7705_2:	GPIOB->BSRR =	GPIO_BSRR_BS8 |(GPIO_BSRR_BR12)| GPIO_BSRR_BS11 | GPIO_BSRR_BS9 | GPIO_BSRR_BS10;		break;	//	对应位复位		日均A
	case CS7705_3:	GPIOB->BSRR =	GPIO_BSRR_BS8 | GPIO_BSRR_BS12 |(GPIO_BSRR_BR11)| GPIO_BSRR_BS9 | GPIO_BSRR_BS10;		break;	//	对应位复位		日均B
	case CS7705_4:	GPIOB->BSRR =	GPIO_BSRR_BS8 | GPIO_BSRR_BS12 | GPIO_BSRR_BS11 |(GPIO_BSRR_BR9)| GPIO_BSRR_BS10;		break;	//	对应位复位		时均C	
	case CS7705_5:	GPIOB->BSRR =	GPIO_BSRR_BS8 | GPIO_BSRR_BS12 | GPIO_BSRR_BS11 | GPIO_BSRR_BS9 |(GPIO_BSRR_BR10);		break;	//	对应位复位		时均D
//case CS7705_all:	GPIOA->BSRR = (GPIO_BSRR_BR2)|(GPIO_BSRR_BR3)|(GPIO_BSRR_BR4);	break;	//	全部复位
	default:
	case CS7705_none: GPIOB->BSRR = GPIO_BSRR_BS8| GPIO_BSRR_BS9 | GPIO_BSRR_BS10| GPIO_BSRR_BS11 | GPIO_BSRR_BS12;	break;	//	全部置位
	}
}	

void	Fan1_OutCmd( bool NewState )		//	PA1(高电平有效)A 电源
{
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	GPIOA->BSRR = ( NewState ? GPIO_BSRR_BS1 : GPIO_BSRR_BR1 );
	MODIFY_REG( GPIOA->CRL, 0x000000F0u, 0x00000030u );
}

void	Fan2_OutCmd( bool NewState )		//	PA2(高电平有效)B 电机
{
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	GPIOA->BSRR = ( NewState ? GPIO_BSRR_BS2 : GPIO_BSRR_BR2 );
	MODIFY_REG( GPIOA->CRL, 0x00000F00u, 0x00000300u );
}

void	Heater_OutCmd( bool	NewState )		//	PC13(高电平有效)
{//	PC.13
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPCEN );
	GPIOC->BSRR = ( NewState ? GPIO_BSRR_BS13 : GPIO_BSRR_BR13 );
	MODIFY_REG( GPIOC->CRH, 0x00F00000u, 0x00300000u );
}
void	PowerAIR_OutCmd( bool	NewState )		//	PC15(高电平有效)
{//	PC.15
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPCEN );
	GPIOC->BSRR = ( NewState ? GPIO_BSRR_BS15 : GPIO_BSRR_BR15 );
	MODIFY_REG( GPIOC->CRH, 0xF0000000u, 0x30000000u );
}

void	HCBoxFan_OutCmd( BOOL NewState )		//	PA3(高电平有效)
{	//	PA.3, on-off mode
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPAEN );
	GPIOA->BSRR = NewState ? GPIO_BSRR_BS3 : GPIO_BSRR_BR3;
	MODIFY_REG( GPIOA->CRL, 0x0000F000u, 0x00003000u );
}


void	MotorPower_OutCmd( bool NewState )	//	PC14(高电平有效)	 //TSP
{//	PC.14
	SET_BIT( RCC->APB2ENR, RCC_APB2ENR_IOPCEN );
	GPIOC->BSRR = ( NewState ? GPIO_BSRR_BS14 : GPIO_BSRR_BR14 );
	MODIFY_REG( GPIOC->CRH, 0x0F000000u, 0x03000000u );
}


/********************************** 功能说明 ***********************************
*	当通讯失效时，延时一段时间后，自动关闭电机控制输出。
*********************************** 设计说明 ***********************************/
static	uint32_t m_tick;
void	MonitorTickReset( void )
{
		m_tick = 0u;
}


bool	MonitorTickTimeout( void )
{
	const	uint32_t	Interval_set = 50;	//	5 sec.

	return	( m_tick >= Interval_set );	//	超时返回TRUE.
}


void	MonitorTickInit( void )
{
	m_tick = 0u;
	SysTick_Config( SystemCoreClock / 10 );	//	中断频率10Hz	//100ms
}


uint8_t HCBoxFlag = FALSE;
__irq
void	SysTick_Handler( void )
{	
	static	uint32_t		HCBoxtick;
	uint32_t		tmp = m_tick + 1;
	
	if ( ( tmp != 0u ) )	//	未溢出
	{
		m_tick = tmp;
	}
	if( ( ++ HCBoxtick ) % 10 == 0 )
	{
		HCBoxFlag = TRUE;
	}
}

/********************************** 功能说明 ***********************************
*	不使用操作系统时，需要自定义SVC的套接程序
*********************************** 设计说明 ***********************************
用户触发SVC中断，如果不能立即响应，会上访成硬件故障，所以，触发时要格外小心。
触发SVC后，可能会发生别的中断抢占，所以入口参数不能直接用，应使用堆栈中的值。
在用户级线程模式下，入口参数使用PSP栈，所以取参数时需要判断使用的是哪个堆栈。
*******************************************************************************/
__irq
__asm	void	SVC_Handler (void)
{
	// 判断入口参数保存在哪个栈中，并取回入口参数
	TST			LR, #0x4 				;// 测试EXC_RETURN的第2位
	ITE	 	EQ											;
	MRSEQ	R0, MSP					;// 0: 使用主堆栈，故把MSP的值取出
	MRSNE	R0, PSP 				;// 1: 使用进程栈，故把PSP的值取出

// 	LDR		R1, [R0,#24] 			;// 从栈中读取PC的值
// 	LDRB	R0, [R1,#-2]			;// 从SVC指令中读取立即数放到R0
	LDM		 R0, { R0-R3, R12 }			;// 从栈中取得入口参数
	
	//	执行用户程序
	PUSH	{ LR }
	BLX		 R12										 ; Call SVC Function 
	POP		{ LR }

	//	判断入口参数保存在哪个栈中，并回写返回值
	TST		 LR, #2					;// 测试EXC_RETURN的第2位
	ITE	 	EQ											;
	MRSEQ	 R12, MSP								;// 0: 使用主堆栈(MSP)
	MRSNE	 R12, PSP								;// 1: 使用进程栈(PSP)
	STM		 R12,{ R0-R3 }			;// 回写函数返回值

	//	中断返回
	BX			LR
}


void	BIOS_Init( void )
{
	//	配置PendSV为最低的优先级
	NVIC_SetPriorityGrouping( 7 );
	NVIC_SetPriority( PendSV_IRQn, 255u );

	STM32_ADC_Init();		//	NTC
	TIM1_Init();				//	计数器
	TIM2_Configure();		//	粉尘泵
	TIM3_Configure();		//	小流量
	TIM15_Configure();	//	恒温箱
	bus_SPI1xPortInit();//	Modbus
	
	MonitorTickInit();	//	泵保护
}

/********	(C) COPYRIGHT 2015 青岛金仕达电子科技有限公司	**** End Of File ****/
