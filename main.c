#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include "LCD_I2C.h"
#include "HW_INIT.h"

// globalis allapot jelzok megszakitashoz
volatile uint16_t SpikeTimerValue;
volatile bool SpikeFlag = false;


//puffer adatok
uint16_t puffer[8] = {0,0,0,0,0,0,0,0};
uint8_t puffer_index = 0;
uint32_t running_sum = 0;
bool PufferIsFull = false;


//statisztikai adatok
uint16_t Avarage_deltaT;
uint16_t Bph;




// SYNC fazis flages es valtozok
bool SYNC_Starting = true;
uint16_t SYNC_previousT;
//uint16_t SYNC_presentT;
// lokalis valtozo az ido atmeneti tarolasara
uint16_t SYNC_localT;
uint16_t SYNC_deltaT;




typedef enum{
    SYNC = 1,
    /*
     feladat: megtalalni a legelso tiszta tusket a zajban
     ha jon egy tuske meg kell nezni hogy mennyi ido telt el az elozo es az uj ota
     ha tobb mint 80ms akkor valoszinuleg egy uj valid tuske nem pedig egy viszhang vagy egy random zaj
     ha validnak talalta az algoritmus akkor a state BLANK_PERIOD LESZ
     */
    BLANK_PERIOD,
    PROCESSSING,
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
                            GlobalState = PROCESSSING;
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
            case PROCESSSING:

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

                    // statisztikai adatok
                    Avarage_deltaT = running_sum >> 3;
                    Bph  = 225000000UL / Avarage_deltaT;

                    // TODO lcd, uart kommunikacio

                }

                GlobalState = BLANK_PERIOD;
                break;

        }
    }
}
