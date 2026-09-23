/* =================================================================================================================== */
/*
 * @file main.c
 * @author Marcos Roberto Braga / Pedro Henrique Cerqueira Braga
 * @brief  CDC-P v3.0 - Concurrency Deterministic Control with Preemption and Situational Urgency
 * @version 3.0 (MODULAR)
 * @date 2024-03-22 (Atualizado em 2026-07-31)
 * 
 * @details RTOS compacto, eficiente e AUTOCONSCIENTE com reflexos situacionais.
 *          MCU: CH32V003F4P6 (RISC-V RV32EC) operando em 48MHz (HSE)
 *          
 *          ═══════════════════════════════════════════════════════════════
 *          BIBLIOTECAS UTILIZADAS
 *          ═══════════════════════════════════════════════════════════════
 *          - CH32V003_IO_V2.h       (GPIO - configuração e manipulação)
 *          - CH32V003_USART_V4.h    (USART + DMA + IDLE - buffer ajustável)
 *          - CH32V003_SYSTICK_V3.h  (SysTick - base de tempo do kernel)
 *          - CH32V003_ADC_V3.h      (ADC + DMA - conversão analógica)
 *          - CH32V003_DMA_V3.h      (DMA genérico - transferências)
 *          
 *          ═══════════════════════════════════════════════════════════════
 *          CARACTERÍSTICAS DO CDC-P
 *          ═══════════════════════════════════════════════════════════════
 *          
 *          PRIORIDADE TRIDIMENSIONAL:
 *            1. Espacial: posição na fila do despachador
 *            2. Temporal: período configurado da tarefa (taskX)
 *            3. Situacional: flag de urgência por evento (URG-S)
 *          
 *          PREEMPÇÃO REAL:
 *            - Via URG-S (fura-fila imediato na mesma iteração)
 *            - Via CDC-P (aceleração por período reduzido a 1 tick)
 *            - SEM troca de contexto (tarefas atômicas)
 *            - SEM stack individual (compartilhada)
 *          
 *          7 CAMADAS DE PROTEÇÃO:
 *            1. Auto-regulagem temporal (atrasou → desacelera)
 *            2. Bloqueio por violação de deadline
 *            3. Diagnóstico de pane (Xerife - Task10)
 *            4. Recuperação progressiva (Síndico - Task9)
 *            5. Fail-safe final (reset se Síndico falhar)
 *            6. Aceleração automática (CDC-P)
 *            7. URG-S (resposta imediata a eventos)
 *          
 *          ISOLAMENTO DE FALHAS:
 *            - Tarefas com excesso de falhas são ISOLADAS permanentemente
 *            - Só reset do sistema libera tarefa em pane
 *          
 *          ═══════════════════════════════════════════════════════════════
 *          TAREFAS DO SISTEMA
 *          ═══════════════════════════════════════════════════════════════
 *          
 *          Task1:  LED1 (PC0) - 20ms (2 ticks)
 *          Task2:  LED2 (PC1) - 40ms (4 ticks)
 *          Task3:  Comandos seriais - 10ms (1 tick)
 *          Task4:  Status 60s - 60 segundos
 *          Task5:  Botão (PC2) - 30ms (3 ticks)
 *          Task6:  ADC (PC4) - 500ms (50 ticks)
 *          Task9:  Síndico (recuperação) - 10ms (1 tick)
 *          Task10: Xerife (diagnóstico) - 10ms (1 tick)
 *          
 *          ═══════════════════════════════════════════════════════════════
 *          SISTEMA DE DEBUG
 *          ═══════════════════════════════════════════════════════════════
 *          
 *          Comando 's': Status resumido do sistema
 *          Comando 'd': Diagnóstico completo e detalhado
 *          Comando 'p': Ativa preempção na Task1
 *          Comando 'n': Desativa preempção
 *          Comando 'e': Dispara preempção manualmente
 *          Comando 'u': Seta urgência na Task1
 *          Comando '1': Tick de 1ms
 *          Comando '2': Tick de 10ms (padrão)
 *          Comando '3': Tick de 100ms
 *          
 *          ═══════════════════════════════════════════════════════════════
 *          FILOSOFIA
 *          ═══════════════════════════════════════════════════════════════
 *          
 *          "A bagunça tolerada por abundância de recursos." — A crítica.
 *          
 *          O CDC-P é determinístico, preemptivo REAL e imune a:
 *          - Condições de corrida
 *          - Deadlocks
 *          - Inversão de prioridade
 *          - Falhas de sincronismo
 *          
 * @copyright Copyright (c) 2026
 */
/* =================================================================================================================== */
/* INCLUDES                                  */
/* =================================================================================================================== */
#include "debug.h"                              /* Debug com printf (formatação de strings) */
#include <stdint.h>                             /* Tipos padrão (uint8_t, uint16_t, uint32_t) */
#include <stdbool.h>                            /* Tipos booleanos (true/false) */
#include <string.h>                             /* Funções de string (memset, strcmp, strncmp) */
#include "CH32V003_IO_V2.h"                     /* Biblioteca de GPIO (TRIS, Set, Reset, Toggle) */
#include "CH32V003_USART_V4.h"                  /* Biblioteca de USART + DMA + IDLE (buffer ajustável) */
#include "CH32V003_SYSTICK_V3.h"                /* Biblioteca de SysTick (base de tempo) */
#include "CH32V003_ADC_V3.h"                    /* Biblioteca de ADC + DMA (leitura analógica) */
#include "CH32V003_DMA_V3.h"                    /* Biblioteca de DMA genérico */

/* =================================================================================================================== */
/* DEFINES DO SISTEMA                         */
/* =================================================================================================================== */
/* ============================================ BUFFERS ============================================================= */
#define USART_BUFFER_SIZE   64                  /* Tamanho do buffer USART (1 a 64 bytes por pacote) */
#define ADC_BUFFER_SIZE     10                  /* Buffer ADC: 10 amostras (1 canal) */

/* ============================================ LIMITES DE PANE ==================================================== */
#define sistema_em_pane 5                       /* Limite de reincidências (tarefas base tick = 5 falhas) */
#define sistema_em_pane_seg 1                   /* Limite de reincidências (tarefas base segundo = 1 falha) */

/* ============================================ DEADLINES ========================================================== */
#define tempo_maximo_task1 3                    /* Task1 bloqueia se período > 3 ticks (30ms) */
#define tempo_maximo_task2 5                    /* Task2 bloqueia se período > 5 ticks (50ms) */
#define tempo_maximo_task3 20                   /* Task3 bloqueia se período > 20 ticks (200ms) */
#define tempo_maximo_task4 61                   /* Task4 bloqueia se período > 61 segundos */
#define tempo_maximo_task5 4                    /* Task5 bloqueia se período > 4 ticks (40ms) */
#define tempo_maximo_task6 60                   /* Task6 bloqueia se período > 60 ticks (600ms) */
#define tempo_maximo_task9 2                    /* Task9 reset se período > 2 ticks (20ms) */

