# Registro de evidências: periféricos do d-BMO v1

## 1. Cabeçalho

**Escopo.** Fatos elétricos e de interface dos periféricos do d-BMO v1 (controlador ESP32-S3 N16R8), reunidos para decidir o que pode compartilhar barramento (SPI, I2C, I2S): controlador ILI9341 e módulo de tela MSP3218, CC1101, PN532 (CI e módulo Elechouse V3), MCP23017, INMP441, MAX98357A, EEPROM 24C02, receptor e emissor de infravermelho e cartão SD em modo SPI. Este registro não cobre o ESP32-S3, a bateria, o carregador nem os reguladores, e não fecha nenhuma decisão em aberto.

**Data de acesso.** 2026-11-12, para todas as fontes.

**Método.** Cada documento foi baixado, convertido para texto (`pdftotext -layout`) e lido localmente; os trechos citados entre aspas foram copiados do texto extraído. Cada documento é identificado pelo número, revisão e data impressos nele próprio, e pelo SHA-256 do arquivo lido (seção 2). Os arquivos baixados não foram adicionados ao repositório. A busca na web serviu só para localizar cópias dos documentos; resumo de resultado de busca não foi aceito como evidência e, quando citado, aparece como "indício indireto" em linha NÃO VERIFICADO.

**Limitação de acesso que afeta todo o registro.** Nesta sessão, os sites dos fabricantes não puderam ser abertos. Tentativas feitas e recusadas pela política de rede da sessão: `www.ti.com`, `ww1.microchip.com`, `docs.espressif.com` e `www.lcdwiki.com`. Depois dessas quatro recusas, os demais sites de fabricantes (`nxp.com`, `analog.com`, `invensense.tdk.com`, `st.com`, `vishay.com`, `sdcard.org`, `elechouse.com`) não foram tentados. Dos domínios testados, só o GitHub respondeu (`raw.githubusercontent.com`, `github.com`). Por isso:

- os documentos lidos são **os documentos dos fabricantes, mas em cópias hospedadas em repositórios de terceiros no GitHub**. A URL canônica do fabricante aparece na seção 2 apenas como referência e está marcada como não aberta;
- não foi possível conferir se cada cópia é a revisão mais recente publicada pelo fabricante. Onde isso importa, está dito na linha correspondente;
- a documentação do ESP-IDF foi lida no código-fonte da documentação, no repositório oficial `espressif/esp-idf` (ramo `release/v5.5`), e não na página renderizada de `docs.espressif.com`;
- nada do LCDwiki (módulo MSP3218) pôde ser lido. Toda a seção do módulo de tela está como NÃO VERIFICADO;
- o manual de usuário UM0701-02 e o datasheet completo do PN532 (NXP) não foram encontrados em cópia legível. O que se sabe do protocolo I2C do PN532 vem da biblioteca da Elechouse (fabricante do módulo), e está identificado assim.

Para refazer a verificação nas fontes canônicas, é preciso liberar esses domínios nas configurações de rede da sessão.

**Significado da coluna Status.**

- **CONFIRMADO na fonte**: lido nesta sessão no documento indicado na coluna Fonte, no local indicado. Quando a fonte é código (biblioteca ou driver) e não um datasheet, isso está escrito na linha.
- **NÃO VERIFICADO**: não lido em fonte primária nesta sessão. Inclui o que só apareceu em resumo de resultado de busca (marcado como "indício indireto"), o que depende de peça ainda não identificada e o que é inferência.

Valores marcados como "derivado" foram calculados a partir de valores confirmados; a conta está indicada.

## 2. Fontes abertas nesta sessão

| Ref. | Documento (identificação impressa no documento) | URL aberta nesta sessão | URL canônica do fabricante (não aberta) | SHA-256 do arquivo lido |
|---|---|---|---|---|
| F-ILI | ILI Technology Corp., "ILI9341 a-Si TFT LCD Single Chip Driver 240RGBx320 Resolution and 262K color". Capa: "Version: V1.11", "Document No.: ILI9341_DS_V1.11.pdf"; o histórico de revisões vai até V1.13 (2011/07/20); título nos metadados do PDF: `ILI9341_DS_V1.13_20110805`. 245 páginas | <https://raw.githubusercontent.com/m5stack/M5Stack_TFT_ILI9341/m5stack-default/Documents/ILI9341.pdf> | Sem URL pública do fabricante verificada | `a9bbfdf6d078f54a6aca7a56cba91246905358d3a4ed738817bfd3f582b5741c` |
| F-CC | Texas Instruments, "CC1101 Low-Power Sub-1 GHz RF Transceiver", SWRS061I (histórico: 2013.11.05). 98 páginas numeradas mais adendo de encapsulamento; PDF gerado em 2025-03-19 | <https://raw.githubusercontent.com/CaelC/cc1101_only/master/cc1101.pdf> | <https://www.ti.com/lit/ds/symlink/cc1101.pdf> (recusada) | `a9314dc36ea0ca424bb47f5e5ce12125a76b3405467092fdf483b3c8cb8e9d6f` |
| F-PN | NXP, "PN532/C1 Near Field Communication (NFC) controller", Product short data sheet, Rev. 3.0, 10 November 2011, documento 120130. 27 páginas | <https://raw.githubusercontent.com/bento-electronics/NFC-Module-PN532/master/datasheet.pdf> | Datasheet completo: <https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf>; manual UM0701-02: <https://www.nxp.com/docs/en/user-guide/141520.pdf> (nenhum dos dois aberto) | `9f10d7b2aa4e948908c73b50a70dfda47e5986d8a8f6a898fd0533b93f71ab36` |
| F-ELM | Elechouse, "PN532 NFC RFID Module User Guide, Version 3", revisão B, "Nov. 5th, 2013". 13 páginas | <https://raw.githubusercontent.com/bento-electronics/NFC-Module-PN532/master/module-manual.pdf> | Endereço no site da Elechouse não verificado nesta sessão | `3800d5935532d664015514c0199d2ce4313b3feccf563759642b424048a0cf07` |
| F-ELL | Elechouse, biblioteca `PN532` (código-fonte), ramo `PN532_HSU`: `PN532_I2C/PN532_I2C.cpp`, `PN532/PN532Interface.h`, `PN532_HSU/PN532_HSU.cpp`, `PN532_SPI/PN532_SPI.cpp` | <https://github.com/elechouse/PN532/tree/PN532_HSU> | É o repositório oficial do fabricante do módulo | não se aplica (texto) |
| F-MCP-D | Microchip, "MCP23017/MCP23S17 16-Bit I/O Expander with Serial Interface", DS20001952D, "2005-2022", "Revision D (June 2022)". 40 páginas | <https://raw.githubusercontent.com/Longan-Labs/XIAO_IO_EXPANDER_BOARD/main/MCP23017_Data_Sheet_DS20001952-2998473.pdf> | <https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf> (recusada) | `63cb5f2bec44434cdeeada1790d0316c9dc06b33febb489ad87bb0e2d540496a` |
| F-MCP-B | Microchip, mesmo título, DS21952B, "© 2007" (revisão B). 48 páginas. Usado só para comparação entre revisões | <https://raw.githubusercontent.com/mcauser/micropython-mcp23017/master/docs/mcp23017.pdf> | — | `da9b6082298056ef32b19cebc863df947bab3b286b434924b5a2f4fd5d1eff20` |
| F-MIC | InvenSense, "INMP441 Omnidirectional Microphone with Bottom Port and I2S Digital Output", DS-INMP441-00, Revision 1.1, Rev Date 05/21/2014. 21 páginas numeradas | <https://raw.githubusercontent.com/adrianviniciuscs/inmp441_mic/main/docs/INMP441-datasheet.pdf> | <https://invensense.tdk.com/wp-content/uploads/2015/02/INMP441.pdf> (não aberta) | `b33fd575c8c84584ede5ba16f11c0863f055fd24ef506b709c58df599af7908e` |
| F-AMP | Maxim Integrated, "MAX98357A/MAX98357B Tiny, Low-Cost, PCM Class D Amplifier with Class AB Performance", 19-6779; Rev 13; 7/19. 38 páginas | <https://raw.githubusercontent.com/Xinyuan-LilyGO/LilyGo-HAL/master/MAX98357A/MAX98357A-MAX98357B.pdf> | <https://www.analog.com/media/en/technical-documentation/data-sheets/max98357a-max98357b.pdf> (não aberta) | `08c8df5d99eda52b415097c2edafd4d78d869db8dfb0a9821d0c23ff1276f251` |
| F-EE-M | Microchip, "24AA02/24LC02B/24FC02 2K I2C Serial EEPROM", DS20001709N, "2007-2021". 51 páginas | <https://raw.githubusercontent.com/sparkfun/SparkFun_External_EEPROM_Arduino_Library_Docs/main/24LC02%20-%202k.pdf> | Site da Microchip (recusado) | `1964da9dcb7ebd2623a08284a958a1f0f1f8b65dec13effe1d1fca99c2582d40` |
| F-EE-A | Atmel, "Two-wire Serial EEPROM AT24C01A / AT24C02 / AT24C04 / AT24C08A / AT24C16A", 0180Z–SEEPR–11/06. 27 páginas. O próprio documento marca o AT24C02 como "Not Recommended for new design; Please refer to AT24C02B datasheet" | <https://raw.githubusercontent.com/yhyuan/STM32-Examples/master/16-M3-IIC%20(EEPROM-24C02)/datasheet/AT24C02.pdf> | Site da Microchip (recusado) | `cfee606d7ba7e89087b8a517067e010d0025a95768a4ca8be95788d9fca26e39` |
| F-EE-S | STMicroelectronics, pacote X-CUBE-EEPRMA1 (código-fonte), arquivo `Drivers/BSP/EEPRMA2/eeprma2_m24.c`, ramo `main` | <https://raw.githubusercontent.com/STMicroelectronics/X-CUBE-EEPRMA1/main/Drivers/BSP/EEPRMA2/eeprma2_m24.c> | Datasheet do M24C02 em `st.com` (não aberto) | não se aplica (texto) |
| F-IR | Vishay Semiconductors, "TSOP22.., TSOP24.., TSOP48.., TSOP44.. IR Receiver Modules for Remote Control Systems", Document Number 82459, Rev. 2.2, 14-Dec-2021. 8 páginas | <https://raw.githubusercontent.com/tarantula3/IR-Remote-Tester-and-Decoder/main/Datasheet-tsop48.pdf> | Site da Vishay (não aberto) | `f18a5dfe78850e98704d332dd38b5f34afbbf09e159c1cadbcf808fa92ba4da5` |
| F-SD | SD Group / SD Card Association, "SD Specifications Part 1 Physical Layer Simplified Specification Version 4.10", January 22, 2013. 202 páginas | <https://raw.githubusercontent.com/simon-77/VDHL-SD-Library/master/SD%20Specification_Part%201_Physical%20Layer_Simplified_v4.10_2013.pdf> | `sdcard.org` (não aberto) | `7c52029fbc950d1fbb88669d3b91268ff62f253c47f98f5c87eaa29201ffb85d` |
| F-IDF-1 | Espressif, ESP-IDF, ramo `release/v5.5`, `docs/en/api-reference/peripherals/sdspi_share.rst` ("Sharing the SPI Bus Among SD Cards and Other SPI Devices") | <https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/docs/en/api-reference/peripherals/sdspi_share.rst> | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/sdspi_share.html> (recusada) | não se aplica (texto) |
| F-IDF-2 | Idem, `docs/en/api-reference/peripherals/sd_pullup_requirements.rst` ("SD Pull-up Requirements") | <https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/docs/en/api-reference/peripherals/sd_pullup_requirements.rst> | Página equivalente em `docs.espressif.com` (recusada) | não se aplica (texto) |
| F-IDF-3 | Idem, `docs/en/api-reference/peripherals/sdspi_host.rst` ("SD SPI Host Driver") | <https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/docs/en/api-reference/peripherals/sdspi_host.rst> | Página equivalente em `docs.espressif.com` (recusada) | não se aplica (texto) |
| F-IDF-4 | Idem, código-fonte `components/esp_driver_sdspi/src/sdspi_host.c` e `components/sdmmc/include/sd_protocol_types.h` | <https://github.com/espressif/esp-idf/tree/release/v5.5/components> | — | não se aplica (texto) |

