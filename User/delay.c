#include "delay.h"

/*
 * 基于 SysTick 的延时函数
 * 系统时钟 72MHz，所以 1 微秒 = 72 个时钟周期
 *
 * 注意：SysTick->LOAD 是 24 位，最大 16777215
 *       所以单次 Delay_us 不要超过 16777215 / 72 ≈ 233000 微秒（约 233ms）
 *       更长的时间请用 Delay_ms，它内部是循环调用 Delay_us(1000)
 */
void Delay_us(uint32_t us)
{
    SysTick->LOAD = 72 * us;        /* 重装值：72 乘微秒数 */
    SysTick->VAL  = 0x00;           /* 清空当前计数值 */
    SysTick->CTRL = 0x00000005;     /* bit0=使能  bit2=时钟源选 HCLK(72MHz) */

    while (!(SysTick->CTRL & 0x00010000));   /* 等 COUNTFLAG 置位（计数器减到 0） */

    SysTick->CTRL = 0x00000004;     /* 关闭计数器 */
    SysTick->VAL  = 0x00;
}

void Delay_ms(uint32_t ms)
{
    while (ms--)
    {
        Delay_us(1000);
    }
}
