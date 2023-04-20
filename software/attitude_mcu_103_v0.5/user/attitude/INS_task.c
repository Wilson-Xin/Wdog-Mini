/**
  ****************************(C) COPYRIGHT 2019 DJI****************************
  * @file       INS_task.c/h
  * @brief      use bmi088 to calculate the euler angle. no use ist8310, so only
  *             enable data ready pin to save cpu time.enalbe bmi088 data ready
  *             enable spi DMA to save the time spi transmit
  *             ��Ҫ����������bmi088��������ist8310�������̬���㣬�ó�ŷ���ǣ�
  *             �ṩͨ��bmi088��data ready �ж�����ⲿ�������������ݵȴ��ӳ�
  *             ͨ��DMA��SPI�����ԼCPUʱ��.
  * @note       
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     Dec-26-2018     RM              1. done
  *  V2.0.0     Nov-11-2019     RM              1. support bmi088, but don't support mpu6500
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2019 DJI****************************
  */

#include "INS_task.h"

#include "main.h"

#include "tim.h"
//#include "spi.h"
#include "bsp_spi.h"
#include "can.h"
#include "dma.h"
#include "bmi088driver.h"
#include "ist8310driver.h"
#include "pid.h"

#include "MahonyAHRS.h"
#include "math.h"

#include "dwt.h"

extern SPI_HandleTypeDef hspi1;

#define IMU_temp_PWM(pwm)   __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pwm)//pwm����


/**
  * @brief          control the temperature of bmi088
  * @param[in]      temp: the temperature of bmi088
  * @retval         none
  */
/**
  * @brief          ����bmi088���¶�
  * @param[in]      temp:bmi088���¶�
  * @retval         none
  */
static void imu_temp_control(fp32 temp);

/**
  * @brief          open the SPI DMA accord to the value of imu_update_flag
  * @param[in]      none
  * @retval         none
  */
/**
  * @brief          ����imu_update_flag��ֵ����SPI DMA
  * @param[in]      temp:bmi088���¶�
  * @retval         none
  */
static void imu_cmd_spi_dma(void);


void AHRS_init(fp32 quat[4], fp32 accel[3], fp32 mag[3]);
void AHRS_update(fp32 quat[4], fp32 time, fp32 gyro[3], fp32 accel[3], fp32 mag[3]);
void get_angle(fp32 quat[4], fp32 *yaw, fp32 *pitch, fp32 *roll);
void send_angle_can(fp32 *INS,fp32 temp);