Fontes do próprio repositório usadas para comparação: `README.md`, `firmware/config/include/dbmo/config/pins.h`, `firmware/bruce/bmo.ini`, `firmware/teste_tela/teste_tela.ino`, `firmware/teste_cc1101/teste_cc1101.ino`. Os números de linha do `README.md` e dos sketches de bancada valem para a árvore de trabalho de 2026-11-12. Como `pins.h` e `bmo.ini` estavam sendo editados por outras frentes durante esta sessão, eles são citados por nome de símbolo ou de flag, não por linha.

## 3. ILI9341 (controlador da tela)

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| ILI-01 | Alimentação lógica (VDDI) | 1,65 V a 3,3 V | [ILI9341 datasheet][f-ili] | Seção 2 "Features", p. 8; seção 4 "Pin Descriptions", p. 10 | CONFIRMADO na fonte |
| ILI-02 | Alimentação analógica (VCI) | 2,5 V a 3,3 V | [ILI9341 datasheet][f-ili] | Seção 2, p. 8; seção 4, p. 10 | CONFIRMADO na fonte |
| ILI-03 | Níveis lógicos de entrada | VIH mínimo 0,7 × VDDI; VIL máximo 0,3 × VDDI | [ILI9341 datasheet][f-ili] | Seção 18.2.1 "General DC Characteristics" | CONFIRMADO na fonte |
| ILI-04 | Interface serial de 4 linhas | Sinais CSX, D/CX, SCL, SDI e SDO; "the MSB is transmitted first"; a interface é reiniciada com CSX em alto: "The serial interface is initialized when CSX is high status. In this state, SCL clock pulse and SDA data are no effect." | [ILI9341 datasheet][f-ili] | Seções 7.1.8 e 7.1.9, p. 33 a 35 | CONFIRMADO na fonte |
| ILI-05 | Borda de amostragem | Entrada: "The data is applied on the rising edge of the SCL signal." Saída (SDO): "The data is outputted on the falling edge of the SCL signal." | [ILI9341 datasheet][f-ili] | Seção 4, p. 11; seção 7.1.10, p. 36 | CONFIRMADO na fonte |
| ILI-06 | "Modo SPI 0" | O documento não usa a expressão "mode 0". A amostragem na borda de subida é compatível com CPOL=0/CPHA=0 (inferência). O `firmware/teste_tela` usa a biblioteca Adafruit ILI9341 e o README registra a tela como testada | [ILI9341 datasheet][f-ili] | Não há trecho literal | NÃO VERIFICADO |
| ILI-07 | Clock máximo de escrita (serial, 4 linhas) | Ciclo mínimo de 100 ns ("twc Serial clock cycle (Write) 100 ns min"), ou seja, 10 MHz (derivado: 1/100 ns) | [ILI9341 datasheet][f-ili] | Tabela da seção 18.3.4, p. 243 | CONFIRMADO na fonte |
| ILI-08 | Clock máximo de leitura (serial, 4 linhas) | Ciclo mínimo de 150 ns ("trc Serial clock cycle (Read) 150 ns min"), ou seja, cerca de 6,67 MHz (derivado: 1/150 ns) | [ILI9341 datasheet][f-ili] | Tabela da seção 18.3.4, p. 243 | CONFIRMADO na fonte |
| ILI-09 | Condições das tabelas de tempo | "Ta = 25 °C, VDDI=1.65V to 3.3V, VCI=2.5V to 3.3V"; tempos de subida e descida de até 15 ns | [ILI9341 datasheet][f-ili] | Nota da seção 18.3.4, p. 243 | CONFIRMADO na fonte |
| ILI-10 | SDO em alta impedância quando CSX está em alto | O documento não traz frase explícita. Só há o parâmetro "tod Output disable time (Read)", 10 ns mín. e 50 ns máx., e a instrução "If not used, open this pin" para o SDO | [ILI9341 datasheet][f-ili] | Tabela da seção 18.3.4, p. 243; seção 4, p. 11 | NÃO VERIFICADO |
| ILI-11 | Pinos não usados | Barramento paralelo não usado: "must be connected to GND"; SDO não usado: "open this pin" | [ILI9341 datasheet][f-ili] | Seção 7.1.8, p. 33; seção 4, p. 11 | CONFIRMADO na fonte |

## 4. Módulo de tela MSP3218 (LCDwiki)

Nenhuma página do LCDwiki pôde ser aberta. A página do produto seria <http://www.lcdwiki.com/3.2inch_SPI_Module_ILI9341_SKU:MSP3218> (endereço recusado pela rede da sessão). Tudo abaixo depende de leitura da página, do esquema elétrico do módulo ou de medição na peça.

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| MSP-01 | Tensão de alimentação do módulo (pino VCC) | Não lido. Indício indireto (resumo de busca citando cópia da tabela do LCDwiki em outro site): "VCC power voltage 3.3V~5V" | LCDwiki, página do MSP3218 (não aberta) | — | NÃO VERIFICADO |
| MSP-02 | Tensão lógica dos pinos | Não lido. Indício indireto (mesma origem): "Logic IO port voltage 3.3V(TTL)" | LCDwiki (não aberta) | — | NÃO VERIFICADO |
| MSP-03 | Regulador de 3,3 V na placa e jumper de alimentação | Não lido | LCDwiki, esquema elétrico (não aberto) | — | NÃO VERIFICADO |
| MSP-04 | Conversão de nível nos sinais | Não lido | LCDwiki, esquema elétrico (não aberto) | — | NÃO VERIFICADO |
| MSP-05 | Corrente do backlight e forma de controle (pino LED direto nos LEDs ou por transistor) | Não lido. Um resumo de busca citou "Backlight Current 95 mA" para outro produto (linha MSP3222); esse valor **não** vale para o MSP3218 | LCDwiki (não aberta) | — | NÃO VERIFICADO |
| MSP-06 | Pinos do slot SD e existência de resistores em série ou buffer | Não lido | LCDwiki, esquema elétrico (não aberto) | — | NÃO VERIFICADO |
| MSP-07 | Controlador de toque e pinos T_CLK, T_CS, T_DIN, T_DO, T_IRQ (sem uso no v1) | Não lido | LCDwiki (não aberta) | — | NÃO VERIFICADO |
| MSP-08 | O slot no verso é de SD tamanho normal | Observação física datada de 02/11/2026 | `README.md` | Linha 134 (lista de componentes) | NÃO VERIFICADO em fonte de fabricante; registrado no repositório |
| MSP-09 | A tela funcionou com VCC e LED em 3,3 V e devolveu o ID 0x9341 a 10 MHz | Registro de bancada | `README.md`; `firmware/teste_tela/teste_tela.ino`; `pins.h` | README linhas 72 e 631; comentário do sketch (linhas 2, 11 e 29); comentário de `spi::kDisplayHz` em `pins.h` | NÃO VERIFICADO em fonte de fabricante; VALIDADO EM BANCADA segundo o repositório |

