/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       can_receive.c/h
  * @brief      there is CAN interrupt function  to receive motor data,
  *             and CAN send function to send motor current to control motor.
  *             ������CAN�жϽ��պ��������յ������,CAN���ͺ������͵���������Ƶ��.
  * @note       
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     Dec-26-2018     RM              1. done
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 DJI****************************
  */

#include "CAN_receive.h"
#include "main.h"
#include "pid.h"
#include "usart.h"
extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;
//motor data read
#define get_motor_measure(ptr, data)                                    \
    {                                                                   \
        (ptr)->last_ecd = (ptr)->ecd;                                   \
        (ptr)->ecd = (uint16_t)((data)[0] << 8 | (data)[1]);            \
        (ptr)->speed_rpm = (uint16_t)((data)[2] << 8 | (data)[3]);      \
        (ptr)->given_current = (uint16_t)((data)[4] << 8 | (data)[5]);  \
        (ptr)->temperate = (data)[6];                                   \
    }

#define ABS(x)		((x>0)? (x): (-x)) 
/*
motor data,  0:chassis motor1 3508;1:chassis motor3 3508;2:chassis motor3 3508;3:chassis motor4 3508;
4:yaw gimbal motor 6020;5:pitch gimbal motor 6020;6:trigger motor 2006;
�������, 0:���̵��1 3508���,  1:���̵��2 3508���,2:���̵��3 3508���,3:���̵��4 3508���;
4:yaw��̨��� 6020���; 5:pitch��̨��� 6020���; 6:������� 2006���*/
static motor_measure_t motor_chassis[7];

static CAN_TxHeaderTypeDef  gimbal_tx_message;
static uint8_t              gimbal_can_send_data[8];
static CAN_TxHeaderTypeDef  chassis_tx_message;
static uint8_t              chassis_can_send_data[8];
int set_spd[6]={500},angle=1000;
int test_mode=control_position;
uint8_t tail[4]={0x00,0x00,0x80,0x7f};
/**
  * @brief          hal CAN fifo call back, receive motor data
  * @param[in]      hcan, the point to CAN handle
  * @retval         none
  */
/**
  * @brief          hal��CAN�ص�����,���յ������
  * @param[in]      hcan:CAN���ָ��
  * @retval         none 
  */

void only_pid_struct()
{
			for(int i=0; i<4; i++)
    {
		PID_struct_init(&pid_spd[i], POSITION_PID, 1000, 1000,
									6.5f,	0.01f,0.01f);  //4 motos angular rate closeloop.
		}
		PID_struct_init(&pid_pos, POSITION_PID, 1000, 1000,
									10.0f,0.01f,0.02f);
		PID_struct_init(&pid_pit, POSITION_PID, 1000, 1000,
									6.5f,0.01f,0.01f);
		PID_struct_init(&pid_yaw, POSITION_PID, 1000, 1000,
									6.5f,0.01f,0.01f); 
}

int postion_control(float angle,pid_t* pid,pos*Pos)
{
	int current;
	float N=360.0f/8192.0f;
	if(ABS((motor_chassis[0].ecd-angle))<=4096)
	{
		current=angle-motor_chassis[0].ecd;
		Pos->now_3508=motor_chassis[0].ecd*N;
		Pos->set_3508=angle*N;
	}
	else
	{
		if(angle<4096)
		{
			current=1;
			Pos->now_3508=motor_chassis[0].ecd*N;
		  Pos->set_3508=8192*N;
		}
		else
		{
			current=-1;
		  Pos->now_3508=motor_chassis[0].ecd*N;
		  Pos->set_3508=0;
		}
	}
	pid_calc(pid, Pos->now_3508,Pos->set_3508);
	return current;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data);

    switch (rx_header.StdId)
    {
        case CAN_3508_M1_ID:
        case CAN_3508_M2_ID:
        case CAN_3508_M3_ID:
        case CAN_3508_M4_ID:
        case CAN_YAW_MOTOR_ID:
        case CAN_PIT_MOTOR_ID:
        case CAN_TRIGGER_MOTOR_ID:
        {
            static uint8_t i = 0;
            //get motor id
            i = rx_header.StdId - CAN_3508_M1_ID;
            get_motor_measure(&motor_chassis[i], rx_data);
//					if(test_mode==control_position)
//					{
//						int get_current=postion_control(angle,&pid_pos,&POS);
//					  pid_calc(&pid_spd[0], motor_chassis[0].speed_rpm, pid_pos.pos_out);
//						  CAN_cmd_chassis(pid_spd[0].pos_out, 
//									pid_spd[1].pos_out,
//									pid_spd[2].pos_out,
//									pid_spd[3].pos_out);
//					  else
//					  {
//						  CAN_cmd_chassis(-pid_spd[0].pos_out, 
//									-pid_spd[1].pos_out,
//									-pid_spd[2].pos_out,
//									-pid_spd[3].pos_out);
//					  }
//				  }
//					else
//					{
//						for(int i=0; i<4; i++)
//						{
//							pid_calc(&pid_spd[i], motor_chassis[i].speed_rpm, set_spd[i]);
//						}
//						CAN_cmd_chassis(pid_spd[0].pos_out, 
//									pid_spd[1].pos_out,
//									pid_spd[2].pos_out,
//									pid_spd[3].pos_out);
//					}
//						float data[4];
//						data[0]=motor_chassis[0].speed_rpm;
//						data[1]=set_spd[0];
//						data[2]=POS.now;
//					  data[3]=POS.set;
//						HAL_UART_Transmit(&huart6,(uint8_t*)data,4*sizeof(float),100);
//		        HAL_UART_Transmit(&huart6,tail,4*sizeof(uint8_t),100);
            break;
        }

        default:
        {
            break;
        }
    }
}