uint8_t gyro_dma_rx_buf[SPI_DMA_GYRO_LENGHT];
uint8_t gyro_dma_tx_buf[SPI_DMA_GYRO_LENGHT] = {0x82,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

uint8_t accel_dma_rx_buf[SPI_DMA_ACCEL_LENGHT];
uint8_t accel_dma_tx_buf[SPI_DMA_ACCEL_LENGHT] = {0x92,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

uint8_t accel_temp_dma_rx_buf[SPI_DMA_ACCEL_TEMP_LENGHT];
uint8_t accel_temp_dma_tx_buf[SPI_DMA_ACCEL_TEMP_LENGHT] = {0xA2,0xFF,0xFF,0xFF};


volatile uint8_t task_start_flag = 0;

volatile uint8_t gyro_update_flag = 0;
volatile uint8_t accel_update_flag = 0;
volatile uint8_t accel_temp_update_flag = 0;
volatile uint8_t mag_update_flag = 0;
volatile uint8_t imu_start_dma_flag = 0;
static uint8_t can_tx_flag=0;

bmi088_real_data_t bmi088_real_data;
ist8310_real_data_t ist8310_real_data;

static const fp32 imu_temp_PID[3] = {TEMPERATURE_PID_KP, TEMPERATURE_PID_KI, TEMPERATURE_PID_KD};
static pid_type_def imu_temp_pid;

fp32 INS_quat[4] = {0.0f, 0.0f, 0.0f, 0.0f};
fp32 INS_angle[3] = {0.0f, 0.0f, 0.0f};      //euler angle, unit rad.ŷ���� ��λ rad



/**
  * @brief          imu task, init bmi088, ist8310, calculate the euler angle
  * @param[in]      pvParameters: NULL
  * @retval         none
  */
/**
  * @brief          imu����, ��ʼ�� bmi088, ist8310, ����ŷ����
  * @param[in]      pvParameters: NULL
  * @retval         none
  */

void INS_init()
{
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);//temperature regulation start
	HAL_Delay(INS_TASK_INIT_TIME);
	  while(BMI088_init())
    {
        HAL_Delay(100);
    }
    while(ist8310_init())
    {
        HAL_Delay(100);
    }
		
	BMI088_read(bmi088_real_data.gyro, bmi088_real_data.accel, &bmi088_real_data.temp);
	PID_init(&imu_temp_pid, PID_POSITION, imu_temp_PID, TEMPERATURE_PID_MAX_OUT, TEMPERATURE_PID_MAX_IOUT);	
	IMU_temp_PWM(0);
	AHRS_init(INS_quat, bmi088_real_data.accel, ist8310_real_data.mag);	
	SPI1_DMA_init((uint32_t)gyro_dma_tx_buf, (uint32_t)gyro_dma_rx_buf, SPI_DMA_GYRO_LENGHT);
  imu_start_dma_flag = 1;
		 
}

uint32_t code_start=0;
uint32_t code_end=0;
float code_time=0;

void INS_task()
{


        //wait spi DMA tansmit done
        //�ȴ�SPI DMA����
        if (task_start_flag == 1)
        {
        code_end=DWT_CYCCNT;
				code_time=(float)(code_end-code_start)/MY_MCU_SYSCLK;//the code cycle /s
				code_start=DWT_CYCCNT;

        if(gyro_update_flag & (1 << IMU_NOTIFY_SHFITS))
        {
            gyro_update_flag &= ~(1 << IMU_NOTIFY_SHFITS);
            BMI088_gyro_read_over(gyro_dma_rx_buf + BMI088_GYRO_RX_BUF_DATA_OFFSET, bmi088_real_data.gyro);
        }

        if(accel_update_flag & (1 << IMU_UPDATE_SHFITS))
        {
            accel_update_flag &= ~(1 << IMU_UPDATE_SHFITS);
            BMI088_accel_read_over(accel_dma_rx_buf + BMI088_ACCEL_RX_BUF_DATA_OFFSET, bmi088_real_data.accel, &bmi088_real_data.time);
        }

        if(accel_temp_update_flag & (1 << IMU_UPDATE_SHFITS))
        {
            accel_temp_update_flag &= ~(1 << IMU_UPDATE_SHFITS);
            BMI088_temperature_read_over(accel_temp_dma_rx_buf + BMI088_ACCEL_RX_BUF_DATA_OFFSET, &bmi088_real_data.temp);
            imu_temp_control(bmi088_real_data.temp);
        }


        AHRS_update(INS_quat, code_time, bmi088_real_data.gyro, bmi088_real_data.accel, ist8310_real_data.mag);
        get_angle(INS_quat, INS_angle + INS_YAW_ADDRESS_OFFSET, INS_angle + INS_PITCH_ADDRESS_OFFSET, INS_angle + INS_ROLL_ADDRESS_OFFSET);
				if(can_tx_flag==1)
				{
					send_angle_can(INS_angle,bmi088_real_data.temp);
					can_tx_flag=0;
				}
				task_start_flag=0;
			}
    
}

void AHRS_init(fp32 quat[4], fp32 accel[3], fp32 mag[3])
{
    quat[0] = 1.0f;
    quat[1] = 0.0f;
    quat[2] = 0.0f;
    quat[3] = 0.0f;

}

void AHRS_update(fp32 quat[4], fp32 time, fp32 gyro[3], fp32 accel[3], fp32 mag[3])
{
    MahonyAHRSupdate(quat, gyro[0], gyro[1], gyro[2], accel[0], accel[1], accel[2], mag[0], mag[1], mag[2]);
}
void get_angle(fp32 q[4], fp32 *yaw, fp32 *pitch, fp32 *roll)//ת���Ƕ�
{
    *yaw = 57.29578f*atan2f(2.0f*(q[0]*q[3]+q[1]*q[2]), 2.0f*(q[0]*q[0]+q[1]*q[1])-1.0f);
    *pitch = 57.29578f*asinf(-2.0f*(q[1]*q[3]-q[0]*q[2]));
    *roll = 57.29578f*atan2f(2.0f*(q[0]*q[1]+q[2]*q[3]),2.0f*(q[0]*q[0]+q[3]*q[3])-1.0f);
}

void send_angle_can(fp32 *INS,fp32 temp)
{	
	uint8_t txbuf[8];
	uint16_t yaw,pitch,roll,tempx;
	yaw=(uint16_t)((INS[INS_YAW_ADDRESS_OFFSET]+360)/720.0f*16384);
	pitch=(uint16_t)((INS[INS_PITCH_ADDRESS_OFFSET]+360)/720.0f*16384);
	roll=(uint16_t)((INS[INS_ROLL_ADDRESS_OFFSET]+360)/720.0f*16384);
	tempx=(int16_t)(temp+0.5f);//��������
	
	txbuf[0]=0x01;//������
	txbuf[1]=(uint8_t)(yaw/256);//yaw��
	txbuf[2]=(uint8_t)(yaw%256);//yaw��
	txbuf[3]=(uint8_t)(pitch/256);//pitch��
	txbuf[4]=(uint8_t)(pitch%256);//pitch��
	txbuf[5]=(uint8_t)(roll/256);//roll��
	txbuf[6]=(uint8_t)(roll%256);//roll��
	txbuf[7]=tempx;//�¶�

	CAN_senddata(&hcan,txbuf);
}

CAN_RxHeaderTypeDef RXHeader;
uint8_t RXmessage[8];

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)//��������0�����жϻص�����
{
	if(hcan->Instance==CAN1)
	{
		HAL_CAN_GetRxMessage(hcan,CAN_FILTER_FIFO0,&RXHeader,RXmessage);//��ȡ����
		if(RXHeader.StdId==0x01&&RXmessage[0]==0xde)
		{
			can_tx_flag=1;
		}
    }
}

