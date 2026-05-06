#pragma once
#include <WiFi.h>
#include <ESP_Mail_Client.h>

/*
SMTP / EMAIL konfiguracia
    SMTP_SERVER       - adresa SMTP servera
    SMTP_PORT         - port pre SSL/TLS komunikaciu
    EMAIL_ODOSIELATEL - email, z ktoreho sa odosiela upozornenie
    EMAIL_HESLO       - heslo alebo app-specific password
    EMAIL_PRIJEMCA    - email prijemcu upozornenia
*/
#define SMTP_SERVER       "smtp.gmail.com"
#define SMTP_PORT         465
#define EMAIL_ODOSIELATEL "termocam90640@gmail.com"
#define EMAIL_HESLO       "kify mzkg fvft xnhp"
#define EMAIL_PRIJEMCA    "adrian.porubec@gmail.com"

/*
WIFI pripojovacie udaje
    extern aby sa dali definovat v CPP subore
*/
extern const char* WIFI_SSID;
extern const char* WIFI_HESLO;

/*
Konstanta pre prevod millis() na hodiny
    Pouziva sa pri vypocte zostavajuceho casu prevadzky
*/
constexpr float MILLISEKUNDY_NA_HODINY = 3600000.0f;

/*
Trieda Komunikacia
    Riadi WiFi pripojenie, odosielanie emailov
    a vypocet zostavajuceho casu prevadzky zariadenia
*/
class Komunikacia {
public:

    /*
    Cas spustenia zariadenia
        Pouziva sa pri vypocte zostavajuceho casu
    */
    uint32_t casSpustenia = 0;

    /*
    Kapacita baterie a odbery zariadenia
        kapacitaBaterie - mAh
        odberESP        - mA
        odberKamera     - mA
        odberMotor      - mA
    */
    float kapacitaBaterie = 2000.0;   // mAh
    float odberESP    = 150.0;        // mA
    float odberKamera = 65.0;         // mA
    float odberMotor  = 350.0;        // mA

    /*
    Stav odoslania emailu
        Aby sa zabránilo spamovaniu emailov pri neustalej detekcii
    */
    bool emailUzOdoslany = false;
    /*
    Verejne metody triedy
        inicializaciaWiFi      - pripoji ESP zariadenie k WiFi sieti
        posliEmailOhen         - odosle upozornenie pri detekcii ohna
        spracujAlarm           - riadi odosielanie emailov podla alarmu
        zostavajuciCasNormal   - vypocita zostavajuci cas batérie v normalnom rezime
        zostavajuciCasEco      - vypocita zostavajuci cas batérie v eco rezime
    */
    void inicializaciaWiFi();
    void posliEmailOhen(float teplota);
    void spracujAlarm(bool ohen, float maxTeplotaNova);
    float zostavajuciCasNormal(bool ecoRezim);
    float zostavajuciCasEco();
};
