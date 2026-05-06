#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <AccelStepper.h>
#include <ESP_Mail_Client.h>
#include <algorithm>

#include "Kamera.h"
#include "Motor.h"
#include "Komunikacia.h"
#include "Stranka.h"

class ThermalCam {
private:
    Kamera       kamera;
    Motor        motor;
    Komunikacia  komunikacia;
    Stranka      stranka;

public:
/* 
 Inicializuje cely system (HW + SW casti)
    Spusti serial komunikaciu
    Nastavi cas spustenia programu
    Inicializuje motor (kroky, piny)
    Inicializuje I2C zbernicu pre kameru
    Inicializuje samotnu termokameru MLX90640
    Pripoji sa na WiFi siet
    Prepoji objekty medzi sebou (kamera, motor, komunikacia)
    Spusti web server a websocket
    Pripravi mapovanie pixelov a farebne tabulky
*/
    void inicializacia() {
        Serial.begin(115200);
        komunikacia.casSpustenia = millis();

        motor.inicializaciaMotor();
        kamera.inicializaciaI2C();
        kamera.inicializaciaKamera();
        komunikacia.inicializaciaWiFi();

        // Prepojenie referencii pre Stranka
        stranka.kamera      = &kamera;
        stranka.motor       = &motor;
        stranka.komunikacia = &komunikacia;
        stranka.inicializaciaServer();

        kamera.inicializaciaMapovania();
        kamera.inicializaciaTabuliekFarieb();
    }

/* 
Hlavna riadiaca slucka programu (bezi stale dokola)
    Spracuje HTTP poziadavky zo servera
    Spracuje websocket komunikaciu
    Kontroluje stav tlacidla na motore
    Kontroluje ci je cas spravit novu snimku
    Nacita data z kamery (teplotna mapa)
    Analyzuje teplotu (hlada maxima, atd.)
    Detekuje ohen (na zaklade teploty)
    Spracuje alarm (napr. email, signalizacia)
    Vytvori obraz zo snimky (farebna mapa)
    Posle snimku klientovi cez websocket
    Aktualizuje LED podla stavu

*/
    void loop() {
        stranka.server.handleClient();
        stranka.ws.loop();
        motor.kontrolaTlacidla();

        if (!stranka.jeCasNaSnimku()) return;
        if (!kamera.nacitajSnimku()) return;

        kamera.analyzujTeplotu();
        bool ohen = kamera.detekujOhen();
        komunikacia.spracujAlarm(ohen, kamera.maxTeplotaNova);
        kamera.vytvorObraz(stranka.paket);
        stranka.posliSnimku(kamera.maxTeplotaNova, kamera.cieloveX, kamera.cieloveY, ohen, motor.ecoRezim);
        motor.aktualizujLED();
    }
};

ThermalCam cam;
/*
Arduino setup funkcia - spusti sa raz po starte
    Zavola inicializaciu celeho systemu
*/
void setup() { 
    cam.inicializacia(); 
}
/*
Arduino loop funkcia - bezi donekonecna
    Vola hlavnu logiku triedy ThermalCam
*/
void loop() { 
    cam.loop(); 
}
