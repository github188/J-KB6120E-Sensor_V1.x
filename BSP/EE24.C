/**************** (C) COPYRIGHT 2012 青岛金仕达电子科技有限公司 ****************
* 文 件 名: EE24.C
* 创 建 者: Dean
* 描  述  : 读写24系列的EEPROM器件
*         : 
* 最后修改: 2012年4月14日
*********************************** 修订记录 ***********************************
* 版  本: 
* 修订人: 
*******************************************************************************/

#include "BSP.H"
//	#include "Pin.H"


/*******************************************************************************
* Function Name  : I2C_EE_WaitEepromStandbyState
* Description    : Wait for EEPROM Standby state
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static	BOOL	polling( uint8_t SlaveAddress  )
{
	uint16_t	iRetry;
	
	/*	在总线速率400K时, 10ms时间最多发送400个地址字节 */
	for ( iRetry = 400U; iRetry != 0x00U; --iRetry )
	{
		if ( bus_i2c_start( SlaveAddress, I2C_Write ))
		{
			bus_i2c_stop();
			return	TRUE;
		}
	}
	return	FALSE;
}

/*******************************************************************************
* 函数名称: FM24C04A_save/load
* 功能说明: FM24C04A 存取
* 输入参数: 地址/指针/数据长度
* 输出参数: None
* 返 回 值: None
*******************************************************************************/
BOOL	_EE_Save_inside_page( uint16_t address, uint8_t const * buffer, uint8_t count )
{
	// send sub address
	if ( ! bus_i2c_start( _SLAVE_AT24C02, I2C_Write ))	{ bus_i2c_stop(); return FALSE; }
//    if ( ! bus_i2c_shout((uint8_t)( address / 0x100U ))){ bus_i2c_stop(); return FALSE; }
    if ( ! bus_i2c_shout((uint8_t)( address % 0x100U ))){ bus_i2c_stop(); return FALSE; }

	// continue send write data.
    do
	{
		if ( ! bus_i2c_shout((uint8_t)~(*buffer++))){	bus_i2c_stop(); return FALSE; 	}
	}
	while ( --count );

    bus_i2c_stop();

    // acknowledge polling.
	return	polling( _SLAVE_AT24C02 );	/*	铁电存储器不需要 polling, 但加上也无所谓 */
}

BOOL	_EE_Load_inside_page( uint16_t address, uint8_t * buffer, uint8_t count )
{
	// send sub address
	if ( ! bus_i2c_start( _SLAVE_AT24C02, I2C_Write )) { bus_i2c_stop(); return FALSE; }
//	if ( ! bus_i2c_shout((uint8_t)( address / 0x100U ))){ bus_i2c_stop(); return FALSE; }
	if ( ! bus_i2c_shout((uint8_t)( address % 0x100U ))){ bus_i2c_stop(); return FALSE; }

	// Send read command and receive data.
	if ( ! bus_i2c_start( _SLAVE_AT24C02, I2C_Read ))	{ bus_i2c_stop(); return FALSE; }
	while ( --count )
	{
		*buffer++ =  (uint8_t)~bus_i2c_shin( I2C_ACK );	// Receive and send ACK
	}
	*buffer =  (uint8_t)~bus_i2c_shin( I2C_NoACK );		// Receive and send NoACK
	bus_i2c_stop();

	return TRUE;
}

BOOL	_EE_Load( uint16_t address, uint8_t * buffer, uint8_t count )
{
	uint8_t	len = _EE_Page_Len - ( address % _EE_Page_Len );
	
	while ( len < count )
	{
		if ( !_EE_Load_inside_page( address, buffer, len ))
		{
			return	FALSE;
		}
		address += len;
		buffer  += len;
		count   -= len;
		
		len = _EE_Page_Len;
	}
	return	_EE_Load_inside_page( address, buffer, count );
}

BOOL	_EE_Save( uint16_t address, uint8_t const * buffer, uint8_t count )
{
	uint8_t	len = _EE_Page_Len - ( address % _EE_Page_Len );	//	len 恒不等于 0
	
	while ( len < count )
	{
		if ( !_EE_Save_inside_page( address, buffer, len ))
		{
			return	FALSE;
		}
		address += len;
		buffer  += len;
		count   -= len;		// ∵ (len < count ), ∴ ( count - len ) > 0
		
		len = _EE_Page_Len;
	}
	return	_EE_Save_inside_page( address, buffer, count );
}

////////////////////////////////////////////////////////////////////////////////
BOOL	E24C02_Load( uint8_t address, void * buffer, uint8_t count )
{
	BOOL	state;

	bus_i2c_mutex_apply();
	state = _EE_Load( address, buffer, count );
	bus_i2c_mutex_release();
	
	return	state;
}

BOOL	E24C02_Save( uint8_t address, void const * buffer, uint8_t count )
{
	BOOL	state;
	
	bus_i2c_mutex_apply();
	state = _EE_Save( address, buffer, count );
	bus_i2c_mutex_release();

	return	state;
}

/********  (C) COPYRIGHT 2012 青岛金仕达电子科技有限公司  **** End Of File ****/
