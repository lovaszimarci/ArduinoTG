#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdbool.h>
#define avarage_size 8

volatile uint32_t old_delta_t;
volatile uint32_t new_delta_t;
volatile uint8_t is_new_data;

// init function
//timer input capture interrupt rutin
// timer owerflow rutin
// while loop


//      moving avarage --> circle puffer
uint32_t delta_t_values[avarage_size];
uint8_t array_index = 0;
uint32_t delta_t_sum = 0;
uint8_t puffer_is_full = 0;
uint32_t avarage_delta_t = 0;


void setup(){}

int main(){
    //moving avarage FIFO init phase
    while(puffer_is_full == 0){

        // need more functions from the normal while loop
        // need to calculate the seperate while loop and if statement process needs


        if(is_new_data == 1){
            delta_t_values[array_index] = new_delta_t;
            delta_t_sum = delta_t_sum + new_delta_t;
            if(array_index == 7){
                array_index = 0;
                puffer_is_full = 1;
            }
            else{
                array_index++;
            }
        }

    }
    while(true){

        if(is_new_data == 1){

        // moving avarage logic in the true part of the if statement of the new value

        //Refreshing the FIFO with the new value and adjusting the sum
        delta_t_sum = delta_t_sum - delta_t_values[array_index];
        delta_t_sum = delta_t_sum + new_delta_t;
        delta_t_values[array_index] = new_delta_t;
        if(array_index == 7){
            array_index = 0;
        }
        else{
            array_index++;
        }

        //adjusting the avarage (>>3 is the equivalent of /8 in the number system 2)
        avarage_delta_t = delta_t_sum >>3;




        }



    }
}

//      usart communication
//
//
