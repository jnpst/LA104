#include <library.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_tim.h>

#include "ir_tx.h"
#include "platform_config.h"

#define IR_TX_CARRIER_FREQ       38000
#define IR_TX_CARRIER_PWM_PERIOD         (SystemCoreClock / IR_TX_CARRIER_FREQ)
#define IR_TX_DELAY_PRESCALER            (72 - 1)

volatile uint16_t g_tx_bufferIndex;
volatile uint16_t g_tx_repeatCount;
volatile IrCode* g_tx_code = 0;

void _ir_tx_on();
void _ir_tx_off();

void ir_tx_setup() 
{
  TIM_TimeBaseInitTypeDef timeBaseInit;
  TIM_OCInitTypeDef ocInit;
  GPIO_InitTypeDef irTxGpioInit;
  NVIC_InitTypeDef nvicInit;

  RCC_APB1PeriphClockCmd(IR_TX_CARRIER_TIMER_RCC, ENABLE);
  RCC_APB2PeriphClockCmd(IR_TX_CARRIER_RCC, ENABLE);

  GPIO_StructInit(&irTxGpioInit);
  irTxGpioInit.GPIO_Pin = IR_TX_CARRIER_PIN;
  irTxGpioInit.GPIO_Mode = GPIO_Mode_AF_PP;
  irTxGpioInit.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(IR_TX_CARRIER_PORT, &irTxGpioInit);

  // Time base configuration
  TIM_TimeBaseStructInit(&timeBaseInit);
  timeBaseInit.TIM_Period = IR_TX_CARRIER_PWM_PERIOD;
  timeBaseInit.TIM_Prescaler = 0;
  timeBaseInit.TIM_ClockDivision = 0;
  timeBaseInit.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(IR_TX_CARRIER_TIMER, &timeBaseInit);

  TIM_OCStructInit(&ocInit);
  ocInit.TIM_OCMode = TIM_OCMode_PWM1;
  ocInit.TIM_OCPolarity = TIM_OCPolarity_High;
  ocInit.TIM_OutputState = TIM_OutputState_Enable;
  ocInit.TIM_Pulse = 0;
  IR_TX_CARRIER_TIMER_CH_Init(IR_TX_CARRIER_TIMER, &ocInit);
  IR_TX_CARRIER_TIMER_CH_PreloadConfig(IR_TX_CARRIER_TIMER, TIM_OCPreload_Enable);

  TIM_SelectOnePulseMode(IR_TX_CARRIER_TIMER, TIM_OPMode_Repetitive);
  TIM_ARRPreloadConfig(IR_TX_CARRIER_TIMER, ENABLE);
  IR_TX_CARRIER_TIMER_CH_SetCompare(IR_TX_CARRIER_TIMER, IR_TX_CARRIER_PWM_PERIOD / 2);
  TIM_Cmd(IR_TX_CARRIER_TIMER, ENABLE);

  // setup delay timer
  RCC_APB2PeriphClockCmd(IR_TX_DELAY_TIMER_RCC, ENABLE);

  TIM_TimeBaseStructInit(&timeBaseInit);
  timeBaseInit.TIM_Period = 0xffff;
  timeBaseInit.TIM_Prescaler = IR_TX_DELAY_PRESCALER;
  timeBaseInit.TIM_ClockDivision = 0;
  timeBaseInit.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(IR_TX_DELAY_TIMER, &timeBaseInit);

  nvicInit.NVIC_IRQChannel = IR_TX_DELAY_TIMER_IRQ;
  nvicInit.NVIC_IRQChannelPreemptionPriority = 1;
  nvicInit.NVIC_IRQChannelSubPriority = 0;
  nvicInit.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInit);
}

void ir_tx_start()
{
  TIM_ITConfig(IR_TX_DELAY_TIMER, TIM_IT_Update, ENABLE);
  _ir_tx_off();
}

void ir_tx_stop()
{
  TIM_ITConfig(IR_TX_DELAY_TIMER, TIM_IT_Update, DISABLE);
}

void ir_tx_send(IrCode* code) 
{
  g_tx_bufferIndex = 0;
  g_tx_repeatCount = code->repeatCount;
  g_tx_code = code;

  TIM_SetCounter(IR_TX_DELAY_TIMER, 1);
  TIM_SetAutoreload(IR_TX_DELAY_TIMER, g_tx_code->code[g_tx_bufferIndex++]);
  _ir_tx_on();

  TIM_Cmd(IR_TX_DELAY_TIMER, ENABLE);
}

void ir_irq() 
{
  if (TIM_GetITStatus(IR_TX_DELAY_TIMER, TIM_IT_Update) != RESET) 
  {
    if (g_tx_code)
    {
      if((g_tx_bufferIndex % 2) == 0)
        _ir_tx_on();
      else
        _ir_tx_off();

      if(g_tx_bufferIndex < g_tx_code->codeLength - 1) 
      {
        TIM_SetCounter(IR_TX_DELAY_TIMER, 1);
        TIM_SetAutoreload(IR_TX_DELAY_TIMER, g_tx_code->code[g_tx_bufferIndex++]);
      } else 
      {
        g_tx_repeatCount--;
        _ir_tx_off();
        if(g_tx_repeatCount == 0) 
        {
          g_tx_code = 0;
          TIM_Cmd(IR_TX_DELAY_TIMER, DISABLE);
        } else 
        {
          g_tx_bufferIndex = 0;
          TIM_SetCounter(IR_TX_DELAY_TIMER, 1);
          TIM_SetAutoreload(IR_TX_DELAY_TIMER, g_tx_code->gap);
        }
      }
    }
    TIM_ClearITPendingBit(IR_TX_DELAY_TIMER, TIM_IT_Update);
  }
}

template<uint32_t nPortBase, int nPin, int nState>
void SetGpioState()
{
  constexpr int nPinPos = nPin & 7;
  constexpr uint32_t dwMask = ~(0xf << (nPinPos*4));
  constexpr uint32_t dwBits = nState << (nPinPos*4);

  if (nPin < 8)
  {
    uint32_t* pCRL = (uint32_t*)nPortBase;
    *pCRL = (*pCRL & dwMask) | dwBits;
  } else
  {
    uint32_t* pCRH = (uint32_t*)(nPortBase + 4);
    *pCRH = (*pCRH & dwMask) | dwBits;
  }
}

void _ir_tx_on() 
{
  SetGpioState<GPIOB_BASE, 9, GPIO_Speed_50MHz | GPIO_Mode_AF_PP>();
}

void _ir_tx_off() 
{
  SetGpioState<GPIOB_BASE, 9, GPIO_Speed_50MHz | GPIO_Mode_Out_PP>();
}