/* =================================================================================================================== */
/* VARIÁVEIS GLOBAIS DAS BIBLIOTECAS          */
/* =================================================================================================================== */
/* ============================================ SYSTICK ============================================================= */
/*
 * Contador de ticks do SysTick.
 * Incrementado automaticamente pela ISR na biblioteca CH32V003_SYSTICK_V3.h.
 * O kernel CDC-P usa este valor como base de tempo principal.
 */
volatile uint32_t Systick_Counter = 0;

/* ============================================ USART DMA =========================================================== */
/*
 * Buffers e flags da USART V4.0:
 * - TX Buffer: onde os dados são colocados para transmissão DMA
 * - RX Buffer: onde os dados recebidos são armazenados (1 a 64 bytes)
 * - TxBusy: 1 = transmissão em andamento (não enviar novo pacote)
 * - RxReady: 1 = pacote completo recebido (IDLE detectado)
 * - RxSize: tamanho máximo do buffer de recepção
 */
uint8_t  USART_TxBuffer[USART_BUFFER_SIZE] = {0};
uint8_t  USART_RxBuffer[USART_BUFFER_SIZE] = {0};
volatile uint8_t USART_TxBusy = 0;
volatile uint8_t USART_RxReady = 0;
uint8_t  USART_RxSize = USART_BUFFER_SIZE;

/* ============================================ ADC DMA ============================================================= */
/*
 * Buffer do ADC para modo DMA (não usado no modo SINGLE).
 * Mantido para compatibilidade com a biblioteca ADC_V3.
 */
uint16_t ADC_DMA_Buffer[ADC_BUFFER_SIZE] = {0};
volatile uint8_t ADC_DMA_Ready = 0;

/* ============================================ DMA GENÉRICO ======================================================= */
/*
 * Flags de status dos 7 canais DMA.
 * Usado pela biblioteca DMA_V3 para indicar transferências completas.
 */
volatile uint8_t DMA_Flags[7] = {0};

/* =================================================================================================================== */
/* VARIÁVEIS DO KERNEL CDC-P                  */
/* =================================================================================================================== */
/* ============================================ BASE DE TEMPO ======================================================= */
/*
 * ALIAS: tick = Systick_Counter
 * 
 * O kernel CDC-P usa 'tick' como referência temporal principal.
 * A biblioteca SysTick incrementa Systick_Counter na ISR.
 * 
 * DERIVAÇÃO DO SEGUNDO:
 * - 'clock' acumula ticks
 * - Quando clock >= _segundo, incrementa 'segundo'
 * - 'segundo' é INDEPENDENTE do tick configurado (1ms, 10ms, 100ms)
 */
#define tick Systick_Counter

volatile uint32_t segundo = 0;                  /* Contador de segundos (para tarefas de longo período) */
uint32_t clock = 0;                             /* Auxiliar para gerar base de 1 segundo */
uint32_t _segundo = 100;                        /* Ticks para completar 1 segundo (10ms × 100 = 1000ms) */

/* =================================================================================================================== */
/* SEMÁFOROS (Máscaras de Bits)               */
/* =================================================================================================================== */
/*
 * SISTEMA DE SEMÁFOROS:
 * Cada bit representa o estado de bloqueio de uma tarefa.
 * 
 * BIT 1: Task1 (LED PC0)
 * BIT 2: Task2 (LED PC1)
 * BIT 3: Task3 (Comandos seriais)
 * BIT 4: Task4 (Status 60s)
 * BIT 5: Task5 (Botão PC2)
 * BIT 6: Task6 (ADC PC4)
 * BIT 9: Task9 (Síndico - recuperação)
 * 
 * TASK10 (XERIFE) NÃO POSSUI SEMÁFORO:
 * O Xerife nunca pode ser bloqueado, pois é o responsável
 * por diagnosticar e isolar tarefas problemáticas.
 */
uint16_t semaforos = 0;                        /* Registrador de semáforos (16 bits) */

/* Máscaras para SETAR/LIMPAR/TESTAR semáforos
 * NOTA: (uint16_t)(1U << N) garante tipo unsigned e evita UB em bits altos. */
#define SEMA_TASK1_MASK  ((uint16_t)(1U << 1)) /* Bit 1: 0x0002 - Semáforo Task1 */
#define SEMA_TASK2_MASK  ((uint16_t)(1U << 2)) /* Bit 2: 0x0004 - Semáforo Task2 */
#define SEMA_TASK3_MASK  ((uint16_t)(1U << 3)) /* Bit 3: 0x0008 - Semáforo Task3 */
#define SEMA_TASK4_MASK  ((uint16_t)(1U << 4)) /* Bit 4: 0x0010 - Semáforo Task4 */
#define SEMA_TASK5_MASK  ((uint16_t)(1U << 5)) /* Bit 5: 0x0020 - Semáforo Task5 */
#define SEMA_TASK6_MASK  ((uint16_t)(1U << 6)) /* Bit 6: 0x0040 - Semáforo Task6 (ADC) */
#define SEMA_TASK9_MASK  ((uint16_t)(1U << 9)) /* Bit 9: 0x0200 - Semáforo Task9 (Síndico) */

/* Macros para TESTAR semáforos (retornam 0 ou não-zero) */
#define sema_task1  (semaforos & SEMA_TASK1_MASK)
#define sema_task2  (semaforos & SEMA_TASK2_MASK)
#define sema_task3  (semaforos & SEMA_TASK3_MASK)
#define sema_task4  (semaforos & SEMA_TASK4_MASK)
#define sema_task5  (semaforos & SEMA_TASK5_MASK)
#define sema_task6  (semaforos & SEMA_TASK6_MASK)
#define sema_task9  (semaforos & SEMA_TASK9_MASK)

/* =================================================================================================================== */
/* FLAGS DE URGÊNCIA SITUACIONAL (URG-S)      */
/* =================================================================================================================== */
/*
 * SISTEMA DE URGÊNCIA (FURA-FILA):
 * Permite que eventos externos forcem a execução IMEDIATA
 * de uma tarefa na PRÓXIMA iteração do despachador.
 * 
 * COMO FUNCIONA:
 * 1. Evento externo chama set_urgent(task_id)
 * 2. Flag de urgência é setada
 * 3. No despachador, NÍVEL 1 verifica urgências PRIMEIRO
 * 4. Tarefa urgente executa ANTES de todas as outras
 * 5. Flag é limpa automaticamente após execução
 * 
 * EXEMPLO:
 * - Chegou comando crítico pela serial
 * - set_urgent(1) é chamado
 * - Task1 executa IMEDIATAMENTE (mesmo se período não venceu)
 */
uint16_t flags_urgencia = 0;                   /* Registrador de urgências (16 bits) */

/* Máscaras para urgências */
#define URG_TASK1_MASK  ((uint16_t)(1U << 1))  /* Bit 1: Urgência Task1 */
#define URG_TASK2_MASK  ((uint16_t)(1U << 2))  /* Bit 2: Urgência Task2 */
#define URG_TASK3_MASK  ((uint16_t)(1U << 3))  /* Bit 3: Urgência Task3 */
#define URG_TASK4_MASK  ((uint16_t)(1U << 4))  /* Bit 4: Urgência Task4 */
#define URG_TASK5_MASK  ((uint16_t)(1U << 5))  /* Bit 5: Urgência Task5 */
#define URG_TASK6_MASK  ((uint16_t)(1U << 6))  /* Bit 6: Urgência Task6 */

