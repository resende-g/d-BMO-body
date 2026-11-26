# Placa principal: ligações e peças

Gerado por `hardware/pcb/gerar_ligacoes.py` a partir de `pins.h` (perfil `target`). Não editar à mão: mudar o gerador e rodar de novo. Contexto, regras e pendências em [`README.md`](README.md).

## Ligações

| Ligação | GPIO | Pontos (ref.pino = nome na serigrafia) |
|---|---:|---|
| `SPI_MOSI` | 11 | U1.J1-17 = 11, DS1.SDI, DS1.SD_MOSI, U2.6 = MOSI |
| `SPI_SCK` | 12 | U1.J1-18 = 12, DS1.SCK, DS1.SD_SCK, U2.5 = SCK |
| `SPI_MISO` | 13 | U1.J1-19 = 13, DS1.SDO, DS1.SD_MISO, U2.7 = MISO |
| `DISPLAY_CS` | 10 | U1.J1-16 = 10, DS1.CS |
| `DISPLAY_DC` | 17 | U1.J1-10 = 17, DS1.DC |
| `DISPLAY_RESET` | 18 | U1.J1-11 = 18, DS1.RESET |
| `DISPLAY_BACKLIGHT` | 16 | U1.J1-9 = 16, JP2.3 = BL |
| `SD_CS` | 21 | U1.J3-18 = 21, DS1.SD_CS |
| `CC1101_CS` | 14 | U1.J1-20 = 14, U2.4 = CSN |
| `CC1101_GDO0` | 15 | U1.J1-8 = 15, U2.3 = GDO0 |
| `I2C_SDA` | 8 | U1.J1-12 = 8, U8.13 = SDA, U3.SDA, R1.1 |
| `I2C_SCL` | 9 | U1.J1-15 = 9, U8.12 = SCL, U3.SCL, R2.1 |
| `I2S_BCLK` | 40 | U1.J3-8 = 40, U4.SCK, U5.BCLK |
| `I2S_LRCLK` | 41 | U1.J3-7 = 41, U4.WS, U5.LRC |
| `I2S_MIC_DATA` | 7 | U1.J1-7 = 7, U4.SD |
| `I2S_SPEAKER_DATA` | 42 | U1.J3-6 = 42, U5.DIN |
| `AMP_SHUTDOWN` | 3 | U1.J1-13 = 3, U5.SD |
| `IR_RX` | 39 | U1.J3-9 = 39, U6.OUT |
| `IR_TX` | 47 | U1.J3-17 = 47, U7.DAT |
| `POWER_BUTTON` | 2 | U1.J3-5 = 2, SW11.3, CN1.6 = POWER_BUTTON |
| `VBAT_SENSE` | 1 | U1.J3-4 = 1, CN1.4 = VBAT_SENSE |
| `POWER_HOLD` | 48 | U1.J3-16 = 48, CN1.5 = POWER_HOLD |
| `CHARGER_STATUS` | 4 | U1.J1-4 = 4, CN1.7 = CHARGER_STATUS |
| `LCD_LED` |  | DS1.LED, JP2.2 = LED |
| `AMP_VIN` |  | U5.VIN, JP1.2 = VIN |
| `BTN_DPADUP` |  | SW1.2, U8.21 = GPA0 |
| `BTN_DPADDOWN` |  | SW2.2, U8.22 = GPA1 |
| `BTN_DPADLEFT` |  | SW3.2, U8.23 = GPA2 |
| `BTN_DPADRIGHT` |  | SW4.2, U8.24 = GPA3 |
| `BTN_ACTIONA` |  | SW5.1, U8.25 = GPA4 |
| `BTN_ACTIONB` |  | SW6.1, U8.26 = GPA5 |
| `BTN_ACTIONX` |  | SW7.1, U8.27 = GPA6 |
| `BTN_ACTIONY` |  | SW8.1, U8.1 = GPB0 |
| `BTN_SELECT` |  | SW9.4, U8.2 = GPB1 |
| `BTN_START` |  | SW10.3, U8.3 = GPB2 |
| `GND` |  | U1.J1-22 = G, U1.J3-22 = G, DS1.GND, U2.1 = GND, U3.GND, U4.GND, U4.L/R, U5.GND, U6.GND, U7.GND, U8.10 = VSS, C1.2, U8.15 = A0, U8.16 = A1, U8.17 = A2, SW11.2, SW1.3, SW2.3, SW3.3, SW4.3, SW5.4, SW6.4, SW7.4, SW8.4, SW9.1, SW10.2, CN1.1 = GND, CN1.8 = GND |
| `+3V3` |  | U1.J1-1 = 3V3, DS1.VCC, U2.2 = VCC, U3.VCC, U4.VDD, U6.VCC, U7.VCC, U8.9 = VDD, U8.18 = RESET, C1.1, R1.2, R2.2, JP2.1 = 3V3, JP1.3 = 3V3, CN1.3 = +3V3 |
| `+5V` |  | U1.J1-21 = 5V, JP1.1 = 5V, CN1.2 = +5V |

