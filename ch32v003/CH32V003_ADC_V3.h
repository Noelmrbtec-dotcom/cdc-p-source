/*****************************************************************************/
/**                        ADC V3.0 (COMPACTA)                              **/
/** Data: 01/05/2026                          IDE: Mounriver Studio 2.4.0   **/
/** Autor: Marcos Roberto Braga               MOD: 01/05/2026               **/
/**                                                                         **/
/** Descrição: Biblioteca ADC compacta para CH32V003.                       **/
/**            Modos: Single, Multi-Canal, Polling e DMA Circular.          **/
/**            Tudo configurável via parâmetros.                            **/
/*****************************************************************************/

#ifndef __CH32V003_ADC_V3_H
#define __CH32V003_ADC_V3_H

#include "ch32v00x.h"

/* ======================================================================== */
/* DEFINES DE CANAIS ADC (PARA FACILITAR)                                    */
/* ======================================================================== */
#define ADC_CH0     ADC_Channel_0    /* PA0 */
#define ADC_CH1     ADC_Channel_1    /* PA1 */
#define ADC_CH2     ADC_Channel_2    /* PC4 */
#define ADC_CH3     ADC_Channel_3    /* PD2 */
#define ADC_CH4     ADC_Channel_4    /* PD3 */
#define ADC_CH5     ADC_Channel_5    /* PD4 */
#define ADC_CH6     ADC_Channel_6    /* VDDA (interno) */
#define ADC_CH7     ADC_Channel_7    /* PD6 */

/* ======================================================================== */
/* MODOS DE OPERAÇÃO                                                         */
/* ======================================================================== */
#define ADC_MODE_SINGLE      0x00    /* Canal único, leitura sob demanda     */
#define ADC_MODE_SCAN        0x01    /* Multi-canal sequencial                */
#define ADC_MODE_DMA         0x02    /* DMA Circular (multi-canal automático) */

/* ======================================================================== */
/* BUFFER DMA (ALOCADO PELO USUÁRIO)                                         */
/* ======================================================================== */
extern uint16_t ADC_DMA_Buffer[];           /* Buffer circular do DMA         */
extern volatile uint8_t ADC_DMA_Ready;      /* Flag: 1=meio, 2=cheio         */

/* ======================================================================== */
/* VARIÁVEL INTERNA (NÃO MEXER)                                              */
/* ======================================================================== */
static uint8_t  adc_channels[8];            /* Lista de canais configurados   */
static uint8_t  adc_num_channels = 0;       /* Número de canais               */
static uint8_t  adc_mode = 0;               /* Modo atual                     */

/* ======================================================================== */
/* INICIALIZAÇÃO PRINCIPAL                                                   */
/* ======================================================================== */

/**
 * @brief  Inicializa o ADC com opções de modo, canais e DMA
 * @param  mode     : ADC_MODE_SINGLE, ADC_MODE_SCAN ou ADC_MODE_DMA
 * @param  channels : Lista de canais (ex: {ADC_CH2, ADC_CH4, ADC_CH7})
 * @param  num_ch   : Número de canais na lista (1 para SINGLE)
 * @param  buf_size : Tamanho do buffer DMA (ignorado se SINGLE/SCAN)
 * @note   SINGLE:  1 canal, leitura manual (ADC_Read)
 *         SCAN:    Até 8 canais sequenciais
 *         DMA:     DMA Circular com buffer (ideal para Super Loop)
 *         
 *         ⚠️  Configure os pinos como AIN antes de chamar esta função!
 *         Ex: GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
 */