/* Macros para TESTAR urgências */
#define urg_task1  (flags_urgencia & URG_TASK1_MASK)
#define urg_task2  (flags_urgencia & URG_TASK2_MASK)
#define urg_task3  (flags_urgencia & URG_TASK3_MASK)
#define urg_task4  (flags_urgencia & URG_TASK4_MASK)
#define urg_task5  (flags_urgencia & URG_TASK5_MASK)
#define urg_task6  (flags_urgencia & URG_TASK6_MASK)

/* Macros para ATIVAR urgências */
#define set_urg_task1()   (flags_urgencia |= URG_TASK1_MASK)
#define set_urg_task2()   (flags_urgencia |= URG_TASK2_MASK)
#define set_urg_task3()   (flags_urgencia |= URG_TASK3_MASK)
#define set_urg_task4()   (flags_urgencia |= URG_TASK4_MASK)
#define set_urg_task5()   (flags_urgencia |= URG_TASK5_MASK)
#define set_urg_task6()   (flags_urgencia |= URG_TASK6_MASK)

/* Macros para LIMPAR urgências */
#define clr_urg_task1()   (flags_urgencia &= ~URG_TASK1_MASK)
#define clr_urg_task2()   (flags_urgencia &= ~URG_TASK2_MASK)
#define clr_urg_task3()   (flags_urgencia &= ~URG_TASK3_MASK)
#define clr_urg_task4()   (flags_urgencia &= ~URG_TASK4_MASK)
#define clr_urg_task5()   (flags_urgencia &= ~URG_TASK5_MASK)
#define clr_urg_task6()   (flags_urgencia &= ~URG_TASK6_MASK)

/* =================================================================================================================== */
/* FLAGS DE PANE PERMANENTE                   */
/* =================================================================================================================== */
/*
 * SISTEMA DE ISOLAMENTO DE FALHAS:
 * Tarefas que excedem o limite de reincidências são ISOLADAS.
 * 
 * COMO FUNCIONA:
 * 1. Tarefa viola deadline → Síndico recupera (reduz período)
 * 2. Se reincidir 5 vezes → Xerife marca como PANE
 * 3. Tarefa em pane NUNCA mais executa
 * 4. Só RESET do sistema libera
 * 
 * ISSO GARANTE:
 * - Uma tarefa problemática não derruba o sistema
 * - As outras tarefas continuam funcionando
 * - O sistema se auto-protege
 */
uint16_t pane_permanente = 0;                  /* Registrador de panes (16 bits) */

/* Máscaras para panes */
#define PANE_TASK1_MASK  ((uint16_t)(1U << 1)) /* Bit 1: Pane Task1 */
#define PANE_TASK2_MASK  ((uint16_t)(1U << 2)) /* Bit 2: Pane Task2 */
#define PANE_TASK3_MASK  ((uint16_t)(1U << 3)) /* Bit 3: Pane Task3 */
#define PANE_TASK4_MASK  ((uint16_t)(1U << 4)) /* Bit 4: Pane Task4 */
#define PANE_TASK5_MASK  ((uint16_t)(1U << 5)) /* Bit 5: Pane Task5 */
#define PANE_TASK6_MASK  ((uint16_t)(1U << 6)) /* Bit 6: Pane Task6 */

/* Macros para TESTAR panes */
#define pane_task1  (pane_permanente & PANE_TASK1_MASK)
#define pane_task2  (pane_permanente & PANE_TASK2_MASK)
#define pane_task3  (pane_permanente & PANE_TASK3_MASK)
#define pane_task4  (pane_permanente & PANE_TASK4_MASK)
#define pane_task5  (pane_permanente & PANE_TASK5_MASK)
#define pane_task6  (pane_permanente & PANE_TASK6_MASK)

/* =================================================================================================================== */
/* TIMERS DAS TAREFAS (Registro de Última Execução) */
/* =================================================================================================================== */
/*
 * Cada tarefa possui um timer que armazena o valor do tick/segundo
 * da sua ÚLTIMA execução.
 * 
 * O despachador usa este valor para calcular se a tarefa deve executar:
 * - Base tick:    if((tick - timer_taskX) >= taskX)
 * - Base segundo: if((segundo - timer_taskX) >= taskX)
 * 
 * EXEMPLO:
 * task1 = 2 (20ms)
 * timer_task1 = 100 (última execução no tick 100)
 * tick = 102 (tick atual)
 * (102 - 100) = 2 >= 2 → EXECUTA!
 * 
 * NOTA: A subtração é aritmética modular uint32_t (ISO C11 §6.2.5/9),
 *       portanto correta mesmo após wrap-around de 32 bits.
 */
uint32_t timer_task1 = 0;                       /* Último tick da Task1 */
uint32_t timer_task2 = 0;                       /* Último tick da Task2 */
uint32_t timer_task3 = 0;                       /* Último tick da Task3 */
uint32_t timer_task4 = 0;                       /* Último segundo da Task4 */
uint32_t timer_task5 = 0;                       /* Último tick da Task5 */
uint32_t timer_task6 = 0;                       /* Último tick da Task6 (ADC) */
uint32_t timer_task9 = 0;                       /* Último tick da Task9 (Síndico) */
uint32_t timer_task10 = 0;                      /* Último tick da Task10 (Xerife) */

/* =================================================================================================================== */
/* TIMERS DE EXECUÇÃO (Medição de Overhead)   */
/* =================================================================================================================== */
/*
 * Medem o tempo REAL gasto na execução de cada tarefa.
 * Usados pelo sistema de AUTO-REGULAGEM TEMPORAL.
 * 
 * CÁLCULO:
 * timer_ex_taskX = (tick - timer_taskX) / taskX
 * 
 * INTERPRETAÇÃO:
 * - timer_ex = 0: tarefa executou no tempo certo (SAUDÁVEL)
 * - timer_ex > 0: tarefa atrasou (SOBRECARREGADA)
 * 
 * AÇÃO:
 * Se timer_ex > 0, a tarefa AUMENTA seu período (desacelera)
 * para reduzir a carga do sistema.
 */
uint32_t timer_ex_task1 = 0;                    /* Overhead da Task1 */
uint32_t timer_ex_task2 = 0;                    /* Overhead da Task2 */
uint32_t timer_ex_task3 = 0;                    /* Overhead da Task3 */
uint32_t timer_ex_task4 = 0;                    /* Overhead da Task4 */
uint32_t timer_ex_task5 = 0;                    /* Overhead da Task5 */
uint32_t timer_ex_task6 = 0;                    /* Overhead da Task6 (ADC) */
uint32_t timer_ex_task9 = 0;                    /* Overhead da Task9 */

