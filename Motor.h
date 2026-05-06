#pragma once
#include <AccelStepper.h>

/*
Piny pre krokovy motor (4-vodicovy)
    PIN_MOTOR_1..4 - piny pripojene na cievky motora
*/
#define PIN_MOTOR_1 10
#define PIN_MOTOR_2 11
#define PIN_MOTOR_3 12
#define PIN_MOTOR_4 13

/*
Piny pre tlacidlo a LED indikátor
    PIN_TLACIDLO_ECO - tlacidlo pre prepnutie eco rezimu
    PIN_LED          - LED indikátor stavu
*/
#define PIN_TLACIDLO_ECO 7
#define PIN_LED 16

/*
Trieda Motor
    Riadi krokovy motor pomocou AccelStepper kniznice
    Podporuje automaticky a manualny rezim
    Riadi LED indikátor a spracovanie tlacidla ECO
*/
class Motor {
public:

    /*
    Rezim motora
        MOTOR_AUTOMATICKY - automaticky pohyb medzi ulozenymi polohami
        MOTOR_MANUALNY    - manualny pohyb podla tlacidla
        MOTOR_ZASTAVENY   - motor zastaveny
    */
    enum RezimMotora { MOTOR_AUTOMATICKY, MOTOR_MANUALNY, MOTOR_ZASTAVENY };
    RezimMotora motorStav = MOTOR_ZASTAVENY;

    /*
    Objekt AccelStepper pre krokovy motor
        FULL4WIRE rezim pre 4-vodicovy motor
        Piny definovane vyssie
    */
    AccelStepper krokovyMotor{AccelStepper::FULL4WIRE, PIN_MOTOR_1, PIN_MOTOR_3, PIN_MOTOR_2, PIN_MOTOR_4};

    /*
    Premenne pre ulozene polohy a manualny pohyb
        poloha0, poloha1       - ulozene pozicie pre automaticky rezim
        krokDozadu, krokDopredu - pocet krokov pri manualnom pohybe
        poloha0Ulozena, poloha1Ulozena - indikuje, ci su polohy ulozene
        smerPohybu             - aktualny smer manualneho pohybu (-1, 0, 1)
    */
    int poloha0 = 0, poloha1 = 0;
    int krokDozadu = -20;
    int krokDopredu = 20;
    bool poloha0Ulozena = false;
    bool poloha1Ulozena = false;
    int smerPohybu = 0;

    /*
    Indikator eco rezimu
        true  - eco rezim aktivny (manualny pohyb)
        false - normalny rezim (automaticky)
    */
    bool ecoRezim = false;

    /*
    Verejne metody triedy
        inicializaciaMotor() - nastavi piny, parametre a spusti task motora
        chodMotora()        - nekonecna uloha pre ovladanie motora
        kontrolaTlacidla()  - kontroluje stlacenie tlacidla ECO a prepina rezim
        aktualizujLED()      - blika LED podla aktivneho rezimu
    */
    void inicializaciaMotor();
    void chodMotora();
    void kontrolaTlacidla();
    void aktualizujLED();


};
