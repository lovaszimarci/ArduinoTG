#include "DS3231.h"
#include "LCD_I2C.h"


#define DS3231_ADDR (0x68 << 1)

//============================
//KULSO ORA FELPROGRAMOZASA
//============================

void DS3231_Init_1Hz(void) {

    I2C_start();
    I2C_write(DS3231_ADDR); // ora megszolitasa
    I2C_write(0x0E); // ramutatunk a controll registerre (ebbe szeretnenk irni)
    I2C_write(0x00); // 0x00 kikapcsolja a belso alarm megszakitast es beallitja SQW labat pontosan 1Hz re
    I2C_stop();
}
