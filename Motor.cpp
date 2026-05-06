#include "Motor.h"
#include <Arduino.h>

/*
Inicializacia krokoveho motora
    Nastavi piny pre tlacidlo a LED
    Nastavi zakladne parametre motora (speed, acceleration, pozicia)
    Testovaci pohyb: pohyb z nuly na 2048 a spat
    Spustenie nekonecnej ulohy pre chod motora na druhom jadre (FreeRTOS task)
*/
void Motor::inicializaciaMotor() {

    pinMode(PIN_TLACIDLO_ECO, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);

    krokovyMotor.setMaxSpeed(100);
    krokovyMotor.setAcceleration(100);
    krokovyMotor.setCurrentPosition(0);

    krokovyMotor.moveTo(2048);
    while (krokovyMotor.distanceToGo() != 0) krokovyMotor.run();
    krokovyMotor.moveTo(0);
    while (krokovyMotor.distanceToGo() != 0) krokovyMotor.run();

    krokovyMotor.moveTo(2048);

    // Spustenie motora na druhom jadre (FreeRTOS task)
    xTaskCreatePinnedToCore([](void* arg) { 
        ((Motor*)arg)->chodMotora(); 
        vTaskDelete(NULL); 
    }, "motorTask", 4096, this, 1, NULL, 0);
}

/*
Nekonecna uloha pre riadenie motora
    Riadi motor podla stavu:
        MOTOR_AUTOMATICKY - automaticke pohyby medzi ulozenymi polohami
        MOTOR_MANUALNY    - manualne pohyby podla tlacidiel
        MOTOR_ZASTAVENY   - zastavenie motora
    Pouziva krokovyMotor.run() pre postupny pohyb
    Bezi ako FreeRTOS task na jadre 0
*/
void Motor::chodMotora() {
    while (true) {
        switch (motorStav) {
            case MOTOR_AUTOMATICKY:
                if (!ecoRezim) {
                    krokovyMotor.run();
                    if (krokovyMotor.distanceToGo() == 0) {
                        if (poloha0Ulozena && poloha1Ulozena) {
                            if (krokovyMotor.currentPosition() == poloha1)
                                krokovyMotor.moveTo(poloha0);
                            else
                                krokovyMotor.moveTo(poloha1);
                        } else {
                            krokovyMotor.stop();
                        }
                    }
                }
                break;

            case MOTOR_MANUALNY:
                if (ecoRezim) {
                    if (smerPohybu == -1) {
                        krokovyMotor.move(krokDozadu);
                        smerPohybu = 0;
                    }
                    if (smerPohybu == 1) {
                        krokovyMotor.move(krokDopredu);
                        smerPohybu = 0;
                    }
                    krokovyMotor.run();
                } else {
                    if (smerPohybu == -1) {
                        krokovyMotor.move(krokDozadu);
                        smerPohybu = 0;
                    }
                    if (smerPohybu == 1) {
                        krokovyMotor.move(krokDopredu);
                        smerPohybu = 0;
                    }
                    krokovyMotor.run();
                    if (poloha0Ulozena && poloha1Ulozena) {
                        motorStav = MOTOR_AUTOMATICKY;
                    }
                }
                break;

            case MOTOR_ZASTAVENY:
                krokovyMotor.stop();
                break;
        }
        vTaskDelay(2);
    }
}

/*
Kontrola tlacidla ECO pre prepnutie rezimu
    Detekuje stlacenie s debouncingom (200ms)
    Prepne ecoRezim a upravi stav motora
        ecoRezim == true  -> MOTOR_MANUALNY
        ecoRezim == false -> MOTOR_AUTOMATICKY
*/
void Motor::kontrolaTlacidla() {
    static bool poslednyStavTlacidla = HIGH;
    static uint32_t poslednyCasTlacidla = 0;
    bool aktualnyStav = digitalRead(PIN_TLACIDLO_ECO);

    if (aktualnyStav != poslednyStavTlacidla && millis() - poslednyCasTlacidla > 200) {
        poslednyCasTlacidla = millis();

        if (aktualnyStav == LOW) {
            ecoRezim = !ecoRezim;
            if (ecoRezim) {
                krokovyMotor.stop();
                motorStav = MOTOR_MANUALNY;
            } else {
                motorStav = MOTOR_AUTOMATICKY;
            }
        }
    }
    poslednyStavTlacidla = aktualnyStav;
}

/*
Aktualizacia LED indikatora
    Blika podla rezimu:
        ecoRezim == true  -> interval 5000ms
        ecoRezim == false -> interval 500ms
    Pouziva millis() pre casovanie
*/
void Motor::aktualizujLED() {
    static uint32_t posledny = 0;
    static bool stav = false;

    uint32_t interval;
    if (ecoRezim) {
        interval = 5000;
    } else {
        interval = 500;
    }

    if (millis() - posledny >= interval) {
        posledny = millis();
        stav = !stav;
        if (stav){
            digitalWrite(PIN_LED, HIGH);
        }else{
            digitalWrite(PIN_LED, LOW);
        }
    }
}
