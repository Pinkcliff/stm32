/*
 * STM32 LED点灯程序
 * 假设使用的是STM32F103系列，LED连接到PC13引脚
 */

#include "stm32f10x.h"

void LED_Init(void);
void Delay(uint32_t count);

int main(void)
{
    // 初始化LED引脚
    LED_Init();

    while(1)
    {
        // 点亮LED (低电平点亮)
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        Delay(500000);

        // 熄灭LED
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        Delay(500000);
    }
}

/**
 * @brief  LED初始化函数
 * @param  无
 * @retval 无
 */
void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能GPIOC时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // 配置PC13为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 初始状态下熄灭LED
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

/**
 * @brief  简单延时函数
 * @param  count: 延时计数值
 * @retval 无
 */
void Delay(uint32_t count)
{
    uint32_t i;
    for(i = 0; i < count; i++);
}