/**
  * @brief          control the temperature of bmi088
  * @param[in]      temp: the temperature of bmi088
  * @retval         none
  */
/**
  * @brief          ����bmi088���¶�
  * @param[in]      temp:bmi088���¶�
  * @retval         none
  */
static void imu_temp_control(fp32 temp)
{
    uint16_t tempPWM;
			
        PID_calc(&imu_temp_pid, temp, 45.0f);
	
        if (imu_temp_pid.out < 0.0f)
        {
            imu_temp_pid.out = 0.0f;
        }
				
        tempPWM = (uint16_t)imu_temp_pid.out;
        IMU_temp_PWM(tempPWM);


}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == INT1_ACCEL_Pin)
    {
        accel_update_flag |= 1 << IMU_DR_SHFITS;
        accel_temp_update_flag |= 1 << IMU_DR_SHFITS;
        if(imu_start_dma_flag)
        {
            imu_cmd_spi_dma();
        }
				
    
    }
     if(GPIO_Pin == INT1_GYRO_Pin)
    {
        gyro_update_flag |= 1 << IMU_DR_SHFITS;
        if(imu_start_dma_flag)
        {
            imu_cmd_spi_dma();
        }
    }
    else if(GPIO_Pin == IST8310_DRDY_Pin)
    {
        mag_update_flag |= 1 << IMU_DR_SHFITS;

         ist8310_read_mag(ist8310_real_data.mag);
        
    }
   else if(GPIO_Pin == GPIO_PIN_4)
    {
			
        //wake up the task
        //��������
        if (task_start_flag != 1)
        {
						task_start_flag=1;
        }
    }


}

/**
  * @brief          open the SPI DMA accord to the value of imu_update_flag
  * @param[in]      none
  * @retval         none
  */
/**
  * @brief          ����imu_update_flag��ֵ����SPI DMA
  * @param[in]      temp:bmi088���¶�
  * @retval         none
  */
