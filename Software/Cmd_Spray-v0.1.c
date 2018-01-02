/* _______________________________________________________________________________
 *
 * 			P R O G R A M M E    'Cmd_Spray'
 * _______________________________________________________________________________

 Change History :
 ---------------
 * Rev   Date         Description
 * 0.0   17/11/2017   Initial release (non fonctionnel car la position du relais instable)
 * 0.1   01/01/2018   Passage de la tempo anti-Rebond à 500ms au lieu de 250ms.
  
 Configuration Hard:
 -------------------
 Fosc = 32Mhz (limite de la Freq interne de ce PIC)
  
 PIN OUTPUT:
 ¨¨¨¨¨¨¨¨¨¨¨
 LED verte EstEnVie  => RA4
 Relais Spray => RC0
 Commande EV => RA2

 PIN INPUT:
 ¨¨¨¨¨¨¨¨¨
 BP pour ativation du Spray => RA5
 Pédal => RC2
 Config_1 => RC4
 Config_2 => RC3
  
 Description:
 ------------
 Ce montage commande une Electrovalve servant à activer le spray sur un fauteuil dentaire.
 Deux micro-switch BP de PEDAL sont relus par le microcontrôleur et détecte respectivement
 une demande de mise en service du Spray, et un appui sur la pédale de mise en route des instruments.

 Il existe 4 modes de fonctionnement:
  ?	Mode 0 : Normal avec interlocking : Une impulsion sur BP sans PEDAL  active/désactive SPRAY
  ?	Mode 1 : Normal sans interlocking : Une impulsion sur BP quel que soit l?état de PEDAL  active/désactive SPRAY
  ?	Mode 2 : Old fashion way  i.e. SPRAY actif tant que BP appuyé
  ?	Mode 3 : Backup - SPRAY toujours actif

 Dans tous les cas, il faut au moins avoir PEDAL pour avoir EV d?où
 EV = PEDAL.

 Pour la partie Hardware, on a un schéma à 2 étages :
  ?	1er étage : relais miniature qui bascule en fonction de l?état de BP :
        o   1 contact qui commande le 2eme étage.
        o   1 contact sec utilisé pour l?affichage led au module central
  ?	2ème  étage : transistor qui commande l?EV qui balance l?air + LED de signalisation en fonction de l?état de EV.
  */

#include <pic16f1825.h>
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

/********** C O N F I G U R A T I O N   B I T S ******************************/

#pragma config CPD = OFF        // Data Memory Code Protection disable
#pragma config BOREN = OFF      // Brown-out Reset disabled in hardware and software
#pragma config IESO = OFF       // Internal/External Switchover is disable
#pragma config FOSC = INTOSC    // Internal oscillator => Voir registre OSCCON
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor disable
#pragma config MCLRE = ON       // MCLR/VPP pin function is MCLR
#pragma config WDTE = OFF       // Watchdog Timer disable
#pragma config CP = OFF         // Flash Program Memory Code Protection is Off
#pragma config CLKOUTEN = OFF   // CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin
#pragma config PLLEN = ON       // 4x PLL enabled even on internal oscillator.
#pragma config WRT = OFF        // Flash Memory Self-Write Protection Off
#pragma config STVREN = ON      // ON Stack Overflow or Underflow will cause a Reset
#pragma config BORV = HI        // Brown-out Reset Voltage (Vbor), high trip point selected.
#pragma config LVP = OFF        // High-voltage on MCLR/VPP must be used for programming

/********** DEFINE ******************************/
//SIMPLIFICATION LANGUAGE
#define TRUE         1		// ON
#define FALSE        0  	// OFF
#define INPUT_PIN    1		// INPUT PIN
#define OUTPUT_PIN   0		// INPUT OUT
#define ON           1		// ON
#define OFF          0		// OFF
#define HIGH         1		// ON
#define LOW          0  	// OFF
#define IO           0		// Entrée digital
#define AD           1		// Entrée analogique

//MASK
#define SPBRG_9600_BAUD     207             //  Valeur de spbrg : BAUD = FOSC / (16 * (spbrg + 1))
#define _XTAL_FREQ          32000000        //oscillator frequency en Hz

//FLAG 
#define Appui_BP	Flag.bits.b0        // Indique qu'il y a eu un appui BP

//PINS ATTRIBUTION
#define BP              !PORTAbits.RA5
#define PEDAL           !PORTCbits.RC2
#define SPRAY           LATCbits.LATC0
#define EV              LATAbits.LATA2
#define LED             LATAbits.LATA4
#define CONFIG_1        !PORTCbits.RC4
#define CONFIG_2        !PORTCbits.RC3

