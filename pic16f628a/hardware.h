//**************************************Macros*****************************************************************
#define preset 6                         //Pré carga do timer para 8ms.
//**************************************Definição do hardware(I/Os)********************************************
#byte porta = 0x05                       //Endereço do port em SFRs.
#byte trisa = 0x85                       //Endereço de direção do port A em SFRs.
#byte portb = 0x06                       //Endereço do port em SFRs.
#byte trisb = 0x86                       //Endereço de direção do port B em SFRs.
#bit d_led1 = trisa.0                    //Direção do pino do led.
#bit led1 = porta.0                      //Led de sinalização ligado no portb pino 1.
#bit d_rx = portb.1                      //Pino de entrada serial por hardware.
#bit d_tx = portb.2                      //Pino de saída serial por hardware.
#bit d_led2 = trisb.0                    //Direção do pino do led.
#bit led2 = portb.0                      //Led de sinalização ligado no portb pino 2.
#bit d_clk_timer0 = trisa.2              //Direção do pino de sinalização de interrupção do timer0.
#bit clk_timer0 = porta.2                //Led de sinalização ligado no porta pino 2.
#bit d_bt = trisa.1                      //Direção da entrada do botão.
#bit bt = porta.1                        //Pino conectado no botão com pullup de 10k.
#bit d_led3 = trisa.3                    //Direção de sinalização de botão precionado.
#bit led3 = porta.3                      //Led de sinalização de botão precionado.
//#bit  d_ext_int0 = trisb.0               //Direção do pino de interrupção externa.
#bit d_tempo_despachador = trisb.6       //Direção do pino que informa tempo do despachador.
#bit tempo_despachador = portb.6         //Sinalizo tempo do despachador.
#bit d_monitor_tasks = trisb.7           //Direção do pino para monitorar task9!!!!(Reset cpu).
#bit monitor_tasks = portb.7             //Pino para monitorar reset por pane temporal!!!!!
//***************************************Usado para configuração da (RS232)************************************
#byte spbrg = 0x99                       //Registros envolvidos na serial (Vide datasheet).
#byte rcsta = 0x18                       //    "
#bit spen = rcsta.7                      //    "
#bit ferr = rcsta.2                      //    "
#bit oerr = rcsta.1                      //    "
#bit cren = rcsta.4                      //    "
#byte rcreg = 0x1A                       //    "
#byte txreg = 0x19                       //    "
#byte pie1 = 0x8C                        //    "
#bit txie = pie1.4                       //    "
#byte pir1 = 0x0C                        //    "
#bit rxif = pir1.5                       //    "
#bit txif = pir1.4                       //    "
#bit rcie = pie1.5                       //    "
#byte txsta = 0x98                       //    "
#bit brgh = txsta.2                      //    "
#bit sync = txsta.4                      //    "
#bit txen = txsta.5                      //    "
#bit trmt = txsta.1                      //    "
//***************************************Controle geral interrupção********************************************
#byte intcon = 0x0B                      //Endereço do registro de controle de interrupçao em SFRs.
#byte option_reg = 0x81                  //Endereço do registro de opções de controle em SFRs.
#bit peie = intcon.6                     //Endereço do registro de opções de controle em SFRs.
#bit gie = intcon.7                      //Chave geral de interrupções.
//***************************************Usado para configurar timer0******************************************
#byte timer0 = 0x01                      //Endereço do timer 0 em SFRs.
#bit t0ie = intcon.5                     //Define bit de abilitação da interrupção do timer 0.
#bit t0cs = option_reg.5                 //Define se timer será incrementado interna ou externamente.
#bit ps0 = option_reg.0                  //Configura fator de divisão.(Prescaler)
#bit ps1 = option_reg.1                  //Configura fator de divisão.(Prescaler)
#bit ps2 = option_reg.2                  //Configura fator de divisão.(Prescaler)
#bit psa = option_reg.3                  //Bit de atribuição do Prescaler (timer0).
//***************************************Interrupção Externa****************************************************
//#bit  inte = intcon.4                    //Abilita interruoção externa.
//#bit  intedg = option_reg.6              //Borda da interrupção.
//**************************************************************************************************************
