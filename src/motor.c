#include "inc/motor.h"

#define MOTOR_STATUS_ON   PB3
#define MOTOR_STATUS_OFF  PB1
#define MOTOR_STATUS_PORT PORTB
#define MOTOR_STATUS_DDR  DDRB

#define MOTOR_CMD_ON   PD1
#define MOTOR_CMD_OFF  PD2
#define MOTOR_CMD_PORT PORTD
#define MOTOR_CMD_DDR  DDRD
#define MOTOR_CMD_IN   PIND

// F_CPU = 8MHz, Prescaler = 1024
// 1 Tick = 1/( 8000000/1024 ) = 0.128ms
// 1000ms = 1000 / 0.128 = 7812 Ticks

#define TICKS_PER_MS 8 // (8000000 / 1024) / 1000 ≈ 7.8 => 1ms pro 8 Ticks

typedef enum
{
    MOTOR_OFF,
    MOTOR_STARTING,
    MOTOR_RUNNING,
    MOTOR_BRAKING
} MotorState; // Zustandsmaschine

static MotorState state = MOTOR_OFF; // Am Anfang Maschine aus
static uint8_t step = 0;             // Anlaufschritt 0–9
static uint16_t step_timer = 0;      // Zeit innerhalb eines Schritts
static uint16_t blink_timer = 0;     // Blink-Timer
static uint8_t led_state = 0;        // LED3 (MOTOR_STATUS_ON) toggle

// Hilfsfunktionen
static void led_on(uint8_t pin) // Schaltet ein bestimmtes LED an
{
    MOTOR_STATUS_PORT &= ~(1 << pin); // LOW = AN (Active LOW STK500)
}

static void led_off(uint8_t pin) // Schaltet ein bestimmtes LED aus
{
    MOTOR_STATUS_PORT |= (1 << pin); // HIGH = AUS
}

static uint8_t cmd_on_pressed(void) // Überprüft, ob cmd_on beziehungsweise S1 gedrückt wurde
{
    return !(MOTOR_CMD_IN & (1 << MOTOR_CMD_ON));
}

static uint8_t cmd_off_pressed(void) // Überprüft, ob cmd_on beziehungsweise S2 gedrückt wurde
{
    return !(MOTOR_CMD_IN & (1 << MOTOR_CMD_OFF));
}

static uint16_t timer_get_ticks(void) // Gibt vergangene Ticks seit letztem Aufruf zurück
{
    static uint16_t last = 0;
    uint16_t now = TCNT1;
    uint16_t diff = now - last; // Überlauf wird automatisch korrekt behandelt
    last = now;
    return diff;
}
// ==========

void motor_init(void)
{
    MOTOR_STATUS_DDR |= (1 << MOTOR_STATUS_ON) | (1 << MOTOR_STATUS_OFF); // PB3, PB1 als Ausgang
    MOTOR_CMD_DDR &= ~((1 << MOTOR_CMD_ON) | (1 << MOTOR_CMD_OFF));       // PD1, PD2 als Eingang

    MOTOR_STATUS_PORT |= (1 << MOTOR_STATUS_ON) | (1 << MOTOR_STATUS_OFF); // Beide LEDs AUS
    led_on(MOTOR_STATUS_OFF);

    MOTOR_CMD_PORT = (1 << MOTOR_CMD_ON) | (1 << MOTOR_CMD_OFF); // Interne Pull-Ups fuer Tasten aktivieren

    // Timer1-Einsatz
    TCCR1A = 0x00;                      // Timer zählt hoch von 0 bis 65535, Kein PWM-Ausgang
    TCCR1B = (1 << CS12) | (1 << CS10); // Prescaler 1024, Timer läuft
    TCNT1 = 0;                          // Zähler auf 0 zurücksetzen
}

void motor_run(void)
{
    uint16_t ticks = timer_get_ticks();

    switch (state)
    {

    case MOTOR_OFF: {
        if (cmd_on_pressed())
        {
            led_off(MOTOR_STATUS_OFF); // LED1 AUS
            step = 0;
            step_timer = 0;
            blink_timer = 0;
            led_state = 0;
            state = MOTOR_STARTING; // Setzt state als MOTOR_STARTING
        }
        break;
    }
    case MOTOR_STARTING: {
        step_timer += ticks;
        blink_timer += ticks;

        uint16_t half_period = (500 * TICKS_PER_MS) / (step + 1);

        if (blink_timer >= half_period)
        {
            blink_timer = 0;
            if (led_state)
            {
                led_on(MOTOR_STATUS_ON);
                led_state = 0;
            }
            else
            {
                led_off(MOTOR_STATUS_ON);
                led_state = 1;
            }
        }

        if (step_timer >= (1000 * TICKS_PER_MS))
        {
            step_timer = 0;
            step++;
            if (step >= 10)
            {
                led_on(MOTOR_STATUS_ON); // LED3 Dauerlicht
                state = MOTOR_RUNNING;
            }
        }
        break;
    }

    case MOTOR_RUNNING: {
        if (cmd_off_pressed())
        {
            step = 0;
            step_timer = 0;
            blink_timer = 0;
            led_state = 0;
            state = MOTOR_BRAKING;
        }
        break;
    }

    case MOTOR_BRAKING: {
        step_timer += ticks;
        blink_timer += ticks;

        uint16_t half_period = (500 * TICKS_PER_MS) / (10 - step);

        if (blink_timer >= half_period)
        {
            blink_timer = 0;
            if (led_state)
            {
                led_on(MOTOR_STATUS_ON);
                led_state = 0;
            }
            else
            {
                led_off(MOTOR_STATUS_ON);
                led_state = 1;
            }
        }

        if (step_timer >= (1000 * TICKS_PER_MS))
        {
            step_timer = 0;
            step++;
            if (step >= 10)
            {
                led_off(MOTOR_STATUS_ON);
                led_on(MOTOR_STATUS_OFF);
                state = MOTOR_OFF;
            }
        }
        break;
    }
    }
}