#define TRIS_BP      	TRISAbits.TRISA5    // Direction du port In ou Out
#define TRIS_PEDAL      TRISCbits.TRISC2
#define TRIS_SPRAY      TRISCbits.TRISC0
#define TRIS_EV         TRISAbits.TRISA2
#define TRIS_LED        TRISAbits.TRISA4
#define TRIS_CONFIG_1   TRISCbits.TRISC4
#define TRIS_CONFIG_2   TRISCbits.TRISC3

//#define  ANSEL_BP	ANSELAbits.ANSA5    // Place le port en I/O
#define  ANSEL_PEDAL	ANSELCbits.ANSC2
#define  ANSEL_SPRAY	ANSELCbits.ANSC0
#define  ANSEL_EV       ANSELAbits.ANSA2
#define  ANSEL_LED      ANSELAbits.ANSA4
//#define  ANSEL_CONFIG_2   ANSELCbits.ANSC34	// Pas applicable pour RC4
#define  ANSEL_CONFIG_2	ANSELCbits.ANSC3

#define WPU_BP          WPUAbits.WPUA5     // Résistance de pull-up sur le port
#define WPU_PEDAL       WPUCbits.WPUC2
#define WPU_CONFIG_1    WPUCbits.WPUC4
#define WPU_CONFIG_2    WPUCbits.WPUC3

#define INT_BP          INTCONbits.IOCIE   // interrupt-on-change pour le boutton BP
#define FLG_BP          IOCAFbits.IOCAF5   // Flag d'interruption sur le BP
#define BP_F_EDGE       IOCANbits.IOCAN5   // Falling Edge génère une interruption sur BP
#define BP_R_EDGE       IOCAPbits.IOCAP5   // Rising Edge génère une interruption sur BP

#define delay_Rebond	__delay_ms(500)

/********** DEFINITION STRUCTURE ******************************/
typedef unsigned char   BYTE;               // 8-bit unsigned
typedef union _BYTE_VAL
{
    BYTE Val;
    struct
    {
        unsigned char b0:1;
        unsigned char b1:1;
        unsigned char b2:1;
        unsigned char b3:1;
        unsigned char b4:1;
        unsigned char b5:1;
        unsigned char b6:1;
        unsigned char b7:1;
    } bits;
} BYTE_VAL, BYTE_BITS;


/********** DEFINITION des variables globalles ******************************/
volatile BYTE_VAL Flag;
volatile unsigned int n_TMR1, mode;

/* ____________________________________________________________________________
 * 
 *                            Function Prototype
 * ____________________________________________________________________________
 */

void InitializeSystem(void);
void interrupt Interruption_Haute_Priorite(void);
void ISR_TMR1(void);
void ISR_BP(void);

/* ____________________________________________________________________________
 * 
 *                            Interruptions
 * ____________________________________________________________________________
 */

/* Routine d'interruption priorité haute - High priority interrupt routine */
void interrupt Interruption_Haute_Priorite(void)
{
    NOP();
    NOP();
    if(PIR1bits.TMR1IF)
    {
        ISR_TMR1();                     // Routine d'interruption de TMR1
        PIR1bits.TMR1IF=0;              // Reset Flag
    }
    else if (FLG_BP)
    {
        ISR_BP();                       // Routine d'interruption du BP
        FLG_BP=0;                       // Reset Flag
    }
} // End Interruption_Haute_Priorite()



/********************************************************************
 * Function:        void InitializeSystem(void)
 *******************************************************************/
