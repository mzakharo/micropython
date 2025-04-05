
#ifndef MICROPY_INCLUDED_PSXCONTROLLER_H
#define MICROPY_INCLUDED_PSXCONTROLLER_H

int psxReadInput();
void psxcontrollerInit();
void gamepad_get_adc(uint16_t *leftX ,uint16_t *leftY,uint16_t *rightX,uint16_t *rightY);
void gamepad_get_key(uint8_t *keyByte5,uint8_t * keyByte6);

#endif