Pinos sem ligação (33): SW11.1, SW11.4, U2.8 (GDO2), SW9.2, SW9.3, SW10.1, SW10.4, U8.4 (GPB3), U8.5 (GPB4), U8.6 (GPB5), U8.7 (GPB6), U8.8 (GPB7), U8.11 (NC11), U8.14 (NC14), U8.19 (INTB), U8.20 (INTA), U8.28 (GPA7), SW1.1, SW1.4, SW2.1, SW2.4, SW3.1, SW3.4, SW4.1, SW4.4, SW5.2, SW5.3, SW6.2, SW6.3, SW7.2, SW7.3, SW8.2, SW8.3.

Posições de barra fêmea **sem ligação, que não precisam de ilha** (tirar o contato da barra ou cortá-la em pedaços; numa placa de uma face, a trilha passa pelo vão): U1: J1-2, J1-3, J1-5, J1-6, J1-14, J3-1, J3-2, J3-3, J3-10, J3-11, J3-12, J3-13, J3-14, J3-15, J3-19, J3-20, J3-21; DS1: T_CLK, T_CS, T_DIN, T_DO, T_IRQ; U5: GAIN.

## Conector da placa de energia (CN1)

| Pino | Sinal | GPIO | Sem a placa de energia (só o USB da DevKitC) |
|---:|---|---:|---|
| 1 | `GND` |  | GND |
| 2 | `+5V` |  | 5 V do USB, saindo da DevKitC (J1-21) |
| 3 | `+3V3` |  | 3,3 V do regulador da DevKitC (J1-1) |
| 4 | `VBAT_SENSE` | 1 | solto: a leitura da bateria não vale nada |
| 5 | `POWER_HOLD` | 48 | saída sem carga |
| 6 | `POWER_BUTTON` | 2 | o botão SW11 continua no GPIO 2 |
| 7 | `CHARGER_STATUS` | 4 | solto: com o pull-up interno, lê nível alto (sem carga) |
| 8 | `GND` |  | GND |

## Peças

| Ref | Peça | Montagem na placa | Observação |
|---|---|---|---|
| U6 | receptor IR 38 kHz (V1222) | barra fêmea 1x3 | tem (testado) |
| U7 | transmissor IR (V1221) | barra fêmea 1x3 | tem (nunca transmitiu) |
| SW11 | energia (GPIO 2) | botão táctil 6x6 mm PTH | comprar |
| U4 | INMP441 | 2 x barra fêmea 1x3 | tem (testado) |
| U2 | CC1101 E07-M1101D 433 MHz | barra fêmea 2x4 | tem (testado em recepção) |
| U1 | ESP32-S3-DevKitC-1 N16R8 | 2 x barra fêmea 1x22, 2,54 mm | tem (testado) |
| DS1 | MSP3218 (ILI9341 + SD) | barra fêmea 1x14 + 1x4, 2,54 mm | tem (testado, com o SD) |
| JP2 | luz de fundo | barra macho 1x3 + jumper | comprar barra macho e jumper |
| SW9 | select | botão táctil 6x6 mm PTH | comprar |
| SW10 | start | botão táctil 6x6 mm PTH | comprar |
| U8 | MCP23017-E/SP | DIP-28 300 mil (soquete) | comprar (não possui) + soquete DIP-28 |
| C1 | 100 nF | capacitor cerâmico, passo 5,08 mm | kit "104" (esperado) |
| R1 | 4,7 kΩ | resistor 1/4 W axial, passo 10,16 mm | kit 1 % (esperado) |
| R2 | 4,7 kΩ | resistor 1/4 W axial, passo 10,16 mm | kit 1 % (esperado) |
| SW1 | cima | botão táctil 6x6 mm PTH | comprar |
| SW2 | baixo | botão táctil 6x6 mm PTH | comprar |
| SW3 | esquerda | botão táctil 6x6 mm PTH | comprar |
| SW4 | direita | botão táctil 6x6 mm PTH | comprar |
| SW5 | A | botão táctil 6x6 mm PTH | comprar |
| SW6 | B | botão táctil 6x6 mm PTH | comprar |
| SW7 | X | botão táctil 6x6 mm PTH | comprar |
| SW8 | Y | botão táctil 6x6 mm PTH | comprar |
| U3 | PN532 NFC V3 (modo I2C) | barra fêmea 1x4 | tem (barra não soldada) |
| U5 | MAX98357A (borne do alto-falante na placa dele) | barra fêmea 1x7 | tem (nunca ligado) |
| JP1 | alimentação do amplificador | barra macho 1x3 + jumper | comprar barra macho e jumper |
| CN1 | conector da placa de energia | barra macho 1x8, 2,54 mm | comprar barra macho; o par fêmea fica na placa de energia |

