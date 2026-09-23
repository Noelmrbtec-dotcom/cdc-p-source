/**************************************************************************************************************
CDC-P v3.0 - Concurrency Deterministic Control with Preemption and Situational Urgency
Autor: Marcos Roberto Braga.
Aluno: Pedro Henrique Cerqueira Braga.
Data: 22/03/2024 (Atualizado em 22/06/2026).
MCU utilizada no protoboard(PIC16F628A).
Overhead do despachador medido: 55us.

Descrição: RTOS compacto, eficiente e AUTOCONSCIENTE com reflexos situacionais.
           - Tick ajustável em tempo de execução: 2ms, 4ms, 8ms (padrão) e 50ms.
           - Prioridade TRIDIMENSIONAL: 
             1. Espacial: posição na fila do despachador.
             2. Temporal: período configurado da tarefa (taskX).
             3. Situacional: flag de urgência por evento (URG-S).
           - Preempção REAL RASTREÁVEL via URG-S (resposta na mesma iteração).
           - Preempção cooperativa por período variável (CDC-P).
             IMUNE a condições de corrida, deadlocks e inversão de prioridade.
           - 7 camadas de proteção:
             1. Auto-regulagem temporal (cada tarefa se desacelera se atrasar).
             2. Bloqueio por violação de deadline.
             3. Diagnóstico de pane nativo (Task10 - O Xerife).
             4. Recuperação progressiva de tarefas bloqueadas (Task9 - O Síndico).
             5. Fail-safe final: reset do sistema se o curador falhar.
             6. Aceleração automática por preempção (Task8 - O Acelerador).
             7. URG-S: Resposta IMEDIATA (mesma iteração) a eventos urgentes.
           - NOVO v3.0: Task10 agora ISOLA permanentemente tarefas que atingem
             o limite de reincidências (sistema_em_pane), bloqueando-as até
             intervenção externa (manutenção do sistema).
             O sistema respira, hiperventila, TEM REFLEXOS e agora também
             isola os componentes problemáticos.
           - Tarefas extraídas como funções separadas que SEMPRE retornam.
           - Arquitetura inspirada nos princípios de Edsger W. Dijkstra.
           - "A bagunça tolerada por abundância de recursos." — A crítica.
**************************************************************************************************************/
/**************************************************************************************************************
                   _         _                 _
                  / \  _   _| | __ _ ___    __| | ___ 
                 / _ \| | | | |/ _` / __|  / _` |/ _ \
                / ___ \ |_| | | (_| \__ \ | (_| |  __/
               /_/   \_\__,_|_|\__,_|___/  \__,_|\___|
                                       
                                                  /\/|  __  /\/|   ___       
 _ __  _ __ ___   __ _ _ __ __ _ _ __ ___   __ _ |/\/ _/ _)|/\/   / ,_\ ___  
| '_ \| '__/ _ \ / _` | '__/ _` | '_ ` _ \ / _` | /_\/ \ \  /_\ _| |_  / _ \ 
| |_) | | | (_) | (_| | | | (_| | | | | | | (_| |/ _ \ \\ \/ _ \ | |__| (_) |
| .__/|_|  \___/ \__, |_|  \__,_|_| |_| |_|\__,_/_/ \_\ \_/_/ \_(_,____\___/ 
|_|              |___/                               (__/                    
/* ============================================================================================================
 *  MATERIAL DIDÁTICO — CDC-P v3.0
 * ============================================================================================================
 * 
 *  Este arquivo é um exemplo completo do método CDC-P, projetado para 
 *  ensino de sistemas de tempo real determinísticos em MCUs de 8 bits.
 * 
 *  ESTRUTURA:
 * 
 *    1. Configuração do hardware (I/Os, timers, USART)
 *    2. Tarefas de aplicação (Task1 ... Task5)
 *    3. Tarefas de kernel (Task9 = Síndico, Task10 = Xerife)
 *    4. Funções de preempção (CDC-P)
 *    5. Funções de urgência (URG-S)
 *    6. Ajuste dinâmico do tick
 *    7. ISRs (Timer0, RX, TX)
 *    8. Despachador (4 níveis de prioridade)
 * 
 *  CONCEITOS ENSINADOS:
 * 
 *    - Tarefas atômicas (sempre retornam, sem preempção no meio)
 *    - Auto-regulagem temporal (cada tarefa ajusta seu próprio período)
 *    - Semáforos por bit (sem mutex, sem deadlock)
 *    - Prioridade tridimensional (espacial + temporal + situacional)
 *    - Bases temporais separadas (tick + segundo)
 *    - Supervisão autônoma (Síndico + Xerife + fail-safe)
 *    - Preempção cooperativa (por período, sem contexto salvo)
 *    - URG-S (urgência situacional com resposta na mesma iteração)
 * 
 *  COMANDOS SERIAIS:
 * 
 *    'p' ? ativa preempção na Task1
 *    'n' ? desativa preempção
 *    'e' ? dispara preempção
 *    '2' ? tick = 2ms
 *    '4' ? tick = 4ms
 *    '8' ? tick = 8ms (padrão)
 *    '5' ? tick = 50ms
 *    'u' ? urgência na Task2
 * 
 *  FLUXO DE EXECUÇÃO:
 * 
 *    1. clock_kernel()    ? configura Timer0 (8ms)
 *    2. on_clock_kernel() ? liga interrupção do Timer0
 *    3. config_io()       ? configura I/Os
 *    4. config_int_rx_tx()? configura USART (9600bps)
 *    5. on_rtos()         ? liga interrupção global
 *    6. while(true)       ? despachador de 4 níveis
 * 
 * ============================================================================================================
 */
