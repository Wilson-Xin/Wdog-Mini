#include "dwt.h"

 
static int SYSCLK = 0;;
 
void DWT_INIT(int sys_clk)
{
  DEMCR |= TRCENA;
  DWT_CTRL |= CYCCNTENA;
  
  SYSCLK = sys_clk; // ���浱ǰϵͳ��ʱ�����ڣ�eg. 72,000,000(72MHz). 
}
 
// ΢����ʱ
void DWT_DELAY_uS(int uSec)
{
  int ticks_start, ticks_end, ticks_delay;
  
  ticks_start = DWT_CYCCNT;
  
  if ( !SYSCLK )
    DWT_INIT( MY_MCU_SYSCLK );
  
  ticks_delay = ( uSec * ( SYSCLK / (1000*1000) ) ); // ��΢��������ɵδ���          
  
  ticks_end = ticks_start + ticks_delay;
  
  if ( ticks_end > ticks_start )
  {
    while( DWT_CYCCNT < ticks_end );
  }
  else // �����������ת
  {
    while( DWT_CYCCNT >= ticks_end ); // ��ת���ֵ�����ticks_endС
    while( DWT_CYCCNT < ticks_end );
  }
}