void ADC_InitEx(uint8_t mode, uint8_t *channels, uint8_t num_ch, uint16_t buf_size)
{
    ADC_InitTypeDef ADC_InitStructure = {0};
    DMA_InitTypeDef DMA_InitStructure = {0};
    
    uint8_t i;
    
    /* Guarda configuração */
    adc_mode = mode;
    adc_num_channels = num_ch;
    for(i = 0; i < num_ch; i++)
        adc_channels[i] = channels[i];
    
    /* ======== 1. CLOCKS ======== */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    if(mode == ADC_MODE_DMA)
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
    
    /* ======== 2. ADC BÁSICO ======== */
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode               = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode       = (num_ch > 1) ? ENABLE : DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = (mode == ADC_MODE_DMA) ? ENABLE : DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv   = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign          = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel       = num_ch;
    ADC_Init(ADC1, &ADC_InitStructure);
    
    /* ======== 3. CALIBRAÇÃO ======== */
    ADC_Calibration_Vol(ADC1, ADC_CALVOL_50PERCENT);
    if(mode == ADC_MODE_DMA)
        ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    
    /* ======== 4. CONFIGURA CANAIS (ORDEM INVERSA NO RSQR3) ======== */
    for(i = 0; i < num_ch; i++)
    {
        ADC_RegularChannelConfig(ADC1, channels[i], i + 1, ADC_SampleTime_241Cycles);
    }
    
    /* ======== 5. DMA (SOMENTE SE MODO DMA) ======== */
    if(mode == ADC_MODE_DMA)
    {
        DMA_DeInit(DMA1_Channel1);
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->RDATAR;
        DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)ADC_DMA_Buffer;
        DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
        DMA_InitStructure.DMA_BufferSize         = buf_size;
        DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
        DMA_InitStructure.DMA_Mode               = DMA_Mode_Circular;
        DMA_InitStructure.DMA_Priority           = DMA_Priority_VeryHigh;
        DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
        DMA_Init(DMA1_Channel1, &DMA_InitStructure);
        
        DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
        DMA_ITConfig(DMA1_Channel1, DMA_IT_HT, ENABLE);
        NVIC_EnableIRQ(DMA1_Channel1_IRQn);
        DMA_Cmd(DMA1_Channel1, ENABLE);
        
        /* Inicia conversão contínua */
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    }
}

/* ======================================================================== */
/* LEITURA (POLLING) - MODO SINGLE                                           */
/* ======================================================================== */

/**
 * @brief  Lê um canal ADC específico (bloqueante)
 * @param  channel : Canal a ser lido (ADC_CH0 a ADC_CH7)
 * @return uint16_t: Valor da conversão (10 bits: 0-1023)
 * @note   Funciona em qualquer modo. No modo DMA, pausa a conversão contínua.
 */
uint16_t ADC_Read(uint8_t channel)
{
    /* Se DMA está ativo, desabilita temporariamente */
    if(adc_mode == ADC_MODE_DMA)
    {
        ADC_SoftwareStartConvCmd(ADC1, DISABLE);
        DMA_Cmd(DMA1_Channel1, DISABLE);
    }
    
    /* Configura o canal desejado */
    ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_241Cycles);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    /* Aguarda fim da conversão */
    while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    
    uint16_t val = ADC_GetConversionValue(ADC1);
    
    /* Reativa DMA se necessário */
    if(adc_mode == ADC_MODE_DMA)
    {
        ADC_SoftwareStartConvCmd(ADC1, DISABLE);
        
        /* Reconfigura canais originais */
        for(uint8_t i = 0; i < adc_num_channels; i++)
            ADC_RegularChannelConfig(ADC1, adc_channels[i], i + 1, ADC_SampleTime_241Cycles);
        
        DMA_Cmd(DMA1_Channel1, ENABLE);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    }
    
    return val;
}

/* ======================================================================== */
/* LEITURA (DMA) - MODO CIRCULAR                                             */
/* ======================================================================== */

/**
 * @brief  Verifica se há dados disponíveis no buffer DMA
 * @return 0=aguardando, 1=meio buffer, 2=buffer cheio
 */
uint8_t ADC_DMA_Available(void)
{
    return ADC_DMA_Ready;
}

/**
 * @brief  Extrai valores de um canal específico do buffer DMA
 * @param  channel_idx : Índice do canal (0 = primeiro configurado)
 * @param  buf         : Buffer de destino para os valores
 * @param  count       : Número de amostras a extrair (máx = buf_size/num_ch)
 * @note   Exemplo: ADC_DMA_GetChannel(0, buf, 10) -> 10 amostras do 1º canal
 */