/* =================================================================================================================== */
/* PERÍODOS DINÂMICOS (Auto-ajustáveis)       */
/* =================================================================================================================== */
/*
 * Valores ajustados DINAMICAMENTE pelo sistema:
 * 
 * AUTO-REGULAGEM (dentro da própria tarefa):
 * - Atrasou → taskX AUMENTA (desacelera)
 * - Excedeu deadline → BLOQUEIA (semáforo)
 * 
 * RECUPERAÇÃO (pelo Síndico - Task9):
 * - Bloqueada → taskX DIMINUI (acelera)
 * - Voltou ao normal → LIBERA (semáforo limpo)
 * 
 * PREEMPÇÃO (CDC-P):
 * - Tarefa escolhida → taskX = 1 (máxima velocidade)
 * 
 * VALORES INICIAIS (SysTick 10ms):
 * - Task1: 2 ticks = 20ms
 * - Task2: 4 ticks = 40ms
 * - Task3: 1 tick = 10ms (processa comandos rápido)
 * - Task4: 60 segundos (usa 'segundo')
 * - Task5: 3 ticks = 30ms
 * - Task6: 50 ticks = 500ms (leitura ADC)
 * - Task9: 1 tick = 10ms (Síndico - MÁXIMA PRIORIDADE)
 * - Task10: 1 tick = 10ms (Xerife - MÁXIMA PRIORIDADE)
 */
uint32_t task1 = 2;                             /* Período Task1: 2 ticks (20ms) */
uint32_t task2 = 4;                             /* Período Task2: 4 ticks (40ms) */
uint32_t task3 = 1;                             /* Período Task3: 1 tick (10ms) */
uint32_t task4 = 60;                            /* Período Task4: 60 segundos */
uint32_t task5 = 3;                             /* Período Task5: 3 ticks (30ms) */
uint32_t task6 = 50;                            /* Período Task6: 50 ticks (500ms) - ADC */
uint32_t task9 = 1;                             /* Período Task9: 1 tick (10ms) */
uint32_t task10 = 1;                            /* Período Task10: 1 tick (10ms) */

/* =================================================================================================================== */
/* CONTADORES DE BLOQUEIO (Diagnóstico de Pane) */
/* =================================================================================================================== */
/*
 * Contam quantas vezes cada tarefa foi RECUPERADA pelo Síndico.
 * 
 * SE EXCEDER O LIMITE:
 * - Tarefas base tick: 5 reincidências → PANE
 * - Tarefas base segundo: 1 reincidência → PANE (mais rigoroso)
 * 
 * O XERIFE (Task10) monitora estes contadores e ISOLA
 * permanentemente tarefas com excesso de falhas.
 */
uint32_t cont_desbloqueio_task1 = 0;            /* Reincidências Task1 */
uint32_t cont_desbloqueio_task2 = 0;            /* Reincidências Task2 */
uint32_t cont_desbloqueio_task3 = 0;            /* Reincidências Task3 */
uint32_t cont_desbloqueio_task4 = 0;            /* Reincidências Task4 */
uint32_t cont_desbloqueio_task5 = 0;            /* Reincidências Task5 */
uint32_t cont_desbloqueio_task6 = 0;            /* Reincidências Task6 (ADC) */
uint32_t cont_desbloqueio_task9 = 0;            /* Reincidências Task9 */

/* =================================================================================================================== */
/* VARIÁVEIS DE CONTROLE DO KERNEL            */
/* =================================================================================================================== */
/*
 * monitor_tasks: Flag de monitoração do Xerife
 *   - 0 = sistema sob análise (alguma tarefa em pane)
 *   - 1 = sistema normal
 * 
 * preempt_enabled: Flag global de preempção
 *   - 0 = CDC normal (sem preempção)
 *   - 1 = Preempção ativa
 * 
 * preempt_task_id: ID da tarefa acelerada (1-6)
 * 
 * preempt_original_period: Backup do período original
 *   - Usado para restaurar quando desativar preempção
 *   - CORRIGIDO: uint32_t (antes uint8_t, truncava períodos > 255)
 * 
 * preempt_triggered: Flag de disparo externo
 *   - 1 = executar tarefa preemptiva na próxima iteração
 */
uint8_t monitor_tasks = 0;
uint8_t preempt_enabled = 0;
uint8_t preempt_task_id = 0;
uint32_t preempt_original_period = 0;           /* CORRIGIDO: uint32_t */
uint8_t preempt_triggered = 0;

/* =================================================================================================================== */
/* VARIÁVEIS DE ESTADO DA APLICAÇÃO           */
/* =================================================================================================================== */
/*
 * led1: Estado atual do LED1 (PC0)
 *   - 0 = DESLIGADO
 *   - 1 = LIGADO
 * 
 * led2: Estado atual do LED2 (PC1)
 * 
 * old_botao: Estado ANTERIOR do botão (PC2)
 *   - Usado para detecção de borda de subida
 * 
 * adc_value: Último valor lido do ADC (0-1023)
 * 
 * adc_mv: Último valor convertido em milivolts (0-3300)
 */
uint8_t led1 = 0;
uint8_t led2 = 0;
uint8_t old_botao = 0;
uint16_t adc_value = 0;
uint16_t adc_mv = 0;

/* =================================================================================================================== */
/* PROTÓTIPOS DAS FUNÇÕES                     */
/* =================================================================================================================== */
/* Funções de preempção (CDC-P) */
void CDC_EnablePreempt(uint8_t task_id);        /* Ativa preempção em uma tarefa */
void CDC_DisablePreempt(void);                  /* Desativa preempção */
void CDC_TriggerPreempt(void);                  /* Dispara preempção por evento */

/* Funções de urgência (URG-S) */
void set_urgent(uint8_t task_id);               /* Seta flag de urgência */
void clear_all_urgency(void);                   /* Limpa todas as urgências */
void msg_boot(void);                            /* Imprime mensagem inicial */

/* Funções das tarefas */
void task1_func(void);                          /* Tarefa 1: LED1 (PC0) */
void task2_func(void);                          /* Tarefa 2: LED2 (PC1) */
void task3_func(void);                          /* Tarefa 3: Comandos seriais */
void task4_func(void);                          /* Tarefa 4: Status 60s */
void task5_func(void);                          /* Tarefa 5: Botão (PC2) */
void task6_func(void);                          /* Tarefa 6: ADC (PC4) */
void task9_func(void);                          /* Tarefa 9: Síndico */
void task10_func(void);                         /* Tarefa 10: Xerife */

/* =================================================================================================================== */
/* FUNÇÕES DE PREEMPÇÃO (CDC-P)               */
/* =================================================================================================================== */

/**
 * @brief  Ativa o modo preemptivo para uma tarefa específica
 * @param  task_id: ID da tarefa a ser acelerada (1-6)
 * 
 * @note   FUNCIONAMENTO:
 *         1. Verifica se não há outra preempção ativa
 *         2. Verifica se a tarefa NÃO está em pane permanente
 *         3. Salva o período original (para restaurar depois)
 *         4. Reduz o período para 1 tick (máxima velocidade)
 *         
 * @note   EFEITO:
 *         A tarefa escolhida passa a executar em TODAS as
 *         iterações do kernel, respondendo rapidamente a eventos.
 */
