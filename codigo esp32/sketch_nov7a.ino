#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// ==========================================
// 1. CONFIGURACAO DO WIFI
// ==========================================

const char* ssid = "{Coloque aqui o nome da rede}";
const char* password = "{Coloque aqui a senha}";

WebServer server(80);

// ==========================================
// 2. CONFIGURACAO DOS SENSORES
// ==========================================

#define DHT_PIN 4
#define DHT_TYPE DHT22

#define MQ135_PIN 34
#define LDR_PIN 35

#define SDA_PIN 22
#define SCL_PIN 21

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;

// ==========================================
// 3. VARIAVEIS DOS SENSORES
// ==========================================

float temperatura = NAN;
float umidade = NAN;
float pressao = NAN;
float altitude = NAN;

int qualidadeArRaw = 0;
float tensaoMQ135 = 0;

int luminosidadeRaw = 0;
int luminosidade = 0;

bool bmpDisponivel = false;

unsigned long ultimaLeitura = 0;
const unsigned long intervaloLeitura = 2000;

// ==========================================
// 4. PAGINA HTML DO DASHBOARD
// ==========================================

const char paginaHTML[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="pt-BR">

<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>Estacao Meteorologica ESP32</title>

<style>

* {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}

body {
    font-family: Arial, sans-serif;
    background: #101827;
    color: #ffffff;
    padding: 20px;
}

.container {
    max-width: 1100px;
    margin: auto;
}

header {
    margin-bottom: 25px;
}

header h1 {
    font-size: 26px;
    margin-bottom: 8px;
}

header p {
    color: #9ca3af;
}

.status {
    display: inline-block;
    background: #064e3b;
    color: #6ee7b7;
    padding: 7px 12px;
    border-radius: 20px;
    font-size: 12px;
    margin-top: 12px;
}

.grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 15px;
}

.card {
    background: #1e293b;
    border: 1px solid #334155;
    border-radius: 12px;
    padding: 22px;
}

.card h3 {
    font-size: 14px;
    color: #94a3b8;
    margin-bottom: 18px;
}

.valor {
    font-size: 34px;
    font-weight: bold;
}

.unidade {
    font-size: 15px;
    color: #94a3b8;
}

.descricao {
    color: #94a3b8;
    font-size: 12px;
    margin-top: 12px;
}

.progress {
    height: 8px;
    background: #334155;
    border-radius: 10px;
    margin-top: 15px;
    overflow: hidden;
}

.progress-bar {
    height: 100%;
    width: 0%;
    background: #38bdf8;
    transition: width 0.5s;
}

.section {
    margin-top: 25px;
}

.section h2 {
    margin-bottom: 15px;
    font-size: 18px;
}

.footer {
    margin-top: 25px;
    color: #64748b;
    font-size: 12px;
    text-align: center;
}

@media(max-width: 600px) {
    .grid {
        grid-template-columns: 1fr;
    }

    .valor {
        font-size: 30px;
    }
}

</style>
</head>

<body>

<div class="container">

<header>

<h1>🌤 Estacao Meteorologica</h1>

<p>Monitoramento ambiental em tempo real</p>

<div class="status">
● ESP32 ONLINE
</div>

</header>

<div class="grid">

<div class="card">
<h3>🌡 Temperatura</h3>
<div class="valor">
<span id="temperatura">--</span>
<span class="unidade">°C</span>
</div>
<div class="descricao">Sensor DHT22</div>
</div>

<div class="card">
<h3>💧 Umidade relativa</h3>
<div class="valor">
<span id="umidade">--</span>
<span class="unidade">%</span>
</div>
<div class="descricao">Sensor DHT22</div>
</div>

<div class="card">
<h3>🌎 Pressao atmosferica</h3>
<div class="valor">
<span id="pressao">--</span>
<span class="unidade">hPa</span>
</div>
<div class="descricao">Sensor BMP280</div>
</div>

