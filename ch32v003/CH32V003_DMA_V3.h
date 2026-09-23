/*****************************************************************************/
/**                        DMA V3.0 (COMPACTA)                              **/
/** Data: 01/05/2026                          IDE: Mounriver Studio 2.4.0   **/
/** Autor: Marcos Roberto Braga               MOD: 01/05/2026               **/
/**                                                                         **/
/** Descrição: Biblioteca DMA compacta para CH32V003.                       **/
/**            Canais: 1(ADC), 4(USART_TX), 5(USART_RX), 6(I2C/SPI_TX),    **/
/**                    7(I2C/SPI_RX).                                       **/
/**            Modos: Normal e Circular.                                    **/
/**                                                                         **/
/** ⚠️  NOTA: As ISRs dos canais 4 e 5 já estão na USART V3.0.             **/
/**     As ISRs dos canais 6 e 7 já estão na I2C/SPI V3.0.                 **/
/**     Esta biblioteca contém apenas as ISRs dos canais 1, 2 e 3.         **/
/*****************************************************************************/

#ifndef __CH32V003_DMA_V3_H
#define __CH32V003_DMA_V3_H

#include "ch32v00x.h"

/* ======================================================================== */
/* DEFINES DOS CANAIS DMA                                                   */
/* ======================================================================== */
#define DMA_CH1         0       /* ADC1                                     */
#define DMA_CH2         1       /* Reservado                                 */
#define DMA_CH3         2       /* Reservado                                 */
#define DMA_CH4         3       /* USART1 TX                                */
#define DMA_CH5         4       /* USART1 RX                                */
#define DMA_CH6         5       /* I2C1 TX / SPI1 TX                        */
#define DMA_CH7         6       /* I2C1 RX / SPI1 RX                        */

/* ======================================================================== */
/* MODOS DE OPERAÇÃO                                                        */
/* ======================================================================== */
#define DMA_MODE_NORMAL     0x00    /* Para após transferir                  */
#define DMA_MODE_CIRCULAR   0x01    /* Reinicia automaticamente              */

/* ======================================================================== */
/* DIREÇÃO DA TRANSFERÊNCIA                                                 */
/* ======================================================================== */
#define DMA_DIR_PERIPH_TO_MEM   0x00    /* Periférico → Memória (RX)         */
#define DMA_DIR_MEM_TO_PERIPH   0x01    /* Memória → Periférico (TX)         */

/* ======================================================================== */
/* TAMANHO DOS DADOS                                                        */
/* ======================================================================== */
#define DMA_SIZE_8BIT       0x00    /* 8 bits (byte)                         */
#define DMA_SIZE_16BIT      0x01    /* 16 bits (half-word)                   */
#define DMA_SIZE_32BIT      0x02    /* 32 bits (word)                        */

/* ======================================================================== */
/* PRIORIDADE                                                               */
/* ======================================================================== */
#define DMA_PRIORITY_LOW        0x00
#define DMA_PRIORITY_MEDIUM     0x01
#define DMA_PRIORITY_HIGH       0x02
#define DMA_PRIORITY_VERYHIGH   0x03

/* ======================================================================== */
/* FLAGS DO USUÁRIO                                                         */
/* ======================================================================== */
extern volatile uint8_t DMA_Flags[7];    /* Flags: [ch]=1 = transferência OK */

/* ======================================================================== */
/* MACROS ÚTEIS                                                             */
/* ======================================================================== */
#define DMA_GET_CHANNEL(ch)  ((ch == 0) ? DMA1_Channel1 : \
                              (ch == 1) ? DMA1_Channel2 : \
                              (ch == 2) ? DMA1_Channel3 : \
                              (ch == 3) ? DMA1_Channel4 : \
                              (ch == 4) ? DMA1_Channel5 : \
                              (ch == 5) ? DMA1_Channel6 : \
                                          DMA1_Channel7)

/* ======================================================================== */
/* INICIALIZAÇÃO PRINCIPAL                                                   */
/* ======================================================================== */

