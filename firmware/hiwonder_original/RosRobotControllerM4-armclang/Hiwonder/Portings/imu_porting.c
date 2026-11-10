//#include "global.h"
//#include "lwmem_porting.h"
//#include "packet_reports.h"
//#include "global_conf.h"
//#include "imu.h"
//#include "imu_mpu6050.h"

//IMU_ObjectTypeDef *imus[1];

//static int i2c_write_byte_to_mem(MPU6050ObjectTypeDef *self, uint8_t reg_addr, uint8_t data);
//static int i2c_read_from_mem(MPU6050ObjectTypeDef *self, uint8_t reg_addr, uint32_t length, uint8_t *buf);
//static void DelayMs(uint32_t ms);

//void imus_init(void)
//{
//    MPU6050ObjectTypeDef *mpu6050 = LWMEM_CCM_MALLOC(sizeof(MPU6050ObjectTypeDef));
//    mpu6050_object_init(mpu6050, MPU6050_DEV_ADDR_1);
//    mpu6050->i2c_write_byte_to_mem = i2c_write_byte_to_mem;
//    mpu6050->i2c_read_from_mem = i2c_read_from_mem;
//    mpu6050->sleep_ms = DelayMs;
//    imus[0] = (IMU_ObjectTypeDef*)mpu6050;
//}

//#if ENABLE_IMU
///**
// * @brief  imu task 入口函数
// *
// */

//void imu_task_entry(void *argument)
//{
//	//声明IMU的外部句柄
//    extern osSemaphoreId_t mpu6050_data_readyHandle;

//	//初始化IMU
//    imus_init();
//	//重置参数
//    imus[0]->reset(imus[0]);

//    for(;;) {
//		//等待IMU中断发送的信号量，若没有，则一直阻塞等待 osWaitForever
//        if( osOK == osSemaphoreAcquire(mpu6050_data_readyHandle, osWaitForever))
//		{
//			HAL_GPIO_WritePin(LED_SYS_GPIO_Port, LED_SYS_Pin, GPIO_PIN_SET);
//			//读取IMU的数据，更新姿态参数
//			imus[0]->update(imus[0]);
//			HAL_GPIO_WritePin(LED_SYS_GPIO_Port, LED_SYS_Pin, GPIO_PIN_RESET);
//		}
////		osDelay(50);
//    }
//}

//#endif


//static int i2c_write_byte_to_mem(MPU6050ObjectTypeDef *self, uint8_t reg_addr, uint8_t data)
//{
//	int value = 0;
//	//进入中断级代码临界区
//	uint32_t ret = taskENTER_CRITICAL_FROM_ISR(); 
//	value = HAL_I2C_Mem_Write(&hi2c2, self->dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, &data, 1, 0xFF);
//	//退出中断级代码临界区
//	taskEXIT_CRITICAL_FROM_ISR(ret);
//    return value;
////	return HAL_I2C_Mem_Write(&hi2c2, self->dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, &data, 1, 0xFF);
//}

//static int i2c_read_from_mem(MPU6050ObjectTypeDef *self, uint8_t reg_addr, uint32_t length, uint8_t *buf)
//{
//// return HAL_I2C_Mem_Read(&hi2c2, self->dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, buf, length, 0xFF);
////    return HAL_I2C_Mem_Read_DMA(&hi2c2, self->dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, buf, length);
//	int value = 0;
//	//进入中断级代码临界区
//	uint32_t ret = taskENTER_CRITICAL_FROM_ISR(); 
//	value = HAL_I2C_Mem_Read(&hi2c2, self->dev_addr << 1, reg_addr, I2C_MEMADD_SIZE_8BIT, buf, length, 0xFF);
//	//退出中断级代码临界区
//	taskEXIT_CRITICAL_FROM_ISR(ret);
//    return value;
//}

//static void DelayMs(uint32_t ms)
//{
//    osDelay(ms);
//}

#include "global.h"
#include "lwmem_porting.h"
#include "packet_reports.h"
#include "global_conf.h"
#include "QMI8658.h"
#include "u8g2_porting.h"

#if ENABLE_IMU
/**
 * @brief  imu task 入口函数
 *
 */

struct QMI8658 qmi8658;
		
void imu_task_entry(void *argument)
{

	//声明IMU的外部句柄
    extern osSemaphoreId_t IMU_data_readyHandle;
	extern osMutexId_t oled_mutexHandle;
	//声明存储姿态数据数组
    float rpy[3];

    if(begin() == 0) 
    {
        printf("qmi8658_init fail");
    }else{
		printf("qmi8658_init success");
	}
    osDelay(100);
    PacketReportIMU_Raw_TypeDef report;
		
    for(;;) {
		//等待IMU中断发送的信号量，若没有，则一直阻塞等待 osWaitForever
        osSemaphoreAcquire(IMU_data_readyHandle, osWaitForever);

		//获取欧拉角
		//GetEulerAngles(&rpy[0],&rpy[1],&rpy[2]);
        //printf("%f,%f,%f\r\n",rpy[0],rpy[1],rpy[2]);

		read_xyz(report.array.accel_array,report.array.gyro_array);
			
		//获取Roll、Pitch、重力加速度值请放开下2行注释(第2行注释语句需要配合树莓派等上位机运行搭配ros开发功能包使用) 
        //printf("%d,%d,%d\r\n",(int)report.array.accel_array[0],(int)report.array.accel_array[1],(int)report.array.accel_array[2]);
        //packet_transmit(&packet_controller, PACKET_FUNC_IMU, &report, sizeof(PacketReportIMU_Raw_TypeDef));
        osDelay(50);
    }
}
#endif
