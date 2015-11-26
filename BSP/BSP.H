#include <stdint.h>
#ifndef	__bool_true_false_are_defined
	#include <stdbool.h>
	typedef	bool	BOOL;
	#define	TRUE	true
	#define	FALSE	false
#endif

typedef	float	FP32;
	
#include <assert.h>

#define	LOBYTE(__w)		((uint8_t)((__w)&0x0FF))
#define	HIBYTE(__w)		((uint8_t)(((__w)>>8)&0x0FF))

extern	void	delay( uint16_t ms );
extern	void	delay_us ( uint32_t us );

//	BIOS.C - OneWire ( DS18B20 )
extern	BOOL	OW_1_Init( void );
extern	BOOL	OW_1_Reset( void );
extern	BOOL	OW_1_Slot( BOOL bitOut );

extern	BOOL	OW_2_Init( void );
extern	BOOL	OW_2_Reset( void );
extern	BOOL	OW_2_Slot( BOOL bitOut );

extern	BOOL	OW_3_Init( void );
extern	BOOL	OW_3_Reset( void );
extern	BOOL	OW_3_Slot( BOOL bitOut );

extern	BOOL	OW_4_Init( void );
extern	BOOL	OW_4_Reset( void );
extern	BOOL	OW_4_Slot( BOOL bitOut );

//	BIOS.C - I2C
enum	I2C_DirectSet { I2C_Write, I2C_Read };
enum	I2C_AcknowlegeSet { I2C_ACK, I2C_NoACK };
extern	BOOL	bus_i2c_start( uint8_t Address8Bit, enum I2C_DirectSet DirectSet );
extern	void	bus_i2c_stop( void );
extern	BOOL	bus_i2c_shout( uint8_t OutByte );
extern	uint8_t	bus_i2c_shin( enum I2C_AcknowlegeSet AcknowlegeSet );

//	BIOS.C - SPI
extern	uint8_t bus_SPI1xShift( uint8_t OutByte );
extern	void	bus_SPI1xPortInit( void );

//	BIOS.C - GPIO
extern	void	MB_485_Direct_Transmit( void );
extern	void	MB_485_Direct_Receive( void );

void	MotorPower_OutCmd   ( bool NewState );		//	PC14(高电平有效)
void	Heater_OutCmd( bool	NewState );				//	PC13(高电平有效)
void	Fan2_OutCmd   ( bool NewState );			//	PA1(高电平有效)
void	Fan1_OutCmd   ( bool NewState );			//	PA2(高电平有效)
void	PowerAIR_OutCmd( bool	NewState );		//	PC15(高电平有效)
void	HCBoxHeat_OutCmd( uint16_t OutValue );		//	PB14(高电平有效)
void	HCBoxCool_OutCmd( uint16_t OutValue );		//	PB15(高电平有效)
void	HCBoxFan_OutCmd( BOOL NewState );

void	MotorPWM_SetOutput( uint16_t OutValue );	//	PA1(高电平有效)
void	PWM1_SetOutput( uint16_t OutValue );		//	PA6(低电平有效)
void	PWM2_SetOutput( uint16_t OutValue );		//	PA7(低电平有效)
void	PWM3_SetOutput( uint16_t OutValue );		//	PA6(低电平有效)
void	PWM4_SetOutput( uint16_t OutValue );		//	PA7(低电平有效)

extern	void	PWM1_OutputSet( uint16_t OutValue );
extern	void	DAC1_OutputSet( uint16_t OutValue );
extern	void	HVPower_OutCmd( BOOL NewState );
extern	uint16_t	STM32_ADC1_Readout( uint8_t Channel );

extern	void	BIOS_Init( void );

extern	uint16_t	get_VDD_Volt( void );
extern	int16_t 	get_CPU_Temp( void );
extern	uint16_t	get_Bat_Volt( void );
extern	uint16_t	get_HV_Volt( void );


//	SENSOR
extern	FP32	get_NTC1_Temp( void );
extern	FP32	get_NTC2_Temp( void );

//	DS18B20
extern	BOOL	DS18B20_1_Read( int16_t * pT16S );
extern	BOOL	DS18B20_2_Read( int16_t * pT16S );
extern	BOOL	DS18B20_3_Read( int16_t * pT16S );
extern	BOOL	DS18B20_4_Read( int16_t * pT16S );
//////	AD7705
enum	enumCS7705
{
  CS7705_1, CS7705_2, CS7705_3, 
	CS7705_4, CS7705_5, CS7705_Max,
	CS7705_none,
};

extern	void	Select7705( uint8_t cs );

extern  void	Initialize7705( void );
extern	bool		Convert7705( enum enumCS7705 cs, uint8_t xs );
extern	uint16_t	Readout7705( enum enumCS7705 cs, uint8_t xs );

extern	void	MODBUS_Init( uint8_t MBAddress );
extern	uint16_t	usRegInputBuf[];
extern	uint16_t	usRegHoldingBuf[];
extern	uint8_t		ucRegDiscBuf[];
extern	uint8_t		ucRegCoilsBuf[];

//HCBox
extern	uint8_t HCBoxFlag;
extern	void	HCBoxControl( void );
extern	uint16_t	get_HCBoxFanSpeed( void );
extern	uint16_t	get_HCBoxOutput( void );
//BIOS.C - MOTORSHUT
void	MonitorTickReset( void );
bool	MonitorTickTimeout( void );
void	MonitorTickInit( void );

extern	bool	FM24C64_Save( uint16_t Address, const uint8_t * Buffer, uint16_t NBytes );
extern	bool	FM24C64_Load( uint16_t Address,       uint8_t * Buffer, uint16_t NBytes );

#define	NVRAM_Load	FM24C64_Load
#define	NVRAM_Save	FM24C64_Save
