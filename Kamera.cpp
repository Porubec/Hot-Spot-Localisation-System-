#include "Kamera.h"

/*
Inicializuje I2C komunikaciu pre MLX90640
    Nastavi SDA a SCL piny (8, 9)
    Nastavi vysoku frekvenciu 1 MHz pre rychle citanie dat
*/
void Kamera::inicializaciaI2C() {
    Wire.begin(8, 9);               
    Wire.setClock(1000000);         
}

/*
Inicializuje termokameru MLX90640
    Pokusi sa inicializovat kameru
    Ak zlyha, zastavi program (while(1))
    Nastavi rezim citania (chess pattern)
    Nastavi rozlisenie ADC (18 bit)
    Nastavi obnovovaciu frekvenciu (~16 FPS)
*/
void Kamera::inicializaciaKamera() {
    delay(100);
    if (!kamera.begin()) {
        Serial.println("Chyba inicializacie MLX90640!");
        while (1);
    }

    kamera.setMode(MLX90640_CHESS);           
    kamera.setResolution(MLX90640_ADC_18BIT); 
    kamera.setRefreshRate(MLX90640_32_HZ);            //MLX90640_16_HZ    
}

/*
Nacita novu snimku z kamery (32x24 hodnoty)
    Zavola getFrame() z kniznice Adafruit
    Ulozi teploty do pola snimka[]
    Pri chybe vypise hlasku
        Vystupy(true = snimka OK, false = chyba citania)
*/
bool Kamera::nacitajSnimku() {
    if (kamera.getFrame(snimka) != 0) {
        Serial.println("Chyba pri citani snimky z MLX90640");
        return false;
    }
    return true;
}

/*
Vytvori mapovanie pre scaling obrazu
    Prepocita suradnice z maleho obrazu (32x24)
    Na velky vystup (128x96)
    Pouziva sa pri interpolacii
        Vystupy(naplnia sa polia mapovanieX[], mapovanieY[])
*/
void Kamera::inicializaciaMapovania() {
    for (int i = 0; i < KAMERA_VYSTUP_SIRKA; i++) {
        mapovanieX[i] = (float)i / (KAMERA_VYSTUP_SIRKA - 1) * (KAMERA_VSTUP_SIRKA - 1);
    }
    for (int j = 0; j < KAMERA_VYSTUP_VYSKA; j++) {
        mapovanieY[j] = (float)j / (KAMERA_VYSTUP_VYSKA - 1) * (KAMERA_VSTUP_VYSKA - 1);
    }
}

/*
Analyzuje teploty v snimke
    Najde minimalnu a maximalnu teplotu
    Najde najteplejsi pixel (hotspot)
    Prepocita jeho poziciu na vystupne rozlisenie
        Vstupy (snimka[])
        Vystupy (minTeplotaNova, maxTeplotaNova, cieloveX, cieloveY)
*/
void Kamera::analyzujTeplotu() {
    float minT = snimka[0];
    float maxT = snimka[0];
    indexNajteplejsieho = 0;

    int pocetPixelov = KAMERA_VSTUP_SIRKA * KAMERA_VSTUP_VYSKA;

    for (int i = 1; i < pocetPixelov; i++) {
        float t = snimka[i];
        if (t < minT){
             minT = t;
        }
        if (t > maxT) {
            maxT = t;
            indexNajteplejsieho = i;
        }
    }

    minTeplotaNova = minT;
    maxTeplotaNova = maxT;

    // Prevod 1D indexu na 2D suradnice originalnej snimky
    int senzorX = indexNajteplejsieho % KAMERA_VSTUP_SIRKA;
    int senzorY = indexNajteplejsieho / KAMERA_VSTUP_SIRKA;

    // Prepoctenie na suradnice zvacseného obrazu (128x96)
    cieloveX = (float)senzorX / (KAMERA_VSTUP_SIRKA - 1) * (KAMERA_VYSTUP_SIRKA - 1);
    cieloveY = (float)senzorY / (KAMERA_VSTUP_VYSKA - 1) * (KAMERA_VYSTUP_VYSKA - 1);
}

