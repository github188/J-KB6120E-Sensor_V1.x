/**************** (C) COPYRIGHT 2014 �ൺ���˴���ӿƼ����޹�˾ ****************
* �� �� ��: HCBox.C
* �� �� ��: ����
* ��	��	: KB-6120E �������¶ȿ���
* ����޸�: 2015��11��28��
*********************************** �޶���¼ ***********************************/
// Header:		(C) COPYRIGHT 2014 �ൺ���˴���ӿƼ����޹�˾
// File Name: HCBox.C
// Author:		...
// Date:			2015��11��28��

#include "Pin.H"
#include "BSP.H"
// #include "math.h"
struct	uHCBox
{
	uint8_t		SetMode;		//	�趨�Ŀ��Ʒ�ʽ����ֹ�����ȡ����䡢�Զ� ���ַ�ʽ
	FP32		SetTemp;			//	�趨�Ŀ����¶ȣ�
	FP32		RunTemp;			//	ʵ��������¶ȣ�
	uint16_t	OutValue;		//	�����ź����ֵ[0,+2000]��>1000��ʾ���ȣ�<1000��ʾ���䡣
}HCBox;

enum
{	//	����������¶ȵķ�ʽ
	MD_Shut,
	MD_Heat,
	MD_Cool,
	MD_Auto
};


/********************************** ����˵�� ***********************************
*	��������ת��
*******************************************************************************/
#define	fanCountListLen	(4u+(1u+2u))
static	uint16_t	fanCountList[fanCountListLen];
static	uint8_t		fanCountList_index = 0;

uint16_t	FanSpeed_fetch( void )
{
	/*	�̶����1s��¼����ת��Ȧ������������
	 *	���μ����������˲��Ľ��������ת�١�
	 */
	uint8_t 	ii, index = fanCountList_index;
	uint16_t	sum = 0u;
	uint16_t	max = 0u;
	uint16_t	min = 0xFFFFu;
	uint16_t	x0, x1, speed;

	x1 = fanCountList[index];
	for ( ii = fanCountListLen - 1u; ii != 0; --ii )
	{
		//	�����������õ��ٶ�
		x0 = x1;
		if ( ++index >= fanCountListLen ){  index = 0u; }
		x1 = fanCountList[index];
		speed = ( x1 - x0 );
		//	�Զ�����ݽ����˲�
		if ( speed > max ) {  max = speed; }
		if ( speed < min ) {  min = speed; }
		sum += speed;
	}

	speed = (uint16_t)( sum - max - min ) / ( fanCountListLen - (1u+2u));
	
	return	speed  * 60u;
}


uint16_t	HCBoxFan_Circle_Read( void )
{
	return	TIM1->CNT;		//��ͣ�ļ���
}


static	uint16_t	FanSpeed;
uint16_t	volatile  fan_shut_delay;
void	HCBoxFan_Update( void )
{	//	�������¼ת��Ȧ��
	fanCountList[fanCountList_index] = HCBoxFan_Circle_Read();
	if ( ++fanCountList_index >= fanCountListLen )
	{
		fanCountList_index = 0u;
	}
	FanSpeed = FanSpeed_fetch();
	//	���ȿ��ص���̬����
	if ( fan_shut_delay > 0u )
		fan_shut_delay --;
	else
		HCBoxFan_OutCmd( FALSE );
}



void	set_HCBoxTemp( FP32 TempSet, uint8_t ModeSet )
{
	HCBox.SetTemp = TempSet;
	HCBox.SetMode = ModeSet;
}



FP32	get_HCBoxTemp( void )
{
	return	usRegInputBuf[5] * 0.0625;
}



uint16_t	get_HCBoxOutput( void )
{
	return	HCBox.OutValue;
}



uint16_t	get_HCBoxFanSpeed( void )
{
	return	FanSpeed;
}

/********************************** ����˵�� ***********************************
*  ������ƣ�����ѭ����ʱ���ܣ�
*******************************************************************************/