## 5. CC1101 (rádio sub-GHz)

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| CC-01 | Tensão de operação | 1,8 V a 3,6 V ("All supply pins must have the same voltage") | [CC1101 SWRS061I][f-cc] | Table 2, p. 8 | CONFIRMADO na fonte |
| CC-02 | Máximos absolutos | Alimentação: −0,3 V a 3,9 V. Qualquer pino digital: −0,3 V a VDD + 0,3 V, máximo 3,9 V | [CC1101 SWRS061I][f-cc] | Table 1, p. 8 | CONFIRMADO na fonte |
| CC-03 | Níveis lógicos | Entrada: nível 0 até 0,7 V; nível 1 a partir de VDD − 0,7 V. Saída: até 0,5 V e a partir de VDD − 0,3 V, para até 4 mA | [CC1101 SWRS061I][f-cc] | Table 17, p. 20 | CONFIRMADO na fonte |
| CC-04 | Faixas de frequência | 300–348 MHz; 387–464 MHz ("If using a 27 MHz crystal, the lower frequency limit for this band is 392 MHz"); 779–928 MHz | [CC1101 SWRS061I][f-cc] | Table 3, p. 8 | CONFIRMADO na fonte |
| CC-05 | Interface SPI | 4 fios (SI, SO, SCLK, CSn), CC1101 como escravo, bit mais significativo primeiro; cabeçalho com bit R/W, bit de rajada e 6 bits de endereço | [CC1101 SWRS061I][f-cc] | Seção 10, p. 29 | CONFIRMADO na fonte |
| CC-06 | Clock SPI máximo | 10 MHz com atraso de 100 ns entre endereço e dado (e entre bytes na rajada); 9 MHz em acesso simples sem atraso; 6,5 MHz em rajada sem atraso | [CC1101 SWRS061I][f-cc] | Table 22, p. 30 | CONFIRMADO na fonte |
| CC-07 | Borda de amostragem e "modo SPI 0" | Os tempos de setup e hold são dados em relação à borda de subida do SCLK ("Setup data (negative SCLK edge) to positive edge on SCLK"; "Hold data after positive edge on SCLK"). A expressão "mode 0" não aparece; compatibilidade com modo 0 é inferência | [CC1101 SWRS061I][f-cc] | Table 22 e Figure 15, p. 30 | CONFIRMADO na fonte (bordas); NÃO VERIFICADO (rótulo "modo 0") |
| CC-08 | SO em alta impedância com CSn em alto | A Figure 15 marca SO como "Hi-Z" antes e depois da transferência. Texto: "GDO1 is the same pin as the SO pin on the SPI interface, thus the output programmed on this pin will only be valid when CSn is high. The default value for GDO1 is 3-stated which is useful when the SPI interface is shared with other devices." | [CC1101 SWRS061I][f-cc] | Figure 15, p. 30; seção 26, p. 61; IOCFG1 (padrão 0x2E), p. 71 | CONFIRMADO na fonte |
| CC-09 | CHIP_RDYn no pino SO | "When CSn is pulled low, the MCU must wait until CC1101 SO pin goes low before starting to transfer the header byte. This indicates that the crystal is running. Unless the chip was in the SLEEP or XOFF states, the SO pin will always go low immediately after taking CSn low." | [CC1101 SWRS061I][f-cc] | Seção 10, p. 29; seção 10.1 e Table 23, p. 31 | CONFIRMADO na fonte |
| CC-10 | Tempo de CSn baixo até o primeiro clock ao sair de power-down | 150 µs mínimo (medido com o cristal do projeto de referência; depende do cristal); 20 ns em modo ativo | [CC1101 SWRS061I][f-cc] | Table 22 e nota, p. 30 | CONFIRMADO na fonte |
| CC-11 | Função dos pinos GDO | GDO0: "Digital I/O" (inclui "Serial input TX data"); GDO2: "Digital Output"; SO (GDO1): "Optional general output pin when CSn is high" | [CC1101 SWRS061I][f-cc] | Table 19, p. 21 | CONFIRMADO na fonte |
| CC-12 | Valores padrão após o reset | GDO0: CLK_XOSC/192 (0x3F), "a 135-141 kHz clock output"; GDO1: alta impedância (0x2E); GDO2: CHIP_RDYn (0x29). O datasheet recomenda trocar a função do GDO0 para otimizar o desempenho em RX e TX | [CC1101 SWRS061I][f-cc] | Seção 26, p. 61; Table 41, p. 62; IOCFG2/1/0, p. 71; seção 19.1, p. 50 e 51 | CONFIRMADO na fonte |
| CC-13 | Registradores de identificação | PARTNUM (0x30, lido como 0xF0): 0x00. VERSION (0x31, lido como 0xF1): 0x14 (20), "Subject to change without notice". O histórico registra a mudança de 0x04 para 0x14 na revisão SWRS061I | [CC1101 SWRS061I][f-cc] | Seção 29.3, p. 92; seção 33.1, p. 97 | CONFIRMADO na fonte |
| CC-14 | Leitura dos registradores de status | Para endereços 0x30 a 0x3D, o bit de rajada em 1 seleciona registrador de status e em 0 seleciona comando; "burst access is not available for status registers and they must be accessed one at a time" | [CC1101 SWRS061I][f-cc] | Seção 10.2, p. 31 e 32 | CONFIRMADO na fonte |
| CC-15 | Corrente em recepção, 433 MHz | 16,0 mA (1,2 kBaud, sinal no limite de sensibilidade) e 15,0 mA (sinal bem acima); 15,7 mA e 15,0 mA (38,4 kBaud); 17,1 mA e 15,7 mA (250 kBaud). Condições: 25 °C, VDD = 3,0 V, valores típicos | [CC1101 SWRS061I][f-cc] | Table 4, p. 10 | CONFIRMADO na fonte |
| CC-16 | Corrente em transmissão, 433 MHz | +10 dBm: 29,2 mA; 0 dBm: 16,0 mA; −6 dBm: 13,1 mA (típicos, 25 °C, 3,0 V) | [CC1101 SWRS061I][f-cc] | Table 4, p. 10 | CONFIRMADO na fonte |
| CC-17 | Corrente em transmissão por nível de potência, 433 MHz, com indutores multicamada | −30 dBm: 11,9 mA; −20: 12,4; −15: 13,1; −10: 14,4; 0: 15,9; +5: 19,4; +7: 24,2; +10: 29,1 mA. Ajuste padrão de PATABLE (0xC6): 7,8 dBm e 25,2 mA | [CC1101 SWRS061I][f-cc] | Table 39 e Table 40, p. 60 | CONFIRMADO na fonte |
| CC-18 | Potência de saída em 433 MHz | Máxima: +10 dBm; mínima: −30 dBm ("Output power may be restricted by regulatory limits") | [CC1101 SWRS061I][f-cc] | Table 10, p. 16 | CONFIRMADO na fonte |
| CC-19 | Correntes de repouso | SLEEP: 0,2 µA típico, 1 µA máximo; XOFF: 165 µA; IDLE: 1,7 mA; FSTXON: 8,4 mA | [CC1101 SWRS061I][f-cc] | Table 4, p. 9 | CONFIRMADO na fonte |
| CC-20 | Reset na energização | Rampa de alimentação de até 5 ms (0 V a 1,8 V); sequência manual com SRES descrita no documento | [CC1101 SWRS061I][f-cc] | Table 18, p. 20; seção 19.1, p. 50 e 51 | CONFIRMADO na fonte |
| CC-21 | Dados do módulo comprado (frequência do cristal, casamento de antena para 433 MHz, capacitores) | Peça não caracterizada; o README registra o módulo como recebido e ainda não testado | `README.md` | Linhas 72 e 128 | NÃO VERIFICADO |

## 6. PN532 (CI, NXP)

O único documento da NXP lido foi a folha de dados resumida (F-PN), que não descreve o protocolo do host. As linhas PN-08 a PN-12 foram confirmadas no código da biblioteca da Elechouse, não em documento da NXP.

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| PN-01 | Alimentação (VBAT) | 2,7 V a 5,5 V | [PN532/C1 short data sheet][f-pn] | Table 1, p. 3; seção 2, p. 2 | CONFIRMADO na fonte |
| PN-02 | Alimentação da interface com o host (PVDD) | 1,6 V a 3,6 V | [PN532/C1 short data sheet][f-pn] | Table 1, p. 3 | CONFIRMADO na fonte |
| PN-03 | Valores-limite | PVDD: −0,5 V a 4 V; VBAT: −0,5 V a 6,0 V; corrente máxima em TVDD: 150 mA | [PN532/C1 short data sheet][f-pn] | Table 12, p. 20 | CONFIRMADO na fonte |
| PN-04 | Regulador interno | "An embedded low-dropout voltage regulator allows the device to be connected directly to a battery." Saída do LDO: 2,7 / 3 / 3,4 V (mín./típ./máx.) com VBAT > 3,4 V | [PN532/C1 short data sheet][f-pn] | Seção 1, p. 2; Table 1, p. 3 | CONFIRMADO na fonte |
| PN-05 | Interfaces com o host | SPI, I2C e High Speed UART (HSU); "Dedicated host interrupts"; "Automatic wake-up on I2C, HSU and SPI interfaces when device is in Power-down mode" | [PN532/C1 short data sheet][f-pn] | Seção 1 e seção 2, p. 2 | CONFIRMADO na fonte |
| PN-06 | Correntes com VBAT = 5 V | Digital (IDVDD): 25 mA típico; analógica (IAVDD): 6 mA típico; transmissor (ITVDD) durante a transmissão de RF: 60 mA típico com antena sintonizada em 50 Ω, 150 mA máximo. Soma dos típicos com o campo ligado: 91 mA (derivado: 25 + 6 + 60) | [PN532/C1 short data sheet][f-pn] | Table 1 e notas, p. 3 | CONFIRMADO na fonte |
| PN-07 | Modos de baixo consumo | Hard-Power-Down: 1 µA típico, 2 µA máximo. Soft-Power-Down: 22 µA típico, 45 µA máximo (detector de nível de RF ligado) | [PN532/C1 short data sheet][f-pn] | Seção 2, p. 2; Table 1, p. 3 | CONFIRMADO na fonte |
| PN-08 | Endereço I2C | `#define PN532_I2C_ADDRESS (0x48 >> 1)`, ou seja, 0x24 em 7 bits (0x48 em 8 bits, escrita). Fonte: código da biblioteca da Elechouse | [Biblioteca Elechouse PN532][f-ell] | `PN532_I2C/PN532_I2C.cpp`, linha 9 | CONFIRMADO na fonte (código da Elechouse) |
| PN-09 | Endereço I2C segundo a NXP | Não lido. Indício indireto (resumo de busca sobre o UM0701-02): "write operation at PN532 address (0x48), read operation at PN532 address (0x49)" | NXP UM0701-02 (não aberto) | — | NÃO VERIFICADO |
| PN-10 | Sinalização de pronto em I2C | A biblioteca lê o dispositivo e testa o bit 0 do primeiro byte ("check first byte --- status ... PN532 is ready"), repetindo a cada 1 ms até o tempo limite; não usa a linha IRQ. Depois do comando, lê um quadro de ACK | [Biblioteca Elechouse PN532][f-ell] | `PN532_I2C/PN532_I2C.cpp`, funções `getResponseLength` e `writeCommand` (linhas 28 a 115) | CONFIRMADO na fonte (código da Elechouse) |
| PN-11 | Formato de quadro | Preâmbulo 0x00; código de início 0x00 0xFF; comprimento e seu complemento; identificador 0xD4 (host para PN532) ou 0xD5 (PN532 para host); dados; soma de verificação; pós-âmbulo 0x00 | [Biblioteca Elechouse PN532][f-ell] | `PN532/PN532Interface.h` (constantes) e `PN532_I2C.cpp`, linhas 33 a 70 | CONFIRMADO na fonte (código da Elechouse) |
| PN-12 | Alternativas HSU e SPI | HSU: `begin(115200)` e sequência de despertar iniciada por 0x55 0x55. SPI: `SPI_MODE0` ("PN532 only supports mode0"), bit menos significativo primeiro, comentário "set clock 2MHz(max: 5MHz)" | [Biblioteca Elechouse PN532][f-ell] | `PN532_HSU/PN532_HSU.cpp`, linhas 12 a 20; `PN532_SPI/PN532_SPI.cpp`, linhas 22 a 28 | CONFIRMADO na fonte (código da Elechouse) |
| PN-13 | Velocidade máxima de I2C, uso das linhas IRQ e H_REQ, esticamento de clock, seleção de interface pelos pinos I0/I1 | Não lido | NXP UM0701-02 e datasheet completo PN532_C1 (não abertos) | — | NÃO VERIFICADO |