static void imu_cmd_spi_dma(void)
{
		//�ر����жϣ���δ����Ͻ����  
		__disable_irq();
    //���������ǵ�DMA����
    if( (gyro_update_flag & (1 << IMU_DR_SHFITS) ) && !(hspi1.hdmarx->Instance->CCR & DMA_CCR_EN) && !(hspi1.hdmatx->Instance->CCR & DMA_CCR_EN) 
    && !(accel_update_flag & (1 << IMU_SPI_SHFITS)) && !(accel_temp_update_flag & (1 << IMU_SPI_SHFITS)))
    {
        gyro_update_flag &= ~(1 << IMU_DR_SHFITS);
        gyro_update_flag |= (1 << IMU_SPI_SHFITS);

        HAL_GPIO_WritePin(CS1_GYRO_GPIO_Port, CS1_GYRO_Pin, GPIO_PIN_RESET);
        SPI1_DMA_enable((uint32_t)gyro_dma_tx_buf, (uint32_t)gyro_dma_rx_buf, SPI_DMA_GYRO_LENGHT);
				//HAL_SPI_TransmitReceive_DMA(&hspi1, gyro_dma_tx_buf, gyro_dma_rx_buf,SPI_DMA_GYRO_LENGHT);
			__enable_irq();
        return;
    }
    //�������ٶȼƵ�DMA����
    if((accel_update_flag & (1 << IMU_DR_SHFITS)) && !(hspi1.hdmarx->Instance->CCR & DMA_CCR_EN) && !(hspi1.hdmatx->Instance->CCR & DMA_CCR_EN)
    && !(gyro_update_flag & (1 << IMU_SPI_SHFITS)) && !(accel_temp_update_flag & (1 << IMU_SPI_SHFITS)))
    {
        accel_update_flag &= ~(1 << IMU_DR_SHFITS);
        accel_update_flag |= (1 << IMU_SPI_SHFITS);

        HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_RESET);
        SPI1_DMA_enable((uint32_t)accel_dma_tx_buf, (uint32_t)accel_dma_rx_buf, SPI_DMA_ACCEL_LENGHT);
				//HAL_SPI_TransmitReceive_DMA(&hspi1, accel_dma_tx_buf, accel_dma_rx_buf,SPI_DMA_ACCEL_LENGHT);
			__enable_irq();
        return;
    }
    


    
    if((accel_temp_update_flag & (1 << IMU_DR_SHFITS)) && !(hspi1.hdmarx->Instance->CCR & DMA_CCR_EN) && !(hspi1.hdmatx->Instance->CCR & DMA_CCR_EN)
    && !(gyro_update_flag & (1 << IMU_SPI_SHFITS)) && !(accel_update_flag & (1 << IMU_SPI_SHFITS)))
    {
        accel_temp_update_flag &= ~(1 << IMU_DR_SHFITS);
        accel_temp_update_flag |= (1 << IMU_SPI_SHFITS);

        HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_RESET);
        SPI1_DMA_enable((uint32_t)accel_temp_dma_tx_buf, (uint32_t)accel_temp_dma_rx_buf, SPI_DMA_ACCEL_TEMP_LENGHT);
				//HAL_SPI_TransmitReceive_DMA(&hspi1, accel_temp_dma_tx_buf, accel_temp_dma_rx_buf,SPI_DMA_ACCEL_TEMP_LENGHT);
			__enable_irq();
        return;
    }
		
		
		__enable_irq();
}


void DMA1_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_IRQn 0 */

  /* USER CODE END DMA1_Channel2_IRQn 0 */
  //HAL_DMA_IRQHandler(&hdma_spi1_rx);
  /* USER CODE BEGIN DMA1_Channel2_IRQn 1 */
	
		 if(__HAL_DMA_GET_FLAG(hspi1.hdmarx,__HAL_DMA_GET_TC_FLAG_INDEX(hspi1.hdmarx))!=RESET)//�ȴ�DMA1_Steam0�������
			{ 	
				__HAL_DMA_CLEAR_FLAG(hspi1.hdmarx,__HAL_DMA_GET_TC_FLAG_INDEX(hspi1.hdmarx));//���DMA1_Steam0������ɱ�־ 
				__HAL_DMA_DISABLE(hspi1.hdmarx);
				__HAL_DMA_DISABLE(hspi1.hdmatx);
	 
				//gyro read over
        //�����Ƕ�ȡ���
        if(gyro_update_flag & (1 << IMU_SPI_SHFITS))
        {
            gyro_update_flag &= ~(1 << IMU_SPI_SHFITS);
            gyro_update_flag |= (1 << IMU_UPDATE_SHFITS);

            HAL_GPIO_WritePin(CS1_GYRO_GPIO_Port, CS1_GYRO_Pin, GPIO_PIN_SET);
            
        }

        //accel read over
        //���ٶȼƶ�ȡ���
        if(accel_update_flag & (1 << IMU_SPI_SHFITS))
        {
            accel_update_flag &= ~(1 << IMU_SPI_SHFITS);
            accel_update_flag |= (1 << IMU_UPDATE_SHFITS);

            HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_SET);
        }
        //temperature read over
        //�¶ȶ�ȡ���
        if(accel_temp_update_flag & (1 << IMU_SPI_SHFITS))
        {
            accel_temp_update_flag &= ~(1 << IMU_SPI_SHFITS);
            accel_temp_update_flag |= (1 << IMU_UPDATE_SHFITS);

            HAL_GPIO_WritePin(CS1_ACCEL_GPIO_Port, CS1_ACCEL_Pin, GPIO_PIN_SET);
        }

        imu_cmd_spi_dma();

        if(gyro_update_flag & (1 << IMU_UPDATE_SHFITS))
        {
            gyro_update_flag &= ~(1 << IMU_UPDATE_SHFITS);
            gyro_update_flag |= (1 << IMU_NOTIFY_SHFITS);

            __HAL_GPIO_EXTI_GENERATE_SWIT(GPIO_PIN_4);
						//EXTI->SWIER |= 1<<4;
        }
    	 
		 
	 //����ж�

	}

	

  /* USER CODE END DMA1_Channel2_IRQn 1 */
}
