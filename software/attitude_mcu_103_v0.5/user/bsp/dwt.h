#ifndef BMI088REG_H
#define BMI088REG_H
#include "main.h"

#define MY_MCU_SYSCLK 72000000
// 0xE000EDFC DEMCR RW Debug Exception and Monitor Control Register.
#define DEMCR           ( *(unsigned int *)0xE000EDFC )
#define TRCENA          ( 0x01 << 24) // DEMCR的DWT使能位
 
// 0xE0001000 DWT_CTRL RW The Debug Watchpoint and Trace (DWT) unit
#define DWT_CTRL        ( *(unsigned int *)0xE0001000 )
#define CYCCNTENA       ( 0x01 << 0 ) // DWT的SYCCNT使能位
// 0xE0001004 DWT_CYCCNT RW Cycle Count register, 
#define DWT_CYCCNT      ( *(unsigned int *)0xE0001004) // 显示或设置处理器的周期计数值
 
//#define DWT_DELAY_mS(mSec)    DWT_DELAY_uS(mSec*1000)

void DWT_INIT(int sys_clk);
void DWT_DELAY_uS(int uSec);


#endif