/*
Detekuje ohen na zaklade teploty
    Kontroluje ci teplota prekrocila prah
    Pouziva hysterezu (aby neblikalo zap/vyp)
    Vyzaduje viac po sebe iducich snimkov
        Vstupy (maxTeplotaNova)
        Vystupy (true = detekovany ohen, false = nic)
*/
bool Kamera::detekujOhen() {
    static bool ohenPredosly = false;
    static int pocetPotvrdenych = 0;

    bool prekrocenyPrah;

    if (ohenPredosly) {
        prekrocenyPrah = maxTeplotaNova >= (OHEN_TEPLOTA_PRAH - OHEN_HYSTEREZA);
    } else {
        prekrocenyPrah = maxTeplotaNova >= OHEN_TEPLOTA_PRAH;
    }

    if (prekrocenyPrah) {
        pocetPotvrdenych++;
        if (pocetPotvrdenych < OHEN_POCET_SNIMIEK) {
            prekrocenyPrah = false;
        }
    } else {
        pocetPotvrdenych = 0;
    }

    ohenPredosly = prekrocenyPrah;
    return prekrocenyPrah;
}

/*
Bilinearna interpolacia hodnoty
    Zoberie 4 susedne pixely
    Vypocita medzihodnotu podla pozicie
        Vstupy (mapa (pole teplot), sirka, vyska, x, y (float pozicia))
        Vystupy (interpolovana teplota)
*/
float Kamera::ziskajHodnotu(float* snimka, int sirka, int vyska, float poziciaX, float poziciaY) {
    // indexy susedných pixelov
    int xLavy = (int)poziciaX;
    int yHore = (int)poziciaY;

    int xPravy = std::min(xLavy + 1, sirka - 1);
    int yDole  = std::min(yHore + 1, vyska - 1);

    // váhy pre interpoláciu
    float deltaX = poziciaX - xLavy;
    float deltaY = poziciaY - yHore;

    // hodnoty susedných pixelov
    float bodLavyHore  = snimka[yHore * sirka + xLavy];
    float bodPravyHore = snimka[yHore * sirka + xPravy];
    float bodLavyDole  = snimka[yDole * sirka + xLavy];
    float bodPravyDole = snimka[yDole * sirka + xPravy];

    // interpolácia vodorovne
    float hornyRiadok = bodLavyHore * (1 - deltaX) + bodPravyHore * deltaX;
    float dolnyRiadok = bodLavyDole * (1 - deltaX) + bodPravyDole * deltaX;

    // interpolácia zvisle
    return hornyRiadok * (1 - deltaY) + dolnyRiadok * deltaY;
}

/*
Vytvori RGB565 obraz (128x96)
    Pre kazdy pixel spravi interpolaciu
    Normalizuje teploty na rozsah 0-255
    Vyberie farbu (farebne alebo BW)
    Ulozi data do vystupneho paketu
        Vstupy (paket (buffer na vystupne data))
        Vystupy (poslany packet s obrazom na stranku)
*/
void Kamera::vytvorObraz(uint8_t* paket) {
    float rozsahTeplot = maxTeplotaNova - minTeplotaNova;
    if (rozsahTeplot < 5.0f) {
        rozsahTeplot = 5.0f;
    }

    int paketIndex = 21;

    for (int i = 0; i < KAMERA_VYSTUP_VYSKA; i++) {
        float srcY = mapovanieY[i];

        for (int j = 0; j < KAMERA_VYSTUP_SIRKA; j++) {
            float srcX = mapovanieX[j];

            float teplota = ziskajHodnotu(snimka, KAMERA_VSTUP_SIRKA, KAMERA_VSTUP_VYSKA, srcX, srcY);

            float normalizovane = (teplota - minTeplotaNova) / rozsahTeplot;
            if (normalizovane < 0.0f) {
                normalizovane = 0.0f;
            }
            if (normalizovane > 1.0f) {
                normalizovane = 1.0f;
            }

            uint8_t hodnota8bit = (uint8_t)(normalizovane * 255.0f);
            uint16_t rgb565 = vyberFarbu(hodnota8bit);

            paket[paketIndex++] = rgb565 & 0xFF;
            paket[paketIndex++] = rgb565 >> 8;
        }
    }
}

