
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

//--------------------- CONFIG BITS (iguais ao projeto anterior) ----------------
#pragma config OSC = HS, FCMEN = OFF, IESO = OFF
#pragma config PWRT = OFF, BOREN = SBORDIS, BORV = 3
#pragma config WDT = OFF, WDTPS = 32768
#pragma config CCP2MX = PORTC, PBADEN = OFF, LPT1OSC = OFF, MCLRE = ON
#pragma config STVREN = ON, LVP = OFF, XINST = OFF
#pragma config CP0 = OFF, CP1 = OFF, CP2 = OFF, CP3 = OFF
#pragma config CPB = OFF, CPD = OFF
#pragma config WRT0 = OFF, WRT1 = OFF, WRT2 = OFF, WRT3 = OFF
#pragma config WRTC = OFF, WRTB = OFF, WRTD = OFF
#pragma config EBTR0 = OFF, EBTR1 = OFF, EBTR2 = OFF, EBTR3 = OFF
#pragma config EBTRB = OFF

#define _XTAL_FREQ 20000000UL   // cristal 20 MHz

//--------------------- Estado do cronômetro ------------------------------------
static volatile uint8_t min_dez = 0;   // dezenas de minuto (0..5)
static volatile uint8_t min_uni = 0;   // unidades de minuto (0..9)
static volatile uint8_t seg_dez = 0;   // dezenas de segundo (0..5)
static volatile uint8_t seg_uni = 0;   // unidades de segundo (0..9)
static volatile bool contando = false;

//--------------------- Preload Timer0 para 1s ---------------------------------
#define TMR0_PRELOAD_H  0xB3
#define TMR0_PRELOAD_L  0xCD

static inline void tmr0_reload(void) {
    TMR0H = TMR0_PRELOAD_H;
    TMR0L = TMR0_PRELOAD_L;
}

//--------------------- Atualiza exibição ---------------------------------------
static inline void display_update(void)
{
    // PORTD = segundos (BCD)
    LATD = (uint8_t)((seg_dez << 4) | (seg_uni & 0x0F));
    // PORTC = minutos (BCD)
    LATC = (uint8_t)((min_dez << 4) | (min_uni & 0x0F));
}

//--------------------- Incremento estilo relógio -------------------------------
static inline void tick_1s(void)
{
    if (!contando) return;

    seg_uni++;
    if (seg_uni >= 10) {
        seg_uni = 0;
        seg_dez++;
        if (seg_dez >= 6) {
            seg_dez = 0;
            min_uni++;
            if (min_uni >= 10) {
                min_uni = 0;
                min_dez++;
                if (min_dez >= 6) {
                    // overflow total → volta para 00:00
                    min_dez = 0;
                    min_uni = 0;
                }
            }
        }
    }
    display_update();
}

//--------------------- Inicialização -------------------------------------------
static void hw_init(void)
{
    ADCON1 = 0x0F;
    CMCON  = 0x07;

    TRISD = 0x00; LATD = 0x00; // segundos
    TRISC = 0x00; LATC = 0x00; // minutos

    TRISBbits.TRISB0 = 1; // INT0
    TRISBbits.TRISB1 = 1; // INT1
    INTCON2bits.RBPU = 0;

    INTCON2bits.INTEDG0 = 0;
    INTCON2bits.INTEDG1 = 0;
    INTCONbits.INT0IF   = 0;
    INTCON3bits.INT1IF  = 0;
    INTCONbits.INT0IE   = 1;
    INTCON3bits.INT1IE  = 1;

    T0CONbits.T08BIT = 0;
    T0CONbits.T0CS   = 0;
    T0CONbits.PSA    = 0;
    T0CONbits.T0PS2  = 1;
    T0CONbits.T0PS1  = 1;
    T0CONbits.T0PS0  = 1;
    tmr0_reload();
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;
    T0CONbits.TMR0ON  = 1;

    INTCONbits.PEIE = 1;
    INTCONbits.GIE  = 1;

    min_dez = min_uni = seg_dez = seg_uni = 0;
    contando = false;
    display_update();
}

//--------------------- Programa principal --------------------------------------
int main(void)
{
    hw_init();
    for (;;) {
        NOP();
    }
}

//--------------------- ISR -----------------------------------------------------
void __interrupt() isr(void)
{
    if (INTCONbits.TMR0IF) {
        INTCONbits.TMR0IF = 0;
        tmr0_reload();
        tick_1s();
    }
    if (INTCONbits.INT0IF) {
        INTCONbits.INT0IF = 0;
        contando = !contando;
    }
    if (INTCON3bits.INT1IF) {
        INTCON3bits.INT1IF = 0;
        contando = false;
        min_dez = min_uni = seg_dez = seg_uni = 0;
        display_update();
    }
}
