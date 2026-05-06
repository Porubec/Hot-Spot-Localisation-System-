#pragma once
#include <WebServer.h>
#include <WebSocketsServer.h>
#include "Kamera.h"
#include "Motor.h"
#include "Komunikacia.h"

/*
Trieda Stranka
    Zabezpecuje webove rozhranie pre aplikaciu
    Obsahuje HTTP server a WebSocket server
    Umoznuje ovladanie motora, zobrazenie dat z kamery a komunikaciu s klientom
*/
class Stranka {
public:

    // HTTP server na porte 80
    WebServer server{80};
    // WebSocket server na porte 81 (pre real-time data)
    WebSocketsServer ws{81};

    /*
    Referencie na ostatne triedy
        kamera      - spracovanie termokamery
        motor       - riadenie motora
        komunikacia - wifi, email a bateria
        Nastavuju sa z hlavnej triedy (dependency injection)
    */
    Kamera* kamera = nullptr;
    Motor* motor = nullptr;
    Komunikacia* komunikacia = nullptr;

    /*
    Premenne pre odosielanie dat
        casPoslednejSnimky - pouziva sa na obmedzenie FPS (~30)
        paket - binarny buffer pre websocket:
            21 bajtov metadata + obraz z kamery (RGB565) + 1 bajt eco rezim
    */
    uint32_t casPoslednejSnimky = 0;
    uint8_t paket[21 + KAMERA_VYSTUP_SIRKA * KAMERA_VYSTUP_VYSKA * 2 + 1];

    /*
    Verejne metody
        inicializaciaServer() - spusti HTTP a WebSocket server
        posliSnimku(...)      - odosle data klientovi cez websocket
        jeCasNaSnimku()       - kontrola casovania snimok
    */
    void inicializaciaServer();
    void posliSnimku(float maxTeplotaNova, float cieloveX, float cieloveY, bool ohen, bool ecoRezim);
    bool jeCasNaSnimku();

private:
    /*
    obsluhaRoot()
        Odosle hlavnu HTML stranku klientovi
    */
    void obsluhaRoot();
};
