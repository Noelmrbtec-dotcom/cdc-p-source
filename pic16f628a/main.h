#include <16F628A.h>
#FUSES NOWDT                    //No Watch Dog Timer
#FUSES NOPROTECT
#FUSES XT
#use delay(clock=4000000)
#use rs232(baud=9600, xmit=pin_B2,rcv=pin_B1)




