/**************** (C) COPYRIGHT 2014 青岛金仕达电子科技有限公司 ****************
* 文 件 名: DS18B20.C
* 创 建 者: 董峰
* 描  述  : 读写 DS18B20。
* 最后修改: 2014年5月5日
*********************************** 修订记录 ***********************************
* 版  本: V2.0
* 修订人: 董峰
* 说  明: 使用OW驱动模块访问。
*******************************************************************************/
#include "BSP.H"

struct OW_DRV
{
	BOOL ( *Init )( void );
	BOOL ( *Reset )( void );
	BOOL ( *Slot )( BOOL IO_Slot );
};

struct OW_DRV const DS18B20_Temp1 = 
{
	OW_1_Init,
	OW_1_Reset,
	OW_1_Slot,
};

struct OW_DRV const DS18B20_Temp2 =
{
	OW_2_Init,
	OW_2_Reset,
	OW_2_Slot,
};

struct OW_DRV const DS18B20_Temp3 =
{
	OW_3_Init,
	OW_3_Reset,
	OW_3_Slot,
};

struct OW_DRV const DS18B20_Temp4 =
{
	OW_4_Init,
	OW_4_Reset,
	OW_4_Slot,
};


static	BOOL	OW_Init( struct OW_DRV const * OW )
{
	return	OW->Init();
}

static	BOOL	OW_Reset( struct OW_DRV const * OW )
{
	return	OW->Reset();
}

static	BOOL	OW_Slot( struct OW_DRV const * OW, BOOL IO_Slot )
{
	return	OW->Slot( IO_Slot );
}

static	BOOL	OW_isReady( struct OW_DRV const * OW )
{
	return	OW_Slot( OW, TRUE);
}

static	uint8_t  OW_Slot8( struct OW_DRV const * OW, uint8_t IOByte )
{
	BOOL	inBit, outBit;
	uint_fast8_t	i;
	
	for ( i = 8u; i != 0u; --i )
	{
		outBit = ( IOByte & 0x01u ) ? ( 1 ) : ( 0 );

		inBit = OW_Slot( OW, outBit );
	
		IOByte = ( inBit ) ? (( IOByte >> 1 ) | 0x80u ) : (( IOByte >> 1 ) & 0x7Fu );
	}

	return	IOByte;
}


static	BOOL	DS18B20_Load( struct OW_DRV const * OW, uint8_t DS18B20_Buf[9] )
{
	static	uint8_t	const DallasCRC8[256] = 
	{
		0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
		157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
		35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
		190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
		70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
		219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
		101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
		248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
		140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
		17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
		175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
		50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
		202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
		87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
		233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
		116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53
	};

	uint8_t i;
	uint8_t CRC8;
	
	if ( ! OW )	{  return	FALSE;	}

	if ( ! OW_Init( OW ))    {	return	FALSE;	}
	if ( ! OW_isReady( OW )) {	return	FALSE;	}
	if ( ! OW_Reset( OW ))   {	return	FALSE;	}

	( void )OW_Slot8( OW, 0xCCu );  // Skip ROM Command
	( void )OW_Slot8( OW, 0xBEu );  // Read Scrachpad Command

	CRC8 = 0u;
	for ( i = 0u; i < 9u; ++i )
	{
		DS18B20_Buf[i] = OW_Slot8( OW, 0xFFu );
		CRC8 = DallasCRC8[ CRC8 ^ DS18B20_Buf[i]];
	}
	
	if ( 0u != CRC8 )
	{
		return FALSE;
	}

	//	启动转换以备一下次读取
	( void )OW_Reset( OW );
	( void )OW_Slot8( OW, 0xCCu );	// Skip ROM Command
	( void )OW_Slot8( OW, 0x44u );	// Temperature Convert Command

	return TRUE;
}

static	BOOL	DS18B20_Read( struct OW_DRV const * OW, int16_t * pT16S )
{
	uint8_t	DS18B20_Buf[9];
	
	if ( ! DS18B20_Load( OW, DS18B20_Buf ))
	{
		return	FALSE;
	}
	
	*pT16S = (int16_t)( DS18B20_Buf [1] * 256 + DS18B20_Buf[0] );
	return	TRUE;
}


BOOL	DS18B20_1_Read( int16_t * pT16S )
{
	return	DS18B20_Read( &DS18B20_Temp1, pT16S );
}

BOOL	DS18B20_2_Read( int16_t * pT16S )
{
	return	DS18B20_Read( &DS18B20_Temp2, pT16S );
}

BOOL	DS18B20_3_Read( int16_t * pT16S )
{
	return	DS18B20_Read( &DS18B20_Temp3, pT16S );
}
BOOL	DS18B20_4_Read( int16_t * pT16S )
{
	return	DS18B20_Read( &DS18B20_Temp4, pT16S );
}
/********  (C) COPYRIGHT 2014 青岛金仕达电子科技有限公司  **** End Of File ****/