void CDC_EnablePreempt(uint8_t task_id)
{
   if(preempt_enabled == 0)                     /* Só ativa se não houver preempção */
   {
      /* Verifica se a tarefa NÃO está em pane permanente */
      if(task_id == 1 && pane_task1) return;
      if(task_id == 2 && pane_task2) return;
      if(task_id == 3 && pane_task3) return;
      if(task_id == 4 && pane_task4) return;
      if(task_id == 5 && pane_task5) return;
      if(task_id == 6 && pane_task6) return;

      /* Ativa o modo preemptivo global */
      preempt_enabled = 1;
      preempt_task_id = task_id;

      /* Salva período original e reduz para 1 tick
       * NOTA: preempt_original_period é uint32_t, compatível com task1..task6 */
      if(task_id == 1) { preempt_original_period = task1; task1 = 1; }
      if(task_id == 2) { preempt_original_period = task2; task2 = 1; }
      if(task_id == 3) { preempt_original_period = task3; task3 = 1; }
      if(task_id == 4) { preempt_original_period = task4; task4 = 1; }
      if(task_id == 5) { preempt_original_period = task5; task5 = 1; }
      if(task_id == 6) { preempt_original_period = task6; task6 = 1; }

      USART_Printf("Preempcao ATIVADA na Task%d\r\n", task_id);
   }
   else {
      USART_Println("Erro: Preempcao ja ativa!");
   }
}

/**
 * @brief  Desativa o modo preemptivo
 * @note   Restaura o período original da tarefa que estava acelerada
 */
void CDC_DisablePreempt(void)
{
   if(preempt_enabled == 1)                     /* Só desativa se houver preempção */
   {
      /* Restaura o período original */
      if(preempt_task_id == 1) task1 = preempt_original_period;
      if(preempt_task_id == 2) task2 = preempt_original_period;
      if(preempt_task_id == 3) task3 = preempt_original_period;
      if(preempt_task_id == 4) task4 = preempt_original_period;
      if(preempt_task_id == 5) task5 = preempt_original_period;
      if(preempt_task_id == 6) task6 = preempt_original_period;

      /* Limpa todas as variáveis de controle */
      preempt_enabled = 0;
      preempt_task_id = 0;
      preempt_original_period = 0;

      USART_Println("Preempcao DESATIVADA");
   }
   else {
      USART_Println("Erro: Nao ha preempcao ativa");
   }
}

/**
 * @brief  Dispara a preempção por evento externo
 * @note   Força a execução IMEDIATA na próxima iteração
 */
void CDC_TriggerPreempt(void)
{
   if(preempt_enabled == 1)
   {
      preempt_triggered = 1;                    /* Seta flag de disparo */
      USART_Println("Preempcao DISPARADA!");
   }
   else {
      USART_Println("Erro: Ative a preempcao primeiro");
   }
}

/* =================================================================================================================== */
/* FUNÇÕES DE URGÊNCIA (URG-S)                */
/* =================================================================================================================== */

/**
 * @brief  Seta flag de urgência para uma tarefa
 * @param  task_id: ID da tarefa (1-6)
 * @note   A tarefa executa na PRÓXIMA iteração (fura-fila)
 */
void set_urgent(uint8_t task_id)
{
   if(task_id == 1) set_urg_task1();
   if(task_id == 2) set_urg_task2();
   if(task_id == 3) set_urg_task3();
   if(task_id == 4) set_urg_task4();
   if(task_id == 5) set_urg_task5();
   if(task_id == 6) set_urg_task6();
}

/**
 * @brief  Limpa todas as flags de urgência
 */
void clear_all_urgency(void)
{
   flags_urgencia = 0;
}

/* =================================================================================================================== */
/* FUNÇÕES DE MSG  (BOOT)                  */
/* =================================================================================================================== */

void msg_boot()
{
   USART_Println(
        "========================================\r\n"
        "  CDC-P v3.0 - SISTEMA INICIADO\r\n"
        "  CH32V003F4P6 - MODO MODULAR\r\n"
        "  RTOS Autoconsciente/Cyber Organismo\r\n"
        "========================================\r\n"
        "Libs: IO+USART+SysTick+ADC+DMA\r\n"
        "Tarefas: 1-6 + 9-10\r\n"
        "ADC: CH2 (PC4) a cada 500ms\r\n"
        "Buffer RX: 1 a 64 bytes (IDLE)\r\n"
        "========================================\r\n"
        "COMANDOS:\r\n"
        "  p = Preempcao Task1\r\n"
        "  n = Desativar preempcao\r\n"
        "  e = Disparar preempcao\r\n"
        "  u = Urgencia Task1\r\n"
        "  s = Status resumido\r\n"
        "  d = Diagnostico completo\r\n"
        "  1 = Tick 1ms\r\n"
        "  2 = Tick 10ms\r\n"
        "  3 = Tick 100ms\r\n"
        "========================================\r\n"
    );
}
/* =================================================================================================================== */
/* TAREFA 1: Controle do LED1 (PC0)           */
/* =================================================================================================================== */
/**
 * @brief  Inverte o LED no pino PC0 a cada 20ms
 * 
 * @note   AUTO-REGULAGEM:
 *         - Atrasou → aumenta período (desacelera)
 *         - Excedeu deadline → bloqueia (semáforo)
 */
void task1_func(void)
{
   GPIO_Toggle(GPIOC, PC0);                    /* Inverte LED1 */
   led1 = !led1;                               /* Atualiza estado */

   /* Auto-regulagem temporal */
   timer_ex_task1 = (tick - timer_task1) / task1;
   if(timer_ex_task1) { task1 = task1 + 1; }   /* Desacelera se atrasou */
   if(task1 >= tempo_maximo_task1) { semaforos |= SEMA_TASK1_MASK; }  /* Bloqueia se excedeu */
}

/* =================================================================================================================== */
/* TAREFA 2: Controle do LED2 (PC1)           */
/* =================================================================================================================== */
/**
 * @brief  Inverte o LED no pino PC1 a cada 40ms
 * @note   Mesmo sistema de auto-regulagem da Task1
 */
void task2_func(void)
{
   GPIO_Toggle(GPIOC, PC1);                    /* Inverte LED2 */
   led2 = !led2;                               /* Atualiza estado */

   /* Auto-regulagem temporal */
   timer_ex_task2 = (tick - timer_task2) / task2;
   if(timer_ex_task2) { task2 = task2 + 1; }   /* Desacelera se atrasou */
   if(task2 >= tempo_maximo_task2) { semaforos |= SEMA_TASK2_MASK; }  /* Bloqueia se excedeu */
}

/* =================================================================================================================== */
/* TAREFA 3: Processamento de Comandos        */
/* =================================================================================================================== */
/**
 * @brief  Processa comandos recebidos via DMA (buffer ajustável 1-64 bytes)
 * 
 * @note   COMANDOS DISPONÍVEIS:
 *         - 'p': Ativa preempção na Task1
 *         - 'n': Desativa preempção
 *         - 'e': Dispara preempção
 *         - 'u': Seta urgência na Task1
 *         - 's': Status resumido
 *         - 'd': Diagnóstico completo
 *         - '1': Tick 1ms
 *         - '2': Tick 10ms
 *         - '3': Tick 100ms
 */
