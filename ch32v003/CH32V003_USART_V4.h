/*****************************************************************************/
/**                       USART V4.0 (COMPACTA)                             **/
/** Data: 01/05/2026                          IDE: Mounriver Studio 2.4.0   **/
/** Autor: Marcos Roberto Braga               MOD: 31/07/2026               **/
/**                                                                         **/
/** Descrição: Biblioteca USART compacta para CH32V003.                     **/
/**            Modos: Polling e DMA Full-Duplex.                            **/
/**            Buffer de recepção AJUSTÁVEL (1 byte até N bytes).           **/
/**            Usa interrupção IDLE para detectar fim de pacote.            **/
/**                                                                         **/
/** NOVO NA V4.0:                                                           **/
/** - Interrupção IDLE para detecção de fim de pacote                       **/
/** - Buffer de recepção ajustável (1 a N bytes)                            **/
/** - Compatível com CDC-P v3.0 (comandos de tamanho variável)              **/
/*****************************************************************************/

#ifndef __CH32V003_USART_V4_H
#define __CH32V003_USART_V4_H

#include "ch32v00x.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* ======================================================================== */
/* OPÇÕES DE CONFIGURAÇÃO (PASSE NA INICIALIZAÇÃO)                          */
/* ======================================================================== */
#define USART_MODE_POLLING   0x00    /* Polling (TX/RX bloqueante)            */
#define USART_MODE_DMA_FULL  0x03    /* DMA no TX e RX com IDLE              */

#define USART_REMAP_NONE     0x00    /* Pinagem padrão (PD5=TX, PD6=RX)      */
#define USART_REMAP_ALT      0x01    /* Pinagem alternativa (PD6=TX, PD5=RX) */

/* ======================================================================== */
/* BUFFERS DMA (DEVEM SER ALOCADOS PELO USUÁRIO NO main.c)                  */
/* ======================================================================== */
extern uint8_t  USART_TxBuffer[];       /* Buffer de transmissão DMA          */
extern uint8_t  USART_RxBuffer[];       /* Buffer de recepção DMA             */
extern volatile uint8_t USART_TxBusy;   /* Flag: 1 = DMA TX em andamento     */
extern volatile uint8_t USART_RxReady;  /* Flag: 1 = pacote completo no RX   */
extern uint8_t  USART_RxSize;          /* Tamanho do buffer RX               */

/* ======================================================================== */
/* ISR DO DMA CANAL 4 (TX COMPLETE)                                          */
/* ======================================================================== */

/**
 * @brief  ISR do DMA Canal 4 (TX Complete)
 * @note   Chamada automaticamente quando a transmissão DMA termina.
 *         Limpa a flag e libera tx_busy.
 */
void DMA1_Channel4_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel4_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC4))
    {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        DMA_Cmd(DMA1_Channel4, DISABLE);
        USART_TxBusy = 0;
    }
}

/* ======================================================================== */
/* ISR DA USART1 (INTERRUPÇÃO IDLE)                                          */
/* ======================================================================== */

/**
 * @brief  ISR da USART1 para detecção de IDLE (fim de pacote)
 * @note   Chamada quando o pino RX fica em nível HIGH por 1 frame time.
 *         Isso indica que o transmissor terminou de enviar o pacote.
 *         
 *         FUNCIONAMENTO:
 *         1. IDLE detectado → pacote completo recebido
 *         2. Seta USART_RxReady = 1 (pacote disponível)
 *         3. Desabilita DMA RX temporariamente
 *         
 *         A função USART_DMA_Read calcula o tamanho real do pacote
 *         e rearma o DMA para o próximo pacote.
 */
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        /* Limpa flag IDLE (leitura STATR + DATAR) */
        volatile uint8_t dummy;
        dummy = USART1->STATR;
        dummy = USART1->DATAR;
        (void)dummy;
        
        /* IDLE detectado = fim do pacote */
        USART_RxReady = 1;
        
        /* Desabilita DMA RX para processamento */
        DMA_Cmd(DMA1_Channel5, DISABLE);
    }
}

/* ======================================================================== */
/* INICIALIZAÇÃO PRINCIPAL                                                   */
/* ======================================================================== */

/**
 * @brief  Inicializa a USART1 com opções de modo e remapeamento
 * @param  baudrate : Baud rate (9600, 115200, etc.)
 * @param  mode     : USART_MODE_POLLING ou USART_MODE_DMA_FULL
 * @param  remap    : USART_REMAP_NONE ou USART_REMAP_ALT
 * @param  buf_size : Tamanho dos buffers DMA (ignorado se POLLING)
 * 
 * @note   Se mode = USART_MODE_DMA_FULL:
 *         - Habilita DMA TX (Canal 4) e DMA RX (Canal 5)
 *         - Habilita interrupção IDLE para detecção de pacotes
 */