## 7. Módulo Elechouse PN532 NFC V3

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| ELE-01 | Alimentação do módulo | "VCC: 3.3V~5V" | [Elechouse PN532 User Guide V3][f-elm] | Seção "Interface" | CONFIRMADO na fonte |
| ELE-02 | Níveis de I2C e UART | "I2C/UART: 3.3V~24V TTL" (texto literal do manual; o valor de 24 V é do documento e não foi conferido em esquema) | [Elechouse PN532 User Guide V3][f-elm] | Seção "Interface" | CONFIRMADO na fonte |
| ELE-03 | Conversor de nível | "On-board level shifter, Standard 5V TTL for I2C and UART, 3.3V TTL SPI" | [Elechouse PN532 User Guide V3][f-elm] | Seção "Features", item 5 | CONFIRMADO na fonte |
| ELE-04 | Nível do SPI | "SPI: 3.3V TTL with 100 ohm resistors in series." | [Elechouse PN532 User Guide V3][f-elm] | Seção "Interface" | CONFIRMADO na fonte |
| ELE-05 | Modo padrão | "The HSU mode is configured as the default mode." | [Elechouse PN532 User Guide V3][f-elm] | Seção "Interface" | CONFIRMADO na fonte |
| ELE-06 | Chave de seleção de interface | HSU: canal 1 OFF, canal 2 OFF. I2C: canal 1 ON, canal 2 OFF. SPI: canal 1 OFF, canal 2 ON | [Elechouse PN532 User Guide V3][f-elm] | Tabela "Working Interface / Channel 1 / Channel 2" | CONFIRMADO na fonte |
| ELE-07 | Pinos compartilhados | "The I2C and HSU shares the same pins." | [Elechouse PN532 User Guide V3][f-elm] | Seção "Interface" | CONFIRMADO na fonte |
| ELE-08 | Dimensões e alcance declarados | 42,7 mm × 40,4 mm × 4 mm; leitura a 5–7 cm (declaração do fabricante do módulo) | [Elechouse PN532 User Guide V3][f-elm] | Lista de melhorias da V3 | CONFIRMADO na fonte |
| ELE-09 | Regulador na placa, valor dos resistores de pull-up de I2C e tensão em que estão ligados, pinos IRQ e RSTPDN no conector | O texto do manual não informa. O manual aponta para um esquema ("PN532 Module Schematic PDF"), que não foi aberto | Esquema do módulo (não aberto) | — | NÃO VERIFICADO |
| ELE-10 | O "PN532 Set" comprado corresponde ao projeto da Elechouse V3 | Peça ainda não recebida | `README.md` | Linha 129 | NÃO VERIFICADO |

## 8. MCP23017 (expansor de E/S)

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| MCP-01 | Revisão lida | DS20001952D, "Revision D (June 2022)", rodapé "2005-2022 Microchip Technology Inc. and its subsidiaries" | [MCP23017 DS20001952D][f-mcp-d] | Rodapés; Appendix A, p. 37 | CONFIRMADO na fonte |
| MCP-02 | Existe revisão posterior à D | Não conferido no site da Microchip. Indício indireto (resumo de busca): haveria uma cópia "DS20001952E" em site de distribuidor, com a mesma restrição de GPA7 e GPB7 | Site da Microchip (recusado) | — | NÃO VERIFICADO |
| MCP-03 | GPA7 e GPB7, lista de características | "16-Bit Remote Bidirectional I/O Port (Pins GPA7, GPB7 are output only for MCP23017)" | [MCP23017 DS20001952D][f-mcp-d] | "Features", p. 1 | CONFIRMADO na fonte |
| MCP-04 | GPA7 e GPB7, tabela de pinos | Para GPB7 e para GPA7: "Bidirectional I/O pin. Can be enabled for interrupt-on-change and/or internal weak pull-up resistor./ Output only (MCP23017)." | [MCP23017 DS20001952D][f-mcp-d] | Table 2-1, p. 11 | CONFIRMADO na fonte |
| MCP-05 | GPA7 e GPB7, registrador de direção | "Note: For MCP23017, IO7 must be set via I2C interface to "0" (output)." | [MCP23017 DS20001952D][f-mcp-d] | Seção 3.5.1, p. 18 | CONFIRMADO na fonte |
| MCP-06 | Onde a revisão D mudou | "Updated description in Section "Features". Updated Table 2-1. Updated drawings for MCP23017 in Section "Package Types". Updated Section 3.5.1 "I/O Direction Register"." | [MCP23017 DS20001952D][f-mcp-d] | Appendix A, "Revision D (June 2022)", p. 37 | CONFIRMADO na fonte |
| MCP-07 | A revisão B não trazia a restrição | No DS21952B (2007), a característica é "16-bit remote bidirectional I/O port"; a expressão "output only" não ocorre no documento | [MCP23017 DS21952B][f-mcp-b] | "Features", p. 1; busca no texto inteiro | CONFIRMADO na fonte |
| MCP-08 | Tensão de operação | 1,8 V a 5,5 V | [MCP23017 DS20001952D][f-mcp-d] | Table 1-1, parâmetro D001, p. 4 | CONFIRMADO na fonte |
| MCP-09 | Máximos absolutos | VDD: −0,3 V a +5,5 V; demais pinos: −0,6 V a VDD + 0,6 V; corrente por pino: 25 mA (drenada ou fornecida); máximo saindo por VSS: 150 mA; máximo entrando por VDD: 125 mA; dissipação total: 700 mW | [MCP23017 DS20001952D][f-mcp-d] | "Absolute Maximum Ratings", p. 3 | CONFIRMADO na fonte |
| MCP-10 | Velocidades de I2C por tensão | 100 kHz: 1,8 V a 5,5 V. 400 kHz: 2,7 V a 5,5 V. 1,7 MHz: 4,5 V a 5,5 V | [MCP23017 DS20001952D][f-mcp-d] | "Features", p. 1; Table 1-3, p. 6 | CONFIRMADO na fonte |
| MCP-11 | Endereço I2C | Byte de controle "0 1 0 0 A2 A1 A0 R/W"; "Three Hardware Address Pins to Allow Up to Eight Devices On the Bus". Em 7 bits, 0x20 a 0x27 (derivado do padrão de bits) | [MCP23017 DS20001952D][f-mcp-d] | "Features", p. 1; seção 3.3.1 e Figure 3-4, p. 15 | CONFIRMADO na fonte |
| MCP-12 | Pinos de endereço e de reset | A0, A1, A2: "Hardware address pin. Must be externally biased." RESET: "Hardware reset. Must be externally biased." | [MCP23017 DS20001952D][f-mcp-d] | Table 2-1, p. 11 | CONFIRMADO na fonte |
| MCP-13 | Pull-up interno | "If a bit is set and the corresponding pin is configured as an input, the corresponding port pin is internally pulled up with a 100 kΩ resistor." Corrente de pull-up (D070): 40 / 75 / 115 µA com VDD = 5 V | [MCP23017 DS20001952D][f-mcp-d] | Seção 3.5.7, p. 22; Table 1-1, p. 4 | CONFIRMADO na fonte |
| MCP-14 | Pinos de interrupção | INTA e INTB: "Can be configured as active-high, active-low or open-drain." Podem operar separados ou em OU (bit MIRROR); interrupção por mudança de estado ou por comparação com DEFVAL | [MCP23017 DS20001952D][f-mcp-d] | Table 2-1, p. 11; seção 3, p. 12; seção 3.5.6, p. 20 e 21 | CONFIRMADO na fonte |
| MCP-15 | Consumo | Corrente de alimentação: 1 mA máximo (clock de 1 MHz). Repouso: 1 µA máximo de −40 °C a +85 °C | [MCP23017 DS20001952D][f-mcp-d] | Table 1-1, D004 e D005, p. 4 | CONFIRMADO na fonte |
| MCP-16 | Limiares lógicos | SCL, SDA, GPIO e RESET: VIH mínimo 0,8 × VDD; VIL máximo 0,2 × VDD | [MCP23017 DS20001952D][f-mcp-d] | Table 1-1, D031 e D041, p. 4 | CONFIRMADO na fonte |

