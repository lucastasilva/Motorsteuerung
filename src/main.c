/**
 * @file main.c
 * @author Lucas Tavares Amaral Silva
 * @date 21/05/2026
 * @brief Main-Funktion des Motorsteuerungsprojekts
 */

#include "inc/motor.h"

int main(void)
{
    motor_init();

    while (1)
    {
        motor_run();
    }

    return 0;
}