/*************************************************************************************************************/
//***************************************Includes**************************************************************
#include <main.h>
#include <hardware.h>
//*************************************************************************************************************
#priority int_timer0, int_rda, int_tbe   //Prioridade(PIC16F só um vetor!!!!). 
#define osc_freq 4000000                 //Frequencia do oscilador.
//*************************************************************************************************************
#define limite_buffer 1                  //Limite de dados recepcionados(Buffer circular).
#define sistema_em_pane 5                //Definição de sistema em pane operacional!!!!.
//***************************************Variáveis*************************************************************
char dado[limite_buffer];                //Guarda dados recepcionado via RB0.
char rx_error = 0;                       //Guardo caracter de erro da serial!!!
int8 index_rx = 0;                       //Indexador para buffer de rx.
int8 index_tx = 0;                       //Indexador para envio de ponteiro_texto(byte a byte).
int8 tick = 0;                           //Variável de subclock.
int8 clock = 0;                          //Variável usada em base de tempo de 1 segundo.(Variável segundo).
int8 segundo = 0;                        //Usado em tarefas com tempos maiores(Multiplos de segundo).
int8 preset_atual = preset;              //Inicializa variável com padrão de 8ms para o timer 0.
int16 semaforos = 0;                     //Flags de semaforos das tarefas.
int16 flags_urgencia = 0;                //Flags de urgência situacional (URG-S).
int16 pane_permanente = 0;               //Flags de pane permanente por tarefa.
int1 old_bt = 0;                         //Guarda histórico do botão.
int8 ponteiro_texto = 0;                 //Seletor de Mensagem.
/* Acrescente um timer para cada task */
int8 timer_task1 = 0;                    //Timers das tarefas.
int8 timer_task2 = 0;                    //       "
int8 timer_task3 = 0;                    //       "
int8 timer_task4 = 0;                    //       "
int8 timer_task5 = 0;                    //       "
int8 timer_task9 = 0;                    //       "
int8 timer_task10 = 0;                   //       "
//*************************************************************************************************************
/* Acrescente um timer de tempo de execução para cada task */
int8 timer_ex_task1 = 0;                 //Guarda tempo levado para executar tarefa!(Controle do kernel)
int8 timer_ex_task2 = 0;                 //       "
int8 timer_ex_task3 = 0;                 //       "
int8 timer_ex_task4 = 0;                 //       "
int8 timer_ex_task5 = 0;                 //       "
int8 timer_ex_task9 = 0;                 //       "
//*************************************************************************************************************
/* Variáveis para o mecanismo de preempção (CDC-P) */
int8 preempt_enabled = 0;                //0=CDC normal, 1=Preempção ativa
int8 preempt_task_id = 0;                //Qual tarefa está em modo preemptivo (0=nenhuma)
int8 preempt_original_period = 0;        //Salva o período original da tarefa preemptiva
int8 preempt_triggered = 0;              //Flag: tarefa preemptiva foi acionada por evento externo
//***************************************Variáveis*************************************************************
/* Tarefa dinamica (Acescente um para cada task)*/
int8 task1 = 2;                          //2*8ms.
int8 task2 = 4;                          //4*8ms.
int8 task3 = 2;                          //2*8ms.
int8 task4 = 60;                         //1*60s.(Base segundo)
int8 task5 = 3;                          //3*8ms.
int8 task9 = 1;                          //1*8ms.(Prioridade alta(Monitora tarefas bloqueadas!!!!))
int8 task10 = 1;                         //1*8ms.(Prioridade alta(Monitora limite de bloqueios!!!!))
//***************************************Variáveis(Sinaliza vezes bloqueadas)**********************************
/* Acrescente um contador de desbloqueio para cada task */
int8 cont_desbloqueio_task1 = 0;         //Contador de task bloqueada!!!!
int8 cont_desbloqueio_task2 = 0;         //        "
int8 cont_desbloqueio_task3 = 0;         //        "
int8 cont_desbloqueio_task4 = 0;         //        "
int8 cont_desbloqueio_task5 = 0;         //        "
int8 cont_desbloqueio_task9 = 0;         //        "
//*************************************************************************************************************
int8 _segundo = 125;                     //125*8ms = 1s.
const char texto1[] = {'B','T',' ','P','r','e','s','s','i','o','n','a','d','o','!',10,13};
const char texto2[] = {'C','D','C','-','P',' ','P','I','C','1','6','F','6','2','8',10,13};
//***************************************Flags semaforo********************************************************
/* Acrescente um flag de semáforo para cada task (Menos o Xerife (Task10))*/
#bit sema_task1 = semaforos.1            //
#bit sema_task2 = semaforos.2            //
#bit sema_task3 = semaforos.3            //
#bit sema_task4 = semaforos.4            //
#bit sema_task5 = semaforos.5            //
#bit sema_task9 = semaforos.9            //
//***************************************Flags de Urgência Situacional (URG-S)*********************************
/* Acrescente um flag de urgência para cada task (Menos o Xerife e Síndico (Task10 e 9))*/
#bit urg_task1 = flags_urgencia.1        //Flag de urgência para Task1
#bit urg_task2 = flags_urgencia.2        //Flag de urgência para Task2
#bit urg_task3 = flags_urgencia.3        //Flag de urgência para Task3
#bit urg_task4 = flags_urgencia.4        //Flag de urgência para Task4
#bit urg_task5 = flags_urgencia.5        //Flag de urgência para Task5
//***************************************Flags de pane permanente por tarefa***********************************
/* Acrescente um flag de pânico para cada task (Menos o Xerife e Síndico (Task10 e 9))*/
#bit pane_task1 = pane_permanente.1      //Flag pane permanente
#bit pane_task2 = pane_permanente.2      //        "
#bit pane_task3 = pane_permanente.3      //        "
#bit pane_task4 = pane_permanente.4      //        "
#bit pane_task5 = pane_permanente.5      //        "
//*************************************************************************************************************
/* Acrescente tempo máximo de execução para cada task (Menos Xerife (task10))*/
#define tempo_maximo_task1 3             //        "
#define tempo_maximo_task2 5             //        "
#define tempo_maximo_task3 3             //        "
#define tempo_maximo_task4 61            //        "
#define tempo_maximo_task5 4             //        "
#define tempo_maximo_task9 2             //        " 
//***************************************Protótipos das Funções de Preempção e Urgência************************
/* Funções do kenel do CDC-P(Não alterar)*/
void CDC_EnablePreempt(int8 task_id);
void CDC_DisablePreempt(void);
void CDC_TriggerPreempt(void);
int1 CDC_IsPreemptActive(void);
int8 CDC_GetPreemptTask(void);
void ajustar_tick(int8 novo_tick_ms);
void set_urgent(int8 task_id);           //NOVO: Seta flag de urgência situacional
void clear_all_urgency(void);            //NOVO: Limpa todas as flags de urgência
//***************************************Config clock kernel***************************************************
/*
Carrega timer com base de tempo para 8ms.
(Clock MCU 4Mhz/4) = 1Mhz = 1/1Mhz = 1us(Tempo de cada bit incrementado).
(256-6*1us*32 = 8ms).
(preset = 6).
*/
/* Ajusta o tick para 8ms na inicialização(Padrão) */
void clock_kernel(void){
     psa = 0;ps0 = 0;ps1 = 0;ps2 = 1;t0cs = 0;timer0 = preset_atual;
}
//***************************************Liga interrupção clock kernel*****************************************
/*
Liga interrupção do timer0.
*/
/* Aqui é ligado o coração do sistema(Tick) */
void on_clock_kernel(void){
     t0ie = 1;    
}
//***************************************I/Os config***********************************************************
/*
Ajusta a direção dos i/os utilizados no exemplo
*/
/* Ajustar aqui os I/Os utilizados na aplicação */
void config_io(void){
     porta = 0x00;portb = 0x00; //Evita acionamento indesejado!!!(Formata latch de saída antes de direciona-lo)
     d_clk_timer0 = 0;d_led1 = 0;d_led2 = 0;d_tempo_despachador = 0;d_tx = 0;d_rx = 1;d_monitor_tasks = 0;
     monitor_tasks = 1;d_bt = 1;old_bt = bt;d_led3 = 0;
}
//***************************************Config interrupção TX RX**********************************************
/*
Ajusta interrupção TX e RX em 9600bps.
Ligo interrupção de TX e RX.
*/
/* O hardware de tx e rx(Parâmetros) são ajustados aqui) */
void config_int_rx_tx(void){
     spbrg = (osc_freq/(16*(9600+1))) - 1;
     spen = 1;brgh = 1;sync = 0;cren = 1;
     peie = 1;rcie = 1;txen = 0;txie = 1;
}
//***************************************Liga RTOS*************************************************************
/*
Ligo interrupção geral ("Starta" rtos).
*/
/* Aqui é onde o coração do kernel passa a bater */
void on_rtos(void){
     gie = 1;
}
//***************************************Tarefas como Funções Separadas****************************************
/* Primeira tarefa de cunho didático */
void task1_func(void)
{
   led1 = !led1;
   //delay_ms(16);
/* Ajuste dinâmico da task(Obrigatório) */   
   timer_ex_task1 = (tick - timer_task1)/task1;   
   if(timer_ex_task1){task1 = task1+1;}   
   if(task1 >= tempo_maximo_task1){sema_task1 = 1;}   
}
/* Segunda tarefa de cunho didático */
void task2_func(void)
{
   led2 = !led2;
  //delay_ms(32);
/* Ajuste dinâmico da task(Obrigatório) */   
   timer_ex_task2 = (tick - timer_task2)/task2;   
   if(timer_ex_task2){task2 = task2+1;}  
   if(task2 >= tempo_maximo_task2){sema_task2 = 1;}   
}
/* Terceira tarefa de cunho didático */
void task3_func(void)
{
   //delay_ms(16);
   if(dado[0] == 'p'&& CDC_IsPreemptActive() == 0) { CDC_EnablePreempt(1);dado[0] = 0; }
   if(dado[0] == 'n'&& CDC_IsPreemptActive() == 1) { CDC_DisablePreempt();dado[0] = 0; }
   if(dado[0] == 'e') { CDC_TriggerPreempt();dado[0] = 0; }
   if(dado[0] == '2') { ajustar_tick(2);dado[0] = 0; }
   if(dado[0] == '4') { ajustar_tick(4);dado[0] = 0; }
   if(dado[0] == '8') { ajustar_tick(8);dado[0] = 0; }
   if(dado[0] == '5') { ajustar_tick(50);dado[0] = 0; } 
/* Ajuste dinâmico da task(Obrigatório) */   
   timer_ex_task3 = (tick - timer_task3)/task3;   
   if(timer_ex_task3){task3 = task3+1;}  
   if(task3 >= tempo_maximo_task3){sema_task3 = 1;}   
}
/* Quarta tarefa de cunho didático */
void task4_func(void)
{   
   ponteiro_texto = 2;
   txen = 1;
/* Ajuste dinâmico da task(Obrigatório) */   
   timer_ex_task4 = (segundo - timer_task4)/task4;   
   if(timer_ex_task4){task4 = task4+1;}  
   if(task4 >= tempo_maximo_task4){sema_task4 = 1;}   
}
/* Quinta tarefa de cunho didático */
void task5_func(void)
{   
   if(bt == false && old_bt == true){
              old_bt = bt;
              led3 = !led3;
              ponteiro_texto = 1;
              txen = 1;
              }
   else{old_bt = bt;}
/* Ajuste dinâmico da task(Obrigatório) */   
   timer_ex_task5 = (tick - timer_task5)/task5;   
   if(timer_ex_task5){task5 = task5+1;}  
   if(task5 >= tempo_maximo_task5){sema_task5 = 1;}   
}
/* Nona tarefa (Síndico)Obrigatória) */
void task9_func(void)
{   
   if(sema_task1 && !pane_task1){task1 -= 1;sema_task1 = 0;cont_desbloqueio_task1++;}
   if(sema_task2 && !pane_task2){task2 -= 1;sema_task2 = 0;cont_desbloqueio_task2++;}
   if(sema_task3 && !pane_task3){task3 -= 1;sema_task3 = 0;cont_desbloqueio_task3++;}
   if(sema_task4 && !pane_task4){task4 -= 1;sema_task4 = 0;cont_desbloqueio_task4++;}
   if(sema_task5 && !pane_task5){task5 -= 1;sema_task5 = 0;cont_desbloqueio_task5++;}
//Demais tasks criadas entram aqui!!!!   
   if(sema_task9){task9 -= 1;sema_task9 = 0;cont_desbloqueio_task9++;}
/* Ajuste dinâmico da task(Obrigatório) */    
   timer_ex_task9 = (tick - timer_task9)/task9;
/* Última linha de defesa do sistema(Reset total)*/   
   if(task9 >= tempo_maximo_task9){      
      reset_cpu();/* Se tarefa gestora de bloqueio(Síndico) falhar reseta cpu!!!!*/      
   }   
}
/* Decima tarefa (Xerife)Obrigatória) */
void task10_func(void)
{   
   if(cont_desbloqueio_task1 >= sistema_em_pane){monitor_tasks = 0;sema_task1 = 1;pane_task1 = 1;}
   if(cont_desbloqueio_task2 >= sistema_em_pane){monitor_tasks = 0;sema_task2 = 1;pane_task2 = 1;}
   if(cont_desbloqueio_task3 >= sistema_em_pane){monitor_tasks = 0;sema_task3 = 1;pane_task3 = 1;}
   if(cont_desbloqueio_task4 >= sistema_em_pane){monitor_tasks = 0;sema_task4 = 1;pane_task4 = 1;}
   if(cont_desbloqueio_task5 >= sistema_em_pane){monitor_tasks = 0;sema_task5 = 1;pane_task5 = 1;}
//Demais tasks criadas entram aqui!!!!   
} 