## 9. INMP441 (microfone I2S)

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| MIC-01 | Tensão de alimentação | 1,62 V a 3,63 V | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 1, p. 4 | CONFIRMADO na fonte |
| MIC-02 | Máximos absolutos | VDD: −0,3 V a +3,63 V. Pinos digitais: "−0.3 V to VDD + 0.3 V or 3.63 V, whichever is less" | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 4, p. 6 | CONFIRMADO na fonte |
| MIC-03 | Corrente | Com VDD = 3,3 V: operação normal 2,2 mA típico e 2,5 mA máximo; standby 0,8 mA máximo; power-down 4,5 µA máximo. Com VDD = 1,8 V: 1,4 mA típico e 1,6 mA máximo; standby 0,8 mA; power-down 2 µA | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 1, p. 4 | CONFIRMADO na fonte |
| MIC-04 | Formato I2S | "The slave serial-data port's format is I²S, 24-bit, twos complement. There must be 64 SCK cycles in each WS stereo frame, or 32 SCK cycles per data-word." E ainda: "The INMP441 must always have 64 clock cycles for every stereo data-word (fSCK = 64 × fWS)." Bit mais significativo primeiro, atrasado de um ciclo de SCK | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Seção "I²S Data Interface", p. 11 | CONFIRMADO na fonte |
| MIC-05 | Faixa de clock e de taxa de amostragem | SCK: 0,5 MHz a 3,2 MHz (período mínimo de 312 ns). WS: 7,8 kHz a 50 kHz | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 3, p. 5 | CONFIRMADO na fonte |
| MIC-06 | Pino L/R | "When set low, the microphone outputs its signal in the left channel of the I²S frame. When set high, the microphone outputs its signal in the right channel." | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 6, p. 8 | CONFIRMADO na fonte |
| MIC-07 | Saída em alta impedância fora do canal | "The output data pin (SD) is tri-stated when it is not actively driving I²S output data. SD immediately tri-states after the LSB is output so that another microphone can drive the common data line." O documento recomenda resistor de pull-down de 100 kΩ na linha SD | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 6, p. 8; seção "Data Output Mode", p. 11 | CONFIRMADO na fonte |
| MIC-08 | Tempo de partida | Saída zerada durante os primeiros 2^18 ciclos de SCK após a energização: "85ms with SCK at 3.072 MHz". Retorno do standby: 2^14 ciclos (5 ms). Retorno do power-down: 2^17 ciclos (43 ms). Os expoentes aparecem sobrescritos no PDF; os tempos em milissegundos confirmam os valores | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Seção "Power Management" e "Startup", p. 10 | CONFIRMADO na fonte |
| MIC-09 | Pino CHIPEN e standby | CHIPEN em baixo: power-down; em alto: habilitado. Standby quando o SCK para com CHIPEN em alto. "The INMP441 should not be transitioned from standby to power-down mode, or vice versa." | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 6, p. 8; p. 10 | CONFIRMADO na fonte |
| MIC-10 | Clocks sem alimentação | "It is not recommended to supply active clocks (WS and SCK) to the INMP441 while there is no power supplied to VDD." | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Seção "Power Management", p. 10 | CONFIRMADO na fonte |
| MIC-11 | Limiares lógicos | VIH mínimo 0,7 × VDD; VIL máximo 0,25 × VDD (L/R, WS, SCK) | [INMP441 DS-INMP441-00 rev. 1.1][f-mic] | Table 2, p. 5 | CONFIRMADO na fonte |
| MIC-12 | Placa do módulo comprado (ligação de CHIPEN, pull-down em SD, estado padrão de L/R) | Peça genérica ainda não recebida | `README.md` | Linha 133 | NÃO VERIFICADO |

## 10. MAX98357A (amplificador I2S)

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| AMP-01 | Tensão de alimentação | 2,5 V a 5,5 V; bloqueio por subtensão (UVLO): 1,5 / 1,8 / 2,3 V | [MAX98357A rev. 13][f-amp] | "Electrical Characteristics", p. 4 | CONFIRMADO na fonte |
| AMP-02 | Máximos absolutos | VDD, LRCLK, BCLK e DIN: −0,3 V a +6 V. Demais pinos (inclui SD_MODE e GAIN_SLOT): −0,3 V a VDD + 0,3 V | [MAX98357A rev. 13][f-amp] | "Absolute Maximum Ratings", p. 4 | CONFIRMADO na fonte |
| AMP-03 | Potência de saída em 8 Ω (ganho de 12 dB) | Com THD+N de 10%: 1,8 W a 5 V; 0,93 W a 3,7 V. Com THD+N de 1%: 1,4 W a 5 V; 0,77 W a 3,7 V. Em 4 Ω a 5 V: 3,2 W (10%) e 2,5 W (1%) | [MAX98357A rev. 13][f-amp] | "Electrical Characteristics", p. 5 | CONFIRMADO na fonte |
| AMP-04 | Correntes | Repouso: 2,75 mA típico e 3,35 mA máximo (5 V); 2,4 mA típico e 2,85 mA máximo (3,7 V). Desligado (SD_MODE = 0 V): 0,6 µA típico, 2 µA máximo. Standby (sem BCLK): 340 µA típico, 400 µA máximo. Limite de corrente de saída: 2,8 A típico | [MAX98357A rev. 13][f-amp] | "Electrical Characteristics", p. 4 e 5 | CONFIRMADO na fonte |
| AMP-05 | Limiares do pino SD_MODE | B0: 0,08 / 0,16 / 0,355 V; B1: 0,65 / 0,77 / 0,825 V; B2: 1,245 / 1,4 / 1,5 V (mín./típ./máx.). Resistor interno de pull-down: 92 / 100 / 108 kΩ | [MAX98357A rev. 13][f-amp] | "SD_MODE Comparator Trip Points", p. 7 | CONFIRMADO na fonte |
| AMP-06 | Função do pino SD_MODE | Acima de B2: canal esquerdo. Entre B1 e B2: canal direito. Entre B0 e B1: (esquerdo/2 + direito/2). Abaixo de B0: desligado. Resistores externos: RSMALL (kΩ) = 94,0 × VDDIO − 100; RLARGE (kΩ) = 222,2 × VDDIO − 100; para VDDIO = 3,3 V, 210,2 kΩ e 634 kΩ | [MAX98357A rev. 13][f-amp] | Table 5 e Table 6, p. 17 | CONFIRMADO na fonte |
| AMP-07 | Cuidado com SD_MODE acionado direto em nível alto | "Ensuring that VDD is always greater than VDDIO is one way to prevent SD_MODE from violating the Absolute Maximum Ratings limits. If this is not possible in the application (e.g., if VDD < 3.0V and VDDIO = 3.3V), then it is necessary to add a small resistance (~2kΩ) in series with SD_MODE to limit the current into the SD_MODE pin." | [MAX98357A rev. 13][f-amp] | Seção "SD_MODE and Shutdown Operation", p. 17 | CONFIRMADO na fonte |
| AMP-08 | Taxas de amostragem | "LRCLK ONLY supports 8kHz, 16kHz, 32kHz, 44.1kHz, 48kHz, 88.2kHz and 96kHz frequencies. LRCLK clocks at 11.025kHz, 12kHz, 22.05kHz and 24kHz are NOT supported." Faixas da tabela: 7,6–8,4; 15,2–16,8; 30,4–50,4; 83,8–100,8 kHz | [MAX98357A rev. 13][f-amp] | Seção "LRCLK Polarity", p. 16; "Digital Audio Interface", p. 6 | CONFIRMADO na fonte |
| AMP-09 | Tamanho de palavra e relação de clocks | 16, 24 ou 32 bits em I2S; "BCLK must be 32, 48, or 64X of LRCLK"; BCLK de 0,2432 MHz a 25,804 MHz | [MAX98357A rev. 13][f-amp] | "Digital Audio Interface", p. 6 | CONFIRMADO na fonte |
| AMP-10 | MCLK | "No MCLK Required"; "The ICs eliminate the need for the external MCLK signal" | [MAX98357A rev. 13][f-amp] | "Features", p. 1; seção "MCLK Elimination", p. 16 | CONFIRMADO na fonte |
| AMP-11 | Opções de ganho (pino GAIN_SLOT) | Ao GND por 100 kΩ: 15 dB; ao GND: 12 dB; desconectado: 9 dB; ao VDD: 6 dB; ao VDD por 100 kΩ: 3 dB | [MAX98357A rev. 13][f-amp] | "Electrical Characteristics", p. 5 | CONFIRMADO na fonte |
| AMP-12 | Limiares das entradas de áudio | VIH mínimo 1,3 V; VIL máximo 0,6 V | [MAX98357A rev. 13][f-amp] | "Electrical Characteristics", p. 6 e 7 | CONFIRMADO na fonte |
| AMP-13 | Restrições de clock em operação | "Do not remove LRCLK while BCLK is present. Removing LRCLK while BCLK is present can cause unexpected output behavior including a large DC output voltage." Entra em standby sozinho quando o BCLK para. Na partida, "LRCLK must start within 1/2 LRCLK period of BCLK starting". Tempo de ligar: 7 ms típico | [MAX98357A rev. 13][f-amp] | p. 16 a 18; p. 4 | CONFIRMADO na fonte |
| AMP-14 | Variante e polaridade | MAX98357A aceita I2S; canal esquerdo com LRCLK em baixo; dado amostrado na borda de subida do BCLK | [MAX98357A rev. 13][f-amp] | Table 2 e Table 3, p. 16 | CONFIRMADO na fonte |
| AMP-15 | Placa do módulo comprado ("Small Chip Welding"): resistores ligados a SD_MODE e a GAIN_SLOT | Peça ainda não recebida | `README.md` | Linha 131 | NÃO VERIFICADO |

