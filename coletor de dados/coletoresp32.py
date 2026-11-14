
import sqlite3
import requests
import pandas as pd
import time
from datetime import datetime

# =====================================
# 1. CONFIGURACOES
# =====================================

ESP32_URL = "http://192.168.4.1/dados"
BANCO = "estacao.db"

INTERVALO = 10  # segundos

# =====================================
# 2. CRIAR BANCO E TABELA
# =====================================

conn = sqlite3.connect(BANCO)

conn.execute("""
CREATE TABLE IF NOT EXISTS leituras (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp TEXT,
    temperatura REAL,
    umidade REAL,
    pressao REAL,
    altitude REAL,
    mq135 INTEGER,
    tensaoMQ135 REAL,
    luminosidade INTEGER
)
""")

conn.commit()

print("===================================")
print(" ESTACAO METEOROLOGICA - ESP32")
print("===================================")
print("Banco de dados conectado!")
print("Aguardando dados do ESP32...")
print("===================================")

# =====================================
# 3. COLETA DOS DADOS
# =====================================

try:

    while True:

        try:
            resposta = requests.get(ESP32_URL, timeout=5)

            resposta.raise_for_status()

            dados = resposta.json()

            horario = datetime.now().strftime(
                "%Y-%m-%d %H:%M:%S"
            )

            conn.execute("""
                INSERT INTO leituras (
                    timestamp,
                    temperatura,
                    umidade,
                    pressao,
                    altitude,
                    mq135,
                    tensaoMQ135,
                    luminosidade
                )
                VALUES (?, ?, ?, ?, ?, ?, ?, ?)
            """, (
                horario,
                dados.get("temperatura"),
                dados.get("umidade"),
                dados.get("pressao"),
                dados.get("altitude"),
                dados.get("mq135"),
                dados.get("tensaoMQ135"),
                dados.get("luminosidade")
            ))

            conn.commit()

            print("\nNova leitura registrada!")
            print("Horario:", horario)
            print("Temperatura:", dados.get("temperatura"), "°C")
            print("Umidade:", dados.get("umidade"), "%")
            print("Pressao:", dados.get("pressao"), "hPa")
            print("Altitude:", dados.get("altitude"), "m")
            print("Qualidade do ar:", dados.get("mq135"))
            print("Luminosidade:", dados.get("luminosidade"), "%")

            print("Proxima coleta em 10 segundos...")

        except requests.RequestException as erro:
            print("Erro de comunicacao com ESP32:", erro)

        except ValueError as erro:
            print("Erro ao interpretar JSON:", erro)

        time.sleep(INTERVALO)

except KeyboardInterrupt:

    print("\nColeta interrompida pelo usuario.")

finally:

    # =====================================
    # 4. EXPORTAR RELATORIO CSV
    # =====================================

    df = pd.read_sql_query(
        "SELECT * FROM leituras ORDER BY timestamp DESC",
        conn
    )

    df.to_csv("relatorio_estacao.csv", index=False)

    print("\nRelatorio CSV gerado!")
    print("Total de registros:", len(df))

    conn.close()

    print("Banco de dados fechado.")
