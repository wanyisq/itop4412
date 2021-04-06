#ifndef __BP085_H
#define __BMP085_H

#define BMP_ACK_WAIT_TIME   200     //iic通讯时的ack等待时间  
#define BMP085_DEBUG    1  
#define OSS 0   // 大气压的转换时间,有0-3可选值  

#define BMP085_ADDR 0x77

#define BMP_AC1_ADDR        0XAA      //定义校准寄存器的地址  
#define BMP_AC2_ADDR        0XAC  
#define BMP_AC3_ADDR        0XAE  
#define BMP_AC4_ADDR        0XB0  
#define BMP_AC5_ADDR        0XB2  
#define BMP_AC6_ADDR        0XB4  
#define BMP_B1_ADDR         0XB6  
#define BMP_B2_ADDR         0XB8  
#define BMP_MB_ADDR         0XBA  
#define BMP_MC_ADDR         0XBC  
#define BMP_MD_ADDR         0XBE  
  
#define CONTROL_REG_ADDR    0XF4    //控制寄存器，在这个寄存器中设置不同的值可以设置不同转换时间,同时不同的值可以确认转换大气压或者温度  
#define BMP_COVERT_TEMP     0X2E    //转换温度 4.5MS  
#define BMP_COVERT_PRES_0   0X34    //转换大气压 4.5ms  
#define BMP_COVERT_PRES_1   0X74    //转换大气压 7.5ms  
#define BMP_COVERT_PRES_2   0XB4    //转换大气压 13.5ms  
#define BMP_COVERT_PRES_3   0XF4    //转换大气压 25.5ms  
  
#define BMP_TEMP_PRES_DATA_REG  0XF6    //两个字节温度数据  

#endif