## 11. EEPROM 24C02 (identificação do cartucho)

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| EE-01 | Microchip 24LC02B: alimentação e clock | 2,5 V a 5,5 V; 400 kHz (o 24AA02 vai de 1,7 V a 5,5 V; o 24FC02 chega a 1 MHz) | [Microchip DS20001709N][f-ee-m] | "Device Selection Table", p. 1 | CONFIRMADO na fonte |
| EE-02 | Microchip 24LC02B: tamanho de página e tempo de escrita | "8-Byte Page Write Buffer"; "Page Write Time: 5 ms, Maximum" (TWC de 5 ms máximo) | [Microchip DS20001709N][f-ee-m] | "Features", p. 1; tabela de características AC, p. 5 | CONFIRMADO na fonte |
| EE-03 | Microchip 24LC02B: endereço | Código de controle "1010"; "The next three bits of the control byte are "don't cares" for the 24XX02." Pinos: "The A0, A1 and A2 pins are not used by the 24XX02. They may be left floating or tied to either VSS or VCC." Consequência (derivada): o dispositivo responde em qualquer endereço de 0x50 a 0x57 | [Microchip DS20001709N][f-ee-m] | Seção 2.1, p. 6; seção 5.0 e Figure 5-1, p. 8 | CONFIRMADO na fonte |
| EE-04 | Microchip 24LC02B: pino WP | "This pin must be connected to either VSS or VCC. If tied to VSS, normal memory operation is enabled (read/write the entire memory 00-FF). If tied to VCC, write operations are inhibited." | [Microchip DS20001709N][f-ee-m] | Seção 2.4, p. 6; seção 6.3, p. 9 | CONFIRMADO na fonte |
| EE-05 | Microchip 24LC02B: escrita que cruza o limite de página | "If a page write command attempts to write across a physical page boundary, the result is that the data wrap around to the beginning of the current page" | [Microchip DS20001709N][f-ee-m] | Nota da seção 6.2, p. 9 | CONFIRMADO na fonte |
| EE-06 | Atmel AT24C02: alimentação e clock | Versões de 2,7 V a 5,5 V e de 1,8 V a 5,5 V; "100 kHz (1.8V) and 400 kHz (2.7V, 5V) Compatibility" | [Atmel 0180Z][f-ee-a] | "Features", p. 1 | CONFIRMADO na fonte |
| EE-07 | Atmel AT24C02: tamanho de página e tempo de escrita | "8-byte Page (1K, 2K), 16-byte Page (4K, 8K, 16K) Write Modes"; "Self-timed Write Cycle (5 ms max)" | [Atmel 0180Z][f-ee-a] | "Features", p. 1; tabela AC, p. 5 | CONFIRMADO na fonte |
| EE-08 | Atmel AT24C02: endereço | "The A2, A1 and A0 pins are device address inputs that are hard wired for the AT24C01A and the AT24C02. As many as eight 1K/2K devices may be addressed on a single bus system". Em 7 bits, 0x50 a 0x57 conforme A2..A0 (derivado do padrão 1010 A2 A1 A0) | [Atmel 0180Z][f-ee-a] | "Pin Description", p. 3; "Device Addressing", p. 9 | CONFIRMADO na fonte |
| EE-09 | Atmel AT24C02: pino WP | Ligado ao GND: leitura e escrita normais. Ligado ao VCC: proteção de escrita ativa | [Atmel 0180Z][f-ee-a] | "Pin Description" e Table 2, p. 3 | CONFIRMADO na fonte |
| EE-10 | Datasheet do AT24C02C (peça atual da Microchip) | Não lido | Site da Microchip (recusado) | — | NÃO VERIFICADO |
| EE-11 | ST M24C02: tamanho de página | 16 bytes: `M24PageSize = 16; M24MemorySize = 256;` no caso `EEPRMA2_M24C02_0`. Fonte: código oficial da ST, não o datasheet | [ST X-CUBE-EEPRMA1][f-ee-s] | `eeprma2_m24.c`, bloco `case EEPRMA2_M24C02_0` (linhas 582 a 586) | CONFIRMADO na fonte (código da ST) |
| EE-12 | ST M24C02: alimentação, tempo de escrita, pino de proteção e pinos de endereço | Não lido | Datasheet do M24C02 em `st.com` (não aberto) | — | NÃO VERIFICADO |

## 12. Infravermelho (receptor de 38 kHz e LED emissor)

O conjunto comprado é genérico e ainda não foi identificado (`README.md`, linha 130). Os valores abaixo são de uma família de receptores da Vishay e servem só como ordem de grandeza. O datasheet lido é o da família TSOP48xx (documento 82459); o da família TSOP382xx (documento 82491) não foi aberto.

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| IR-01 | Receptor TSOP48xx: alimentação | 2,5 V a 5,5 V; máximo absoluto −0,3 V a +6 V | [Vishay 82459 rev. 2.2][f-ir] | "Features", p. 1; "Absolute Maximum Ratings", p. 2 | CONFIRMADO na fonte |
| IR-02 | Receptor TSOP48xx: corrente de alimentação | 0,55 / 0,7 / 0,9 mA (sem luz, VS = 5 V) | [Vishay 82459 rev. 2.2][f-ir] | "Electrical and Optical Characteristics", p. 2 | CONFIRMADO na fonte |
| IR-03 | Peças de 38 kHz da família | TSOP4838, TSOP2238, TSOP4438 e TSOP2438 | [Vishay 82459 rev. 2.2][f-ir] | "Parts Table", p. 2 | CONFIRMADO na fonte |
| IR-04 | Saída | O diagrama de blocos mostra 30 kΩ entre VS e OUT. "The demodulated output signal can be directly connected to a microprocessor for decoding." Tensão de saída em nível baixo: até 100 mV com 0,5 mA. Corrente de saída máxima absoluta: 5 mA | [Vishay 82459 rev. 2.2][f-ir] | "Block Diagram" e "Description", p. 1; p. 2 | CONFIRMADO na fonte |
| IR-05 | A pinagem muda entre séries | TSOP44.. e TSOP48..: 1 = OUT, 2 = GND, 3 = VS. TSOP22.. e TSOP24..: 1 = OUT, 2 = VS, 3 = GND | [Vishay 82459 rev. 2.2][f-ir] | "Mechanical Data", p. 1 | CONFIRMADO na fonte |
| IR-06 | Filtro de alimentação | "R1 and C1 recommended to reduce supply ripple for VS < 2.8 V" | [Vishay 82459 rev. 2.2][f-ir] | "Application Circuit", p. 1 | CONFIRMADO na fonte |
| IR-07 | Corrente de LED usada no ensaio de alcance do receptor | "IR diode TSAL6200, IF = 50 mA", com alcance típico de 24 m. É uma condição de ensaio do receptor, não a especificação do LED | [Vishay 82459 rev. 2.2][f-ir] | "Electrical and Optical Characteristics", p. 2 | CONFIRMADO na fonte |
| IR-08 | Peça real do receptor do conjunto comprado, sua tensão e sua pinagem | Não identificada | `README.md` | Linha 130 | NÃO VERIFICADO |
| IR-09 | LED emissor: corrente direta típica e de pico, tensão direta, comprimento de onda | Nenhum datasheet de LED emissor foi aberto | — | — | NÃO VERIFICADO |
| IR-10 | Necessidade de transistor para acionar o LED | Não confirmada em fonte. Depende da corrente do LED (IR-09) e da corrente que um GPIO do ESP32-S3 pode fornecer, que não foi verificada neste registro. O conjunto pode ou não trazer transistor e resistor na placa | — | — | NÃO VERIFICADO |

## 13. Cartão SD em modo SPI

A especificação lida é a versão simplificada 4.10, de 2013. Não foi conferido se há versão mais recente com texto diferente nesses pontos.

| ID | Afirmação | Valor | Fonte | Onde na fonte | Status |
|---|---|---|---|---|---|
| SD-01 | 74 ciclos de clock com CS em alto antes do primeiro comando | "The host shall supply power to the card so that the voltage is reached to Vdd_min within 250ms and start to supply at least 74 SD clocks to the SD card with keeping CMD line to high. In case of SPI mode, CS shall be held to high during 74 clock cycles." | [SD Physical Layer Simplified 4.10][f-sd] | Seção 6.4.1.1, p. 149 | CONFIRMADO na fonte |
| SD-02 | Entrada em modo SPI | "The SD Card is powered up in the SD mode. It will enter SPI mode if the CS signal is asserted (negative) during the reception of the reset command (CMD0)." E: "The only way to return to the SD mode is by entering the power cycle." Na energização: "In case of SPI host, CMD0 shall be the first command to send the card to SPI mode." | [SD Physical Layer Simplified 4.10][f-sd] | Seção 7.2.1, p. 154; seção 6.4.1.1, p. 149 | CONFIRMADO na fonte |
| SD-03 | Clock de identificação de até 400 kHz | Trechos lidos: "The cards are initialized with ... a default driver strength with 400KHz clock frequency"; durante o ACMD41, "Issue continuous clock in the frequency range of 100 KHz-400 KHz". O valor numérico de fOD fica na seção 6.6, que a versão simplificada omite | [SD Physical Layer Simplified 4.10][f-sd] | Seção 4.2.1, p. 25; seção 4.4, p. 61 | CONFIRMADO na fonte (trechos citados); NÃO VERIFICADO (tabela de fOD) |
| SD-04 | Frequência de sondagem usada pelo ESP-IDF | `#define SDMMC_FREQ_PROBING 400 /*!< SD/MMC probing speed */` (em kHz) | [ESP-IDF, código][f-idf-4] | `components/sdmmc/include/sd_protocol_types.h`, linha 220 | CONFIRMADO na fonte |
| SD-05 | Ciclos iniciais no driver do ESP-IDF | "Clock out 80 cycles (10 bytes) before GO_IDLE command"; "For CMD0, clock out 80 cycles to help the card enter idle state, *before* CS is asserted." | [ESP-IDF, código][f-idf-4] | `components/esp_driver_sdspi/src/sdspi_host.c`, linhas 186 a 199 e 515 a 518 | CONFIRMADO na fonte |
| SD-06 | Ordem de inicialização no barramento compartilhado | "1. Initialize the SPI bus properly by spi_bus_initialize. 2. Tie the CS lines of all other devices than the SD card to idle state (by default it's high). ... 3. Mount the card to the filesystem by calling esp_vfs_fat_sdspi_mount. ... 4. Now you can talk to other SPI devices freely!" | [ESP-IDF, sdspi_share][f-idf-1] | Seção "Initialization Sequence" | CONFIRMADO na fonte |
| SD-07 | Motivo da ordem | "This step will put the SD card into the SPI mode, which **should** be done before all other SPI communications on the same bus. Otherwise, the card will stay in the SD mode, in which mode it may randomly respond to any SPI communications on the bus, even when its CS line is not addressed." | [ESP-IDF, sdspi_share][f-idf-1] | Seção "Initialization Sequence", passo 3 | CONFIRMADO na fonte |
| SD-08 | CS dos outros dispositivos | Manter em nível inativo por GPIO configurado; confiar só em pull-up é "**not recommended**" | [ESP-IDF, sdspi_share][f-idf-1] | Seção "Initialization Sequence", passo 2 | CONFIRMADO na fonte |
| SD-09 | Pull-ups | "When an SD card is operating in SPI mode or 1-bit SD mode, the CMD and DATA (DAT0 - DAT3) lines of the SD bus must be pulled up by 10 kΩ resistors. SD cards and SDIO devices should also have pull-ups on all above-mentioned lines (regardless of whether these lines are connected to the host)" | [ESP-IDF, sd_pullup_requirements][f-idf-2] | Segundo parágrafo | CONFIRMADO na fonte |
| SD-10 | Efeito dos pull-ups nos outros dispositivos | "The pull-ups required by SD cards are usually around 10 kOhm to 50 kOhm, which may be too strong for some other SPI devices. Check the specification of your device about its DC output current, it should be larger than 700 μA" | [ESP-IDF, sdspi_share][f-idf-1] | Seção "DC Loading" | CONFIRMADO na fonte |
| SD-11 | Liberação do MISO pelo cartão | Comentário do driver: "Clock out one byte (CS has to be high) to make the card release MISO". A função é chamada ao fim de cada transação | [ESP-IDF, código][f-idf-4] | `sdspi_host.c`, função `release_bus` (linhas 173 a 184) e chamadas nas linhas 510 e 538 | CONFIRMADO na fonte |
| SD-12 | Carga capacitiva e velocidade | Cada dispositivo a mais aumenta a carga dos pinos; a documentação sugere baixar o clock, por exemplo para 400 kHz, para isolar problemas de carga | [ESP-IDF, sdspi_share][f-idf-1] | Seção "AC Loading" e nota da "Initialization Sequence" | CONFIRMADO na fonte |
| SD-13 | Limite de velocidade do SD por SPI no ESP-IDF | "SD over SPI does not support speeds above SDMMC_FREQ_DEFAULT due to the limitations of the SPI driver." `SDMMC_FREQ_DEFAULT` vale 20000 kHz | [ESP-IDF, sdspi_host][f-idf-3]; [código][f-idf-4] | Nota da seção "Other Details"; `sd_protocol_types.h`, linha 218 | CONFIRMADO na fonte |
| SD-14 | Compartilhamento previsto pelo driver | "the SPI bus can be shared among SD cards and other SPI devices. The SPI Master driver will handle exclusive access from different tasks." "The SD SPI driver uses software-controlled CS signal." | [ESP-IDF, sdspi_host][f-idf-3] | Seção "Overview" | CONFIRMADO na fonte |
| SD-15 | Comportamento da biblioteca SD usada pelo Bruce (Arduino): ordem de inicialização e liberação do MISO | Não lido | — | — | NÃO VERIFICADO |
| SD-16 | Módulos microSD baratos com conversor de nível mantêm o MISO ativo com CS em alto (afirmação do `README.md`, linha 134) | Não lido em fonte primária | — | — | NÃO VERIFICADO |