/**
  * @brief          send control current of motor (0x205, 0x206, 0x207, 0x208)
  * @param[in]      yaw: (0x205) 6020 motor control current, range [-30000,30000] 
  * @param[in]      pitch: (0x206) 6020 motor control current, range [-30000,30000]
  * @param[in]      shoot: (0x207) 2006 motor control current, range [-10000,10000]
  * @param[in]      rev: (0x208) reserve motor control current
  * @retval         none
  */
/**
  * @brief          ���͵�����Ƶ���(0x205,0x206,0x207,0x208)
  * @param[in]      yaw: (0x205) 6020������Ƶ���, ��Χ [-30000,30000]
  * @param[in]      pitch: (0x206) 6020������Ƶ���, ��Χ [-30000,30000]
  * @param[in]      shoot: (0x207) 2006������Ƶ���, ��Χ [-10000,10000]
  * @param[in]      rev: (0x208) ������������Ƶ���
  * @retval         none
  */
void CAN_cmd_gimbal(int16_t yaw, int16_t pitch, int16_t shoot, int16_t rev)
{
    uint32_t send_mail_box;
    gimbal_tx_message.StdId = CAN_GIMBAL_ALL_ID;
    gimbal_tx_message.IDE = CAN_ID_STD;
    gimbal_tx_message.RTR = CAN_RTR_DATA;
    gimbal_tx_message.DLC = 0x08;
    gimbal_can_send_data[0] = (yaw >> 8);
    gimbal_can_send_data[1] = yaw;
    gimbal_can_send_data[2] = (pitch >> 8);
    gimbal_can_send_data[3] = pitch;
    gimbal_can_send_data[4] = (shoot >> 8);
    gimbal_can_send_data[5] = shoot;
    gimbal_can_send_data[6] = (rev >> 8);
    gimbal_can_send_data[7] = rev;
    HAL_CAN_AddTxMessage(&GIMBAL_CAN, &gimbal_tx_message, gimbal_can_send_data, &send_mail_box);
}

/**
  * @brief          send CAN packet of ID 0x700, it will set chassis motor 3508 to quick ID setting
  * @param[in]      none
  * @retval         none
  */
/**
  * @brief          ����IDΪ0x700��CAN��,��������3508��������������ID
  * @param[in]      none
  * @retval         none
  */
void CAN_cmd_chassis_reset_ID(void)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = 0x700;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    chassis_can_send_data[0] = 0;
    chassis_can_send_data[1] = 0;
    chassis_can_send_data[2] = 0;
    chassis_can_send_data[3] = 0;
    chassis_can_send_data[4] = 0;
    chassis_can_send_data[5] = 0;
    chassis_can_send_data[6] = 0;
    chassis_can_send_data[7] = 0;

    HAL_CAN_AddTxMessage(&CHASSIS_CAN, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}


/**
  * @brief          send control current of motor (0x201, 0x202, 0x203, 0x204)
  * @param[in]      motor1: (0x201) 3508 motor control current, range [-16384,16384] 
  * @param[in]      motor2: (0x202) 3508 motor control current, range [-16384,16384] 
  * @param[in]      motor3: (0x203) 3508 motor control current, range [-16384,16384] 
  * @param[in]      motor4: (0x204) 3508 motor control current, range [-16384,16384] 
  * @retval         none
  */
/**
  * @brief          ���͵�����Ƶ���(0x201,0x202,0x203,0x204)
  * @param[in]      motor1: (0x201) 3508������Ƶ���, ��Χ [-16384,16384]
  * @param[in]      motor2: (0x202) 3508������Ƶ���, ��Χ [-16384,16384]
  * @param[in]      motor3: (0x203) 3508������Ƶ���, ��Χ [-16384,16384]
  * @param[in]      motor4: (0x204) 3508������Ƶ���, ��Χ [-16384,16384]
  * @retval         none
  */