void	HCBox_Output( FP32 OutValue )
{
	static	volatile	uint16_t	HCBoxOutValue = 0;
	//	�������״̬
	HCBox.OutValue = OutValue * 1000 + 1000;
	
	if      ( HCBox.OutValue < 1000 )
	{
		HCBoxOutValue = ( 1000 - HCBox.OutValue  );
		//	�رռ���
		HCBoxHeat_OutCmd( 0 );
		//	��������
		if      ( HCBoxOutValue > 990 )
		{
			HCBoxCool_OutCmd( 1000 );
		}
		else if ( HCBoxOutValue < 10  )
		{
			HCBoxCool_OutCmd( 0 );
		}
		else
		{	
			HCBoxCool_OutCmd( HCBoxOutValue );
		}
	}
	else if ( HCBox.OutValue > 1000 )
	{
		HCBoxOutValue = HCBox.OutValue - 1000;		
		HCBoxCool_OutCmd( 0 );//	�ر�����
		//	��������
		if      ( HCBoxOutValue > 990 )
		{		
			HCBoxHeat_OutCmd( 1000 );		
		}
		else if ( HCBoxOutValue < 10 )
		{                                                
			HCBoxHeat_OutCmd( 0 );
		}
		else
		{
			HCBoxHeat_OutCmd( HCBoxOutValue );
		}
	}
	else
	{
		//	�رռ���
		HCBoxHeat_OutCmd( 0 );
		//	�ر�����
		HCBoxCool_OutCmd( 0 );
	}

}


/********************************* *����˵��* **********************************
*	�����������乲��һ���¶��źţ����߲���ͬʱʹ�á�
*******************************************************************************/
extern	uint16_t	iRetry;
void	HCBoxValue_Update( void )
{
	if ( iRetry >= 30 )
		HCBox_Output( 0.0f );	//	ע����ȴ�״̬���������һ��
	set_HCBoxTemp( usRegHoldingBuf[5] * 0.0625f, usRegHoldingBuf[6] );
	HCBox.RunTemp = get_HCBoxTemp();
}

/********************************** ����˵�� ***********************************
*  �ȴ�״̬����ֹ���ȡ�����
*******************************************************************************/
volatile	BOOL	EN_Cool = TRUE;
volatile	BOOL	EN_Heat = TRUE;
static		BOOL 	ControlFlag = TRUE;
static	void	HCBox_Wait( void )
{
	//	�����Զ�ģʽ���޷�ȷ��ʵ�ʹ���ģʽ����ʱ����ȴ�״̬
	HCBoxValue_Update();
	HCBox_Output( 0.0f );	//	�ȴ�״̬���
}

#include "math.h"
// struct	uPID_Parament Heat;
// struct	uPID_Parament Cool;
/********************************** ����˵�� ***********************************
*  ����״̬�����ȷ�ʽ����
*******************************************************************************/

static	void	HCBox_Heat( void )  
{
	FP32	Kp = 10.0f / 128.0f;
	FP32	Ki = ( Kp / 240.0f );
	FP32	Kd = ( Kp * 75.0f );

	FP32	TempRun, TempSet;
	static FP32	Ek_1, Ek = 0.0f;
	static FP32	Up = 0.0f, Ui = 0.0f, Ud = 0.0f;
	static FP32	Upid = 0.0f;

	if( EN_Heat )
	{
		HCBoxValue_Update();
		//	����PID����������ֵ��һ����[0.0 ��+1.0]��Χ
		TempRun = HCBox.RunTemp;
		TempSet = HCBox.SetTemp;// - ((FP32)( usRegInputBuf[2] * 0.0625 - HCBox.SetTemp )) * 0.01f;//	�������¶���΢С��ϵ 
		Ek_1 = Ek;
		Ek = ( TempSet - TempRun );
		Up = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.3f ){  Ui = -0.3f; }
		if ( Ui > +0.3f ){  Ui = +0.3f; }//0.25
		if( ( HCBox.RunTemp - HCBox.SetTemp ) >= 0 ) 
			Ud = ( Ud * 0.9f ) + (( Kd * ( - fabs(Ek - Ek_1) )) * 0.1f );
		else
			Ud = ( Ud * 0.8f ) + (( Kd * (Ek - Ek_1)) * 0.2f );
		Upid = ( Up + Ui + Ud );
		if ( Upid <  0.0f ){  Upid = 0.0f; }
		if ( Upid > +1.0f ){  Upid = 1.0f; }
		
		
		HCBox_Output( Upid );	//	����״̬���������ѭ����ʱ���ܣ�
	}
}
/********************************** ����˵�� ***********************************
*  ����״̬�����䷽ʽ����
*******************************************************************************/