<div class="card">
<h3>⛰ Altitude estimada</h3>
<div class="valor">
<span id="altitude">--</span>
<span class="unidade">m</span>
</div>
<div class="descricao">Sensor BMP280</div>
</div>

<div class="card">
<h3>🌫 Qualidade do ar</h3>
<div class="valor">
<span id="mq135">--</span>
<span class="unidade">ADC</span>
</div>
<div class="progress">
<div class="progress-bar" id="barraAr"></div>
</div>
<div class="descricao" id="arDescricao">
Leitura bruta do MQ-135
</div>
</div>

<div class="card">
<h3>☀ Luminosidade</h3>
<div class="valor">
<span id="luminosidade">--</span>
<span class="unidade">%</span>
</div>
<div class="progress">
<div class="progress-bar" id="barraLuz"></div>
</div>
<div class="descricao">Sensor LDR - intensidade relativa</div>
</div>

</div>

<div class="section card">
<h2>📡 Informacoes do sistema</h2>

<p class="descricao">
Rede Wi-Fi: Estacao_Meteorologica
</p>

<p class="descricao">
Endereco local: 192.168.4.1
</p>

<p class="descricao">
Atualizacao automatica: 2 segundos
</p>

<p class="descricao">
Ultima atualizacao: <span id="horario">--</span>
</p>

</div>

<div class="footer">
Projeto de Estacao Meteorologica com ESP32
</div>

</div>

<script>

async function atualizarDashboard() {

    try {

        const resposta = await fetch('/dados');

        if (!resposta.ok) {
            throw new Error("Falha na leitura");
        }

        const dados = await resposta.json();

        function mostrar(id, valor, casas = 1) {
            document.getElementById(id).textContent =
                valor === null ? "--" : Number(valor).toFixed(casas);
        }

        mostrar("temperatura", dados.temperatura);
        mostrar("umidade", dados.umidade);
        mostrar("pressao", dados.pressao);
        mostrar("altitude", dados.altitude);

        mostrar("mq135", dados.mq135, 0);
        mostrar("luminosidade", dados.luminosidade, 0);

        document.getElementById("barraAr").style.width =
            Math.min(100, dados.mq135 / 4095 * 100) + "%";

        document.getElementById("barraLuz").style.width =
            dados.luminosidade + "%";

        document.getElementById("arDescricao").textContent =
            "Tensao analogica: " + dados.tensaoMQ135.toFixed(2) + " V";

        document.getElementById("horario").textContent =
            new Date().toLocaleTimeString("pt-BR");

    } catch (erro) {

        console.error("Erro:", erro);

    }

}

atualizarDashboard();

setInterval(atualizarDashboard, 2000);

</script>

</body>
</html>

)rawliteral";

// ==========================================
// 5. LEITURA DOS SENSORES
// ==========================================

void lerSensores() {

    // DHT22

    float novaTemperatura = dht.readTemperature();
    float novaUmidade = dht.readHumidity();

    if (!isnan(novaTemperatura)) {
        temperatura = novaTemperatura;
    }

    if (!isnan(novaUmidade)) {
        umidade = novaUmidade;
    }

    // BMP280

    if (bmpDisponivel) {

        pressao = bmp.readPressure() / 100.0F;

        altitude = bmp.readAltitude(1013.25);

    }

    // MQ135

    qualidadeArRaw = analogRead(MQ135_PIN);

    tensaoMQ135 = qualidadeArRaw * 3.3 / 4095.0;

    // LDR

    luminosidadeRaw = analogRead(LDR_PIN);

    // Escala relativa de 0 a 100
    // Ajustar conforme a montagem do divisor.

    luminosidade = map(
        luminosidadeRaw,
        0,
        4095,
        0,
        100
    );

    luminosidade = constrain(luminosidade, 0, 100);

}

// ==========================================
// 6. ENVIO DOS DADOS EM JSON
// ==========================================

