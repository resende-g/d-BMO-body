# Matriz de componentes do d-BMO v1

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Evidências | [`sources/ESP32-S3.md`](sources/ESP32-S3.md), [`sources/PERIPHERALS.md`](sources/PERIPHERALS.md), `README.md` ("Compras") |

Colunas de estado:

- **Compra**: situação informada no `README.md` (não conferida nesta sessão).
- **Bancada**: se o componente já funcionou em hardware, com qual firmware.
- **Firmware próprio**: situação em `firmware/` (testado no computador; nunca em hardware,
  salvo indicação).

## 1. Matriz

| Componente | Função | Interface | Alimentação (fonte) | Compra | Bancada | Firmware próprio |
|---|---|---|---|---|---|---|
| ESP32-S3-DevKitC-1 N16R8 (na PCB: módulo ESP32-S3-WROOM-1-N16R8) | Controlador único | — | 3,0 a 3,6 V, fonte ≥ 500 mA (S3-PWR-01, S3-PWR-02) | Disponível | **Validado** com Bruce e com os sketches de teste | Compila na CI; nunca gravado |
| Tela TFT 3,2" ILI9341 (módulo MSP3218, com toque não usado) | Rosto, menu, status | SPI + DC, RST, BL | Lógica 1,65 a 3,3 V; analógica 2,5 a 3,3 V (ILI-01, ILI-02). Módulo: não lido (MSP-01 a MSP-04) | Disponível | **Validado**: ID 0x9341 a 10 MHz, VCC e LED em 3,3 V (MSP-09) | `display`, `rosto`: testados no computador |
| Slot SD da própria tela | Configuração, log, códigos | SPI (CS próprio) | Pinos do slot no módulo: não lidos (MSP-06); slot de SD tamanho normal (MSP-08) | Cartão disponível | Não testado | `storage`: testado no computador |
| CC1101 433 MHz | Rádio sub-GHz | SPI + GDO0 | 1,8 a 3,6 V; máximo 3,9 V (CC-01, CC-02) | Recebido | Não testado | `radio`: testado com simulador |
| PN532 V3 | NFC | I2C (0x24) | VBAT 2,7 a 5,5 V; interface 1,6 a 3,6 V (PN-01, PN-02) | Comprado, aguardando | Não testado | `nfc`: testado com simulador |
| MCP23017 (1 ou 2) | Botões | I2C (0x20, 0x21) | 1,8 a 5,5 V (MCP-08) | Disponível | Não testado | `input`: testado com simulador |
| Botões tácteis | D-pad, ações, ombros, energia | MCP23017 / GPIO | — | Estoque | **3 botões em GPIO validados** | `input` |
| Analógicos de controle de PS3 | 2 sticks (4 eixos + 2 cliques) | ADC1 | 3,3 V, nunca 5 V (README) | Reaproveitar | Não testado | `input`: testado no computador |
| MAX98357A | Amplificador | I2S + SD_MODE | 2,5 a 5,5 V (AMP-01) | Comprado, aguardando | Não testado | `audio`: testado no computador |
| Alto-falante 2030, 8 Ω, 2 W | Saída de som | — | — | Comprado, aguardando | Não testado | — |
| INMP441 | Microfone | I2S | 1,62 a 3,63 V (MIC-01) | Comprado, aguardando | Não testado | `audio` |
| Conjunto IR (emissor + receptor de 38 kHz) | Infravermelho | RMT | Receptor da família TSOP48xx: 2,5 a 5,5 V (IR-01). Peça real não identificada (IR-08) | Comprado, aguardando | Não testado | `infrared`: testado no computador |
| Bateria LiPo 1S ~4000 mAh | Energia | — | 3,0 a 4,2 V (nominal de célula de lítio; datasheet da célula não existe) | Selecionada; compra não confirmada | — | `power`: leitura desabilitada |
| Carga, proteção, regulador, trava | Energia | GPIO / a definir | A definir ([`POWER_ARCHITECTURE.md`](POWER_ARCHITECTURE.md)) | Não comprado | — | `power`: contratos prontos |
| Soquete card edge DK920A-20M | Cartucho | I2C, SPI, UART, GPIO | 3,3 V limitado; 5 V em aberto | Não comprado (fase 4) | — | `cartridges`: testado com simulador |
| EEPROM 24C02 | Identificação do cartucho | I2C (0x50) | 2,5 a 5,5 V na Microchip 24LC02B (EE-01) | Não comprado | — | `cartridges` |