Barra fêmea 2,54 mm: 70 posições com contato; barra macho: 14 posições e 2 jumpers de encaixe.

## Posições sugeridas (placa de 100 x 176 mm, SUPOSTO)

Pino 1 de cada peça e o retângulo do corpo, em mm a partir do canto de cima à esquerda, vista do lado dos componentes. Ordem das zonas: faixa do topo (IR, botão de energia, microfone, CC1101 com a SMA para fora), DevKitC deitada com a antena na borda esquerda, tela, controles e PN532, MAX98357A no canto de baixo à esquerda, CN1 no canto de baixo à direita.

| Ref | Pino 1 (x, y) | Corpo (x0, y0, x1, y1) |
|---|---|---|
| U6 | 30.48, 4.06 | 28.5, -8.0, 37.6, 5.4 |
| U7 | 40.64, 4.06 | 38.6, -8.0, 47.7, 5.4 |
| SW11 | 51.61, 4.86 | 51.7, 3.9, 58.1, 10.3 |
| U4 | 64.01, 3.05 | 61.5, 1.0, 71.6, 15.2 |
| U2 | 81.28, 11.18 | 77.3, -8.0, 92.9, 15.2 |
| U1 | 2.54, 44.20 | 0.5, 20.1, 63.5, 45.5 |
| DS1 | 94.49, 57.91 | 3.0, 46.5, 97.0, 101.5 |
| JP2 | 87.88, 90.42 | 86.6, 89.1, 94.3, 91.7 |
| SW9 | 51.61, 103.41 | 51.7, 102.5, 58.1, 108.9 |
| SW10 | 61.77, 103.41 | 61.8, 102.5, 68.2, 108.9 |
| U8 | 67.06, 114.81 | 32.5, 113.3, 68.6, 123.9 |
| C1 | 42.67, 109.73 | 41.2, 108.2, 49.3, 111.2 |
| R1 | 20.32, 107.70 | 19.3, 106.5, 31.5, 108.9 |
| R2 | 20.32, 110.24 | 19.3, 109.0, 31.5, 111.4 |
| SW1 | 13.01, 121.70 | 13.1, 120.8, 19.5, 127.2 |
| SW2 | 13.01, 139.99 | 13.1, 139.0, 19.5, 145.4 |
| SW3 | 3.86, 130.85 | 3.9, 129.9, 10.3, 136.3 |
| SW4 | 22.15, 130.85 | 22.2, 129.9, 28.6, 136.3 |
| SW5 | 89.71, 130.85 | 89.8, 129.9, 96.2, 136.3 |
| SW6 | 80.57, 139.99 | 80.6, 139.0, 87.0, 145.4 |
| SW7 | 80.57, 121.70 | 80.6, 120.8, 87.0, 127.2 |
| SW8 | 71.43, 130.85 | 71.5, 129.9, 77.9, 136.3 |
| U3 | 46.23, 133.60 | 29.8, 132.1, 70.2, 174.8 |
| U5 | 2.54, 150.37 | 0.5, 148.9, 19.8, 168.4 |
| JP1 | 24.38, 152.40 | 23.1, 151.1, 30.8, 153.7 |
| CN1 | 74.17, 172.72 | 72.9, 171.4, 93.3, 174.0 |
