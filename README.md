# 🌤 Estação Meteorológica com ESP32

Projeto de uma estação meteorológica de baixo custo baseada no **ESP32**, desenvolvido na **UNIP** (2º semestre de 2026).

O ESP32 lê sensores de temperatura, umidade, pressão, altitude, qualidade do ar e luminosidade. Ele cria a própria rede Wi-Fi e publica um **dashboard web em tempo real**. Um script em **Python** coleta os dados periodicamente, grava em um banco **SQLite** e gera um relatório **CSV**.

---

## 📋 Sumário

- [Funcionalidades](#-funcionalidades)
- [Componentes](#-componentes)
- [Esquema de ligação](#-esquema-de-ligação)
- [Estrutura do repositório](#-estrutura-do-repositório)
- [Como usar](#-como-usar)
- [API](#-api)
- [Banco de dados e relatório](#-banco-de-dados-e-relatório)
- [Autores](#-autores)

---

## ✨ Funcionalidades

- Leitura de **temperatura e umidade** (DHT22).
- Leitura de **pressão atmosférica e altitude** (BMP280).
- Leitura da **qualidade do ar** (MQ-135), em valor bruto do ADC e em tensão.
- Leitura da **luminosidade** (LDR), em escala relativa de 0 a 100%.
- O ESP32 funciona como **ponto de acesso Wi-Fi (AP)**, sem precisar de internet ou roteador.
- **Dashboard web** responsivo com atualização automática a cada 2 segundos.
- **Endpoint JSON** (`/dados`) para integração com outras aplicações.
- **Coletor em Python** que registra as leituras em SQLite a cada 10 segundos e exporta um CSV ao final.

---

## 🔧 Componentes

| Componente | Função |
|---|---|
| ESP32 (DOIT ESP32 DEVKIT V1) | Microcontrolador, servidor web e ponto de acesso Wi-Fi |
| DHT22 | Temperatura e umidade |
| BMP280 | Pressão atmosférica e altitude (I²C) |
| MQ-135 | Qualidade do ar |
| LDR | Luminosidade |
| Regulador de tensão MB102 | Alimentação das placas de ligação (trilhos de 5 V e 3,3 V) |
| Protoboard e jumpers | Montagem do circuito |

---

## 🔌 Esquema de ligação

### Pinos do ESP32

| Sensor | Pino do sensor | Pino do ESP32 | Alimentação |
|---|---|---|---|
| DHT22 | DATA | **D4** | 3,3 V |
| BMP280 | SDA | **D22** | 3,3 V |
| BMP280 | SCL | **D21** | 3,3 V |
| MQ-135 | saída (DO) | **D34** | 5 V |
| LDR | saída (DO) | **D35** | 3,3 V |

### Detalhes do BMP280

Os pinos de endereço e de seleção de interface são ligados nos trilhos de alimentação:

- **CSB** → GND (negativo)
- **SDO** → 3,3 V (positivo)

### Convenção de cores dos fios

- 🔴 Vermelho: positivo
- ⚫ Preto: negativo
- 🎨 Coloridos: sinais de dados

O diagrama completo está em [`docs/Diagrama_final_Projeto_esp32.png`](docs/Diagrama_final_Projeto_esp32.png) e a tabela detalhada em [`docs/TABELA_DE_LIGAÇÕES.xlsx`](docs/TABELA_DE_LIGAÇÕES.xlsx).

---

## 📁 Estrutura do repositório

```
.
├── sketch_nov7a.ino          # Firmware do ESP32 (sensores + servidor web + dashboard)
├── coletoresp32.py           # Coletor Python (SQLite + CSV)
├── docs/
│   ├── Diagrama_final_Projeto_esp32.png
│   ├── TABELA_DE_LIGAÇÕES.xlsx
│   └── PRINT_IDE_ARDUINO.png
└── README.md
```

> Ajuste os caminhos conforme a organização real do seu repositório.

---

## 🚀 Como usar

### 1. Firmware do ESP32

**Pré-requisitos**

- [Arduino IDE](https://www.arduino.cc/en/software) 2.x
- Suporte à placa ESP32 instalado (Gerenciador de Placas)
- Bibliotecas (Gerenciador de Bibliotecas):
  - `DHT sensor library` (Adafruit)
  - `Adafruit Unified Sensor`
  - `Adafruit BMP280 Library`

**Passos**

1. Abra `sketch_nov7a.ino` na Arduino IDE.
2. Edite o nome e a senha da rede Wi-Fi que o ESP32 vai criar:

   ```cpp
   const char* ssid = "Estacao_Meteorologica";
   const char* password = "sua_senha_aqui";
   ```

   A senha precisa ter no mínimo 8 caracteres.
3. Selecione a placa **DOIT ESP32 DEVKIT V1** e a porta COM correta.
4. Faça o upload e abra o Monitor Serial a **115200 baud**.
5. O monitor exibirá a mensagem `Servidor HTTP iniciado!` e as leituras a cada 2 segundos.

### 2. Acessando o dashboard

1. No celular ou computador, conecte-se à rede Wi-Fi criada pelo ESP32.
2. Abra no navegador: **http://192.168.4.1**

### 3. Coletor em Python

**Pré-requisitos**

```bash
pip install requests pandas
```

**Execução**

1. Com o computador conectado à rede Wi-Fi do ESP32, execute:

   ```bash
   python coletoresp32.py
   ```

2. O script consulta o ESP32 a cada 10 segundos e grava cada leitura no banco.
3. Para encerrar, pressione `Ctrl + C`. O relatório `relatorio_estacao.csv` é gerado automaticamente.

---

## 🌐 API

| Rota | Método | Descrição |
|---|---|---|
| `/` | GET | Dashboard em HTML |
| `/dados` | GET | Leituras atuais em JSON |

**Exemplo de resposta de `/dados`:**

```json
{
  "temperatura": 24.50,
  "umidade": 61.20,
  "pressao": 1013.25,
  "altitude": 12.40,
  "mq135": 1520,
  "tensaoMQ135": 1.225,
  "luminosidade": 47
}
```

Sensores sem leitura válida retornam `null`.

| Campo | Unidade |
|---|---|
| `temperatura` | °C |
| `umidade` | % |
| `pressao` | hPa |
| `altitude` | m (referência de 1013,25 hPa) |
| `mq135` | ADC bruto (0 a 4095) |
| `tensaoMQ135` | V |
| `luminosidade` | % (escala relativa) |

---

## 🗄 Banco de dados e relatório

O coletor cria o arquivo `estacao.db` com a tabela `leituras`:

| Coluna | Tipo |
|---|---|
| `id` | INTEGER (chave primária, autoincremento) |
| `timestamp` | TEXT |
| `temperatura` | REAL |
| `umidade` | REAL |
| `pressao` | REAL |
| `altitude` | REAL |
| `mq135` | INTEGER |
| `tensaoMQ135` | REAL |
| `luminosidade` | INTEGER |

Ao encerrar o coletor, todos os registros são exportados em ordem decrescente de horário para `relatorio_estacao.csv`.

---

## 👩‍💻 Autores

- **Kayane Peixoto**
- _(adicione aqui os demais integrantes do grupo)_

Projeto acadêmico, UNIP, 2º semestre de 2026.