void task3_func(void)
{
   uint8_t rx_buf[USART_BUFFER_SIZE];
   uint8_t len = USART_DMA_Read(rx_buf, sizeof(rx_buf));
   
   if(len > 0)
   {
       /* ===== COMANDOS DE PREEMPÇÃO ===== */
       if(rx_buf[0] == 'p') { CDC_EnablePreempt(1); }
       if(rx_buf[0] == 'n') { CDC_DisablePreempt(); }
       if(rx_buf[0] == 'e') { CDC_TriggerPreempt(); }
       if(rx_buf[0] == 'u') { set_urgent(1); USART_Println("Urgente!"); }
       
       /* ===== COMANDO 's' - STATUS RESUMIDO ===== */
       if(rx_buf[0] == 's') {
           USART_Println("=== STATUS CDC-P v3.0 ===");
           USART_Printf("Tick: %lu\r\n", (uint32_t)tick);
           USART_Printf("Segundo: %lu\r\n", (uint32_t)segundo);
           USART_Printf("ADC: %d (%d mV)\r\n", adc_value, adc_mv);
           USART_Println("========================");
       }
       
       /* ===== COMANDO 'd' - DIAGNÓSTICO COMPLETO ===== */
       if(rx_buf[0] == 'd') {
           USART_Println("========================================");
           USART_Println("  DIAGNOSTICO COMPLETO CDC-P v3.0");
           USART_Println("========================================");
           
           /* Kernel */
           USART_Println("[KERNEL]");
           USART_Printf("  Tick: %lu\r\n", (uint32_t)tick);
           USART_Printf("  Segundo: %lu\r\n", (uint32_t)segundo);
           USART_Printf("  Base 1s: %lu ticks\r\n", (uint32_t)_segundo);
           USART_Println("");
           
           /* Task 1 */
           USART_Println("[TASK 1] LED1 (PC0)");
           USART_Printf("  Periodo: %lu ticks (%lums)\r\n", (uint32_t)task1, (uint32_t)(task1 * 10));
           USART_Printf("  Estado: %s\r\n", pane_task1 ? "PANE" : sema_task1 ? "BLOQUEADA" : "EXECUTANDO");
           USART_Printf("  Reincidencias: %lu\r\n", (uint32_t)cont_desbloqueio_task1);
           USART_Printf("  LED: %s\r\n", led1 ? "LIGADO" : "DESLIGADO");
           USART_Println("");
           
           /* Task 2 */
           USART_Println("[TASK 2] LED2 (PC1)");
           USART_Printf("  Periodo: %lu ticks (%lums)\r\n", (uint32_t)task2, (uint32_t)(task2 * 10));
           USART_Printf("  Estado: %s\r\n", pane_task2 ? "PANE" : sema_task2 ? "BLOQUEADA" : "EXECUTANDO");
           USART_Printf("  Reincidencias: %lu\r\n", (uint32_t)cont_desbloqueio_task2);
           USART_Printf("  LED: %s\r\n", led2 ? "LIGADO" : "DESLIGADO");
           USART_Println("");
           
           /* Task 3 */
           USART_Println("[TASK 3] Comandos Seriais");
           USART_Printf("  Periodo: %lu ticks (%lums)\r\n", (uint32_t)task3, (uint32_t)(task3 * 10));
           USART_Printf("  Estado: %s\r\n", pane_task3 ? "PANE" : sema_task3 ? "BLOQUEADA" : "EXECUTANDO");
           USART_Printf("  Buffer RX: %d bytes\r\n", USART_BUFFER_SIZE);
           USART_Println("");
           
           /* Task 4 */
           USART_Println("[TASK 4] Status Periodico");
           USART_Printf("  Periodo: %lu segundos\r\n", (uint32_t)task4);
           USART_Printf("  Estado: %s\r\n", pane_task4 ? "PANE" : sema_task4 ? "BLOQUEADA" : "EXECUTANDO");
           USART_Println("");
           
           /* Task 5 */
           USART_Println("[TASK 5] Botao (PC2)");
           USART_Printf("  Periodo: %lu ticks (%lums)\r\n", (uint32_t)task5, (uint32_t)(task5 * 10));
           USART_Printf("  Estado: %s\r\n", pane_task5 ? "PANE" : sema_task5 ? "BLOQUEADA" : "EXECUTANDO");
           USART_Printf("  Botao: %s\r\n", GPIO_ReadPin(GPIOC, PC2) ? "SOLTO" : "PRESSIONADO");
           USART_Println("");
           
           /* Task 6 - ADC */
           USART_Println("[TASK 6] ADC (PC4)");
           USART_Printf("  Periodo: %lu ticks (%lums)\r\n", (uint32_t)task6, (uint32_t)(task6 * 10));
           USART_Printf("  Estado: %s\r\n", pane_task6 ? "PANE" : sema_task6 ? "BLOQUEADA" : "EXECUTANDO");
           USART_Printf("  Reincidencias: %lu\r\n", (uint32_t)cont_desbloqueio_task6);
           USART_Printf("  Valor ADC: %d\r\n", adc_value);
           USART_Printf("  Tensao: %d mV\r\n", adc_mv);
           USART_Println("");
           
           /* Task 9 */
           USART_Println("[TASK 9] O SINDICO");
           USART_Printf("  Periodo: %lu ticks\r\n", (uint32_t)task9);
           USART_Printf("  Estado: %s\r\n", sema_task9 ? "BLOQUEADO" : "ATIVO");
           USART_Printf("  Reincidencias: %lu\r\n", (uint32_t)cont_desbloqueio_task9);
           USART_Println("");
           
           /* Task 10 */
           USART_Println("[TASK 10] O XERIFE");
           USART_Printf("  Periodo: %lu ticks\r\n", (uint32_t)task10);
           USART_Println("  Estado: ATIVO (nunca bloqueado)");
           USART_Printf("  Panes: %d\r\n", (pane_task1?1:0)+(pane_task2?1:0)+(pane_task3?1:0)+(pane_task4?1:0)+(pane_task5?1:0)+(pane_task6?1:0));
           USART_Println("");
           
           /* Preempção */
           USART_Println("[PREEMPCAO CDC-P]");
           USART_Printf("  Estado: %s\r\n", preempt_enabled ? "ATIVA" : "INATIVA");
           if(preempt_enabled) {
               USART_Printf("  Task: %d\r\n", preempt_task_id);
               USART_Printf("  Periodo original: %lu\r\n", (uint32_t)preempt_original_period);
           }
           USART_Println("");
           
           /* Urgências */
           USART_Println("[URGENCIAS URG-S]");
           USART_Printf("  Flags: 0x%04X\r\n", flags_urgencia);
           USART_Printf("  Task1: %s\r\n", urg_task1 ? "URGENTE" : "normal");
           USART_Printf("  Task2: %s\r\n", urg_task2 ? "URGENTE" : "normal");
           USART_Printf("  Task3: %s\r\n", urg_task3 ? "URGENTE" : "normal");
           USART_Printf("  Task4: %s\r\n", urg_task4 ? "URGENTE" : "normal");
           USART_Printf("  Task5: %s\r\n", urg_task5 ? "URGENTE" : "normal");
           USART_Printf("  Task6: %s\r\n", urg_task6 ? "URGENTE" : "normal");
           USART_Println("");
           
           USART_Println("========================================");
           USART_Println("  FIM DO DIAGNOSTICO");
           USART_Println("========================================");
       }
       
       /* ===== COMANDOS DE AJUSTE DE TICK ===== */
       if(rx_buf[0] == '1') { SysTick_Init(1); _segundo = 1000; USART_Println("Tick: 1ms"); }
       if(rx_buf[0] == '2') { SysTick_Init(10); _segundo = 100; USART_Println("Tick: 10ms"); }
       if(rx_buf[0] == '3') { SysTick_Init(100); _segundo = 10; USART_Println("Tick: 100ms"); }
   }

   /* Auto-regulagem temporal */
   timer_ex_task3 = (tick - timer_task3) / task3;
   if(timer_ex_task3) { task3 = task3 + 1; }
   if(task3 >= tempo_maximo_task3) { semaforos |= SEMA_TASK3_MASK; }
}

