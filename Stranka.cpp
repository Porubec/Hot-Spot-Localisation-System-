#include "Stranka.h"
#include <Arduino.h>

/*
Inicializuje web server a websocket
    Definuje endpointy pre ovladanie motora, konfiguraciu, paletu a hlavnu stranku
    Spusta server a websocket
        Vystupy: aktivny HTTP server + websocket
*/
void Stranka::inicializaciaServer() {
    // Hlavna stranka
    server.on("/", [this]() { obsluhaRoot(); });

    // Endpoint pre pohyb motora
    server.on("/motor", [this]() {
        if (server.hasArg("smer")) {
            motor->smerPohybu = server.arg("smer").toInt();
            motor->motorStav = Motor::MOTOR_MANUALNY;
        }
        server.send(200, "text/plain", "OK");
    });

    // Endpoint pre zastavenie motora
    server.on("/motorStop", [this]() {
        motor->smerPohybu = 0;
        server.send(200, "text/plain", "STOP");
    });

    // Endpoint pre kalibraciu poloh motora
    server.on("/kalibracia", [this]() {
        if (server.hasArg("p")) {
            int p = server.arg("p").toInt();
            if (p == 0) {
                motor->poloha0 = motor->krokovyMotor.currentPosition();
                motor->poloha0Ulozena = true;
            }
            if (p == 1) {
                motor->poloha1 = motor->krokovyMotor.currentPosition();
                motor->poloha1Ulozena = true;
            }
        }
        server.send(200, "text/plain", "OK");
    });

    // Endpoint pre nastavenie konfiguracie (reset poloh)
    server.on("/palette", [this]() {
        if (server.hasArg("p")) {
            int hodnota = server.arg("p").toInt();
            if (hodnota == 0) {
                kamera->aktualnyRezimZobrazenia = Kamera::FAREBNE_ZOBRAZENIE;
            } else {
                kamera->aktualnyRezimZobrazenia = Kamera::CIERNO_BIELE_ZOBRAZENIE;
            }
        }
        server.send(200, "text/plain", "OK");
    });

    // Endpoint pre zmenu palety termokamery
    server.on("/config", [this]() {
        motor->motorStav = Motor::MOTOR_MANUALNY;
        motor->krokovyMotor.stop();
        motor->krokovyMotor.setCurrentPosition(motor->krokovyMotor.currentPosition());
        motor->krokovyMotor.moveTo(motor->krokovyMotor.currentPosition());
        motor->poloha0Ulozena = false;   
        motor->poloha1Ulozena = false;  
        server.send(200, "text/plain", "CONFIG");
    });

    // Spustenie HTTP servera
    server.begin();

    // Spustenie websocketu a obsluha pripojenia klienta
    ws.begin();
    ws.onEvent([](uint8_t n, WStype_t t, uint8_t* p, size_t l) {
        if (t == WStype_CONNECTED) Serial.println("WebSocket klient pripojeny");
    });
}

/*
Odosiela snimku a metadata vsetkym pripojenym websocket klientom
    Paket obsahuje:
        maxTeplotaNova (float)
        cieloveX (float)
        cieloveY (float)
        ohen (1/0)
        zostavajuci cas normal (float)
        zostavajuci cas eco (float)
        ecoRezim (1/0)
    Vstupy: maxTeplotaNova, cieloveX, cieloveY, ohen, ecoRezim
    Vystupy: odoslany binarny paket cez websocket
*/
void Stranka::posliSnimku(float maxTeplotaNova, float cieloveX, float cieloveY, bool ohen, bool ecoRezim) {
    float casN = komunikacia->zostavajuciCasNormal(ecoRezim);
    float casE = komunikacia->zostavajuciCasEco();

    // Naplnenie paketu binárnymi dátami
    memcpy(&paket[0], &maxTeplotaNova, 4);
    memcpy(&paket[4], &cieloveX, 4);
    memcpy(&paket[8], &cieloveY, 4);
    if (ohen){
        paket[12] = 1;
    }else{
        paket[12] = 0;
    }
    memcpy(&paket[13], &casN, 4);
    memcpy(&paket[17], &casE, 4);
    if (ecoRezim){
        paket[sizeof(paket)-1] = 1;
    }else{
        paket[sizeof(paket)-1] = 0;
    }

    // Odoslanie paketu všetkým pripojeným klientom
    if (ws.connectedClients() > 0)
        ws.broadcastBIN(paket, sizeof(paket));
}

/*
Kontrola casu medzi snimkami (~30 fps)
    Zabrani prilis rychlemu odosielaniu snimok
    Vystup: true ak je cas na novu snimku, false inak
*/
bool Stranka::jeCasNaSnimku() {
    if (millis() - casPoslednejSnimky < 33) return false;
    casPoslednejSnimky = millis();
    return true;
}

