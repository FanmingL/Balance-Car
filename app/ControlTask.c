#include "main.h"

uint32_t SystemTimeMs=0; 
float position_out=0,speed_out=0,pwm_out_left=0,pwm_out_right=0;
float exp_angle=0.0f,rotate_speed=0.0f;

static void ModeTask(void)
{
	if (ControlMode==1){
		rotate_speed=0.0f;
		exp_angle=-MAX_EXP_ANGLE;
	}
	else if (ControlMode==2)
	{
			rotate_speed=0.0f;
			exp_angle=MAX_EXP_ANGLE;
	}
	else if (ControlMode==3){
		rotate_speed=MAN_ROTATE_SPEED;
		exp_angle=0.0f;
	}
	else if (ControlMode==4){
		rotate_speed=-MAN_ROTATE_SPEED;
		exp_angle=0.0f;
	}
	else if(ControlMode==5){
		rotate_speed=0.0f;
		exp_angle=0.0f;
	}
	else {
		rotate_speed=0.0f;
		exp_angle=0.0f;
	}

}

void Task_2ms(void)
{
	static uint32_t TIME_2MS=0;
	float inner_loop_time = (Get_Time_Micros()-TIME_2MS)/1000000.0f;
	TIME_2MS =Get_Time_Micros(); 
	MPU6050_Read();
	MPU6050_Data_Prepare( inner_loop_time );
 	IMUupdate(0.5f *inner_loop_time,mpu6050.Gyro_deg.x, mpu6050.Gyro_deg.y, mpu6050.Gyro_deg.z, 
						mpu6050.Acc.x, mpu6050.Acc.y, mpu6050.Acc.z,&Roll,&Pitch,&Yaw);
}

void Task_5ms(void)
{	
	static uint32_t TIME_5MS=0;
	float inner_loop_time = (Get_Time_Micros()-TIME_5MS)/1000000.0f;
	TIME_5MS =Get_Time_Micros();
	position_out= -PID_calculate( inner_loop_time,            //����
														0,				//ǰ��
														exp_angle,				//����ֵ���趨ֵ��
														Pitch,			//����ֵ
														&PitchP_arg, //PID�����ṹ��
														&PitchP_val,	//PID���ݽṹ��
														1000			//integration limit�������޷�
														 );			//���	
	speed_out=  PID_calculate( inner_loop_time,            //����
														0,				//ǰ��
														position_out,				//����ֵ���趨ֵ��
														mpu6050.Gyro.y*TO_ANGLE,			//����ֵ
														&PitchS_arg, //PID�����ṹ��
														&PitchS_val,	//PID���ݽṹ��
														1500			//integration limit�������޷�
														 );			//���	
	ModeTask();
	ChassisControl(inner_loop_time);
}

void Task_10ms(void)
{
	ANO_AK8975_Read();
}


void Task_20ms(void)
{
		static uint32_t TIME_20MS=0;
		float inner_loop_time = (Get_Time_Micros()-TIME_20MS)/1000000.0f;
		TIME_20MS =Get_Time_Micros(); 
		GPIO_ToggleBits(GPIOA,GPIO_Pin_5);
}

void ChassisControl(float T)
{
		Refresh_Encoder();
		pwm_out_left=PID_calculate( T,            //����
														0,				//ǰ��
														speed_out,				//����ֵ���趨ֵ��
														left_encoder.speed,			//����ֵ
														&Chassis_arg, //PID�����ṹ��
														&Chassis_left_val,	//PID���ݽṹ��
														32			//integration limit�������޷�
														 );			//���	
		pwm_out_right=pwm_out_left-rotate_speed;
	/*pwm_out_right=PID_calculate( T,            //����
														0,				//ǰ��
														speed_out,				//����ֵ���趨ֵ��
														right_encoder.speed,			//����ֵ
														&Chassis_arg, //PID�����ṹ��
														&Chassis_right_val,	//PID���ݽṹ��
														200			//integration limit�������޷�
														 );			//���	*/
	pwm_out_left+=rotate_speed;	
	if (pwm_out_left<0){LEFT_DOWN();pwm_out_left=(-pwm_out_left);RIGHT_DOWN();}
	else {LEFT_UP();RIGHT_UP();}
	if (pwm_out_right<0){RIGHT_DOWN();pwm_out_right=(-pwm_out_right);}
	else {RIGHT_UP();}
	if (my_abs(Pitch)<50.0f)
	{SetPWMOut((int)(pwm_out_left),(int)pwm_out_right);}
	else {SetPWMOut(0,0);}
}

void ContolLoop(void)
{
	SystemTimeMs++;
	if (SystemTimeMs%2==0)Task_2ms();
	if (SystemTimeMs%5==0)Task_5ms();
	if (SystemTimeMs%10==0)Task_10ms();
	if (SystemTimeMs%20==0)Task_20ms();
	DataTransferTask(SystemTimeMs);
}


