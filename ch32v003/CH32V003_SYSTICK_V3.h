/*****************************************************************************/
/**                       SysTick V3.0 (COMPACTA)                           **/
/** Data: 01/05/2026                          IDE: Mounriver Studio 2.4.0   **/
/** Autor: Marcos Roberto Braga               MOD: 01/05/2026               **/
/**                                                                         **/
/** Descrição: Biblioteca SysTick compacta para CH32V003.                   **/
/**            Base de tempo para o Super Loop.                             **/
/**            Resolução configurável: 1ms, 10ms ou personalizada.          **/
/*****************************************************************************/

#ifndef __CH32V003_SYSTICK_V3_H
#define __CH32V003_SYSTICK_V3_H

#include "ch32v00x.h"

/* ======================================================================== */
/* DEFINES DE RESOLUÇÃO (PASSE NA INICIALIZAÇÃO)                            */
/* ======================================================================== */
#define SYSTICK_RES_1MS      0x01    /* Tick a cada 1ms (1000 Hz)           */
#define SYSTICK_RES_10MS     0x0A    /* Tick a cada 10ms (100 Hz)           */
#define SYSTICK_RES_100MS    0x64    /* Tick a cada 100ms (10 Hz)           */

/* ======================================================================== */
/* VARIÁVEL GLOBAL DO SYSTICK (ALOCADA PELO USUÁRIO)                        */
/* ======================================================================== */
extern volatile uint32_t Systick_Counter;   /* Contador de ticks do SysTick  */

/* ======================================================================== */
/* INICIALIZAÇÃO PRINCIPAL                                                   */
/* ======================================================================== */

/**
 * @brief  Inicializa o SysTick com a resolução desejada
 * @param  resolution_ms : Período do tick em ms (SYSTICK_RES_1MS, _10MS, _100MS)
 *                         Ou um valor personalizado (1 a 1000)
 * @note   Exemplos:
 *         - SysTick_Init(SYSTICK_RES_1MS);   // Tick a cada 1ms
 *         - SysTick_Init(SYSTICK_RES_10MS);  // Tick a cada 10ms (padrão)
 *         - SysTick_Init(5);                 // Tick a cada 5ms
 *         
 *         Fórmula do CMP: (SystemCoreClock / 1000) * resolution_ms - 1
 *         
 *         ⚠️  O SysTick usa o clock do sistema (HCLK).
 *         ⚠️  Resolução máxima: 1ms (SystemCoreClock / 1000)
 *         ⚠️  Resolução mínima: 1000ms (1 segundo)
 */
void SysTick_Init(uint32_t resolution_ms)
{
    uint32_t cmp_value;
    
    /* 1. Desabilita SysTick durante a configuração */
    SysTick->CTLR = 0;
    
    /* 2. Calcula o valor de comparação */
    /*
     * Fórmula: CMP = (SystemCoreClock / 1000) * resolution_ms - 1
     * 
     * Exemplos a 48MHz:
     *   1ms:  (48000000 / 1000) * 1  - 1 = 47999
     *   10ms: (48000000 / 1000) * 10 - 1 = 479999
     *   100ms:(48000000 / 1000) * 100- 1 = 4799999
     */
    cmp_value = ((SystemCoreClock / 1000) * resolution_ms) - 1;
    
    /* 3. Configura o valor de comparação */
    SysTick->CMP = cmp_value;
    
    /* 4. Zera o contador */
    SysTick->CNT = 0;
    
    /* 5. Habilita SysTick com auto-reload e interrupção */
    /*
     * CTLR bits:
     *   Bit 0 (STE)   = 1 → Habilita contador
     *   Bit 1 (STIE)  = 1 → Habilita interrupção
     *   Bit 2 (STCLK) = 1 → Usa HCLK (não HCLK/8)
     *   Bit 3 (STRE)  = 1 → Auto-reload (periódico)
     *   Valor: 0x0F
     */
    SysTick->CTLR = 0x0F;
    
    /* 6. Habilita interrupção no NVIC */
    NVIC_EnableIRQ(SysTick_IRQn);
}

/* ======================================================================== */
/* FUNÇÕES DE LEITURA E CONTROLE                                             */
/* ======================================================================== */

/**
 * @brief  Retorna o valor atual do contador de ticks
 * @return uint32_t: Número de ticks desde o início
 */
