/**************** (C) COPYRIGHT 2013 青岛金仕达电子科技有限公司 ****************
* 文 件 名: FM24.C
* 创 建 者: Dean
* 描  述  : 读写24系列的 FRAM 器件
*         : 
* 最后修改: 2015年7月13日
*********************************** 修订记录 ***********************************
* 版  本: 
* 修订人: 
*******************************************************************************/

#include "BSP.H"

#define	_SLAVE_FM24C64	0xA0

/*******************************************************************************
* 函数名称: FM24C64 Save/Load
* 功能说明: FM24C64 存取
* 输入参数: 地址/指针/数据长度
* 输出参数: None
* 返 回 值: 返回成功标志
*******************************************************************************/
bool	FM24C64_Save( uint16_t address, const uint8_t * buffer, uint16_t NBytes )
{
	// send sub address
	if ( ! bus_i2c_start( _SLAVE_FM24C64, I2C_Write ))	{ bus_i2c_stop(); return FALSE; }
    if ( ! bus_i2c_shout((uint8_t)( address / 0x100U ))){ bus_i2c_stop(); return FALSE; }
    if ( ! bus_i2c_shout((uint8_t)( address % 0x100U ))){ bus_i2c_stop(); return FALSE; }

	// continue send write data.
    do
	{
		if ( ! bus_i2c_shout( *buffer++ )){	bus_i2c_stop(); return FALSE; 	}
	}
	while ( --NBytes );

    bus_i2c_stop();

	/*	铁电存储器不需要 polling */
	return	TRUE;
}

bool	FM24C64_Load( uint16_t address, uint8_t * buffer, uint16_t NBytes )
{
	// send sub address
	if ( ! bus_i2c_start( _SLAVE_FM24C64, I2C_Write ))	{ bus_i2c_stop(); return FALSE; }
	if ( ! bus_i2c_shout((uint8_t)( address / 0x100U ))){ bus_i2c_stop(); return FALSE; }
	if ( ! bus_i2c_shout((uint8_t)( address % 0x100U ))){ bus_i2c_stop(); return FALSE; }

	// Send read command and receive data.
	if ( ! bus_i2c_start( _SLAVE_FM24C64, I2C_Read ))	{ bus_i2c_stop(); return FALSE; }
	while ( --NBytes )
	{
		*buffer++ = bus_i2c_shin( I2C_ACK );	// Receive and send ACK
	}
	*buffer = bus_i2c_shin( I2C_NoACK );		// Receive and send NoACK
	bus_i2c_stop();

	return TRUE;
}

/********  (C) COPYRIGHT 2015 青岛金仕达电子科技有限公司  **** End Of File ****/