/*
Odosiela hlavnu HTML stranku s ovladacim rozhranim
    Obsahuje:
        termokameru (canvas)
        ovládanie motora
        konfiguráciu polôh
        zmenu palety
        históriu najvyšších teplôt
    Vstupy: ziadne
    Vystupy: HTML stranka cez HTTP server
*/
void Stranka::obsluhaRoot() {
    server.send(200, "text/html", R"rawliteral(
            Bakalarska_praca Adrián_Porubec 5ZYP31

                <!DOCTYPE html>
                <html lang="sk">

                <head>
                <meta charset="UTF-8">
                <meta name="viewport" content="width=device-width, initial-scale=1">
                <title>ThermalCam</title>

                <style>

                body{
                    background:black;
                    color:white;
                    font-family:Arial;
                    margin:0;
                    padding:0;
                    display:flex;
                    flex-direction:column;
                    align-items:center;
                }

                h2{
                    margin:10px 0;
                }

                #container{
                    display:flex;
                    flex-wrap:wrap;
                    justify-content:center;
                    gap:20px;
                    width:95vw;
                    margin-top:10px;
                }

                #cameraPanel{
                    flex:2 1 70%;
                    min-width:400px;
                    text-align:center;
                }

                #cameraPanel canvas{
                    width:100%;
                    max-width:1000px;
                    image-rendering:pixelated;
                    border:3px solid #555;
                    transform:rotate(180deg);
                }

                .configBlock{
                    background:#111;
                    border:2px solid #555;
                    border-radius:6px;
                    padding:8px 14px;
                    display:flex;
                    flex-direction:column;
                    align-items:center;
                    min-width:180px;
                }

                .configBlock h4{
                    margin-bottom:8px;
                    color:#89b4fa;
                }

                .configBlock button{
                    padding:6px 14px;
                    font-size:0.95em;
                    border-radius:4px;
                    border:1px solid #555;
                    background:#222;
                    color:white;
                    cursor:pointer;
                }

                .configBlock button:hover{
                    background:#333;
                }

                #historyPanel{
                    flex:1 1 25%;
                    min-width:200px;
                    background:#111;
                    border:2px solid #555;
                    border-radius:8px;
                    padding:10px;
                    display:flex;
                    flex-direction:column;
                    align-items:center;
                }

                #historyPanel h3{
                    margin-bottom:10px;
                    color:#89b4fa;
                    font-size:1em;
                }

                #historyTable{
                    width:100%;
                    border-collapse:collapse;
                    font-size:0.9em;
                }

                #historyTable th,
                #historyTable td{
                    border:1px solid #333;
                    padding:4px 6px;
                    text-align:center;
                }

                #historyTable th{
                    background:#222;
                    color:#aaa;
                }

                #historyTable td{
                    color:#fff;
                }

                .prazdna{
                    color:#555;
                    text-align:center;
                    padding:10px;
                }

                </style>
                </head>


                <body>

                <h2>ThermalCam</h2>

                <div id="container">

                <!-- CAMERA PANEL -->

                <div id="cameraPanel">

                <div style="display:flex; justify-content:center; align-items:center; gap:25px; margin-bottom:8px; width:100%; font-size:1.2em;">

                <div id="modeLabel" style="color:#6ab0ff;">NORMAL</div>
                <div id="batteryLabel" style="color:#f1c40f;">Batéria: --</div>
                <div id="temperatureLabel">Teplota: -- °C</div>

                </div>

                <canvas id="thermalCanvas" width="256" height="192"></canvas>

                <div style="display:flex; justify-content:center; gap:20px; margin-top:10px;">

                <div class="configBlock">

                <h4>Konfigurácia bodov</h4>

                <button onclick="startConfiguration()">Konfigurovať</button>

                <div style="display:flex; gap:10px; margin-top:8px;">
                <button onmousedown="startMotorMove(-1)" onmouseup="stopMotorMove()" onmouseleave="stopMotorMove()">&lt;</button>
                <button onmousedown="startMotorMove(1)" onmouseup="stopMotorMove()" onmouseleave="stopMotorMove()">&gt;</button>
                </div>

                <div style="display:flex; gap:10px; margin-top:8px;">
                <button onclick="savePosition(0)">Nastaviť 0</button>
                <button onclick="savePosition(1)">Nastaviť 1</button>
                </div>

                </div>

                <div class="configBlock">

                <h4>Vizualizácia</h4>

                <div style="display:flex; gap:10px;">
                <button onclick="setPalette(0)">Turbo</button>
                <button onclick="setPalette(1)">WhiteHot</button>
                </div>

                </div>

                </div>
                </div>


                <!-- HISTÓRIA -->

                <div id="historyPanel">

                <h3>Najvyššie teploty</h3>

                <table id="historyTable">

                <thead>
                <tr>
                <th>Čas</th>
                <th>Teplota</th>
                </tr>
                </thead>

                <tbody id="historyBody">
                <tr>
                <td colspan="2" class="prazdna">Zatiaľ žiadne záznamy</td>
                </tr>
                </tbody>

                </table>

                </div>

                </div>


                <script>

                /* UI ELEMENTY */

                const canvas = document.getElementById("thermalCanvas");
                const canvasContext = canvas.getContext("2d");
                const thermalImage = canvasContext.createImageData(256,192);

                const temperatureLabel = document.getElementById("temperatureLabel");
                const modeLabel = document.getElementById("modeLabel");
                const batteryLabel = document.getElementById("batteryLabel");

                const historyBody = document.getElementById("historyBody");


                /* HISTÓRIA ALARMOV */

                function addTemperatureAlarm(temp){

                    const timeString = new Date().toLocaleTimeString("sk-SK");

                    if(historyBody.querySelector(".prazdna")){
                        historyBody.innerHTML="";
                    }

                    const row=document.createElement("tr");

                    row.innerHTML =
                        "<td>"+timeString+"</td>"+
                        "<td style='color:#e74c3c'>"+temp+"°C</td>";

                    historyBody.insertBefore(row,historyBody.firstChild);

                    while(historyBody.rows.length>28){
                        historyBody.deleteRow(historyBody.rows.length-1);
                    }

                }


                /* WEBSOCKET */

                const websocket = new WebSocket("ws://"+location.hostname+":81/");
                websocket.binaryType="arraybuffer";

                websocket.onmessage = event => {

                if(event.data.byteLength<13) return;

                const rawData = new Uint8Array(event.data);
                const dataView = new DataView(event.data);


                /* SENSOR DATA */

                const detectedTemperature = dataView.getFloat32(0,true);
                const firePosX = dataView.getFloat32(4,true);
                const firePosY = dataView.getFloat32(8,true);

                const batteryNormalHours = dataView.getFloat32(13,true);
                const batteryEcoHours = dataView.getFloat32(17,true);

                const fireDetected = rawData[12];
                const ecoModeActive = rawData[rawData.byteLength-1];


                /* MODE */

                if (ecoModeActive) {
                    modeLabel.innerText = "ECO";
                    modeLabel.style.color = "#27ae60";
                } else {
                    modeLabel.innerText = "NORMAL";
                    modeLabel.style.color = "#6ab0ff";
                }


                /* TEPLOTA */

                temperatureLabel.innerText =
                "Teplota: "+detectedTemperature.toFixed(1)+"°C";


                /* BATÉRIA */

                let batteryTime;

                if (ecoModeActive) {
                    batteryTime = batteryEcoHours;
                } else {
                    batteryTime = batteryNormalHours;
                }

                let hours = Math.floor(batteryTime);
                let minutes = Math.floor((batteryTime-hours)*60);

                batteryLabel.innerText =
                "Batéria: "+hours+"h "+minutes+"m";


                /* ALARM */

                if(fireDetected){
                addTemperatureAlarm(detectedTemperature.toFixed(1));
                }


                /* PIXELY TERMOKAMERY */

                const thermalPixels = new Uint8Array(event.data,21);

                let pixelIndex=0;

                for(let i=0;i<thermalPixels.length;i+=2){

                const rgb565 =
                (thermalPixels[i+1]<<8) |
                thermalPixels[i];

                thermalImage.data[pixelIndex++] = ((rgb565>>11)&31)<<3;
                thermalImage.data[pixelIndex++] = ((rgb565>>5)&63)<<2;
                thermalImage.data[pixelIndex++] = (rgb565&31)<<3;
                thermalImage.data[pixelIndex++] = 255;

                }

                canvasContext.putImageData(thermalImage,0,0);


                /* DETEKČNÝ RÁM */

                if (fireDetected) {
                    canvasContext.strokeStyle = "red";
                } else {
                    canvasContext.strokeStyle = "cyan";
                }

                canvasContext.lineWidth=3;

                canvasContext.strokeRect(
                firePosX-10,
                firePosY-10,
                20,
                20
                );

                };


                /* MOTOR */

                let motorInterval=null;

                function startMotorMove(direction){

                stopMotorMove();

                motorInterval=setInterval(function(){
                fetch("/motor?smer="+direction);
                },150);

                }

                function stopMotorMove(){

                if(motorInterval){
                clearInterval(motorInterval);
                motorInterval=null;
                }

                fetch("/motorStop");

                }


                /* KONFIGURÁCIA */

                function savePosition(position){

                fetch("/kalibracia?p="+position)
                .then(function(r){return r.text();})
                .then(function(){

                const msg=document.createElement("div");

                msg.style.marginTop="6px";
                msg.style.fontSize="0.9em";
                msg.innerText="Poloha "+position+" uložená";

                document
                .getElementById("cameraPanel")
                .appendChild(msg);

                setTimeout(function(){msg.remove();},3000);

                });

                }

                function startConfiguration(){
                fetch("/config");
                }


                /* PALETA */

                function setPalette(paletteId){
                fetch("/palette?p="+paletteId);
                }

                </script>

                </body>
            </html>

    )rawliteral");
}
