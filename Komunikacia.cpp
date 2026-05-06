#include "Komunikacia.h"
#include <Arduino.h>

const char* WIFI_SSID  = "T-25TCNS";        // "RB160";
const char* WIFI_HESLO = "193m55d3nhpg";    // "onlyrb160";

/*
Inicializuje WiFi pripojenie
    Nastavi rezim stanice (WIFI_STA)
    Nastavi hostname zariadenia
    Pokusi sa pripojit na WiFi
    Caka kym sa nepripoji (blokujuca slucka)
    Po pripojeni vypise IP adresu
        Vstupy (WIFI_SSID, WIFI_HESLO)
        Vystupy (aktivne WiFi pripojenie)
*/
void Komunikacia::inicializaciaWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("thermalcam");
    WiFi.begin(WIFI_SSID, WIFI_HESLO);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.println(WiFi.localIP());
        delay(500);
    }
    Serial.println("fungujem");
    Serial.println(WiFi.localIP());
}

/*
Odosle email pri detekcii ohna (asynchronne)
    Vytvori novy FreeRTOS task
    Prenesie teplotu ako parameter (heap alokacia)
    Nastavi SMTP session (server, login)
    Vytvori emailovu spravu (text + prijemca)
    Odošle email cez SMTP
    Uvolni pamat a ukonci task
        Vstupy (teplota)
        Vystupy (odoslany email)
*/
void Komunikacia::posliEmailOhen(float teplota) {
    float* ukazatelNaTeplotu = new float(teplota);
    
    xTaskCreate([](void* param) {
        float nameranaTeplota = *(float*)param;
        delete (float*)param;

        ESP_Mail_Session session;
        session.server.host_name = SMTP_SERVER;
        session.server.port = SMTP_PORT;
        session.login.email = EMAIL_ODOSIELATEL;
        session.login.password = EMAIL_HESLO;

        SMTP_Message message;
        message.sender.name = "ThermalCam";
        message.sender.email = EMAIL_ODOSIELATEL;
        message.subject = "FIRE ALARM - Detekovany ohen!";
        message.addRecipient("Majitel", EMAIL_PRIJEMCA);

        String text = "Bol detekovany ohen!\nNamerana teplota: " + String(nameranaTeplota,1) + " °C";
        message.text.content = text.c_str();

        SMTPSession localSmtp;
        localSmtp.connect(&session);
        MailClient.sendMail(&localSmtp, &message);
        localSmtp.closeSession();

        vTaskDelete(NULL);
    }, "emailTask", 8192, ukazatelNaTeplotu, 1, NULL);  
}

/*
Spracuje alarmovy stav (riadi odoslanie emailu)
    Kontroluje ci bol detekovany ohen
    Posle email len raz pri zmene stavu (false -> true)
    Zabranuje opakovanemu spamovaniu emailov
        Vstupy (ohen, maxTeplotaNova)
        Vystupy (pripadne odoslany email)
*/
void Komunikacia::spracujAlarm(bool ohen, float maxTeplotaNova) {
    if (ohen && !emailUzOdoslany) {
        posliEmailOhen(maxTeplotaNova);
        emailUzOdoslany = true;
    }
    if (!ohen) emailUzOdoslany = false;
}

/*
Vypocita zostavajuci cas prevadzky (normalny rezim)
    Spocita celkovy odber (ESP + kamera + motor)
    Vypocita maximalny cas z kapacity baterie
    Odpocita cas od spustenia zariadenia
    Osetri zapornu hodnotu (vrati 0)
        Vstupy (ecoRezim - nepouziva sa priamo)
        Vystupy (zostavajuci cas v hodinach)
*/
float Komunikacia::zostavajuciCasNormal(bool ecoRezim) {
    float prud = odberESP + odberKamera + odberMotor;
    float celkovyCas = kapacitaBaterie / prud;
    float odStartu = (millis() - casSpustenia) / MILLISEKUNDY_NA_HODINY;
    float zostava = celkovyCas - odStartu;
    if (zostava < 0){
        return 0;
    }
    return zostava;
}

/*
Vypocita zostavajuci cas prevadzky (eco rezim)
    Spocita odber bez motora (ESP + kamera)
    Vypocita maximalny cas z kapacity baterie
    Odpocita cas od spustenia zariadenia
    Osetri zapornu hodnotu (vrati 0)
        Vstupy (ziadne)
        Vystupy (zostavajuci cas v hodinach)
*/
float Komunikacia::zostavajuciCasEco() {
    float prud = odberESP + odberKamera;
    float celkovyCas = kapacitaBaterie / prud;
    float odStartu = (millis() - casSpustenia) / MILLISEKUNDY_NA_HODINY;
    float zostava = celkovyCas - odStartu;
    if (zostava < 0){
        return 0;
    }
    return zostava;
}