void USART_InitEx(uint32_t baudrate, uint8_t mode, uint8_t remap, uint8_t buf_size)
{
    GPIO_InitTypeDef  GPIO_InitStructure  = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    DMA_InitTypeDef   DMA_InitStructure   = {0};

    /* ======== 1. CLOCKS ======== */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);
    if(mode == USART_MODE_DMA_FULL)
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    /* ======== 2. PINOS (REMAP OU PADRÃO) ======== */
    if(remap == USART_REMAP_ALT)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        AFIO->PCFR1 |= (1 << 21);

        /* TX = PD6 */
        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        /* RX = PD5 */
        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
    }
    else
    {
        /* TX = PD5 */
        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
        GPIO_Init(GPIOD, &GPIO_InitStructure);

        /* RX = PD6 */
        GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6;
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
        GPIO_Init(GPIOD, &GPIO_InitStructure);
    }

    /* ======== 3. USART ======== */
    USART_InitStructure.USART_BaudRate            = baudrate;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &USART_InitStructure);

    /* ======== 4. DMA (SOMENTE SE MODO DMA) ======== */
    if(mode == USART_MODE_DMA_FULL)
    {
        USART_RxSize = buf_size;

        /* ---- DMA TX (Canal 4, Normal) ---- */
        DMA_DeInit(DMA1_Channel4);
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DATAR;
        DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)USART_TxBuffer;
        DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralDST;
        DMA_InitStructure.DMA_BufferSize         = buf_size;
        DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
        DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
        DMA_Init(DMA1_Channel4, &DMA_InitStructure);

        DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
        NVIC_EnableIRQ(DMA1_Channel4_IRQn);
        USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);

        /* ---- DMA RX (Canal 5, Normal) ---- */
        DMA_DeInit(DMA1_Channel5);
        DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DATAR;
        DMA_InitStructure.DMA_MemoryBaseAddr     = (uint32_t)USART_RxBuffer;
        DMA_InitStructure.DMA_DIR                = DMA_DIR_PeripheralSRC;
        DMA_InitStructure.DMA_BufferSize         = buf_size;
        DMA_InitStructure.DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
        DMA_InitStructure.DMA_MemoryInc          = DMA_MemoryInc_Enable;
        DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMA_InitStructure.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
        DMA_InitStructure.DMA_Mode               = DMA_Mode_Normal;
        DMA_InitStructure.DMA_Priority           = DMA_Priority_High;
        DMA_InitStructure.DMA_M2M                = DMA_M2M_Disable;
        DMA_Init(DMA1_Channel5, &DMA_InitStructure);

        USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
        DMA_Cmd(DMA1_Channel5, ENABLE);
        
        /* ---- HABILITA INTERRUPÇÃO IDLE ---- */
        USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
        NVIC_EnableIRQ(USART1_IRQn);
    }

    /* ======== 5. HABILITA USART ======== */
    USART_Cmd(USART1, ENABLE);
}

/* ======================================================================== */
/* TRANSMISSÃO (POLLING)                                                     */
/* ======================================================================== */

void USART_SendByte(uint8_t data)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, data);
}

void USART_SendString(char *str)
{
    while(*str) USART_SendByte(*str++);
}

void USART_Printf(char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    USART_SendString(buf);
}

/* ======================================================================== */
/* TRANSMISSÃO (DMA)                                                         */
/* ======================================================================== */

/**
 * @brief  Envia dados via DMA (não bloqueante)
 * @param  data : Ponteiro para os dados (ou NULL para usar USART_TxBuffer)
 * @param  len  : Número de bytes a enviar
 * @note   Aguarda tx_busy = 0 antes de iniciar.
 */
void USART_DMA_Send(uint8_t *data, uint8_t len)
{
    while(USART_TxBusy);

    if(data != NULL && data != USART_TxBuffer)
    {
        for(uint8_t i = 0; i < len; i++)
            USART_TxBuffer[i] = data[i];
    }

    USART_TxBusy = 1;
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel4, len);
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

/* ======================================================================== */
/* RECEPÇÃO (POLLING)                                                        */
/* ======================================================================== */

uint8_t USART_Available(void)
{
    return (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET);
}

uint8_t USART_RecvByte(void)
{
    while(!USART_Available());
    return USART_ReceiveData(USART1);
}

/* ======================================================================== */
/* RECEPÇÃO (DMA) - BUFFER AJUSTÁVEL (1 a N bytes)                          */
/* ======================================================================== */

/**
 * @brief  Lê pacote recebido via DMA (TAMANHO VARIÁVEL: 1 a N bytes)
 * @param  buf  : Buffer de destino
 * @param  size : Tamanho máximo do buffer
 * @return Número de bytes no pacote (0 = nenhum pacote)
 * @note   Usa IDLE para detectar fim do pacote.
 *         Funciona com pacotes de 1 byte até size bytes.
 */
uint8_t USART_DMA_Read(uint8_t *buf, uint8_t size)
{
    uint8_t len = 0;
    
    if(USART_RxReady == 0)
        return 0;
    
    uint16_t remaining = DMA_GetCurrDataCounter(DMA1_Channel5);
    len = USART_RxSize - remaining;
    
    if(len > size) len = size;
    
    for(uint8_t i = 0; i < len; i++)
        buf[i] = USART_RxBuffer[i];
    
    USART_RxReady = 0;
    
    DMA_Cmd(DMA1_Channel5, DISABLE);
    DMA1_Channel5->MADDR = (uint32_t)USART_RxBuffer;
    DMA_SetCurrDataCounter(DMA1_Channel5, USART_RxSize);
    DMA_Cmd(DMA1_Channel5, ENABLE);
    
    return len;
}

