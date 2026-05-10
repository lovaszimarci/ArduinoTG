#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <util/delay.h>
#include "LCD_I2C.h"
#include "HW_INIT.h"
#include "UART.h"


// globalis allapot jelzok megszakitashoz
volatile uint16_t SpikeTimerValue;
volatile bool SpikeFlag = false;


//puffer adatok
uint16_t puffer[8] = {0,0,0,0,0,0,0,0};
uint8_t puffer_index = 0;
uint32_t running_sum = 0;
bool PufferIsFull = false;


//statisztikai adatok
// 12 utesenkent kuldi ki a program az lcd-re az infot
#define AVERAGE_COUNT 12
// volt e mar delta t ertek
uint8_t tick_count = 0;
// beat errorhoz hasznalt delta t ertek
uint32_t Previous_deltaT = 0;
// a meresbol szamolt atlag elteres timer lepesben
uint32_t Average_deltaT;
uint32_t Average_deltaT_us;
// a user altal megadott referencia bph ertek
uint16_t ReferenceBph;
// a referencia bph ertekbol kapunk egy referencia delta t erteket
float Reference_deltaT_us;
// az elteresek osszege
float rate_sum =0.0;
// a beat errorok osszege
float beat_error_sum = 0.0;









// SYNC fazis flages es valtozok
bool SYNC_Starting = true;
uint16_t SYNC_previousT;
//uint16_t SYNC_presentT;
// lokalis valtozo az ido atmeneti tarolasara
uint16_t SYNC_localT;
uint16_t SYNC_deltaT;




typedef enum{
    SYNC = 1,
    BLANK_PERIOD,
    PROCESSING,
} State;

State GlobalState = SYNC;



ISR(TIMER1_CAPT_vect){
    //timer érték mentése
    SpikeTimerValue = ICR1;
    // új tüske jelzése, tüske utáni jelérzékelés tiltásának állapota
    SpikeFlag = true;
    // timer megszakitas tiltasa
    TIMSK1 &= ~(1 << ICIE1);
}




int main(){
    //hw setup
    I2C_init();
    LCD_Init();
    UART_Init();
    ReferenceBph = UART_AskForBph();
    // cel elteres referencia bph alapjan
    // Ha a Target_BPH = 21600, akkor 1 oraban (3600 mp -> 3.600.000.000 us) van ennyi ütés.
    Reference_deltaT_us = 3600000000.0 / ReferenceBph;

    SetupTimer();
    SetupComp();





    while(true){
        switch(GlobalState){
///////////////////////////////////////////////////////////////////////////
            case SYNC:
                //ha van uj tuske
                if(SpikeFlag){
                    cli();
                    // timer lokalis valtozoba pakolasa
                    SYNC_localT = SpikeTimerValue;
                    sei();

                    // uj tuske jelzeseneke hamisba allitasa
                    SpikeFlag = false;

                    if(SYNC_Starting){
                        //init fazisban van a rendszer, meg nem tud deltaT-t szamolni
                        SYNC_previousT = SYNC_localT;
                        SYNC_Starting  = false;
                        GlobalState = BLANK_PERIOD;
                    }
                    else{
                        // ket utes kozotti ido - meg nem biztos hogy valid utes
                        SYNC_deltaT = SYNC_localT - SYNC_previousT;

                        //deltaT validalas
                        // ora delta ido ms ben
                        // min: 80ms max: 320ms - ezekre mar ra lett szamitva hibahatar
                        // 1 timer lepes 0.016 ms
                        // 80ms = 5000 timer lepes
                        // 320ms = 20000 timer lepes


                        if(SYNC_deltaT > 5000 && SYNC_deltaT < 20000){
                            //todo korpuffer vagy feldolgozo fazisnak tovabbitani deltaT erteket
                            SYNC_previousT = SYNC_localT;
                            GlobalState = PROCESSING;
                        }
                        else{
                            GlobalState = BLANK_PERIOD;
                        }
                    }
                }
                break;
////////////////////////////////////////////////////////////////////////////
            case BLANK_PERIOD:

            //suket fazis elejenek ideje
            cli();
            uint16_t CurrentTime = TCNT1;
            sei();

            uint16_t PassedTime = CurrentTime - SYNC_previousT;

            // 60ms =  3750 timer step
            if(PassedTime > 3750){
                SpikeFlag = false;
                // input capture flag torles
                TIFR1 = (1<<ICF1);
                //timer it ujra engedelyezes
                TIMSK1 |= (1<<ICIE1);

                GlobalState = SYNC;
            }
            break;

////////////////////////////////////////////////////////////////////////////
            case PROCESSING:

                //puffer init fazis
                if(PufferIsFull == false){
                    puffer[puffer_index] = SYNC_deltaT;
                    running_sum += SYNC_deltaT;
                    puffer_index += 1;
                    if(puffer_index == 8){
                        puffer_index = 0;
                        PufferIsFull = true;
                    }
                }
                else{

                    // korpuffer frissitese
                    running_sum -= puffer[puffer_index];
                    puffer[puffer_index] = SYNC_deltaT;
                    running_sum += SYNC_deltaT;
                    puffer_index += 1;
                    if(puffer_index == 8){
                        puffer_index = 0;
                    }

                    //===========================================
                    // NAPI ELTERES SZAMITAS
                    //===========================================
                    // atlag delta t
                    Average_deltaT = running_sum >> 3;
                    Average_deltaT_us = Average_deltaT * 16;

                    //napi atlag elteres:
                    // ((gyari referencia idokoz - sajat atlagolt idokoz)/ gyari referencia idokoz) * egy nap masodpercekben
                    float Current_rate = ((Reference_deltaT_us - Average_deltaT_us)/Reference_deltaT_us)*86400.0;

                    rate_sum += Current_rate;
                    tick_count++;



                    if(tick_count >= AVERAGE_COUNT){

                        //==========================================
                        // BEAT ERROR SZAMITAS
                        //==========================================
                        for(int i = 1; i < 8;i++ ){
                            uint32_t diff_ticks = (puffer[i] > puffer[i-1]) ?
                                            (puffer[i] - puffer[i-1]) :
                                            (puffer[i-1] - puffer[i]);

                            float diff_ms = diff_ticks * 0.016;
                            beat_error_sum += diff_ms;
                        }


                        float Average_rate = rate_sum / AVERAGE_COUNT;
                        float Average_beat_error = (beat_error_sum / 7.0);

                        char line1[17];
                        char line2[17];

                        char rate_str[10];
                        dtostrf(Average_rate, 4, 1, rate_str);

                        char beat_error_str[10];
                        dtostrf(Average_beat_error, 3, 1, beat_error_str);

                        char plusminus = (Average_rate >= 0) ? '+' : '\0';
                        sprintf(line1, "Rate:%c%s s/d", plusminus, rate_str);

                        sprintf(line2, "B.err: %s ms", beat_error_str);



                        //LCD kiiras

                        LCD_SetCursor(0, 0);
                        LCD_PrintString(line1);
                        LCD_SetCursor(1, 0);
                        LCD_PrintString(line2);

                        //nullazas
                        tick_count = 0;
                        rate_sum = 0.0;
                        beat_error_sum = 0.0;

                    }

                }

                GlobalState = BLANK_PERIOD;
                break;

        }
    }
}
