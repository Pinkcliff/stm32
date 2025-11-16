/*
 * STM32 六LED点灯程序
 * 假设使用的是STM32F103系列
 * LED1连接到PC13，LED2连接到PC14，LED3连接到PC15
 * LED4连接到PB0，LED5连接到PB1，LED6连接到PB2
 */

#include "stm32f10x.h"

void LEDs_Init(void);
void Delay(uint32_t count);
void LED_Show_Pattern(uint8_t pattern);

// LED引脚定义
#define LED1_PIN    GPIO_Pin_13
#define LED2_PIN    GPIO_Pin_14
#define LED3_PIN    GPIO_Pin_15
#define LED4_PIN    GPIO_Pin_0
#define LED5_PIN    GPIO_Pin_1
#define LED6_PIN    GPIO_Pin_2
#define LED_PORT_C  GPIOC
#define LED_PORT_B  GPIOB

// 模式定义
#define LED_ALL_OFF     0x00
#define LED1_ON         0x01
#define LED2_ON         0x02
#define LED3_ON         0x04
#define LED4_ON         0x08
#define LED5_ON         0x10
#define LED6_ON         0x20
#define LED1_2_ON       0x03
#define LED1_3_ON       0x05
#define LED2_3_ON       0x06
#define LED4_5_ON       0x18
#define LED5_6_ON       0x30
#define LED_ALL_ON      0x3F

int main(void)
{
    uint16_t delay_count = 300000;

    // 初始化LED引脚
    LEDs_Init();

    while(1)
    {
        // 模式1：依次点亮LED1、LED2、LED3
        LED_Show_Pattern(LED1_ON);
        Delay(delay_count);

        LED_Show_Pattern(LED2_ON);
        Delay(delay_count);

        LED_Show_Pattern(LED3_ON);
        Delay(delay_count);

        // 模式2：依次点亮LED4、LED5、LED6
        LED_Show_Pattern(LED4_ON);
        Delay(delay_count);

        LED_Show_Pattern(LED5_ON);
        Delay(delay_count);

        LED_Show_Pattern(LED6_ON);
        Delay(delay_count);

        // 模式3：LED1和LED3同时点亮，LED4和LED6同时点亮
        LED_Show_Pattern(LED1_3_ON);
        Delay(delay_count);

        LED_Show_Pattern(LED4_5_ON);
        Delay(delay_count);

        // 模式4：所有LED依次点亮然后熄灭
        LED_Show_Pattern(LED_ALL_ON);
        Delay(delay_count);

        LED_Show_Pattern(LED_ALL_OFF);
        Delay(delay_count);

        // 模式5：交替闪烁（前三个LED和后三个LED交替）
        LED_Show_Pattern(LED1_2_ON | LED3_ON);
        Delay(delay_count/2);

        LED_Show_Pattern(LED4_ON | LED5_6_ON);
        Delay(delay_count/2);

        LED_Show_Pattern(LED_ALL_OFF);
        Delay(delay_count);

        // 逐个点亮形成流水灯效果（6个LED）
        for(int i = 0; i < 6; i++) {
            LED_Show_Pattern(1 << i);
            Delay(delay_count/2);
        }

        // 反向流水灯
        for(int i = 5; i >= 0; i--) {
            LED_Show_Pattern(1 << i);
            Delay(delay_count/2);
        }

        // 对称点亮效果
        LED_Show_Pattern(LED1_ON | LED6_ON);
        Delay(delay_count/2);

        LED_Show_Pattern(LED2_ON | LED5_ON);
        Delay(delay_count/2);

        LED_Show_Pattern(LED3_ON | LED4_ON);
        Delay(delay_count/2);

        LED_Show_Pattern(LED_ALL_OFF);
        Delay(delay_count);
    }
}

/**
 * @brief  六个LED初始化函数
 * @param  无
 * @retval 无
 */
void LEDs_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 使能GPIOC和GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOB, ENABLE);

    // 配置PC13、PC14、PC15为推挽输出
    GPIO_InitStructure.GPIO_Pin = LED1_PIN | LED2_PIN | LED3_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT_C, &GPIO_InitStructure);

    // 配置PB0、PB1、PB2为推挽输出
    GPIO_InitStructure.GPIO_Pin = LED4_PIN | LED5_PIN | LED6_PIN;
    GPIO_Init(LED_PORT_B, &GPIO_InitStructure);

    // 初始状态下熄灭所有LED
    GPIO_SetBits(LED_PORT_C, LED1_PIN | LED2_PIN | LED3_PIN);
    GPIO_SetBits(LED_PORT_B, LED4_PIN | LED5_PIN | LED6_PIN);
}

/**
 * @brief  LED模式显示函数
 * @param  pattern: LED显示模式 (使用位掩码)
 * @retval 无
 */
void LED_Show_Pattern(uint8_t pattern)
{
    // 控制GPIOC上的LED1、LED2、LED3
    if(pattern & LED1_ON) {
        GPIO_ResetBits(LED_PORT_C, LED1_PIN);  // LED1点亮 (低电平)
    } else {
        GPIO_SetBits(LED_PORT_C, LED1_PIN);    // LED1熄灭
    }

    if(pattern & LED2_ON) {
        GPIO_ResetBits(LED_PORT_C, LED2_PIN);  // LED2点亮
    } else {
        GPIO_SetBits(LED_PORT_C, LED2_PIN);    // LED2熄灭
    }

    if(pattern & LED3_ON) {
        GPIO_ResetBits(LED_PORT_C, LED3_PIN);  // LED3点亮
    } else {
        GPIO_SetBits(LED_PORT_C, LED3_PIN);    // LED3熄灭
    }

    // 控制GPIOB上的LED4、LED5、LED6
    if(pattern & LED4_ON) {
        GPIO_ResetBits(LED_PORT_B, LED4_PIN);  // LED4点亮
    } else {
        GPIO_SetBits(LED_PORT_B, LED4_PIN);    // LED4熄灭
    }

    if(pattern & LED5_ON) {
        GPIO_ResetBits(LED_PORT_B, LED5_PIN);  // LED5点亮
    } else {
        GPIO_SetBits(LED_PORT_B, LED5_PIN);    // LED5熄灭
    }

    if(pattern & LED6_ON) {
        GPIO_ResetBits(LED_PORT_B, LED6_PIN);  // LED6点亮
    } else {
        GPIO_SetBits(LED_PORT_B, LED6_PIN);    // LED6熄灭
    }
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