void DMA_InitEx(uint8_t channel, volatile void *periph_addr, 
                volatile void *mem_addr, uint16_t buf_size,
                uint8_t direction, uint8_t data_size,
                uint8_t mode, uint8_t priority,
                uint8_t tc_irq, uint8_t ht_irq)
{
    DMA_InitTypeDef DMA_InitStructure = {0};
    DMA_Channel_TypeDef *DMA_CHx;
    
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    
    DMA_CHx = DMA_GET_CHANNEL(channel);
    DMA_DeInit(DMA_CHx);
    
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)periph_addr;
    DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)mem_addr;
    DMA_InitStructure.DMA_DIR                = (direction == DMA_DIR_MEM_TO_PERIPH) ? 
                                               DMA_DIR_PeripheralDST : DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize         = buf_size;
    DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_Mode               = (mode == DMA_MODE_CIRCULAR) ? 
                                               DMA_Mode_Circular : DMA_Mode_Normal;
    DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
    
    if(data_size == DMA_SIZE_16BIT) {
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
    } else if(data_size == DMA_SIZE_32BIT) {
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Word;
    } else {
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    }
    
    if(priority == DMA_PRIORITY_VERYHIGH)
        DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    else if(priority == DMA_PRIORITY_HIGH)
        DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    else if(priority == DMA_PRIORITY_MEDIUM)
        DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
    else
        DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
    
    DMA_Init(DMA_CHx, &DMA_InitStructure);
    
    if(tc_irq) {
        DMA_ITConfig(DMA_CHx, DMA_IT_TC, ENABLE);
        switch(channel) {
            case DMA_CH1: NVIC_EnableIRQ(DMA1_Channel1_IRQn); break;
            case DMA_CH4: NVIC_EnableIRQ(DMA1_Channel4_IRQn); break;
            case DMA_CH5: NVIC_EnableIRQ(DMA1_Channel5_IRQn); break;
            case DMA_CH6: NVIC_EnableIRQ(DMA1_Channel6_IRQn); break;
            case DMA_CH7: NVIC_EnableIRQ(DMA1_Channel7_IRQn); break;
        }
    }
    
    if(ht_irq)
        DMA_ITConfig(DMA_CHx, DMA_IT_HT, ENABLE);
}

/* ======================================================================== */
/* CONTROLE DO CANAL                                                        */
/* ======================================================================== */

void DMA_Start(uint8_t channel)  { DMA_Cmd(DMA_GET_CHANNEL(channel), ENABLE); }
void DMA_Stop(uint8_t channel)   { DMA_Cmd(DMA_GET_CHANNEL(channel), DISABLE); }

uint16_t DMA_GetRemaining(uint8_t channel) { 
    return DMA_GetCurrDataCounter(DMA_GET_CHANNEL(channel)); 
}

/* ======================================================================== */
/* ISRs (APENAS CANAIS 1, 2, 3 - OS OUTROS JÁ ESTÃO NAS OUTRAS BIBLIOTECAS) */
/* ======================================================================== */
/*
void DMA1_Channel1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel1_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC1)) { DMA_ClearITPendingBit(DMA1_IT_TC1); DMA_Flags[0] = 1; }
    if(DMA_GetITStatus(DMA1_IT_HT1)) { DMA_ClearITPendingBit(DMA1_IT_HT1); }
}
*/
/* ======================================================================== */
/* EXEMPLOS DE USO                                                           */
/* ======================================================================== */

/*
 * ============================================================
 * EXEMPLO 1: DMA ADC (CANAL 1, CIRCULAR)
 * ============================================================
 * 
 * volatile uint8_t DMA_Flags[7] = {0};
 * uint16_t adc_buf[10] = {0};
 * 
 * DMA_InitEx(DMA_CH1, &ADC1->RDATAR, adc_buf, 10,
 *            DMA_DIR_PERIPH_TO_MEM, DMA_SIZE_16BIT,
 *            DMA_MODE_CIRCULAR, DMA_PRIORITY_VERYHIGH, 1, 1);
 * DMA_Start(DMA_CH1);
 *
 * ============================================================
 * EXEMPLO 2: DMA USART TX (CANAL 4, NORMAL)
 * ============================================================
 * 
 * uint8_t msg[] = "Hello DMA!\r\n";
 * 
 * DMA_InitEx(DMA_CH4, &USART1->DATAR, msg, 12,
 *            DMA_DIR_MEM_TO_PERIPH, DMA_SIZE_8BIT,
 *            DMA_MODE_NORMAL, DMA_PRIORITY_HIGH, 1, 0);
 * USART1->CTLR3 |= (1 << 7);  // DMAT
 * DMA_Start(DMA_CH4);
 */

#endif /* __CH32V003_DMA_V3_H */