void InitializeSystem(void)
{
    // Configuration du Registre de control de FOSC
    OSCCONbits.IRCF = 0b1110;   // Internal Oscillator Frequency set to 8 if PPL disable or 32Mhz if PPL enable
								// PPL defined by PLLEN in Config word
								//				    ||====== ||
								// Ici on a Choisi PPL ON => Fosc = || 32Mhz ||
								//				    ||====== ||
    OSCCONbits.SCS = 0b00;      // System Clock determined by FOSC<2:0> in Configuration Word 1.
								// Ici on a choisi internal oscillator


    // Configuration du Timer1
    T1CONbits.TMR1ON = 1;       // Enables Timer1
    T1CONbits.TMR1CS = 0b00;    // Timer1 clock source is Instruction Clock (Fosc/4) => 32/4 = 8Mhz ici
    T1CONbits.T1OSCEN = 0 ;     // Dedicated Timer1 oscillator circuit disabled
    T1CONbits.T1CKPS = 0b11 ;   // 1:8 Prescale value => 8Mhz/8 = 1Mhz => 1us par incrémentation du timer
    T1CONbits.nT1SYNC = 1;      // Do not synchronize external clock inputs
    T1GCONbits.TMR1GE = 0;      // Timer1 counts regardless of Timer1 gate function
    PIE1bits.TMR1IE = 1;        // interrupt TMR1 enable
    INTCONbits.PEIE = 1;        // Peripherical interruption enable

    // Validation des l'interruptions
    INTCONbits.GIE = 1;         // Global Interrupt Enable bit
    INT_BP = ON;                // interrupt-on-change pour le boutton BP
    BP_F_EDGE = OFF;            // Front descendant qui génère une interruption sur BP
    BP_R_EDGE = ON;             // Front montant qui génère une interruption sur BP
	
    // Configuration des PINS
    TRIS_BP = INPUT_PIN;        // Sens I/O
    TRIS_PEDAL = INPUT_PIN;
    TRIS_SPRAY = OUTPUT_PIN;
    TRIS_EV = OUTPUT_PIN;
    TRIS_LED = OUTPUT_PIN;
    TRIS_CONFIG_1 = INPUT_PIN;
    TRIS_CONFIG_2 = INPUT_PIN;

    ANSEL_PEDAL = IO;           // Entrée digital
    ANSEL_SPRAY = IO;
    ANSEL_EV = IO;
    ANSEL_LED = IO;
    ANSEL_CONFIG_2 = IO;

    OPTION_REGbits.nWPUEN=0;    // Autorise les résistances de rappel individuel
    WPU_BP = ON;                // Place une résistance de pull up sur BP
    WPU_PEDAL = ON;
    WPU_CONFIG_1 = ON;
    WPU_CONFIG_2 = ON;

    // Initialisation programme
    SPRAY = ON;
    EV = OFF;
    LED = ON;

} // End InitializeSystem()


/********************************************************************
 * Function:        void ISR_TMR1(void)
 *******************************************************************/
void ISR_TMR1(void)
{
    n_TMR1++;           // Compte le nombre de passage dans TMR1
    if (n_TMR1 > 15)    // On fait clignoter la LED VERTE
    {
        LED = ~LED;     //On fait clignoter la LED
        n_TMR1= 0;
    }
}

/********************************************************************
 * Function:        void ISR_BP(void)
 *******************************************************************/
void ISR_BP(void)
{
    if ((mode == 0 && !PEDAL) ||  mode == 1)	// On autorise le changement uniquement à l'arrêt.
    {
        SPRAY= ~SPRAY;
        Appui_BP = 1;
        INT_BP = 0;                             // coupe l'interruption sur BP
    }
}

/* ____________________________________________________________________________
 *
 *                            Function Principale
 * ____________________________________________________________________________
 */

void main()
{
    InitializeSystem();                 // Initialisation de l'ensemble des périfériques du microcontrôleur
    while (1)
    {
        mode = (CONFIG_2 <<1) + (CONFIG_1<<0);	// Lecture du mode dans lequel on se trouve
        switch(mode)
        {
            case 0:
                if (Appui_BP)           // Le spray est actif/désactivé après une impulsion uniquement à l'arrêt(=!PEDAL)
                {
                    Appui_BP = OFF;
                    delay_Rebond;       // Delais de 500ms pour filtrer le rebond
                    FLG_BP = 0;         // Efface le flag de l'interruption sur BP
                    INT_BP = 1;         // Puis ré-autorise l'interruption sur BP
                }
                break ;
            case 1:                     // Le spray est actif/désactivé quelque soit l'état de PEDAL
                if (Appui_BP)
                {
                    Appui_BP = OFF;
                    delay_Rebond;       // Delais de 500ms pour filtrer le rebond
                    FLG_BP = 0;         // Efface le flag de l'interruption sur BP
                    INT_BP = 1;         // Puis ré-autorise l'interruption sur BP
                }
                break ;
            case 2:
                SPRAY = BP;             // Le spray est actif tant qu'on appuie sur la BP
                break ;
            case 3:
                SPRAY = ON;             // Le spray est toujours actif
                break ;
            default :
                SPRAY = ON;
                break;
        }
        if (EV != PEDAL)                // Dans tous les cas on active l'EV que si PEDAL est appuyé
        {                               // On aurait aussi pu écrire directement EV = PEDAL;
            EV = PEDAL;                 // mais c'était pour introduire un anti-rebond
            delay_Rebond;
        }
    }
}
