#include "HW_INIT.h"
#include <avr/io.h>




void SetupTimer(){
/*

 TCCR1A (timer/counter control register a)
 7      6       5       4       3       2       1       0
 COM1A1 COM1A0  COM1B1  COM1B0  -       -     WGM11     WGM10
 0      0       0       0       0       0       0       0 --> setup mask



 TCCR1B (timer/counter control register b)
 7      6       5       4       3       2       1       0
 ICNC1 ICES1    -     WGM13   WGM12    CS12    CS11    CS10
 0      0       0       0       0       1       0       0

 előosztás: 1 0 0

 */
 TCCR1B |= (1<<CS12);
 /*
 TIMSK1 (timer/counter interrupt mask register)


 7      6       5       4       3       2       1       0
 -      -     ICIE1     -       -     OCIE1B  OCIE1A  TOIE1
 0      0       1       0       0       0       0       0
 (icie1 input capture interrupt enable, )
 */
 TIMSK1 |= (1<<ICIE1);
 /*
 ICR1 --> csak olvasni, itt van az input capture értéke


--------------------------------------------------------------
16bit számláló max érték = 65536
Fclk = 16Mhz = 16 000 000 Hz
prescale = 265
--> 16000000/265 = 62500Hz --> egy másodperc alatt 62500-at lép a timer
1 timer lépés = 0,000016s = 0.016 milisec = 16 mikrosec

Tmax = 65536 x 0.000016s = 1.048576s

Valós idő kiszámítása timer értékből

Idő(milisec) = timerérték x 0.016
 TCCR1C (timer/counter control register c)

 // Tegyük fel, hogy az algoritmusod kiszámolta a két tüske közötti különbséget:
 uint16_t timer_lepesek = 10416; // (Ez csak egy példa érték)

 // 1. Lépés: Átváltás mikroszekundumba (Biztonságos, 32-bites egész számmal)
 // A 32 bit (uint32_t) azért kell, mert 65535 * 16 már nem férne el 16 biten!
 uint32_t eltelt_ido_us = (uint32_t)timer_lepesek * 16;

 // 2. Lépés: Átváltás milliszekundumba (Itt már jöhet a tört szám, azaz a float)
 float eltelt_ido_ms = (float)eltelt_ido_us / 1000.0;

 // Ezt az értéket már ki is küldheted a Serial Monitorra!
 // Ki fogja írni: "166.65 ms"
 --------------------------------------------------------------
 */
}


void SetupComp(){

/*
ADCSRB --> default 0

ACSR (analog comparator control and status register)
7       6       5       4       3       2       1       0
ACD   ACBG     ACO     ACI     ACIE    ACIC   ACIS1   ACIS0
0       0       N/A     0       1       1       1       0
ACD --> default 0 viszont a süketítéshez ezt kell majd használni talán

acis1/0 =10 --> lefutó él

*/


ACSR |= (1<<ACIC);
ACSR |= (1<<ACIS1);

/*


DIDR1 (digital input disable register)

7       6       5       4       3       2       1       0
-       -       -       -       -       -     AIN1D   AIN0D
0       0       0       0       0       0       1       1

(letiltja a digitális bemenetet és csak analog jelet dolgozza fel a pin)
 */

 DIDR1 |= (1<<AIN1D);
 DIDR1 |= (1<<AIN0D);
}