/* =================================================================================================================== */
/* TAREFA 4: Status Periódico (60 segundos)   */
/* =================================================================================================================== */
/**
 * @brief  Envia status a cada 60 segundos
 * @note   Usa 'segundo' como base (independente do tick)
 */
void task4_func(void)
{
   USART_Println("=== Status Periodico (60s) ===");
   USART_Printf("Segundo: %lu\r\n", (uint32_t)segundo);
   USART_Printf("ADC: %d (%d mV)\r\n", adc_value, adc_mv);

   timer_ex_task4 = (segundo - timer_task4) / task4;
   if(timer_ex_task4) { task4 = task4 + 1; }
   if(task4 >= tempo_maximo_task4) { semaforos |= SEMA_TASK4_MASK; }
}

/* =================================================================================================================== */
/* TAREFA 5: Leitura do Botão (PC2)           */
/* =================================================================================================================== */
/**
 * @brief  Lê botão com detecção de borda de subida
 * @note   Mensagem enviada quando botão é SOLTO (0→1)
 */
void task5_func(void)
{
   uint8_t bt_state = GPIO_ReadPin(GPIOC, PC2);
   
   /* Detecção de borda de subida (botão solto) */
   if(bt_state && old_botao == false)
   {
       old_botao = bt_state;
       USART_Println("Botao pressionado!!!");
   }
   else {
       old_botao = bt_state;
   }

   timer_ex_task5 = (tick - timer_task5) / task5;
   if(timer_ex_task5) { task5 = task5 + 1; }
   if(task5 >= tempo_maximo_task5) { semaforos |= SEMA_TASK5_MASK; }
}

/* =================================================================================================================== */
/* TAREFA 6: Leitura ADC (PC4)                */
/* =================================================================================================================== */
/**
 * @brief  Lê o ADC_CH2 (PC4) a cada 50 ticks (500ms)
 * 
 * @note   FUNCIONAMENTO:
 *         1. Lê o valor do ADC no canal 2 (PC4)
 *         2. Converte para milivolts (0-1023 → 0-3300mV)
 *         3. Exibe o valor na serial
 *         4. Auto-regulagem: atrasou → desacelera
 */
void task6_func(void)
{
   /* Lê o canal ADC_CH2 (PC4) */
   adc_value = ADC_Read(ADC_CH2);
   
   /* Converte para milivolts (10 bits: 0-1023 → 0-3300mV) */
   adc_mv = ADC_ToMilliVolts(adc_value);
   
   /* Exibe o valor lido */
   USART_Printf("ADC CH2: %d (%d mV)\r\n", adc_value, adc_mv);

   /* Auto-regulagem temporal */
   timer_ex_task6 = (tick - timer_task6) / task6;
   if(timer_ex_task6) { task6 = task6 + 1; }   /* Desacelera se atrasou */
   if(task6 >= tempo_maximo_task6) { semaforos |= SEMA_TASK6_MASK; }  /* Bloqueia se excedeu */
}

/* =================================================================================================================== */
/* TAREFA 9: O SÍNDICO (Recuperação)          */
/* =================================================================================================================== */
/**
 * @brief  Recupera tarefas bloqueadas reduzindo seus períodos
 * 
 * @note   FUNÇÃO CRÍTICA DO SISTEMA!
 *         - Monitora cada tarefa bloqueada
 *         - Reduz gradualmente o período (acelera)
 *         - Libera o semáforo quando recupera
 *         - Conta reincidências para diagnóstico
 *         - Se o próprio Síndico falhar → RESET
 * 
 * @note   CORREÇÃO APLICADA:
 *         - Proteção contra underflow (taskX > 1) evita divisão por zero
 *         - task9 é verificado antes de decrementar
 */
void task9_func(void)
{
   /* Recupera cada tarefa bloqueada (exceto em pane)
    * NOTA: Proteção contra underflow (taskX > 1) evita divisão por zero
    *       na próxima execução de taskX_func. */
   if(sema_task1 && !pane_task1 && task1 > 1) { task1 -= 1; semaforos &= ~SEMA_TASK1_MASK; cont_desbloqueio_task1++; }
   if(sema_task2 && !pane_task2 && task2 > 1) { task2 -= 1; semaforos &= ~SEMA_TASK2_MASK; cont_desbloqueio_task2++; }
   if(sema_task3 && !pane_task3 && task3 > 1) { task3 -= 1; semaforos &= ~SEMA_TASK3_MASK; cont_desbloqueio_task3++; }
   if(sema_task4 && !pane_task4 && task4 > 1) { task4 -= 1; semaforos &= ~SEMA_TASK4_MASK; cont_desbloqueio_task4++; }
   if(sema_task5 && !pane_task5 && task5 > 1) { task5 -= 1; semaforos &= ~SEMA_TASK5_MASK; cont_desbloqueio_task5++; }
   if(sema_task6 && !pane_task6 && task6 > 1) { task6 -= 1; semaforos &= ~SEMA_TASK6_MASK; cont_desbloqueio_task6++; }
   
   /* Auto-recuperação do próprio Síndico
    * NOTA: Proteção contra underflow (task9 > 1) evita divisão por zero. */
   if(sema_task9 && task9 > 1) { task9 -= 1; semaforos &= ~SEMA_TASK9_MASK; cont_desbloqueio_task9++; }

   timer_ex_task9 = (tick - timer_task9) / task9;

   /* Última linha de defesa */
   if(task9 >= tempo_maximo_task9) {
       USART_Println("FALHA CRITICA: Sindico em pane! Reset...");
       SysTick_Delay(100);
       NVIC_SystemReset();
   }
}

/* =================================================================================================================== */
/* TAREFA 10: O XERIFE (Diagnóstico e Isolamento) */
/* =================================================================================================================== */
/**
 * @brief  Isola permanentemente tarefas com excesso de falhas
 * 
 * @note   FUNÇÃO CRÍTICA DO SISTEMA!
 *         - NUNCA pode ser bloqueada
 *         - Monitora contadores de reincidências
 *         - Marca tarefas problemáticas como PANE PERMANENTE
 *         - Tarefas em pane NUNCA mais executam (até reset)
 */
