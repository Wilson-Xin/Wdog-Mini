#include "dwt.h"

 
static int SYSCLK = 0;;
 
void DWT_INIT(int sys_clk)
{
  DEMCR |= TRCENA;
  DWT_CTRL |= CYCCNTENA;
  
  SYSCLK = sys_clk; // 保存当前系统的时钟周期，eg. 72,000,000(72MHz). 
}
 
// 微秒延时
void DWT_DELAY_uS(int uSec)
{
  int ticks_start, ticks_end, ticks_delay;
  
  ticks_start = DWT_CYCCNT;
  
  if ( !SYSCLK )
    DWT_INIT( MY_MCU_SYSCLK );
  
  ticks_delay = ( uSec * ( SYSCLK / (1000*1000) ) ); // 将微秒数换算成滴答数          
  
  ticks_end = ticks_start + ticks_delay;
  
  if ( ticks_end > ticks_start )
  {
    while( DWT_CYCCNT < ticks_end );
  }
  else // 计数溢出，翻转
  {
    while( DWT_CYCCNT >= ticks_end ); // 翻转后的值不会比ticks_end小
    while( DWT_CYCCNT < ticks_end );
  }
}