void CAN_cmd_chassis(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4)
{
    uint32_t send_mail_box;
    chassis_tx_message.StdId = CAN_CHASSIS_ALL_ID;
    chassis_tx_message.IDE = CAN_ID_STD;
    chassis_tx_message.RTR = CAN_RTR_DATA;
    chassis_tx_message.DLC = 0x08;
    chassis_can_send_data[0] = motor1 >> 8;
    chassis_can_send_data[1] = motor1;
    chassis_can_send_data[2] = motor2 >> 8;
    chassis_can_send_data[3] = motor2;
    chassis_can_send_data[4] = motor3 >> 8;
    chassis_can_send_data[5] = motor3;
    chassis_can_send_data[6] = motor4 >> 8;
    chassis_can_send_data[7] = motor4;

    HAL_CAN_AddTxMessage(&CHASSIS_CAN, &chassis_tx_message, chassis_can_send_data, &send_mail_box);
}

/**
  * @brief          return the yaw 6020 motor data point
  * @param[in]      none
  * @retval         motor data point
  */
/**
  * @brief          ����yaw 6020�������ָ��
  * @param[in]      none
  * @retval         �������ָ��
  */
const motor_measure_t *get_yaw_gimbal_motor_measure_point(void)
{
    return &motor_chassis[4];
}

/**
  * @brief          return the pitch 6020 motor data point
  * @param[in]      none
  * @retval         motor data point
  */
/**
  * @brief          ����pitch 6020�������ָ��
  * @param[in]      none
  * @retval         �������ָ��
  */
const motor_measure_t *get_pitch_gimbal_motor_measure_point(void)
{
    return &motor_chassis[5];
}


/**
  * @brief          return the trigger 2006 motor data point
  * @param[in]      none
  * @retval         motor data point
  */
/**
  * @brief          ���ز������ 2006�������ָ��
  * @param[in]      none
  * @retval         �������ָ��
  */
const motor_measure_t *get_trigger_motor_measure_point(void)
{
    return &motor_chassis[6];
}


/**
  * @brief          return the chassis 3508 motor data point
  * @param[in]      i: motor number,range [0,3]
  * @retval         motor data point
  */
/**
  * @brief          ���ص��̵�� 3508�������ָ��
  * @param[in]      i: ������,��Χ[0,3]
  * @retval         �������ָ��
  */
const motor_measure_t *get_chassis_motor_measure_point(uint8_t i)
{
    return &motor_chassis[(i & 0x03)];
}

void control_motor_3508()
{
		if(test_mode==control_position)
	{
		postion_control(angle,&pid_pos,&POS);
		pid_calc(&pid_spd[0], motor_chassis[0].speed_rpm, pid_pos.pos_out);
			CAN_cmd_chassis(pid_spd[0].pos_out, 
					pid_spd[1].pos_out,
					pid_spd[2].pos_out,
					pid_spd[3].pos_out);

	}
	else
	{
		for(int i=0; i<4; i++)
		{
			pid_calc(&pid_spd[i], motor_chassis[i].speed_rpm, set_spd[i]);
		}
		CAN_cmd_chassis(pid_spd[0].pos_out, 
					pid_spd[1].pos_out,
					pid_spd[2].pos_out,
					pid_spd[3].pos_out);
	}
		float data[4];
		data[0]=motor_chassis[0].speed_rpm;
		data[1]=set_spd[0];
		data[2]=POS.now_3508;
		data[3]=POS.set_3508;
		HAL_UART_Transmit(&huart6,(uint8_t*)data,4*sizeof(float),100);
		HAL_UART_Transmit(&huart6,tail,4*sizeof(uint8_t),100);
}

void control_motor_6020()
{
			if(test_mode==control_position)
	{
		postion_control(angle,&pid_pos,&POS);
		pid_calc(&pid_pit, motor_chassis[4].speed_rpm, pid_pos.pos_out);
		CAN_cmd_gimbal(pid_pit.pos_out, 
					pid_yaw.pos_out,0,0);
	}
	else
	{
		pid_calc(&pid_pit, motor_chassis[4].speed_rpm, set_spd[4]);
		CAN_cmd_gimbal(pid_pit.pos_out, 
					pid_yaw.pos_out,0,0);
	}
		float data[4];
		data[0]=motor_chassis[4].speed_rpm;
		data[1]=set_spd[4];
		data[2]=POS.now_6020;
		data[3]=POS.set_6020;
		HAL_UART_Transmit(&huart6,(uint8_t*)data,4*sizeof(float),100);
		HAL_UART_Transmit(&huart6,tail,4*sizeof(uint8_t),100);
}