void task10_func(void)
{
   /* Verifica cada tarefa (limite: 5 reincidências tick, 1 segundo) */
   if(cont_desbloqueio_task1 >= sistema_em_pane) { semaforos |= SEMA_TASK1_MASK; pane_permanente |= PANE_TASK1_MASK; }
   if(cont_desbloqueio_task2 >= sistema_em_pane) { semaforos |= SEMA_TASK2_MASK; pane_permanente |= PANE_TASK2_MASK; }
   if(cont_desbloqueio_task3 >= sistema_em_pane) { semaforos |= SEMA_TASK3_MASK; pane_permanente |= PANE_TASK3_MASK; }
   if(cont_desbloqueio_task4 >= sistema_em_pane_seg) { semaforos |= SEMA_TASK4_MASK; pane_permanente |= PANE_TASK4_MASK; }
   if(cont_desbloqueio_task5 >= sistema_em_pane) { semaforos |= SEMA_TASK5_MASK; pane_permanente |= PANE_TASK5_MASK; }
   if(cont_desbloqueio_task6 >= sistema_em_pane) { semaforos |= SEMA_TASK6_MASK; pane_permanente |= PANE_TASK6_MASK; }
}

/* =================================================================================================================== */
/* PROGRAMA PRINCIPAL                         */
/* =================================================================================================================== */
/**
 * @brief  Ponto de entrada do CDC-P v3.0
 * 
 * @note   FLUXO DE EXECUÇÃO:
 *         1. Inicializa clock e SysTick (10ms)
 *         2. Delay para permitir gravação SWDIO
 *         3. Configura GPIOs (LEDs, botão, ADC)
 *         4. Inicializa USART com DMA + IDLE
 *         5. Inicializa ADC (modo SINGLE)
 *         6. Exibe mensagem de boot
 *         7. Executa o despachador de tarefas
 */
int main(void)
{
    /* ===== FASE 1: INICIALIZAÇÃO DO HARDWARE ===== */
    SystemCoreClockUpdate();                    /* Atualiza clock do sistema */
    
    SysTick_Init(SYSTICK_RES_10MS);             /* Inicializa SysTick (10ms) */
    _segundo = 100;                             /* 100 ticks = 1 segundo */
    
    SysTick_Delay(2000);                        /* Delay para SWDIO */
    
    /* Configura GPIOs usando a biblioteca IO */
    TRISCbits(PC0, Out30PP);                    /* PC0 = LED1 (saída push-pull) */
    TRISCbits(PC1, Out30PP);                    /* PC1 = LED2 (saída push-pull) */
    TRISCbits(PC2, InPU);                       /* PC2 = Botão (entrada pull-up) */
    TRISCbits(PC4, InAn);                       /* PC4 = ADC_CH2 (entrada analógica) */
    
    /* Inicializa USART com DMA + IDLE */
    USART_InitEx(115200, USART_MODE_DMA_FULL, USART_REMAP_NONE, USART_BUFFER_SIZE);
    
    /* Inicializa ADC - Modo SINGLE (leitura sob demanda) */
    uint8_t canais_adc[] = {ADC_CH2};           /* Canal 2 = PC4 */
    ADC_InitEx(ADC_MODE_SINGLE, canais_adc, 1, 0);
    
    /* Salva estado inicial do botão */
    old_botao = GPIO_ReadPin(GPIOC, PC2);

    /* ===== FASE 2: MENSAGEM DE BOOT ===== */
    msg_boot();

    /* ===== FASE 3: LOOP PRINCIPAL DO DESPACHADOR ===== */
    while(1)
    {
        /* ===== DERIVAÇÃO DO SEGUNDO ===== */
        /* Incrementa 'segundo' a cada 1 segundo REAL */
        if((tick - clock) >= _segundo) {
            segundo++;
            clock = tick;
        }

        /* ===== NÍVEL 1: URGÊNCIAS (URG-S) ===== */
        /* Tarefas urgentes FURAM A FILA e executam PRIMEIRO */
        if(flags_urgencia != 0) {
            if(urg_task1 && !sema_task1 && !pane_task1) { clr_urg_task1(); timer_task1 = tick; task1_func(); }
            if(urg_task2 && !sema_task2 && !pane_task2) { clr_urg_task2(); timer_task2 = tick; task2_func(); }
            if(urg_task3 && !sema_task3 && !pane_task3) { clr_urg_task3(); timer_task3 = tick; task3_func(); }
            if(urg_task4 && !sema_task4 && !pane_task4) { clr_urg_task4(); timer_task4 = segundo; task4_func(); }
            if(urg_task5 && !sema_task5 && !pane_task5) { clr_urg_task5(); timer_task5 = tick; task5_func(); }
            if(urg_task6 && !sema_task6 && !pane_task6) { clr_urg_task6(); timer_task6 = tick; task6_func(); }
        }

        /* ===== NÍVEL 2: PREEMPÇÃO (CDC-P) ===== */
        /* Tarefa preemptiva executa se houve disparo externo */
        if(preempt_triggered && preempt_task_id > 0) {
            preempt_triggered = 0;
            if(preempt_task_id == 1 && !pane_task1) { timer_task1 = tick; task1_func(); }
            if(preempt_task_id == 2 && !pane_task2) { timer_task2 = tick; task2_func(); }
            if(preempt_task_id == 3 && !pane_task3) { timer_task3 = tick; task3_func(); }
            if(preempt_task_id == 4 && !pane_task4) { timer_task4 = tick; task4_func(); }
            if(preempt_task_id == 5 && !pane_task5) { timer_task5 = tick; task5_func(); }
            if(preempt_task_id == 6 && !pane_task6) { timer_task6 = tick; task6_func(); }
        }

        /* ===== NÍVEL 3: TAREFAS DO KERNEL ===== */
        /* Xerife (Task10) e Síndico (Task9) - SEMPRE executam primeiro */
        if((tick - timer_task10) >= task10) { timer_task10 = tick; task10_func(); }
        if((tick - timer_task9) >= task9 && !sema_task9) { timer_task9 = tick; task9_func(); }

        /* ===== NÍVEL 4: TAREFAS DA APLICAÇÃO ===== */
        /* Task1 a Task6 em ordem espacial (posição na fila) */
        if((tick - timer_task1) >= task1 && !sema_task1) { timer_task1 = tick; task1_func(); }
        if((tick - timer_task2) >= task2 && !sema_task2) { timer_task2 = tick; task2_func(); }
        if((tick - timer_task3) >= task3 && !sema_task3) { timer_task3 = tick; task3_func(); }
        if((segundo - timer_task4) >= task4 && !sema_task4) { timer_task4 = segundo; task4_func(); }
        if((tick - timer_task5) >= task5 && !sema_task5) { timer_task5 = tick; task5_func(); }
        if((tick - timer_task6) >= task6 && !sema_task6) { timer_task6 = tick; task6_func(); }
    }
    
    return 0;   /* Nunca chega aqui */
}
/* =================================================================================================================== */
/* FIM DO ARQUIVO                                                                                                      */
/* =================================================================================================================== */