## 14. Síntese para o compartilhamento de barramentos

Esta seção apenas organiza os fatos acima. As classes seguem a convenção do projeto: CONFIRMADO (fabricante), PROPOSTO, PENDENTE DE VALIDAÇÃO FÍSICA, VALIDADO EM BANCADA. Nenhuma linha fecha decisão.

### SPI (tela, CC1101, cartão SD, cartucho)

| Ponto | Base | Classe |
|---|---|---|
| O CC1101 solta o MISO (SO em alta impedância) com CSn em alto | CC-08 | CONFIRMADO (fabricante) |
| O CC1101 tem limite próprio de clock, diferente do da tela: 6,5 MHz em rajada sem atraso, 9 MHz em acesso simples e 10 MHz só com atraso de 100 ns entre bytes. Cada dispositivo precisa do seu próprio ajuste de clock | CC-06 | CONFIRMADO (fabricante) |
| O limite de datasheet do ILI9341 é 10 MHz na escrita e cerca de 6,67 MHz na leitura | ILI-07, ILI-08 | CONFIRMADO (fabricante) |
| A tela funcionou a 10 MHz na protoboard, inclusive na leitura do ID | MSP-09 | VALIDADO EM BANCADA (segundo o repositório) |
| Se o módulo da tela solta o MISO com o CS em alto | ILI-10, MSP-04 | PENDENTE DE VALIDAÇÃO FÍSICA |
| O cartão SD precisa entrar em modo SPI antes de qualquer outro tráfego no barramento, com os CS dos demais em alto | SD-06, SD-07, SD-08 | CONFIRMADO (fabricante: Espressif) |
| O cartão SD só solta o MISO depois de clocks extras com o CS em alto; o driver do ESP-IDF faz isso | SD-11 | CONFIRMADO (fabricante: Espressif) |
| Pull-ups de 10 kΩ nas linhas do cartão | SD-09 | CONFIRMADO (fabricante: Espressif) |
| Ligação elétrica do slot SD do MSP3218 ao conector (resistores em série, buffer) | MSP-06 | PENDENTE DE VALIDAÇÃO FÍSICA |
| Ordem de inicialização que o Bruce usa (tela antes do SD ou o contrário) e seu efeito | SD-15 | PENDENTE DE VALIDAÇÃO FÍSICA |

### I2C (PN532, MCP23017, EEPROM do cartucho)

| Ponto | Base | Classe |
|---|---|---|
| Faixa de endereços do MCP23017: 0x20 a 0x27 | MCP-11 | CONFIRMADO (fabricante) |
| Endereço do PN532: 0x24. Ele está **dentro** da faixa do MCP23017; a combinação A2 = 1, A1 = 0, A0 = 0 no MCP23017 colidiria. Os endereços propostos em `pins.h` (0x20 e 0x21) não colidem | PN-08, MCP-11 | CONFIRMADO (código da Elechouse e datasheet da Microchip); PROPOSTO (escolha 0x20/0x21) |
| Uma EEPROM Microchip 24LC02B responde em toda a faixa 0x50 a 0x57; uma AT24C02 da Atmel responde só no endereço dado por A2..A0 | EE-03, EE-08 | CONFIRMADO (fabricante) |
| A 3,3 V, o MCP23017 aceita 100 kHz e 400 kHz; a 24LC02B aceita 400 kHz. A velocidade máxima do PN532 em I2C não foi verificada | MCP-10, EE-01, PN-13 | CONFIRMADO (fabricante) para MCP23017 e EEPROM; não verificado para o PN532 |
| O valor de 100 kHz em `pins.h` (`i2c::kBusHz`) está dentro do que foi confirmado para MCP23017 e EEPROM | MCP-10, EE-01 | PROPOSTO |
| GPA7 e GPB7 do MCP23017 são só saída: um MCP23017 oferece 14 entradas pela revisão D do datasheet | MCP-03 a MCP-06 | CONFIRMADO (fabricante) |
| Pull-ups de I2C no módulo PN532 (valor e tensão) e sua soma com os 4,7 kΩ previstos | ELE-09 | PENDENTE DE VALIDAÇÃO FÍSICA |
| Tamanho de página da EEPROM: 8 bytes (Microchip, Atmel) ou 16 bytes (ST). Escrever em blocos de até 8 bytes alinhados e esperar 5 ms atende às três | EE-02, EE-07, EE-11 | PROPOSTO |

### I2S (INMP441 e MAX98357A com BCLK e LRCLK comuns)

| Ponto | Base | Classe |
|---|---|---|
| O INMP441 exige exatamente 64 ciclos de SCK por quadro (palavras de 32 bits) | MIC-04 | CONFIRMADO (fabricante) |
| O MAX98357A aceita BCLK de 32, 48 ou 64 vezes o LRCLK e palavras de 16, 24 ou 32 bits | AMP-09 | CONFIRMADO (fabricante) |
| Com os clocks comuns, a configuração que atende aos dois é BCLK = 64 × fS | MIC-04, AMP-09 | PROPOSTO (derivado) |
| Taxas que atendem aos dois: 8, 16, 32, 44,1 e 48 kHz (interseção de MIC-05 com AMP-08). 11,025 kHz e 22,05 kHz não são aceitas pelo amplificador | MIC-05, AMP-08 | PROPOSTO (derivado) |
| Não parar o LRCLK com o BCLK ativo (risco de tensão contínua na saída do amplificador) | AMP-13 | CONFIRMADO (fabricante) |
| O microfone só opera até 3,63 V: fica na linha de 3,3 V, nunca na bateria | MIC-01, MIC-02 | CONFIRMADO (fabricante) |
| Não aplicar clocks ao microfone sem alimentação | MIC-10 | CONFIRMADO (fabricante) |
| Com o amplificador ligado direto na bateria e SD_MODE acionado por GPIO de 3,3 V, o pino pode passar de VDD + 0,3 V quando a bateria está abaixo de 3,0 V; o datasheet indica cerca de 2 kΩ em série | AMP-02, AMP-07 | CONFIRMADO (fabricante); aplicação ao d-BMO: PROPOSTO |

## 15. Pontos não verificados

1. **Módulo MSP3218 (LCDwiki), por inteiro**: tensão de alimentação e lógica, regulador e jumper na placa, conversão de nível, corrente e circuito do backlight, ligação do slot SD (resistores, buffer), controlador de toque e seus pinos (MSP-01 a MSP-07). Falta abrir a página e o esquema elétrico do LCDwiki e medir a peça.
2. **ILI9341, SDO com CS em alto**: o datasheet lido não afirma que o SDO fica em alta impedância (ILI-10). Falta ensaio: com o CS da tela em alto, verificar com resistor de pull-up e de pull-down se a linha MISO flutua.
3. **Revisão corrente do datasheet do ILI9341**: a cópia lida é de 2011 e não há URL pública do fabricante verificada.
4. **PN532 segundo a NXP**: manual UM0701-02 e datasheet completo PN532_C1 não lidos. Ficam sem fonte da NXP: endereço I2C, velocidade máxima de I2C, linhas IRQ e H_REQ, esticamento de clock, formato de quadro, taxa padrão do HSU e limites do SPI (PN-09, PN-13). O que há vem do código da Elechouse (PN-08, PN-10 a PN-12).
5. **Módulo PN532 real**: esquema da Elechouse não aberto; regulador, pull-ups e pinos IRQ/RSTPDN desconhecidos; não se sabe se o "PN532 Set" comprado segue o projeto da Elechouse V3 (ELE-09, ELE-10).
6. **MCP23017, revisão posterior à D**: não conferida no site da Microchip (MCP-02). Também não foi lido nenhum documento da Microchip (errata ou nota de aplicação) que explique o motivo da restrição de GPA7 e GPB7.
7. **EEPROM**: datasheet do ST M24C02 e do AT24C02C não lidos (EE-10, EE-12). O tamanho de página do M24C02 vem de código da ST, não do datasheet.
8. **Infravermelho**: receptor real não identificado; nenhum datasheet de LED emissor lido; datasheet do TSOP382xx não aberto; corrente de GPIO do ESP32-S3 fora do escopo deste registro (IR-08 a IR-10).
9. **Cartão SD**: versão mais recente da especificação não conferida; tabela de fOD ausente da versão simplificada (SD-03); comportamento da biblioteca SD do Arduino usada pelo Bruce (SD-15); afirmação do README sobre módulos microSD com conversor de nível (SD-16).
10. **Documentação renderizada da Espressif**: as páginas de `docs.espressif.com` não foram abertas; foi lido o código-fonte da documentação no ramo `release/v5.5`. Não foi conferido se a versão "stable" publicada difere.
11. **Rótulo "modo SPI 0"** para ILI9341 e CC1101: nenhum dos dois datasheets usa a expressão; a compatibilidade é inferida das bordas de amostragem (ILI-06, CC-07).
12. **Módulos genéricos ainda não recebidos**: placas do MAX98357A, do INMP441 e do conjunto de IR (resistores e ligações de fábrica), e o cristal e o casamento de antena do módulo CC1101 (AMP-15, MIC-12, CC-21).
13. **Atualidade das cópias**: todas as folhas de dados foram lidas em cópias de terceiros. As revisões lidas (por exemplo, INMP441 rev. 1.1 de 2014 e MAX98357A rev. 13 de 2019) podem não ser as últimas.