void ADC_DMA_GetChannel(uint8_t channel_idx, uint16_t *buf, uint8_t count)
{
    if(channel_idx >= adc_num_channels) return;
    
    for(uint8_t i = 0; i < count; i++)
    {
        buf[i] = ADC_DMA_Buffer[(i * adc_num_channels) + channel_idx];
    }
}

/**
 * @brief  Converte valor ADC para tensão em milivolts
 * @param  adc_val : Valor ADC (0-1023)
 * @return uint16_t: Tensão em mV (0-3300)
 */
uint16_t ADC_ToMilliVolts(uint16_t adc_val)
{
    return (uint16_t)(((uint32_t)adc_val * 3300) / 1024);
}

/* ======================================================================== */
/* ISR DO DMA CANAL 1 (ADC)                                                  */
/* ======================================================================== */

/**
 * @brief  ISR do DMA Canal 1 (ADC Complete / Half Complete)
 * @note   Seta ADC_DMA_Ready quando metade ou todo o buffer é preenchido.
 */
void DMA1_Channel1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel1_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC1))
    {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        ADC_DMA_Ready = 2;   /* Buffer cheio */
    }
    if(DMA_GetITStatus(DMA1_IT_HT1))
    {
        DMA_ClearITPendingBit(DMA1_IT_HT1);
        ADC_DMA_Ready = 1;   /* Meio buffer */
    }
}

/* ======================================================================== */
/* EXEMPLOS DE USO                                                           */
/* ======================================================================== */