//***************************************Funções de Preempção (CDC-P)******************************************
void CDC_EnablePreempt(int8 task_id)
{
   if(preempt_enabled == 0)              //Só ativa se não houver outra preempção ativa   
   {
      // Verifica o flag de pane para cada tarefa
      if(task_id == 1 && pane_task1) return;
      if(task_id == 2 && pane_task2) return;
      if(task_id == 3 && pane_task3) return;
      if(task_id == 4 && pane_task4) return;
      if(task_id == 5 && pane_task5) return;
//Demais tasks criadas entram aqui!!!!      
      preempt_enabled = 1;               //Habilita o modo preemptivo
      preempt_task_id = task_id;         //Registra qual tarefa é a preemptiva
      
      /* Salva o período original e reduz para 1 tick (ifs sequenciais) */
      if(task_id == 1) { preempt_original_period = task1; task1 = 1; }
      if(task_id == 2) { preempt_original_period = task2; task2 = 1; }
      if(task_id == 3) { preempt_original_period = task3; task3 = 1; }
      if(task_id == 4) { preempt_original_period = task4; task4 = 1; }
      if(task_id == 5) { preempt_original_period = task5; task5 = 1; }
//Demais tasks criadas entram aqui!!!!      
   }
}
//***************************************Função para desabilitar preempção se ativa(por tarefa))***************
void CDC_DisablePreempt(void)
{
   if(preempt_enabled == 1)              //Só desativa se houver preempção ativa
   {
      /* Restaura o período original da tarefa (ifs sequenciais) */
      if(preempt_task_id == 1) { task1 = preempt_original_period; }
      if(preempt_task_id == 2) { task2 = preempt_original_period; }
      if(preempt_task_id == 3) { task3 = preempt_original_period; }
      if(preempt_task_id == 4) { task4 = preempt_original_period; }
      if(preempt_task_id == 5) { task5 = preempt_original_period; }
//Demais tasks criadas entram aqui!!!!      
      preempt_enabled = 0;               //Desabilita o modo preemptivo
      preempt_task_id = 0;               //Limpa o registro
      preempt_original_period = 0;       //Limpa o backup
   }
}
//***************************************Função(Urgência de preempção(Evento externo))*************************
void CDC_TriggerPreempt(void)
{
   if(preempt_enabled == 1)              //Só dispara se preempção estiver ativa
   {
   
      // Verifica se a tarefa preemptiva NÃO entrou em pane
      if(preempt_task_id == 1 && pane_task1) return;
      if(preempt_task_id == 2 && pane_task2) return; 
      if(preempt_task_id == 3 && pane_task3) return;
      if(preempt_task_id == 4 && pane_task4) return;
      if(preempt_task_id == 5 && pane_task5) return;
//Demais tasks criadas entram aqui!!!!      
      preempt_triggered = 1;             //Seta a flag de urgência(Prioridade na fila)
   }
}
//***************************************Função (Existe preempção ativa?)**************************************
int1 CDC_IsPreemptActive(void)
{
   return preempt_enabled;               //Retorna 0 ou 1
}
//***************************************Função (Qual preempção está ativa)************************************
int8 CDC_GetPreemptTask(void)
{
   return preempt_task_id;               //Retorna ID da task
}
//***************************************Funções de Urgência Situacional (URG-S)*******************************
void set_urgent(int8 task_id)
{
   gie = 0;
   if(task_id == 1) { urg_task1 = 1; }
   if(task_id == 2) { urg_task2 = 1; }
   if(task_id == 3) { urg_task3 = 1; }
   if(task_id == 4) { urg_task4 = 1; }
   if(task_id == 5) { urg_task5 = 1; }
//Demais tasks criadas entram aqui!!!!   
   gie = 1;
}
void set_urgent_isr(int8 task_id)
{
   if(task_id == 1) { urg_task1 = 1; }
   if(task_id == 2) { urg_task2 = 1; }
   if(task_id == 3) { urg_task3 = 1; }
   if(task_id == 4) { urg_task4 = 1; }
   if(task_id == 5) { urg_task5 = 1; }
//Demais tasks criadas entram aqui!!!!   
}
void clear_all_urgency(void)
{
   gie = 0;
   flags_urgencia = 0;
   gie = 1;
}

