/*****************************************************************************/
/**                             IO V2.0                                     **/
/** Data: 01/05/2026                          IDE: Mounriver Studio 2.4.0   **/
/** Autor: Marcos Roberto Braga               MOD: 01/05/2026               **/
/**                                                                         **/
/** Descrição: Biblioteca para configuração e manipulação de portas I/O     **/
/**            do CH32V003 usando o padrão de structs da indústria.         **/
/**            Compatível com toda a família CH32Vxxx.                      **/
/*****************************************************************************/

#ifndef __CH32V003_IO_V2_H
#define __CH32V003_IO_V2_H

/* ======================================================================== */
/* INCLUDES                                                                  */
/* ======================================================================== */
#include "ch32v00x.h"           /* Biblioteca padrão do fabricante           */

/* ======================================================================== */
/* DEFINES DE PINOS (FACILITA A LEITURA)                                    */
/* ======================================================================== */

/* ====================== Port A ====================== */
#define PA0     GPIO_Pin_0
#define PA1     GPIO_Pin_1
#define PA2     GPIO_Pin_2

/* ====================== Port C ====================== */
#define PC0     GPIO_Pin_0
#define PC1     GPIO_Pin_1
#define PC2     GPIO_Pin_2
#define PC3     GPIO_Pin_3
#define PC4     GPIO_Pin_4
#define PC5     GPIO_Pin_5
#define PC6     GPIO_Pin_6
#define PC7     GPIO_Pin_7

/* ====================== Port D ====================== */
#define PD0     GPIO_Pin_0
#define PD1     GPIO_Pin_1
#define PD2     GPIO_Pin_2
#define PD3     GPIO_Pin_3
#define PD4     GPIO_Pin_4
#define PD5     GPIO_Pin_5
#define PD6     GPIO_Pin_6
#define PD7     GPIO_Pin_7

/* ======================================================================== */
/* DEFINES DE CONFIGURAÇÃO (LEGADO - MANTÉM COMPATIBILIDADE)                 */
/* ======================================================================== */
#define Out30PP     GPIO_Mode_Out_PP
#define Out30OD     GPIO_Mode_Out_OD
#define Out30AF_PP  GPIO_Mode_AF_PP
#define Out30AF_OD  GPIO_Mode_AF_OD
#define InFloat     GPIO_Mode_IN_FLOATING
#define InPU        GPIO_Mode_IPU
#define InPD        GPIO_Mode_IPD
#define InAn        GPIO_Mode_AIN

/* ======================================================================== */
/* DEFINES DE PORTAS (ESCRITA RÁPIDA)                                       */
/* ======================================================================== */
#define PORTA       GPIOA
#define PORTC       GPIOC
#define PORTD       GPIOD

/* ======================================================================== */
/* FUNÇÕES DE CONFIGURAÇÃO DOS PINOS (TRIS)                                 */
/* ======================================================================== */

/**
 * @brief  Configura um pino do Port A
 * @param  pin  : Pino a ser configurado (use PA0, PA1, PA2)
 * @param  mode : Modo de operação (use Out30PP, InFloat, etc.)
 * @note   Exemplo: TRISAbits(PC0, Out30PP);
 */