## 2. Níveis lógicos

Todos os componentes operam em lógica de 3,3 V, com uma ressalva por componente:

| Componente | Observação | Evidência |
|---|---|---|
| CC1101 | Máximo absoluto de 3,9 V em qualquer pino: **nunca 5 V** | CC-02 |
| INMP441 | Máximo de 3,63 V | MIC-02 |
| MAX98357A | Entradas de áudio aceitam até 6 V; o pino SD_MODE só até VDD + 0,3 V: com o amplificador na bateria abaixo de 3,3 V, o GPIO em 3,3 V viola o limite sem resistor em série | AMP-02, AMP-07 |
| PN532 | Interface de 1,6 a 3,6 V mesmo com VBAT em 5 V | PN-02 |
| MCP23017 | Limiar alto de 0,8 × VDD: alimentado em 3,3 V, tudo certo; em 5 V, o I2C de 3,3 V não atinge o limiar | MCP-16 |
| Tela | Tensão lógica do módulo não lida; funcionou em 3,3 V na bancada | MSP-02, MSP-09 |

## 3. Riscos por componente

| Componente | Risco | Evidência | Estado |
|---|---|---|---|
| MCP23017 | GPA7 e GPB7 são só saída na revisão D do datasheet: 14 entradas, não 16 | MCP-03 a MCP-07 | Tratado em `pins.h`; os 2 cliques dos analógicos precisam de outro lugar |
| EEPROM 24C02 | Tamanho de página e endereçamento variam por fabricante | EE-02, EE-03, EE-08, EE-11 | Tratado no driver (página de 8, faixa 0x50–0x57 reservada) |
| CC1101 | Cristal e casamento de antena do módulo comprado não conferidos | CC-21 | `radio.xtal_khz` configurável; conferir na peça |
| PN532 | I2C com o ESP32 pode exigir o modo HSU (README); endereço não conferido em fonte da NXP | PN-09, PN-13 | Em aberto |
| Tela | Corrente do backlight desconhecida; leitura acima de 6,67 MHz fora do datasheet | MSP-05, ILI-08 | Em aberto |
| MAX98357A | Estado no boot (GPIO 3 flutuante); resistores de SD_MODE e de ganho da placa comprada não conferidos | S3-STRAP-14, AMP-15 | Em aberto |
| INMP441 | Módulo: ligação de CHIPEN e de L/R não conferida | MIC-12 | Em aberto |
| Conjunto IR | Peça do receptor, corrente do LED e necessidade de transistor desconhecidas | IR-08 a IR-10 | Em aberto |
| Bateria | Dimensão do anúncio inconsistente; sem datasheet; exigência de PCM e NTC | README | Em aberto |
| Cartão SD | Slot da tela no SPI compartilhado: ordem de inicialização e pull-ups | SD-06 a SD-10 | Firmware inicia o cartão primeiro; elétrica em aberto |

## 4. O que não é usado no v1

| Item | Motivo |
|---|---|
| Raspberry Pi ou outro SBC | Decisão de plataforma (`README.md`, "Decisões de projeto") |
| Toque resistivo da tela | Pinos T_* sem ligação |
| Módulo microSD avulso com conversor de nível | Removido pelo README; usa-se o slot da tela |
| Módulos avulsos de carga e de boost | Substituídos por componentes SMD na PCB |
| Antena externa de 2,4 GHz | O módulo tem antena de trilha |