//***************************************Função de ajuste dinâmico do tick(Em tempo de execução)***************
void ajustar_tick(int8 novo_tick_ms) {
    if(novo_tick_ms == 2) {
        // Prescaler 1:8 (ps2=0, ps1=1, ps0=0)
        // (256 - 6) * 1us * 8 = 250 * 8us = 2000us = 2ms ?
        psa = 0; ps0 = 0; ps1 = 1; ps2 = 0;  // Prescaler 1:8
        t0cs = 0;
        preset_atual = 6;
        timer0 = preset_atual;
        _segundo = 500;                      // 500 * 2ms = 1s ?
    } 
    else if(novo_tick_ms == 4) {
        // Prescaler 1:16 (ps2=0, ps1=1, ps0=1)
        // (256 - 6) * 1us * 16 = 250 * 16us = 4000us = 4ms ?
        psa = 0; ps0 = 1; ps1 = 1; ps2 = 0;  // Prescaler 1:16
        t0cs = 0;
        preset_atual = 6;
        timer0 = preset_atual;
        _segundo = 250;                      // 250 * 4ms = 1s ?
    } 
    else if(novo_tick_ms == 8) {
        // Prescaler 1:32 (ps2=1, ps1=0, ps0=0) - ORIGINAL
        // (256 - 6) * 1us * 32 = 250 * 32us = 8000us = 8ms ?
        psa = 0; ps0 = 0; ps1 = 0; ps2 = 1;  // Prescaler 1:32
        t0cs = 0;
        preset_atual = 6;
        timer0 = preset_atual;
        _segundo = 125;                      // 125 * 8ms = 1s ?
    } 
    else if(novo_tick_ms == 50) {
        // Prescaler 1:256 (ps2=1, ps1=1, ps0=1)
        // (256 - 61) * 1us * 256 = 195 * 256us = 49920us ? 50ms ?
        psa = 0; ps0 = 1; ps1 = 1; ps2 = 1;  // Prescaler 1:256
        t0cs = 0;
        preset_atual = 61;
        timer0 = preset_atual;
        _segundo = 20;                       // 20 * 50ms = 1s ?
    }
}
//***************************************Vetor de interrupção do timer0****************************************
/* Vetor de interrupção(Aqui o tick é incrementado) */
#int_timer0
void isr_timer0(void)
{
  clk_timer0 = !clk_timer0;              //Externiza clock do kernel.
  /*Usado no kernel */
  ++tick;
  /*Usado para base de tempo de 1 segundo*/
  ++clock;
  if(clock == _segundo){segundo++;clock = 0;}
  timer0 = preset_atual;                //Ajusta timer 0 corretamente. 
}
//***************************************Interrupção RX********************************************************
/* Vetor de recepção(RX).Os dados da serial são recepcionados aqui!! */
#int_rda
void isr_rs232rx(){
     if (oerr || ferr){cren = 0;cren = 1;rx_error = rcreg;index_rx = 0; return;}
     dado[index_rx++] = rcreg;           //Captura dado via rs232(9600bps).
     if (index_rx == limite_buffer){
         index_rx = 0;
     }
/* O carecter u dispara urgência da task2 para teste (Didático) */     
     if(dado[0] == 'u') { set_urgent_isr(2); dado[0] = 0; }     
}
//**************************************Interrupção TX*********************************************************
/* Vetor de transmissão(TX).Os bytes são transmitidos aqui!!*/
#int_tbe  
void isr_rs232tx()
{ 
  if(ponteiro_texto == 1){  
  txreg = texto1[index_tx++];   
  if (index_tx == sizeof texto1){index_tx = 0;ponteiro_texto = 0;txen = 0;}  
  }
  else if(ponteiro_texto == 2){
  txreg = texto2[index_tx++];   
  if (index_tx == sizeof texto2){index_tx = 0;ponteiro_texto = 0;txen = 0;}
  }else{
  ponteiro_texto = 0;
  txen = 0;
  }
}
//***************************************Principal(Main)*******************************************************
void main(void)
{   
//***********Configura inicialização******
   clock_kernel();               //Ajusta clock do kernel(Inicialização)
   on_clock_kernel();            //Liga interrupção do timer(Coração do kernel)
   config_io();                  //Configura I/Os utilizados na aplicação(Mundo externo)
   config_int_rx_tx();           //COnfigura hardware de TX e RX  
   on_rtos();                    //Ligo interrupção geral(Timer começa a gerar tick(Coração))
//***********Laço infinito****************   
        while(true){        
        tempo_despachador = !tempo_despachador;      //Sinalizo tempo de passagem pelo despachador!!! 
//* Tratamento de urgência vai primeiro no despachador */        
        if(flags_urgencia != 0){/* Só executa se houver urgência */
        /* URG-S: Task1 (Alta prioridade situacional) */
           if(urg_task1 && (!sema_task1)&&(!pane_task1)){
              urg_task1 = 0;                    //Limpa a flag IMEDIATAMENTE
              timer_task1 = tick;
              timer_ex_task1 = 0;
              task1_func();
           }
        
        /* URG-S: Task2 */
           if(urg_task2 && (!sema_task2)&&(!pane_task2)){
             urg_task2 = 0;
             timer_task2 = tick;
             timer_ex_task2 = 0;
             task2_func();
             }
        /* URG-S: Task3 */
           if(urg_task3 && (!sema_task3)&&(!pane_task3)){
             urg_task3 = 0;
             timer_task3 = tick;
             timer_ex_task3 = 0;
             task3_func();
             }
        /* URG-S: Task4 */
           if(urg_task4 && (!sema_task4)&&(!pane_task4)){
             urg_task4 = 0;
             timer_task4 = segundo;
             timer_ex_task4 = 0;
             task4_func();
             }
        /* URG-S: Task5 */
           if(urg_task5 && (!sema_task5)&&(!pane_task5)){
             urg_task5 = 0;
             timer_task5 = tick;
             timer_ex_task5 = 0;
             task5_func();
             }     
//Demais tasks criadas entram aqui!!!!             
        }
/**********************************************/
/* Preempção CDC-P vai em segundo no despachador */
        if(preempt_triggered && preempt_task_id > 0)
        {
            preempt_triggered = 0;         
            if(preempt_task_id == 1 && !pane_task1) {timer_task1 = tick; task1_func();}
            if(preempt_task_id == 2 && !pane_task2) {timer_task2 = tick; task2_func();}
            if(preempt_task_id == 3 && !pane_task3) {timer_task3 = tick; task3_func();}
            if(preempt_task_id == 4 && !pane_task4) {timer_task4 = tick; task4_func();}
            if(preempt_task_id == 5 && !pane_task5) {timer_task5 = tick; task5_func();}
//Demais tasks criadas entram aqui!!!!            
        }
/* Tarefa xerife é testada em primeiro lugar no despachador */        
        /*Task 10(Ninguem me bloqueia(Xerife!!!))*/
        if((tick - timer_task10) >= task10){
           timer_task10 = tick;                      
           task10_func();  // ? Agora é uma chamada de função!
        }
/* Tarefa Síndico é testada em segundo lugar no despachador */        
        /*Task 9 (Síndico de tarefas bloqueadas)*/
        if((tick - timer_task9) >= task9 &&(!sema_task9)){
           timer_task9 = tick;
           timer_ex_task9 = 0;           
           task9_func();  // ? Agora é uma chamada de função!
        }
/* Demais tarefas aqui em ordem de prioridade espacial */        
        /*Task 1*/
        if(((tick - timer_task1) >= task1)&&(!sema_task1)) {
           timer_task1 = tick;
           timer_ex_task1 = 0;           
           task1_func();  // ? Agora é uma chamada de função!
        }
        
        /*Task 2*/
        if((tick - timer_task2) >= task2 &&(!sema_task2)){
           timer_task2 = tick;
           timer_ex_task2 = 0;           
           task2_func();  // ? Agora é uma chamada de função!
        }
        /*Task 3*/
        if((tick - timer_task3) >= task3 &&(!sema_task3)){
           timer_task3 = tick;
           timer_ex_task3 = 0;           
           task3_func();  // ? Agora é uma chamada de função!
        }
        /*Task 4*/
        if((segundo - timer_task4) >= task4 &&(!sema_task4)){
           timer_task4 = segundo;
           timer_ex_task4 = 0;           
           task4_func();  // ? Agora é uma chamada de função!
        }
        /*Task 5*/
        if((tick - timer_task5) >= task5 &&(!sema_task5)){
           timer_task5 = tick;
           timer_ex_task5 = 0;           
           task5_func();  // ? Agora é uma chamada de função!
        }
//Demais tasks criadas entram aqui!!!!        
     }
//**********Fim do laço infinito**********   
}
//*************************************Fim da aplicação********************************************************