uint32_t SysTick_Get(void)
{
    return Systick_Counter;
}

/**
 * @brief  Zera o contador de ticks
 */
void SysTick_Reset(void)
{
    Systick_Counter = 0;
}

/**
 * @brief  Verifica se passou um determinado número de ticks
 * @param  start : Valor de referência (obtido com SysTick_Get)
 * @param  ticks : Número de ticks desejado
 * @return 1 = já passou, 0 = ainda não
 * @note   Exemplo:
 *         uint32_t last = SysTick_Get();
 *         if(SysTick_Elapsed(last, 50)) { // Passaram 50 ticks? }
 */
uint8_t SysTick_Elapsed(uint32_t start, uint32_t ticks)
{
    return (Systick_Counter - start) >= ticks;
}

/**
 * @brief  Delay bloqueante usando SysTick
 * @param  ms : Tempo em milissegundos
 * @note   A resolução depende da configuração do SysTick.
 *         Se configurado para 10ms, delays < 10ms serão arredondados.
 */
void SysTick_Delay(uint32_t ms)
{
    uint32_t start = Systick_Counter;
    uint32_t ticks = ms / SYSTICK_RES_10MS;  /* Ajuste conforme resolução */
    
    /* Se quiser precisão, use a resolução configurada */
    while((Systick_Counter - start) < ticks);
}

/* ======================================================================== */
/* ISR DO SYSTICK                                                            */
/* ======================================================================== */

/**
 * @brief  Handler da interrupção do SysTick
 * @note   Incrementa o contador global a cada tick.
 *         Esta ISR é chamada automaticamente pelo hardware.
 */
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    Systick_Counter++;          /* Incrementa contador de ticks */
    SysTick->SR = 0;            /* Limpa flag de interrupção (RISC-V) */
}

/* ======================================================================== */
/* EXEMPLOS DE USO                                                           */
/* ======================================================================== */

/*
 * ============================================================
 * EXEMPLO 1: PISCAR LED COM SYSTICK DE 10MS
 * ============================================================
 * 
 * #include "CH32V003_SYSTICK_V3.h"
 * #include "CH32V003_IO_V2.h"
 * 
 * volatile uint32_t Systick_Counter = 0;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     TRISCbits(PC0, Out30PP);
 *     
 *     // SysTick a cada 10ms
 *     SysTick_Init(SYSTICK_RES_10MS);
 *     
 *     uint32_t last = 0;
 *     
 *     while(1)
 *     {
 *         // A cada 500ms (50 ticks de 10ms)
 *         if(SysTick_Elapsed(last, 50))
 *         {
 *             last = SysTick_Get();
 *             GPIO_Toggle(GPIOC, PC0);
 *         }
 *     }
 * }
 *
 *
 * ============================================================
 * EXEMPLO 2: DELAY COM SYSTICK
 * ============================================================
 * 
 * volatile uint32_t Systick_Counter = 0;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     SysTick_Init(SYSTICK_RES_1MS);   // Alta precisão
 *     
 *     TRISCbits(PC0, Out30PP);
 *     
 *     while(1)
 *     {
 *         GPIO_Set(GPIOC, PC0);
 *         SysTick_Delay(500);           // 500ms
 *         GPIO_Reset(GPIOC, PC0);
 *         SysTick_Delay(500);
 *     }
 * }
 *
 *
 * ============================================================
 * EXEMPLO 3: MÚLTIPLAS TAREFAS COM SYSTICK
 * ============================================================
 * 
 * volatile uint32_t Systick_Counter = 0;
 * uint32_t task1_timer = 0, task2_timer = 0;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     SysTick_Init(SYSTICK_RES_10MS);
 *     
 *     while(1)
 *     {
 *         // Task 1: a cada 500ms
 *         if(SysTick_Elapsed(task1_timer, 50))
 *         {
 *             task1_timer = SysTick_Get();
 *             // Ação da Task 1...
 *         }
 *         
 *         // Task 2: a cada 1 segundo
 *         if(SysTick_Elapsed(task2_timer, 100))
 *         {
 *             task2_timer = SysTick_Get();
 *             // Ação da Task 2...
 *         }
 *     }
 * }
 */

/* ======================================================================== */
/* FIM DA BIBLIOTECA SysTick V3.0                                           */
/* ======================================================================== */

#endif /* __CH32V003_SYSTICK_V3_H */