/*
Inicializuje LUT tabulky farieb
    Predvypocita 256 farebnych hodnot
    Zrýchli prevod teplota -> farba
        Vystupy (naplni LUT polia)
*/
void Kamera::inicializaciaTabuliekFarieb() {
    for (int i = 0; i < 256; i++) {
        farebneZobrazenieLUT[i] = farebneZobrazenie(i);
        ciernoBieleZobrazenieLUT[i] = ciernoBieleZobrazenie(i);
    }
} 

/*
Prevod 8bit hodnoty na RGB565 (farebna paleta)
    Mapuje hodnotu na RGB (heatmap styl)
    Prevedie na RGB565 format
        Vstupy (hodnota (0-255))
        Vystupy (16-bit farba)
*/
uint16_t Kamera::farebneZobrazenie(uint8_t hodnota) {
    float normalizovanaHodnota = hodnota / 255.0f;
    float red = 0; 
    float green = 0; 
    float blue = 0;

    if (normalizovanaHodnota < 0.2f) { 
        red = 0; 
        green = normalizovanaHodnota*5.0f; 
        blue = 1.0f - normalizovanaHodnota*2.5f;
    }
    else if (normalizovanaHodnota < 0.4f) { 
        red = 0; 
        green = 1.0f; 
        blue = 1.0f - (normalizovanaHodnota-0.2f)*5.0f;
    }
    else if (normalizovanaHodnota < 0.6f) {
        red = (normalizovanaHodnota-0.4f)*5.0f; 
        green = 1.0f; 
        blue = 0;
    }
    else if (normalizovanaHodnota < 0.8f) {
        red = 1.0f; 
        green = 1.0f - (normalizovanaHodnota-0.6f)*5.0f; 
        blue = 0;
    }
    else { 
        red = 1.0f; 
        green = (normalizovanaHodnota-0.8f)*5.0f; 
        blue = (normalizovanaHodnota-0.8f)*5.0f;
    }

    uint8_t R = (uint8_t)(red * 255 + 0.5f);
    uint8_t G = (uint8_t)(green * 255 + 0.5f);
    uint8_t B = (uint8_t)(blue * 255 + 0.5f);
    return ((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3);
}

/*
Prevod na grayscale (white-hot)
    Pouzije gamma korekciu (x^2)
    Prevedie na RGB565
        Vstupy (hodnota (0-255))
            Vystupy (16-bit farba)
*/
uint16_t Kamera::ciernoBieleZobrazenie(uint8_t hodnota) {
    float normalizovanaHodnota = hodnota / 255.0f;
    normalizovanaHodnota = normalizovanaHodnota * normalizovanaHodnota;
    uint8_t hodnotaJasu = (uint8_t)(normalizovanaHodnota * 255);
    return ((hodnotaJasu & 0xF8) << 8) | ((hodnotaJasu & 0xFC) << 3) | (hodnotaJasu >> 3);
}

/*
Vyber farby podla rezimu
    Rozhodne ci pouzit farebnu alebo BW LUT
        Vstupy (hodnota (0-255))
        Vystupy (RGB565 farba)
*/
uint16_t Kamera::vyberFarbu(uint8_t hodnota) {
    if (aktualnyRezimZobrazenia == FAREBNE_ZOBRAZENIE)
        return farebneZobrazenieLUT[hodnota];
    else
        return ciernoBieleZobrazenieLUT[hodnota];
}