static	void	HCBox_Cool( void )
{
	FP32	Kp = 25.0f / 128.0f ; 
	FP32	Ki = ( Kp / 180.0f );
	FP32	Kd = ( Kp * 60.0f ) ;
	static FP32	Ek_1, Ek = 0.0f;
	static FP32	Up = 0.0f, Ui = 0.0f, Ud = 0.0f;
	static FP32	Upid = 0.0f;
	FP32	 TempRun, TempSet;
  
  
	//	����PID����������ֵ��һ����[-1.0�� 0.0]��Χ
	
	if( EN_Cool )
	{
		HCBoxValue_Update();		//	ʵʱ��ȡ�¶�;  if ( ʧ�� ) ת�����״̬
		//	����PID����������ֵ��һ����[-1.0�� 0.0]��Χ
		TempRun = HCBox.RunTemp;
		TempSet = HCBox.SetTemp;// - ((FP32)( usRegInputBuf[2] * 0.0625 - HCBox.SetTemp )) * 0.01f;//	�������¶���΢С��ϵ ;
		Ek_1 = Ek;
		Ek  = ( TempSet - TempRun );
		Up  = Kp * Ek;
		Ui += Ki * Ek;
		if ( Ui < -0.30f ){  Ui = -0.30f; }
		if ( Ui > +0.30f ){  Ui = +0.30f; }	//	0.5
		if( ( HCBox.RunTemp - HCBox.SetTemp ) <= 0 ) 
			Ud =( Ud * 0.9f ) + (( Kd * fabs( Ek - Ek_1 )) * 0.1f ); 
		else
			Ud =( Ud * 0.8f ) + (( Kd * ( Ek - Ek_1 )) * 0.2f ); 
		Upid = Up + Ui + Ud;
		if ( Upid >  0.0f ){  Upid =  0.0f; }
		if ( Upid < -1.0f ){  Upid = -1.0f; }
		//	����������ƣ����䷽ʽ�¿������ȣ��ݲ����٣�2014��1��15�գ�
		if ( Upid < 0.0f )
		{
			fan_shut_delay = 60u;
			HCBoxFan_OutCmd( TRUE );
		}
		//	���
		if ( FanSpeed < 100u )	//	���Ȳ�ת����ֹ����Ƭ����
		{													
				HCBox_Output( 0.0f );	//	ע����ȴ�״̬���������һ��
		}
		else
		{
			HCBox_Output( Upid );	//	����״̬���������ѭ����ʱ���ܣ�
		}
	}

}



/********************************** ����˵�� ***********************************
*	�������¶ȿ���
*******************************************************************************/
static	uint32_t HCBoxCount = 0;

void	HCBoxControl( void )
{   
/**/	FP32	EK;

	if( HCBoxFlag )
	{
	
		HCBoxFlag = FALSE;
		HCBoxFan_Update();			//	��������ת��
		HCBoxValue_Update();		//	ʵʱ������ȡ;
		EK = HCBox.RunTemp - HCBox.SetTemp;
	
		switch ( HCBox.SetMode )
		{
		case MD_Auto:
			if( !ControlFlag )
			{	
				EN_Cool = FALSE;
				EN_Heat = FALSE;
				if( fabs( EK ) >= 1 )
					HCBoxCount ++;
				else
					HCBoxCount = 0;				
				if(( HCBoxCount >= 60 * 10 ) || ( fabs( EK ) >= 5 ))	//	�²����1����С��5�泬��10���ӻ���10�������¶ȱ仯̫�죨�²����5�棩
				{
					ControlFlag = TRUE;
				}
			}
			else
			{  
				HCBoxCount = 0;

				if( EK >=  1 )
				{
					if( EN_Cool == FALSE )
						ControlFlag = FALSE;

					EN_Cool = TRUE;
					EN_Heat = FALSE;
				}

				if( EK <= -1 )
				{
					if( EN_Heat == FALSE )
						ControlFlag = FALSE;

					EN_Cool = FALSE;
					EN_Heat = TRUE;
				}
			}			
			break;
		case MD_Cool:	
			if(  ( HCBox.RunTemp - HCBox.SetTemp ) <= -1 ) 
			{
				ControlFlag = FALSE;
			}
			else
			{
				ControlFlag = TRUE;
				EN_Cool = TRUE; 
				EN_Heat = FALSE;	
			}
			break;
		case MD_Heat:	
			if( ( HCBox.RunTemp - HCBox.SetTemp ) >= 1 ) 
			{
				ControlFlag = FALSE;
			}
			else
			{
				ControlFlag = TRUE;
				EN_Heat = TRUE; 
				EN_Cool = FALSE;	
			}
			break;
		default:
		case MD_Shut:	
			EN_Heat = 
			EN_Cool = FALSE;				
			HCBox_Wait();	
			break;							
		}
		if( ControlFlag )
		{
			if( EN_Heat )
				HCBox_Heat();
			if( EN_Cool )
				HCBox_Cool();
		}
		else
		{	
			HCBox_Wait();
		}
	}

}