void TRISAbits(uint16_t pin, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    /* Habilita clock da porta (se necessário) */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin   = pin;
    GPIO_InitStructure.GPIO_Mode  = mode;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

/**
 * @brief  Configura um pino do Port C
 * @param  pin  : Pino a ser configurado (use PC0, PC1, etc.)
 * @param  mode : Modo de operação (use Out30PP, InFloat, etc.)
 * @note   Exemplo: TRISCbits(PC0, Out30PP);
 */
void TRISCbits(uint16_t pin, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin   = pin;
    GPIO_InitStructure.GPIO_Mode  = mode;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
 * @brief  Configura um pino do Port D
 * @param  pin  : Pino a ser configurado (use PD0, PD1, etc.)
 * @param  mode : Modo de operação (use Out30PP, InFloat, etc.)
 * @note   Exemplo: TRISDbits(PD5, Out30PP);
 */
void TRISDbits(uint16_t pin, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin   = pin;
    GPIO_InitStructure.GPIO_Mode  = mode;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/* ======================================================================== */
/* FUNÇÕES DE CONFIGURAÇÃO DE MÚLTIPLOS PINOS                               */
/* ======================================================================== */

/**
 * @brief  Configura múltiplos pinos do Port C de uma só vez
 * @param  pins : Máscara de pinos (ex: PC0 | PC1 | PC2)
 * @param  mode : Modo de operação
 * @note   Mais eficiente que chamar TRISCbits várias vezes.
 *         Exemplo: TRISC_Init(PC0 | PC1 | PC5, Out30PP);
 */
void TRISC_Init(uint16_t pins, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin   = pins;
    GPIO_InitStructure.GPIO_Mode  = mode;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/**
 * @brief  Configura múltiplos pinos do Port D de uma só vez
 * @param  pins : Máscara de pinos (ex: PD5 | PD6)
 * @param  mode : Modo de operação
 */
void TRISD_Init(uint16_t pins, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin   = pins;
    GPIO_InitStructure.GPIO_Mode  = mode;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/* ======================================================================== */
/* FUNÇÕES DE ESCRITA (SAÍDA) - USANDO BSHR PARA ATOMICIDADE               */
/* ======================================================================== */

/**
 * @brief  Liga um pino (SET)
 * @param  port : Porta (GPIOA, GPIOC ou GPIOD)
 * @param  pin  : Pino a ser ligado
 */
#define GPIO_Set(port, pin)  (port)->BSHR = (pin)

/**
 * @brief  Desliga um pino (RESET)
 * @param  port : Porta (GPIOA, GPIOC ou GPIOD)
 * @param  pin  : Pino a ser desligado
 */
#define GPIO_Reset(port, pin) (port)->BCR = (pin)

/**
 * @brief  Alterna o estado de um pino (TOGGLE)
 * @param  port : Porta (GPIOA, GPIOC ou GPIOD)
 * @param  pin  : Pino a ser alternado
 */
#define GPIO_Toggle(port, pin) (port)->OUTDR ^= (pin)

/* ======================================================================== */
/* FUNÇÕES DE ESCRITA (SAÍDA) - COMPATÍVEIS COM V1.0                        */
/* ======================================================================== */

/**
 * @brief  Escreve nível lógico em um pino
 * @param  port : Porta (GPIOC, GPIOD)
 * @param  pin  : Pino (PC0, PD5, etc.)
 * @param  val  : 1 = HIGH, 0 = LOW
 */
void GPIO_WritePin(GPIO_TypeDef *port, uint16_t pin, uint8_t val)
{
    if(val)
        port->BSHR = pin;
    else
        port->BCR = pin;
}

/* ======================================================================== */
/* MACROS DE COMPATIBILIDADE COM V1.0                                       */
/* ======================================================================== */
#define PORTC_OUT(pin, val)  GPIO_WritePin(GPIOC, (1 << pin), val)
#define PORTD_OUT(pin, val)  GPIO_WritePin(GPIOD, (1 << pin), val)
#define PORTA_OUT(pin, val)  GPIO_WritePin(GPIOA, (1 << pin), val)

/* ======================================================================== */
/* FUNÇÕES DE LEITURA (ENTRADA)                                             */
/* ======================================================================== */

/**
 * @brief  Lê o estado de um pino
 * @param  port : Porta (GPIOA, GPIOC ou GPIOD)
 * @param  pin  : Pino a ser lido
 * @return 1 = HIGH, 0 = LOW
 */
uint8_t GPIO_ReadPin(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->INDR & pin) ? 1 : 0;
}

/* ======================================================================== */
/* MACROS DE LEITURA (COMPATÍVEIS COM V1.0)                                  */
/* ======================================================================== */
#define PA0_READ    GPIO_ReadPin(GPIOA, GPIO_Pin_0)
#define PA1_READ    GPIO_ReadPin(GPIOA, GPIO_Pin_1)
#define PA2_READ    GPIO_ReadPin(GPIOA, GPIO_Pin_2)

#define PC0_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_0)
#define PC1_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_1)
#define PC2_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_2)
#define PC3_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_3)
#define PC4_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_4)
#define PC5_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_5)
#define PC6_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_6)
#define PC7_READ    GPIO_ReadPin(GPIOC, GPIO_Pin_7)

#define PD0_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_0)
#define PD1_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_1)
#define PD2_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_2)
#define PD3_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_3)
#define PD4_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_4)
#define PD5_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_5)
#define PD6_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_6)
#define PD7_READ    GPIO_ReadPin(GPIOD, GPIO_Pin_7)

/* ======================================================================== */
/* FUNÇÃO DE INICIALIZAÇÃO RÁPIDA (VÁRIOS PINOS)                            */
/* ======================================================================== */

/**
 * @brief  Inicializa múltiplos pinos do Port C de uma só vez
 * @param  pins   : Máscara de pinos
 * @param  mode   : Modo dos pinos de saída
 * @note   Versão otimizada que configura todos os pinos de uma vez.
 *         Exemplo: Init_GpioC(PC0 | PC1, Out30PP, PC2, InPU);
 */
void Init_GpioC(uint16_t out_pins, GPIOMode_TypeDef out_mode,
                uint16_t in_pins,  GPIOMode_TypeDef in_mode)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    /* Configura pinos de saída */
    if(out_pins)
    {
        GPIO_InitStructure.GPIO_Pin   = out_pins;
        GPIO_InitStructure.GPIO_Mode  = out_mode;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOC, &GPIO_InitStructure);
    }
    
    /* Configura pinos de entrada */
    if(in_pins)
    {
        GPIO_InitStructure.GPIO_Pin   = in_pins;
        GPIO_InitStructure.GPIO_Mode  = in_mode;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(GPIOC, &GPIO_InitStructure);
    }
}

/* ======================================================================== */
/* EXEMPLOS DE USO                                                          */
/* ======================================================================== */

/*
 * EXEMPLO 1: MÍNIMO
 * 
 * #include "CH32V003_IO_V2.h"
 * 
 * int main(void)
 * {
 *     SystemInit();                    // Da biblioteca padrão
 *     
 *     TRISCbits(PC0, Out30PP);        // LED no PC0
 *     TRISCbits(PC2, InPU);           // Botão no PC2
 *     
 *     while(1)
 *     {
 *         if(PC2_READ)                 // Botão solto (pull-up)
 *             GPIO_Set(GPIOC, PC0);    // Liga LED
 *         else
 *             GPIO_Reset(GPIOC, PC0);  // Desliga LED
 *     }
 * }
 * 
 * EXEMPLO 2: INICIALIZAÇÃO OTIMIZADA
 * 
 * Init_GpioC(PC0 | PC1, Out30PP,     // LEDs
 *            PC2 | PC3, InPU);       // Botões
 */

#endif /* __CH32V003_IO_V2_H */