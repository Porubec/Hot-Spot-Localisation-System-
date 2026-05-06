// poradilo AI aby sa subor zavolal len raz aj ked ho volam na viacerych miestach 
#pragma once
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <algorithm>
/*
Definuje rozmery vstupnej snimky z kamery
    32x24 pixelov (MLX90640)
*/
#define KAMERA_VSTUP_SIRKA 32
#define KAMERA_VSTUP_VYSKA 24
/*
Definuje rozmery vystupneho obrazu
    Pouziva sa po interpolacii (zvacsenie obrazu)
*/
#define KAMERA_VYSTUP_SIRKA 256    //128
#define KAMERA_VYSTUP_VYSKA 192     //96
/*
Parametre detekcie ohna
    OHEN_TEPLOTA_PRAH  - teplota pre spustenie alarmu
    OHEN_HYSTEREZA     - zabranuje blikaniu (zap/vyp)
    OHEN_POCET_SNIMIEK - kolko snimkov musi byt nad prahom
*/
#define OHEN_TEPLOTA_PRAH 60.0
#define OHEN_HYSTEREZA 2.0
#define OHEN_POCET_SNIMIEK 3
/*
Trieda Kamera
    Zabezpecuje pracu s termokamerou MLX90640
    Nacitanie snimky, analyzu teploty
    Detekciu ohna a generovanie vystupneho obrazu
*/
class Kamera {
public:
    /*
    Rezimy zobrazenia obrazu
        FAREBNE_ZOBRAZENIE     - heatmap (farebne)
        CIERNO_BIELE_ZOBRAZENIE - grayscale (white-hot)
    */
    enum RezimZobrazenia { FAREBNE_ZOBRAZENIE = 0, CIERNO_BIELE_ZOBRAZENIE = 1 };
    // Aktualne pouzity rezim zobrazenia
    RezimZobrazenia aktualnyRezimZobrazenia = FAREBNE_ZOBRAZENIE;

    /*
    Pole surovych teplot z kamery
        Velkost: 32x24 = 768 hodnot
        Obsahuje teploty v stupnoch Celzia
    */
    float snimka[KAMERA_VSTUP_SIRKA * KAMERA_VSTUP_VYSKA];

     /*
    Mapovanie pre interpolaciu
        Prepocet suradnic z vystupneho obrazu
        Na vstupne suradnice kamery
    */
    float mapovanieX[KAMERA_VYSTUP_SIRKA];
    float mapovanieY[KAMERA_VYSTUP_VYSKA];

    /*
    Look-Up Table (LUT) pre rychle farbenie
        256 hodnot pre kazdu paletu
        Zrychluje vykreslovanie obrazu
    */
    uint16_t farebneZobrazenieLUT[256];
    uint16_t ciernoBieleZobrazenieLUT[256];

    /*
    Vysledky analyzy snimky
        minTeplotaNova - najnizsia teplota
        maxTeplotaNova - najvyssia teplota
        cieloveX/Y     - pozicia hotspotu (zvacseny obraz)
    */
    float minTeplotaNova = 0.0f;
    float maxTeplotaNova = 0.0f;
    float cieloveX = 0.0f;
    float cieloveY = 0.0f;
    // Index najteplejsieho pixelu v poli snimka[]
    int indexNajteplejsieho = 0;

    /*
    Verejne metody triedy
        Inicializacia, spracovanie snimky a generovanie obrazu
    */
    void inicializaciaI2C();
    void inicializaciaKamera();
    bool nacitajSnimku();
    void inicializaciaMapovania();
    void analyzujTeplotu();
    bool detekujOhen();
    void vytvorObraz(uint8_t* paket);
    void inicializaciaTabuliekFarieb();

private:
    /*
    Objekt kniznice pre MLX90640
        Zabezpecuje komunikaciu so senzorom
        Preberam funkciu getFrame
        Taktiez pouzivam zobrazovanie obrazu z kniznice Adafruit_MLX90640
    */

    Adafruit_MLX90640 kamera;
    
    /*
    Interné pomocne funkcie
        Pouzivaju sa pri generovani obrazu
    */
    float ziskajHodnotu(float* snimka, int sirka, int vyska, float poziciaX, float poziciaY);
    uint16_t farebneZobrazenie(uint8_t hodnota);
    uint16_t ciernoBieleZobrazenie(uint8_t hodnota);
    uint16_t vyberFarbu(uint8_t hodnota);
};