/* ======================================================================== */
/* MACROS DE CONVENIÊNCIA                                                    */
/* ======================================================================== */

#define USART_Print(msg)    USART_SendString(msg)
#define USART_Println(msg)  USART_SendString(msg "\r\n")

/* ======================================================================== */
/* EXEMPLOS DE USO                                                           */
/* ======================================================================== */

/*
 * ============================================================
 * EXEMPLO 1: MODO POLLING (ECO BYTE A BYTE)
 * ============================================================
 * 
 * #include "ch32v00x.h"
 * #include "CH32V003_USART_V4.h"
 * 
 * uint8_t  USART_TxBuffer[64] = {0};
 * uint8_t  USART_RxBuffer[64] = {0};
 * volatile uint8_t USART_TxBusy = 0;
 * volatile uint8_t USART_RxReady = 0;
 * uint8_t  USART_RxSize = 64;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     USART_InitEx(115200, USART_MODE_POLLING, USART_REMAP_NONE, 0);
 *     
 *     USART_Println("Modo Polling ativo!");
 *     
 *     while(1)
 *     {
 *         if(USART_Available())
 *             USART_SendByte(USART_RecvByte());
 *     }
 * }
 *
 *
 * ============================================================
 * EXEMPLO 2: MODO DMA COM BUFFER AJUSTÁVEL (1 a N bytes)
 * ============================================================
 * 
 * #include "ch32v00x.h"
 * #include "CH32V003_USART_V4.h"
 * #include <string.h>
 * 
 * uint8_t  USART_TxBuffer[64] = {0};
 * uint8_t  USART_RxBuffer[64] = {0};
 * volatile uint8_t USART_TxBusy = 0;
 * volatile uint8_t USART_RxReady = 0;
 * uint8_t  USART_RxSize = 64;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     USART_InitEx(115200, USART_MODE_DMA_FULL, USART_REMAP_NONE, 64);
 *     
 *     USART_Println("Modo DMA com IDLE ativo!");
 *     USART_Println("Envie pacotes de 1 a 64 bytes...");
 *     
 *     uint8_t rx_buf[64];
 *     
 *     while(1)
 *     {
 *         uint8_t len = USART_DMA_Read(rx_buf, sizeof(rx_buf));
 *         
 *         if(len > 0)
 *         {
 *             USART_Printf("Pacote de %d bytes: ", len);
 *             
 *             for(uint8_t i = 0; i < len; i++)
 *                 USART_SendByte(rx_buf[i]);
 *             
 *             USART_Println("");
 *         }
 *     }
 * }
 *
 *
 * ============================================================
 * EXEMPLO 3: COMANDOS DE TAMANHO VARIÁVEL
 * ============================================================
 * 
 * #include "ch32v00x.h"
 * #include "CH32V003_USART_V4.h"
 * #include "CH32V003_IO_V2.h"
 * #include <string.h>
 * 
 * uint8_t  USART_TxBuffer[64] = {0};
 * uint8_t  USART_RxBuffer[64] = {0};
 * volatile uint8_t USART_TxBusy = 0;
 * volatile uint8_t USART_RxReady = 0;
 * uint8_t  USART_RxSize = 64;
 * 
 * int main(void)
 * {
 *     SystemInit();
 *     
 *     USART_InitEx(115200, USART_MODE_DMA_FULL, USART_REMAP_NONE, 64);
 *     TRISCbits(PC0, Out30PP);
 *     
 *     USART_Println("Comandos: LED ON, LED OFF, p, s");
 *     
 *     uint8_t rx_buf[64];
 *     
 *     while(1)
 *     {
 *         uint8_t len = USART_DMA_Read(rx_buf, sizeof(rx_buf));
 *         
 *         if(len > 0)
 *         {
 *             // Comando de 1 byte
 *             if(len == 1 && rx_buf[0] == 'p')
 *             {
 *                 USART_Println("Comando 'p' recebido!");
 *             }
 *             
 *             // Comando de múltiplos bytes
 *             if(len == 7 && strncmp((char*)rx_buf, "LED ON", 6) == 0)
 *             {
 *                 GPIO_Set(GPIOC, PC0);
 *                 USART_Println("LED LIGADO!");
 *             }
 *             
 *             if(len == 8 && strncmp((char*)rx_buf, "LED OFF", 7) == 0)
 *             {
 *                 GPIO_Reset(GPIOC, PC0);
 *                 USART_Println("LED DESLIGADO!");
 *             }
 *         }
 *     }
 * }
 */

/* ======================================================================== */
/* FIM DA BIBLIOTECA USART V4.0                                             */
/* ======================================================================== */

#endif /* __CH32V003_USART_V4_H */