void enviarDados() {

    String json = "{";

    json += "\"temperatura\":";

    if (isnan(temperatura)) {
        json += "null";
    } else {
        json += String(temperatura, 2);
    }

    json += ",\"umidade\":";

    if (isnan(umidade)) {
        json += "null";
    } else {
        json += String(umidade, 2);
    }

    json += ",\"pressao\":";

    if (!bmpDisponivel || isnan(pressao)) {
        json += "null";
    } else {
        json += String(pressao, 2);
    }

    json += ",\"altitude\":";

    if (!bmpDisponivel || isnan(altitude)) {
        json += "null";
    } else {
        json += String(altitude, 2);
    }

    json += ",\"mq135\":";
    json += String(qualidadeArRaw);

    json += ",\"tensaoMQ135\":";
    json += String(tensaoMQ135, 3);

    json += ",\"luminosidade\":";
    json += String(luminosidade);

    json += "}";

    server.send(200, "application/json", json);

}

// ==========================================
// 7. PAGINA PRINCIPAL
// ==========================================

void paginaPrincipal() {

    server.send_P(
        200,
        "text/html",
        paginaHTML
    );

}

// ==========================================
// 8. CONFIGURACAO INICIAL
// ==========================================

void setup() {

    Serial.begin(115200);

    delay(1000);

    Serial.println();
    Serial.println("Iniciando Estacao Meteorologica...");

    // Configurar ADC

    analogReadResolution(12);

    analogSetPinAttenuation(
        MQ135_PIN,
        ADC_11db
    );

    analogSetPinAttenuation(
        LDR_PIN,
        ADC_11db
    );

    // Iniciar DHT22

    dht.begin();

    // Iniciar I2C

    Wire.begin(SDA_PIN, SCL_PIN);

    // Iniciar BMP280

    if (bmp.begin(0x76)) {

        bmpDisponivel = true;

        Serial.println("BMP280 encontrado em 0x76");

    } else if (bmp.begin(0x77)) {

        bmpDisponivel = true;

        Serial.println("BMP280 encontrado em 0x77");

    } else {

        bmpDisponivel = false;

        Serial.println("BMP280 nao encontrado!");

    }

    if (bmpDisponivel) {

        bmp.setSampling(
            Adafruit_BMP280::MODE_NORMAL,
            Adafruit_BMP280::SAMPLING_X2,
            Adafruit_BMP280::SAMPLING_X16,
            Adafruit_BMP280::FILTER_X16,
            Adafruit_BMP280::STANDBY_MS_500
        );

    }

    // Criar rede Wi-Fi

    WiFi.mode(WIFI_AP);

    WiFi.softAP(
        ssid,
        password
    );

    IPAddress ip = WiFi.softAPIP();

    Serial.println();
    Serial.println("Rede Wi-Fi criada!");
    Serial.print("SSID: ");
    Serial.println(ssid);

    Serial.print("IP do Dashboard: ");
    Serial.println(ip);

    // Rotas HTTP

    server.on("/", HTTP_GET, paginaPrincipal);

    server.on("/dados", HTTP_GET, enviarDados);

    server.onNotFound([]() {

        server.send(404, "text/plain", "Pagina nao encontrada");

    });

    // Iniciar servidor

    server.begin();

    Serial.println("Servidor HTTP iniciado!");

    // Primeira leitura

    lerSensores();

}

// ==========================================
// 9. LOOP PRINCIPAL
// ==========================================

void loop() {

    server.handleClient();

    unsigned long agora = millis();

    if (agora - ultimaLeitura >= intervaloLeitura) {

        ultimaLeitura = agora;

        lerSensores();

        Serial.println("------------------------");

        Serial.print("Temperatura: ");
        Serial.println(temperatura);

        Serial.print("Umidade: ");
        Serial.println(umidade);

        Serial.print("Pressao: ");
        Serial.println(pressao);

        Serial.print("MQ135 ADC: ");
        Serial.println(qualidadeArRaw);

        Serial.print("Luminosidade: ");
        Serial.println(luminosidade);

    }

}