/*
 * ============================================================
 * EXEMPLO 1: MODO SINGLE (CANAL ÚNICO - POLLING)
 * ============================================================
 * 
 * #include "ch32v00x.h"
 * #include "CH32V003_IO_V2.h"
 * #include "CH32V003_ADC_V3.h"
 * 
 * // Buffer DMA (obrigatório declarar, mesmo sem usar)
 * uint16_t ADC_DMA_Buffer[16] = {0};
 * volatile uint8_t ADC_DMA_Ready = 0;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     
 *     // Configura PC4 como entrada analógica
 *     TRISCbits(PC4, InAn);
 *     
 *     // Inicializa ADC: modo SINGLE, canal 2 (PC4)
 *     uint8_t canais[] = {ADC_CH2};
 *     ADC_InitEx(ADC_MODE_SINGLE, canais, 1, 0);
 *     
 *     while(1)
 *     {
 *         uint16_t val = ADC_Read(ADC_CH2);
 *         uint16_t mv  = ADC_ToMilliVolts(val);
 *         
 *         // Usa printf da USART (se configurada)
 *         // USART_Printf("ADC: %d (%d mV)\r\n", val, mv);
 *         
 *         Delay_Ms(100);
 *     }
 * }
 *
 *
 * ============================================================
 * EXEMPLO 2: MODO DMA (3 CANAIS - SUPER LOOP)
 * ============================================================
 * 
 * #include "ch32v00x.h"
 * #include "CH32V003_IO_V2.h"
 * #include "CH32V003_ADC_V3.h"
 * #include "CH32V003_USART_V3.h"
 * 
 * #define ADC_BUF_SIZE  30   // 3 canais × 10 amostras
 * 
 * // Buffers
 * uint16_t ADC_DMA_Buffer[ADC_BUF_SIZE] = {0};
 * volatile uint8_t ADC_DMA_Ready = 0;
 * 
 * // Buffers USART
 * uint8_t  USART_TxBuffer[64] = {0};
 * uint8_t  USART_RxBuffer[64] = {0};
 * volatile uint8_t USART_TxBusy = 0;
 * volatile uint8_t USART_RxReady = 0;
 * uint8_t  USART_RxSize = 64;
 * 
 * // Variáveis do Super Loop
 * volatile uint32_t tick = 0;
 * uint32_t timer_adc = 0;
 * 
 * void SysTick_Init(void)
 * {
 *     SysTick->CTLR = 0;
 *     SysTick->CMP  = (SystemCoreClock / 100) - 1;
 *     SysTick->CNT  = 0;
 *     SysTick->CTLR = 0x0F;
 *     NVIC_EnableIRQ(SysTick_IRQn);
 * }
 * 
 * void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
 * void SysTick_Handler(void)
 * {
 *     tick++;
 *     SysTick->SR = 0;
 * }
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     SysTick_Init();
 *     
 *     // Configura pinos analógicos
 *     TRISCbits(PC4, InAn);   // ADC_CH2
 *     TRISDbits(PD2, InAn);   // ADC_CH3
 *     TRISDbits(PD3, InAn);   // ADC_CH4
 *     
 *     // Inicializa USART
 *     USART_InitEx(115200, USART_MODE_DMA_FULL, USART_REMAP_ALT, 64);
 *     USART_Println("=== ADC DMA Multicanal ===");
 *     
 *     // Inicializa ADC: modo DMA, 3 canais, buffer 30
 *     uint8_t canais[] = {ADC_CH2, ADC_CH3, ADC_CH4};
 *     ADC_InitEx(ADC_MODE_DMA, canais, 3, ADC_BUF_SIZE);
 *     
 *     uint16_t buf_ch2[10], buf_ch3[10], buf_ch4[10];
 *     
 *     while(1)
 *     {
 *         // Task ADC: a cada 500ms
 *         if((tick - timer_adc) >= 50)
 *         {
 *             timer_adc = tick;
 *             
 *             if(ADC_DMA_Available())
 *             {
 *                 // Extrai 10 amostras de cada canal
 *                 ADC_DMA_GetChannel(0, buf_ch2, 10);
 *                 ADC_DMA_GetChannel(1, buf_ch3, 10);
 *                 ADC_DMA_GetChannel(2, buf_ch4, 10);
 *                 
 *                 // Calcula médias
 *                 uint32_t soma2 = 0, soma3 = 0, soma4 = 0;
 *                 for(uint8_t i = 0; i < 10; i++)
 *                 {
 *                     soma2 += buf_ch2[i];
 *                     soma3 += buf_ch3[i];
 *                     soma4 += buf_ch4[i];
 *                 }
 *                 
 *                 uint16_t med2 = soma2 / 10;
 *                 uint16_t med3 = soma3 / 10;
 *                 uint16_t med4 = soma4 / 10;
 *                 
 *                 USART_Printf("CH2:%4d CH3:%4d CH4:%4d | CH2:%4dmV CH3:%4dmV CH4:%4dmV\r\n",
 *                              med2, med3, med4,
 *                              ADC_ToMilliVolts(med2),
 *                              ADC_ToMilliVolts(med3),
 *                              ADC_ToMilliVolts(med4));
 *                 
 *                 ADC_DMA_Ready = 0;
 *             }
 *         }
 *     }
 * }
 *
 *
 * ============================================================
 * EXEMPLO 3: MODO SCAN (SEM DMA - LEITURA SEQUENCIAL)
 * ============================================================
 * 
 * #include "ch32v00x.h"
 * #include "CH32V003_IO_V2.h"
 * #include "CH32V003_ADC_V3.h"
 * 
 * uint16_t ADC_DMA_Buffer[16] = {0};
 * volatile uint8_t ADC_DMA_Ready = 0;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     
 *     TRISCbits(PC4, InAn);   // ADC_CH2
 *     TRISDbits(PD2, InAn);   // ADC_CH3
 *     TRISDbits(PD3, InAn);   // ADC_CH4
 *     
 *     uint8_t canais[] = {ADC_CH2, ADC_CH3, ADC_CH4};
 *     ADC_InitEx(ADC_MODE_SCAN, canais, 3, 0);
 *     
 *     while(1)
 *     {
 *         // Lê cada canal individualmente
 *         uint16_t ch2 = ADC_Read(ADC_CH2);
 *         uint16_t ch3 = ADC_Read(ADC_CH3);
 *         uint16_t ch4 = ADC_Read(ADC_CH4);
 *         
 *         // Processa...
 *         
 *         Delay_Ms(100);
 *     }
 * }
 */

/* ======================================================================== */
/* FIM DA BIBLIOTECA ADC V3.0                                               */
/* ======================================================================== */

#endif /* __CH32V003_ADC_V3_H */