## 16. Divergências encontradas em relação ao README

As divergências abaixo comparam o `README.md` (e os arquivos de configuração que o seguem) com as fontes lidas. Nenhuma delas é corrigida aqui.

1. **Dezesseis botões em um MCP23017.** O README prevê que um MCP23017 leia os 16 botões (linhas 125, 200 e 237), e `pins.h` coloca `kActionY` no bit 7 (GPA7) e `kStickRightClick` no bit 15 (GPB7). A revisão D do datasheet (junho de 2022) declara GPA7 e GPB7 como somente saída no MCP23017 e manda programar IO7 como saída (MCP-03 a MCP-06). Pela revisão D, um MCP23017 garante 14 entradas. Alternativas a avaliar, sem decisão aqui: usar o segundo MCP23017 (0x21) que o README já prevê como opcional; ler dois botões por GPIO direto; usar outra peça (o datasheet declara a restrição apenas para o MCP23017, de interface I2C, e não para o MCP23S17, de interface SPI). Evidência que falta: confirmar a revisão vigente no site da Microchip e o texto de eventual errata. Um teste de bancada que "funcione" com entradas em GPA7/GPB7 não substitui a restrição do fabricante.
2. **Clock de leitura da tela.** `firmware/bruce/bmo.ini` define `SPI_READ_FREQUENCY=10000000`. O datasheet do ILI9341 dá ciclo mínimo de leitura de 150 ns, cerca de 6,67 MHz (ILI-08). A leitura do ID a 10 MHz funcionou na bancada segundo o repositório, mas está acima do limite de datasheet.
3. **"Na PCB dá para subir a 40 MHz".** O comentário aparece em `bmo.ini` (acima da flag `SPI_FREQUENCY`) e em `teste_tela.ino` (linha 29), e o comentário de `spi::kDisplayHz` em `pins.h` diz que "na PCB pode subir". O datasheet do ILI9341 dá ciclo mínimo de escrita de 100 ns, ou seja, 10 MHz (ILI-07). Operar acima disso fica fora da especificação lida.
4. **Alimentação do MAX98357A descrita de duas formas.** O diagrama da arquitetura (linha 194) e a seção de alimentação (linha 257) ainda dizem 5 V para o amplificador; a lista de componentes (linha 131), o diagrama de energia (linha 252) e os riscos da bateria (linhas 286 e 287) dizem "direto da bateria". As duas ligações cabem na faixa de 2,5 V a 5,5 V (AMP-01), mas a potência muda: em 8 Ω com 1% de distorção, 1,4 W a 5 V contra 0,77 W a 3,7 V (AMP-03). O alto-falante de "8 Ω/2 W" não chega à potência nominal em nenhum dos casos.
5. **Pino SD do MAX98357A tratado só como desligamento.** O README liga o pino SD ao GPIO 3 (linhas 131 e 228). Pelo datasheet, o mesmo pino escolhe o canal: nível alto direto seleciona só o canal esquerdo; a soma dos canais exige resistor de valor alto (634 kΩ para 3,3 V) e o canal direito, 210,2 kΩ (AMP-06). Com o amplificador na bateria, há ainda o cuidado do resistor em série (AMP-07). Os resistores da placa comprada são desconhecidos (AMP-15).
6. **"I2S: MAX98357A e INMP441, mesmo barramento" sem as restrições.** O README (linha 199) não registra que o microfone exige 64 ciclos de clock por quadro e que o amplificador só aceita 8, 16, 32, 44,1, 48, 88,2 e 96 kHz (MIC-04, AMP-08). Taxas de 11,025 kHz e 22,05 kHz não são aceitas pelo amplificador. Não foi verificado aqui que taxas o Bruce e o Retro-Go usam.
7. **EEPROM do cartucho em 0x50.** O README (linhas 198, 218 e 582) e `pins.h` (`kCartridgeEeprom = 0x50`) tratam a 24C02 como uma peça de endereço único e sem particularidades. Uma 24LC02B da Microchip ocupa toda a faixa 0x50 a 0x57 (EE-03), e o tamanho de página varia entre fabricantes: 8 bytes na Microchip e na Atmel, 16 bytes na ST (EE-02, EE-07, EE-11).
8. **Cartão SD no SPI compartilhado sem requisitos de inicialização.** O README (linhas 134, 197 e 220) não registra que o cartão deve ser posto em modo SPI antes de qualquer outra comunicação no barramento, que os CS dos outros dispositivos devem estar em alto por GPIO e que as linhas do cartão pedem pull-up de 10 kΩ (SD-06 a SD-09). A justificativa do README para remover o módulo microSD (MISO preso por conversor de nível) não foi confirmada em fonte primária (SD-16).
9. **Faixa de endereços do MCP23017 inclui o do PN532.** O README usa 0x20 e 0x21 para os MCP23017 e 0x24 para o PN532 (linhas 198 e 218), o que não colide, mas não registra que 0x24 pertence à faixa 0x20 a 0x27 do MCP23017 (MCP-11, PN-08). A ligação dos pinos A2..A0 precisa excluir essa combinação.
10. **Plano B do PN532 em HSU sem pinos previstos.** O README (linha 316) prevê HSU se o I2C for instável. O manual da Elechouse confirma que HSU é o modo padrão e que usa os mesmos pinos do I2C no módulo (ELE-05, ELE-07), mas no lado do controlador o HSU pede uma UART; em `pins.h`, os GPIOs 43 e 44 (UART0) estão propostos para o cartucho (`kCartridgeUartTx`, `kCartridgeUartRx`). O conflito de pinos não está registrado.
11. **Backlight no GPIO 16 "3,3 V fixo ou PWM".** O README (linha 223) supõe que o pino LED do módulo pode ser acionado por GPIO. A corrente e o circuito do backlight do MSP3218 não foram verificados (MSP-05); na bancada, o LED foi ligado ao 3,3 V, não ao GPIO (`teste_tela.ino`, linha 2).

Itens do README que as fontes lidas confirmam: faixa de 2,5 V a 5,5 V do MAX98357A (AMP-01); CC1101 em 3,3 V, com máximo absoluto de 3,9 V (CC-01, CC-02); INMP441 em 3,3 V com L/R ao GND no canal esquerdo (MIC-01, MIC-06); endereço 0x24 do PN532 com a chave em I2C (PN-08, ELE-06); endereços 0x20 e 0x21 do MCP23017 (MCP-11); valores esperados no `teste_cc1101.ino` para PARTNUM (0x00), VERSION (0x14, ou 0x04 em lote antigo) e para o clock de cerca de 135 kHz no GDO0 após o reset (CC-12, CC-13).

[f-ili]: https://raw.githubusercontent.com/m5stack/M5Stack_TFT_ILI9341/m5stack-default/Documents/ILI9341.pdf
[f-cc]: https://raw.githubusercontent.com/CaelC/cc1101_only/master/cc1101.pdf
[f-pn]: https://raw.githubusercontent.com/bento-electronics/NFC-Module-PN532/master/datasheet.pdf
[f-elm]: https://raw.githubusercontent.com/bento-electronics/NFC-Module-PN532/master/module-manual.pdf
[f-ell]: https://github.com/elechouse/PN532/tree/PN532_HSU
[f-mcp-d]: https://raw.githubusercontent.com/Longan-Labs/XIAO_IO_EXPANDER_BOARD/main/MCP23017_Data_Sheet_DS20001952-2998473.pdf
[f-mcp-b]: https://raw.githubusercontent.com/mcauser/micropython-mcp23017/master/docs/mcp23017.pdf
[f-mic]: https://raw.githubusercontent.com/adrianviniciuscs/inmp441_mic/main/docs/INMP441-datasheet.pdf
[f-amp]: https://raw.githubusercontent.com/Xinyuan-LilyGO/LilyGo-HAL/master/MAX98357A/MAX98357A-MAX98357B.pdf
[f-ee-m]: https://raw.githubusercontent.com/sparkfun/SparkFun_External_EEPROM_Arduino_Library_Docs/main/24LC02%20-%202k.pdf
[f-ee-a]: https://raw.githubusercontent.com/yhyuan/STM32-Examples/master/16-M3-IIC%20(EEPROM-24C02)/datasheet/AT24C02.pdf
[f-ee-s]: https://raw.githubusercontent.com/STMicroelectronics/X-CUBE-EEPRMA1/main/Drivers/BSP/EEPRMA2/eeprma2_m24.c
[f-ir]: https://raw.githubusercontent.com/tarantula3/IR-Remote-Tester-and-Decoder/main/Datasheet-tsop48.pdf
[f-sd]: https://raw.githubusercontent.com/simon-77/VDHL-SD-Library/master/SD%20Specification_Part%201_Physical%20Layer_Simplified_v4.10_2013.pdf
[f-idf-1]: https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/docs/en/api-reference/peripherals/sdspi_share.rst
[f-idf-2]: https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/docs/en/api-reference/peripherals/sd_pullup_requirements.rst
[f-idf-3]: https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/docs/en/api-reference/peripherals/sdspi_host.rst
[f-idf-4]: https://github.com/espressif/esp-idf/tree/release/v5.5/components
