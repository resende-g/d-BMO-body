# Registro de evidências — ESP32-S3, módulo ESP32-S3-WROOM-1-N16R8 e placa ESP32-S3-DevKitC-1

## 1. Cabeçalho

**Escopo.** Fatos do fabricante (Espressif) necessários às decisões de hardware do d-BMO v1:
SoC ESP32-S3, módulo ESP32-S3-WROOM-1-N16R8 e placa de desenvolvimento ESP32-S3-DevKitC-1.
Este arquivo registra o que as fontes afirmam; não fecha decisões de projeto (carregador,
power-path, reguladores, pinagem final continuam em aberto).

**Data de acesso.** 2026-11-12.

**Método.**

1. Cada linha das tabelas traz uma afirmação, o valor, a fonte, o local na fonte e o status.
2. Status possíveis: **CONFIRMADO na fonte** (o texto, a tabela, o cabeçalho de código ou a
   figura foi aberto e lido nesta sessão) ou **NÃO VERIFICADO** (a fonte primária não pôde ser
   aberta, ou não foi encontrada evidência).
3. Inferências de projeto (consequências para o d-BMO) ficam separadas, nas seções 6 e 7, e
   usam a classificação do projeto: CONFIRMADO (fabricante) / PROPOSTO / PENDENTE DE VALIDAÇÃO
   FÍSICA / VALIDADO EM BANCADA (este último só para o que o README documenta como testado).

**Limitação de acesso desta sessão (importante).**

- `docs.espressif.com` e `www.espressif.com` não resolveram na ferramenta de leitura de páginas
  (erro `getaddrinfo ENOTFOUND`: fora dos domínios de rede permitidos da sessão). Depois de duas
  falhas iguais, `dl.espressif.com` e `documentation.espressif.com` não foram tentados. Para
  abrir os PDFs citados abaixo, esses quatro hosts precisam ser liberados na configuração de
  rede da sessão.
- Consequência: **nenhum datasheet em PDF foi aberto** (ESP32-S3 Series Datasheet, ESP32-S3-WROOM-1
  Datasheet, esquemático e desenho dimensional da DevKitC-1, Technical Reference Manual, errata).
- O que foi aberto: o **código-fonte oficial da documentação da Espressif** (arquivos `.rst`/`.inc`
  que geram as páginas de `docs.espressif.com`), cabeçalhos do ESP-IDF e o projeto de referência
  oficial do módulo, obtidos por `git clone` somente leitura dos repositórios públicos da
  Espressif no GitHub. São fontes primárias do fabricante, na revisão indicada na seção 2.
- A ferramenta de busca na web devolveu trechos atribuídos aos datasheets. Esses trechos são
  resumo automático, não leitura do documento: aparecem aqui sempre como **NÃO VERIFICADO
  (trecho de busca)** e servem apenas como indicação do que conferir.
- Onde a documentação usa texto condicional por chip (`.. only:: esp32s3`, substituições
  `{IDF_TARGET_...}`), foi lido o ramo que se aplica ao ESP32-S3.

## 2. Fontes

### 2.1 Abertas e lidas nesta sessão

| Chave | Título | Revisão lida | URL |
|---|---|---|---|
| HDG | ESP Hardware Design Guidelines (ESP32-S3) — Schematic Checklist, Download Guidelines, PCB Layout Design. Histórico de revisões do ESP32-S3 no repositório: última entrada v1.8, 2025-07-03 | commit `f6f2603594c2e5f3744da931d605c5f5d4a81772` (2026-11-10) | https://github.com/espressif/esp-hardware-design-guidelines/tree/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en — página publicada equivalente (não aberta): https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32s3/schematic-checklist.html |
| REF | ESP32-S3-WROOM-1 reference design (ZIP oficial, dentro do HDG): esquemático `SCH_ESP32-S3-WROOM-1_V1.3_20220705.pdf` (2 folhas) | mesmo commit do HDG | https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/_static/esp32s3/esp32-s3-wroom-1_reference_design.zip |
| DKC | ESP32-S3-DevKitC-1 User Guide v1.1 e v1.0, Ordering Information, figura de pinagem v1.1 e diagrama de blocos (repositório `esp-dev-kits`) | commit `5c06bf1835329d6525f316475f7218aab6acfe79` (2026-11-02) | https://github.com/espressif/esp-dev-kits/tree/5c06bf1835329d6525f316475f7218aab6acfe79/docs/en/esp32-s3-devkitc-1 — páginas publicadas equivalentes (não abertas): https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html e `.../user_guide_v1.0.html` |
| IDF | ESP-IDF Programming Guide (fontes em `docs/en`) e cabeçalhos em `components/` — ramo `master` | commit `827cc6229fb01c6ecf6362b4840ef27d62d2138e` (2026-11-11) | https://github.com/espressif/esp-idf/tree/827cc6229fb01c6ecf6362b4840ef27d62d2138e — página publicada equivalente do capítulo de GPIO (não aberta): https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/gpio.html |

Observação sobre versão: o ESP-IDF lido é o `master`. O Bruce de bancada usa o core Arduino,
que embute uma versão anterior do ESP-IDF. Os fatos de hardware (mapa de pinos, canais, domínios)
não dependem da versão; as APIs citadas (por exemplo, wake por GPIO em deep sleep) podem não
existir na versão embutida no core Arduino — não verificado nesta sessão.

### 2.2 Citadas pelas fontes acima, mas não abertas (hosts bloqueados)

| Chave | Título | URL (como aparece nas fontes lidas) |
|---|---|---|
| DS-SOC | ESP32-S3 Series Datasheet (a busca na web o lista como "Version 2.2") | https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf |
| DS-MOD | ESP32-S3-WROOM-1 & ESP32-S3-WROOM-1U Datasheet (a busca na web o lista como "Version 1.8") | https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf |
| SCH-DKC | ESP32-S3-DevKitC-1 Schematic V1.1 | https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-DevKitC-1_V1.1_20221130.pdf |
| DIM-DKC | ESP32-S3-DevKitC-1 Dimensions V1.1 (PDF) | https://dl.espressif.com/dl/schematics/esp_idf/DXF_ESP32-S3-DevKitC-1_V1.1_20220429.pdf |
| SCH-DKC0 | ESP32-S3-DevKitC-1 Schematic V1.0 | https://dl.espressif.com/dl/SCH_ESP32-S3-DEVKITC-1_V1_20210312C.pdf |
| DIM-DKC0 | ESP32-S3-DevKitC-1 Dimensions V1.0 (PDF) | https://dl.espressif.com/dl/DXF_ESP32-S3-DevKitC-1_V1_20210312CB.pdf |
| ERRATA | ESP32-S3 Errata | https://www.espressif.com/sites/default/files/documentation/esp32-s3_errata_en.pdf |
| TRM | ESP32-S3 Technical Reference Manual | URL não conferida nesta sessão |

Nas tabelas, a coluna "Fonte" usa links com o título curto do arquivo lido.

## 3. ESP32-S3 (SoC)

### 3.1 GPIOs disponíveis, reservados e estado no reset

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-GPIO-01 | Quantidade e numeração dos GPIOs físicos | 45 GPIOs: GPIO0–GPIO21 e GPIO26–GPIO48 | [IDF — gpio/esp32s3.inc][idf-gpio] | "gpio-summary", linha 12 | CONFIRMADO na fonte |
| S3-GPIO-02 | GPIO22–25 não existem | máscara de GPIOs válidos exclui os bits 22–25; `SOC_GPIO_PIN_COUNT` = 49 | [IDF — soc_caps.h (esp32s3)][idf-caps] | linhas 173 e 180 | CONFIRMADO na fonte |
| S3-GPIO-03 | GPIO26–32 são da flash/PSRAM SPI | "usually used for SPI flash and PSRAM and not recommended for other uses" | [IDF — gpio/esp32s3.inc][idf-gpio] | nota "SPI0/1", linha 222 | CONFIRMADO na fonte |
| S3-GPIO-04 | Com flash ou PSRAM octal, GPIO33–37 ficam ocupados | ligados a SPIIO4–SPIIO7 e SPIDQS; em placas com ESP32-S3R8/R8V "also not recommended for other uses" | [IDF — gpio/esp32s3.inc][idf-gpio] | nota "SPI0/1", linha 222 | CONFIRMADO na fonte |
| S3-GPIO-05 | Idem, no guia de hardware | com SPI octal (1,8 V ou 3,3 V, dentro ou fora do encapsulamento) "GPIO33 ~ GPIO37 are occupied and cannot be used for other functions" | [HDG — esp32s3-chip-core-schem-note.inc][hdg-core] | linha 3 | CONFIRMADO na fonte |
| S3-GPIO-06 | Mapeamento da PSRAM octal de 8 MB dentro do chip (ESP32-S3R8/R8V) | SPICLK=CLK, SPICS1=CE#, SPID/SPIQ/SPIWP/SPIHD=DQ0–DQ3, GPIO33=DQ4, GPIO34=DQ5, GPIO35=DQ6, GPIO36=DQ7, GPIO37=DQS/DM | [HDG — esp32s3-chip-flash-pin-mapping.inc][hdg-flash] | tabela "In-Package Octal SPI PSRAM" | CONFIRMADO na fonte |
| S3-GPIO-07 | Roteamento de sinais | pela matriz de GPIO, entradas de periféricos podem vir de qualquer GPIO e saídas podem ir a qualquer GPIO | [IDF — gpio/esp32s3.inc][idf-gpio] | linha 12 | CONFIRMADO na fonte |
| S3-GPIO-08 | Estado padrão no reset (IE = entrada habilitada; WPU/WPD = pull-up/pull-down fraco) | GPIO0: IE+WPU. GPIO1–3: IE. GPIO4–8, 15, 16, 19, 21: sem IE e sem pull. GPIO9–14, 17, 18, 33–38, 39–42: IE só depois do reset. GPIO20: pull-up de USB. GPIO43/44 (U0TXD/U0RXD): IE+WPU. GPIO45 e GPIO46: IE+WPD. GPIO47/48 (SPICLK_P/N): IE | [HDG — esp32s3-table-io-mux.inc][hdg-iomux] | tabela "IO Pin Default Configuration" | CONFIRMADO na fonte |
| S3-GPIO-09 | Domínio de alimentação dos pinos | GPIO0–21: VDD3P3_RTC. GPIO38–46 (inclui 39–42, 43, 44, 45, 46): VDD3P3_CPU. GPIO33–37, GPIO47 e GPIO48: VDD_SPI / VDD3P3_CPU | [HDG — esp32s3-table-io-mux.inc][hdg-iomux] | coluna "Power" | CONFIRMADO na fonte |
| S3-GPIO-10 | GPIO47 e GPIO48 são SPICLK_P e SPICLK_N | `IO_MUX_GPIO47_REG` = SPICLK_P; `IO_MUX_GPIO48_REG` = SPICLK_N | [IDF — io_mux_reg.h (esp32s3)][idf-iomux-h] | linhas 137–138 | CONFIRMADO na fonte |
| S3-GPIO-11 | GPIO47/48 compartilham o domínio de GPIO33–37 | com flash/PSRAM octal de 1,8 V, SPICLK_P e SPICLK_N também operam em 1,8 V | [HDG — schematic-checklist.rst][hdg-sc] | seção "GPIO", linha 837 | CONFIRMADO na fonte |
| S3-GPIO-12 | Controle de pinos em deep sleep | "Only GPIOs in the VDD3P3_RTC power domain can be controlled in Deep-sleep mode" | [HDG — schematic-checklist.rst][hdg-sc] | seção "GPIO", linha 838 | CONFIRMADO na fonte |
| S3-GPIO-13 | Níveis de força de drive | 4 níveis (`GPIO_DRIVE_CAP_0` a `_3`); padrão = 2 ("medium") | [IDF — hal/gpio_types.h][idf-gpiot] | enum `gpio_drive_cap_t`, linhas 134–141 | CONFIRMADO na fonte |
| S3-GPIO-14 | Corrente por pino (IOH/IOL) por nível de drive | trecho de busca: IOH 40 mA e IOL 28 mA com VDD = 3,3 V e PAD_DRIVER = 3 | DS-MOD / DS-SOC (seção 2.2) | tabela "DC Characteristics" | NÃO VERIFICADO (trecho de busca) |
| S3-GPIO-15 | Resistência dos pull-ups/pull-downs internos | trecho de busca: 45 kΩ típico | DS-SOC (seção 2.2) | tabela "DC Characteristics" | NÃO VERIFICADO (trecho de busca) |
| S3-GPIO-16 | Corrente máxima somada (por domínio ou por chip) | sem evidência encontrada | DS-SOC (seção 2.2) | — | NÃO VERIFICADO |

### 3.2 Pinos de strapping e modo de boot

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-STRAP-01 | Pinos de strapping | GPIO0, GPIO3, GPIO45 e GPIO46 | [HDG — schematic-checklist.rst][hdg-sc] | seção "Strapping Pins", linhas 745–749 | CONFIRMADO na fonte |
| S3-STRAP-02 | Quando são lidos | "At each startup or reset"; "After reset, the strapping pins work as normal function pins" | [HDG — schematic-checklist.rst][hdg-sc] | linha 747 | CONFIRMADO na fonte |
| S3-STRAP-03 | Configuração padrão dos pinos de boot | GPIO0 = 1 (pull-up); GPIO46 = 0 (pull-down) | [HDG — esp32s3-chip-boot-mode-control.inc][hdg-boot] | tabela "Boot Mode Control" | CONFIRMADO na fonte |
| S3-STRAP-04 | SPI Boot (boot normal pela flash) | GPIO0 = 1; GPIO46 = qualquer valor | [HDG — esp32s3-chip-boot-mode-control.inc][hdg-boot] | tabela "Boot Mode Control" | CONFIRMADO na fonte |
| S3-STRAP-05 | Joint Download Boot | GPIO0 = 0 e GPIO46 = 0; aceita download por USB-Serial-JTAG, USB-OTG e UART | [HDG — esp32s3-chip-boot-mode-control.inc][hdg-boot] | tabela e nota 1 | CONFIRMADO na fonte |
| S3-STRAP-06 | Combinação GPIO0 = 0 com GPIO46 = 1 | não consta da tabela lida | DS-SOC / TRM (seção 2.2) | capítulo "Boot Configurations" | NÃO VERIFICADO |
| S3-STRAP-07 | Tempos de setup e hold dos straps | t_SU ≥ 0 ms (antes de CHIP_PU subir); t_H ≥ 3 ms (tempo para o chip ler os straps depois de CHIP_PU alto, antes de os pinos virarem I/O comum) | [HDG — schematic-checklist.rst][hdg-sc] | tabela "Timing Parameters for Strapping Pins", linhas 773–787 | CONFIRMADO na fonte |
| S3-STRAP-08 | Recomendações para GPIO0 | colocar pull-up; "Do not add high-value capacitors at GPIO0, or the chip may enter download mode" | [HDG — schematic-checklist.rst][hdg-sc] | bloco "attention", linhas 791–794 | CONFIRMADO na fonte |
| S3-STRAP-09 | GPIO45 seleciona a tensão de VDD_SPI (com eFuse `EFUSE_VDD_SPI_FORCE` = 0) | GPIO45 = 0 → 3,3 V (VDD3P3_RTC via R_SPI, padrão); GPIO45 = 1 → 1,8 V (regulador interno da flash) | [HDG — esp32-s-series-digital-power-supply.inc][hdg-pwr] | tabela "VDD_SPI Voltage Control" | CONFIRMADO na fonte |
| S3-STRAP-10 | Com `EFUSE_VDD_SPI_FORCE` = 1, GPIO45 é ignorado | tensão definida por `EFUSE_VDD_SPI_TIEH` (0 → 1,8 V; 1 → 3,3 V) | [HDG — esp32-s-series-digital-power-supply.inc][hdg-pwr] | tabela "VDD_SPI Voltage Control" | CONFIRMADO na fonte |
| S3-STRAP-11 | Estado de fábrica do eFuse `VDD_SPI_FORCE` no módulo N16R8 | sem evidência encontrada | DS-MOD (seção 2.2) | — | NÃO VERIFICADO |
| S3-STRAP-12 | Origem padrão do JTAG | por padrão, o JTAG está ligado ao periférico USB_SERIAL_JTAG interno | [IDF — configure-other-jtag.rst][idf-jtag-other] | "Configure eFuses", linha 15 | CONFIRMADO na fonte |
| S3-STRAP-13 | GPIO3 só seleciona a origem do JTAG depois de queimar o eFuse `STRAP_JTAG_SEL` | com o eFuse queimado: GPIO3 baixo no reset → JTAG em GPIO39–42; GPIO3 alto → USB_SERIAL_JTAG | [IDF — configure-other-jtag.rst][idf-jtag-other] e [IDF — jtag-debugging/esp32s3.inc][idf-jtag-s3] | linha 24; substituição `jtag-sel-gpio` = GPIO3, linha 132 | CONFIRMADO na fonte |
| S3-STRAP-14 | GPIO3 no reset | entrada habilitada, sem pull interno (flutuante) | [HDG — esp32s3-table-io-mux.inc][hdg-iomux] | linha do GPIO3 | CONFIRMADO na fonte |
| S3-STRAP-15 | Tabela completa de seleção do JTAG (`EFUSE_DIS_PAD_JTAG`, `EFUSE_DIS_USB_JTAG`, `EFUSE_STRAP_JTAG_SEL`) | não lida | DS-SOC (seção 2.2) | capítulo "Boot Configurations" | NÃO VERIFICADO |
| S3-STRAP-16 | Log da ROM: comportamento padrão | "Always print ROM logs, this is the default behavior"; a mudança é permanente (eFuse, uma única vez) e admite "Log on GPIO High" / "Log on GPIO Low" | [IDF — esp_rom/Kconfig.projbuild][idf-romk] | opção `BOOT_ROM_LOG_SCHEME`, linhas 6–35 | CONFIRMADO na fonte |
| S3-STRAP-17 | GPIO46 é o pino associado ao log da ROM no ESP32-S3 | a figura oficial de pinagem da DevKitC-1 rotula a função de strapping do GPIO46 como "LOG" | [DKC — figura de pinagem v1.1][dk-pin] | rótulo ao lado de GPIO46 | CONFIRMADO na fonte |
| S3-STRAP-18 | Tabela "ROM messages printing control" (GPIO46 + `EFUSE_UART_PRINT_CONTROL`) | não lida; trecho de busca cita `EFUSE_UART_PRINT_CONTROL` e `EFUSE_DIS_USB_SERIAL_JTAG_ROM_PRINT` | DS-SOC (seção 2.2) | capítulo "Boot Configurations" | NÃO VERIFICADO (trecho de busca) |
| S3-STRAP-19 | Tipos de reset em que a ROM consulta os straps | power-on reset, reset de SoC por software e reset de SoC por watchdog: a ROM consulta `GPIO_STRAP_REG` para decidir o modo de boot | [IDF — startup.rst][idf-startup] | "First stage (ROM) bootloader", linhas 35–43 | CONFIRMADO na fonte |
| S3-STRAP-20 | Reset por watchdog RTC durante o boot | "This may cause the SoC to strap into a new boot mode, if the strapping GPIOs have changed" | [IDF — startup.rst][idf-startup] | nota, linha 47 | CONFIRMADO na fonte |
| S3-STRAP-21 | Os straps são reamostrados ao acordar do deep sleep? | não esclarecido nas fontes lidas; trecho de busca: "At Chip Reset, the latches sample the bit values ... and store them until the chip is powered down or shut down" | DS-SOC (seção 2.2) | capítulo "Boot Configurations" | NÃO VERIFICADO (trecho de busca) |

### 3.3 Alimentação, reset e consumo

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-PWR-01 | Fonte única recomendada | 3,3 V com corrente de saída "no less than 500 mA" | [HDG — schematic-checklist.rst][hdg-sc] | seção "Power Supply", linhas 86–90 | CONFIRMADO na fonte |
| S3-PWR-02 | Faixa de tensão dos pinos de alimentação digital (VDD3P3_CPU, VDD3P3_RTC) | 3,0 V a 3,6 V | [HDG — esp32-s-series-digital-power-supply.inc][hdg-pwr] | linhas 5 e 19 | CONFIRMADO na fonte |
| S3-PWR-03 | Faixa de tensão dos pinos analógicos (VDD3P3, VDDA) | 3,0 V a 3,6 V | [HDG — schematic-checklist.rst][hdg-sc] | "Analog Power Supply", linhas 157–171 | CONFIRMADO na fonte |
| S3-PWR-04 | Proteção e capacitância na entrada de alimentação | diodo de proteção ESD e pelo menos 10 µF na entrada principal; 10 µF adicional no trilho VDD3P3 por causa do surto de corrente na transmissão | [HDG — schematic-checklist.rst][hdg-sc] | linhas 91 e 177–179 | CONFIRMADO na fonte |
| S3-PWR-05 | VDD_SPI em 3,3 V | alimentado por VDD3P3_RTC através de um resistor de 14 Ω (há queda de tensão); manter VDD3P3_RTC acima de 3,0 V para atender flash/PSRAM de 3,3 V | [HDG — esp32-s-series-digital-power-supply.inc][hdg-pwr] | linhas 9, 30 e 42 | CONFIRMADO na fonte |
| S3-PWR-06 | VDD_SPI em 1,8 V | LDO interno, corrente típica de 40 mA | [HDG — esp32-s-series-digital-power-supply.inc][hdg-pwr] | linha 29 | CONFIRMADO na fonte |
| S3-PWR-07 | Pino CHIP_PU (EN) | alto habilita, baixo reseta; deve subir depois de os trilhos de 3,3 V estabilizarem; não pode ficar flutuando | [HDG — schematic-checklist.rst][hdg-sc] | "Chip Power-up and Reset Timing", linhas 228–232 e 310 | CONFIRMADO na fonte |
| S3-PWR-08 | Tempos de power-up e reset | t_STBL ≥ 50 µs; t_RST ≥ 50 µs; tensão de reset entre –0,3 V e 0,25 × VDD3P3_RTC | [HDG — schematic-checklist.rst][hdg-sc] | tabela "Timing Parameters for Power-up and Reset", linhas 224–287 | CONFIRMADO na fonte |
| S3-PWR-09 | Circuito RC recomendado em CHIP_PU | R = 10 kΩ e C = 1 µF (ajustar conforme a fonte) | [HDG — schematic-checklist.rst][hdg-sc] | bloco "attention", linha 311 | CONFIRMADO na fonte |
| S3-PWR-10 | Fonte com subida/descida lenta ("such as during battery charging"), liga/desliga frequente ou fonte instável | só o RC pode não atender o timing: o chip pode não entrar em operação normal ou apagamentos de flash podem falhar; reservar um supervisor de tensão com limiar em torno de 3,0 V | [HDG — schematic-checklist.rst][hdg-sc] | bloco "attention", linhas 312–318 | CONFIRMADO na fonte |
| S3-PWR-11 | Consumo medido em modem-sleep (estação Wi-Fi conectada, CPU a 160 MHz, DFS desligado) | média 40,1 / 38,7 / 38,2 mA (DTIM 1 / 3 / 10); máximo 235,8 / 267,2 / 259,7 mA; mínimo 37,8–37,9 mA | [IDF — esp32s3_modem_sleep.inc][idf-cur-ms] | tabela (incluída em [low-power-mode-wifi.rst][idf-lpw]) | CONFIRMADO na fonte |
| S3-PWR-12 | Idem, com DFS ligado | média 20,7 / 19,9 / 19,5 mA; máximo 113,4–113,5 mA; mínimo 15,0 mA | [IDF — esp32s3_modem_sleep.inc][idf-cur-ms] | tabela | CONFIRMADO na fonte |
| S3-PWR-13 | Consumo medido em auto light-sleep com Wi-Fi conectado | média 2,45 / 1,33 / 0,93 mA (DTIM 1 / 3 / 10); máximo ≈120 mA; mínimo 0,28 mA | [IDF — esp32s3_light_sleep.inc][idf-cur-ls] | tabela | CONFIRMADO na fonte |
| S3-PWR-14 | Consumo medido em deep sleep | corrente média 6,8 µA ("the measured data are obtained by testing in a shielded box"; placa e variante do módulo não informadas) | [IDF — esp32s3_summary.inc][idf-cur-sum] | última linha da tabela; nota em [low-power-mode-wifi.rst][idf-lpw], linhas 20–22 | CONFIRMADO na fonte |
| S3-PWR-15 | Medição em um módulo ESP32-S3-WROOM-1 (exemplo `deep_sleep`, variante de memória não informada) | 8,14 µA em deep sleep; cerca de 23,88 mA em modo ativo | [IDF — current-consumption-measurement-modules.rst][idf-ccm] | linhas 124 e 144 | CONFIRMADO na fonte |
| S3-PWR-16 | Corrente de pico de RF (Wi-Fi TX/RX por padrão e potência; Bluetooth LE TX/RX) | não obtida | DS-SOC / DS-MOD (seção 2.2) | "Current Consumption in Active Mode" | NÃO VERIFICADO |
| S3-PWR-17 | Correntes típicas de datasheet em light-sleep, deep-sleep e power-off | não lidas; trecho de busca: deep sleep 7 µA | DS-SOC (seção 2.2) | "Current Consumption in Other Modes" | NÃO VERIFICADO (trecho de busca) |

### 3.4 ADC

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-ADC-01 | Canais do ADC1 | ADC1_CH0–CH9 = GPIO1–GPIO10 | [IDF — adc_channel.h (esp32s3)][idf-adc-h]; [HDG — esp32s3-table-adc-gpio.inc][hdg-adc] | linhas 9–37; tabela "ADC Functions" | CONFIRMADO na fonte |
| S3-ADC-02 | Canais do ADC2 | ADC2_CH0–CH9 = GPIO11–GPIO20 | [IDF — adc_channel.h (esp32s3)][idf-adc-h]; [HDG — esp32s3-table-adc-gpio.inc][hdg-adc] | linhas 39–67; tabela "ADC Functions" | CONFIRMADO na fonte |
| S3-ADC-03 | Unidades e resolução | 2 unidades ADC; resultado bruto de 12 bits por padrão | [IDF — soc_caps.h (esp32s3)][idf-caps]; [IDF — adc/index.rst][idf-adci] | linhas 116 e 122; linha 21 | CONFIRMADO na fonte |
| S3-ADC-04 | ADC2 e Wi-Fi (modo oneshot) | "ADC2 is also used by Wi-Fi. adc_oneshot_read has provided protection between the Wi-Fi driver and ADC oneshot mode driver" | [IDF — adc_oneshot.rst][idf-adc1] | "Hardware Limitations", linha 163 | CONFIRMADO na fonte |
| S3-ADC-05 | Efeito prático da disputa | a leitura "may fail when the ADC is in use by other drivers/peripherals, and return ESP_ERR_TIMEOUT"; o resultado bruto é inválido nesse caso | [IDF — adc_oneshot.rst][idf-adc1] | "Read Conversion Result", linha 127 | CONFIRMADO na fonte |
| S3-ADC-06 | ADC2 em modo contínuo (DMA) no ESP32-S3 | não é mais suportado ("unstable results"); remete à errata do ESP32-S3 | [IDF — adc_continuous.rst][idf-adcc] | "Hardware Limitations", linha 384 | CONFIRMADO na fonte |
| S3-ADC-07 | Níveis de atenuação | 0 dB, 2,5 dB, 6 dB e 12 dB (k ≈ 100 %, 75 %, 50 %, 25 %) | [IDF — adc/index.rst][idf-adci] | "ADC Attenuation", linhas 51–56 | CONFIRMADO na fonte |
| S3-ADC-08 | Tensão de referência | 1100 mV por projeto; entre 1000 mV e 1200 mV de chip para chip | [IDF — adc/index.rst][idf-adci] | linha 35 | CONFIRMADO na fonte |
| S3-ADC-09 | Faixa efetiva de medição e erro total depois da calibração (ESP32-S3) | ATTEN0: 0–850 mV, ±5 mV. ATTEN1: 0–1100 mV, ±6 mV. ATTEN2: 0–1600 mV, ±10 mV. ATTEN3: 0–2900 mV, ±50 mV | [HDG — schematic-checklist.rst][hdg-sc] | seção "ADC", linhas 854–891 | CONFIRMADO na fonte |
| S3-ADC-10 | Capacitor de filtro | 0,1 µF entre o pino e o terra ao usar o ADC | [HDG — schematic-checklist.rst][hdg-sc] | seção "ADC", linha 876 | CONFIRMADO na fonte |
| S3-ADC-11 | Tabela "ADC Characteristics" do datasheet (faixa de entrada por atenuação, DNL/INL, impedância de fonte) | não lida | DS-SOC (seção 2.2) | "ADC Characteristics" | NÃO VERIFICADO |

### 3.5 GPIOs RTC e fontes de wake do deep sleep

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-RTC-01 | GPIOs RTC | RTC_GPIO0–21 = GPIO0–GPIO21 (22 pinos) | [IDF — rtc_io_channel.h (esp32s3)][idf-rtc-h]; [IDF — soc_caps.h][idf-caps] | linhas 10–74; `SOC_RTCIO_PIN_COUNT` = 22, linha 269 | CONFIRMADO na fonte |
| S3-RTC-02 | O ESP32-S3 suporta ext0 e ext1 | `SOC_PM_SUPPORT_EXT0_WAKEUP` = 1 e `SOC_PM_SUPPORT_EXT1_WAKEUP` = 1 | [IDF — soc_caps.h (esp32s3)][idf-caps] | linhas 370–371 | CONFIRMADO na fonte |
| S3-RTC-03 | ext0 | acorda quando um GPIO RTC atinge um nível predefinido; mantém os periféricos RTC ligados; pull-up/pull-down internos utilizáveis | [IDF — sleep_modes.rst][idf-sleep] | "External Wakeup (ext0)", linhas 202–213 | CONFIRMADO na fonte |
| S3-RTC-04 | ext1 | vários GPIOs RTC; acorda se qualquer pino selecionado estiver alto (`ANY_HIGH`) ou se qualquer um estiver baixo (`ANY_LOW`); funciona com os periféricos RTC desligados | [IDF — sleep_modes.rst][idf-sleep] | "External Wakeup (ext1)", linhas 223–256 | CONFIRMADO na fonte |
| S3-RTC-05 | ext0 e ext1 só aceitam GPIOs RTC | "EXT0 and EXT1 wakeup sources, which can only be used with RTC IOs" | [IDF — sleep_modes.rst][idf-sleep] | linha 309 | CONFIRMADO na fonte |
| S3-RTC-06 | Depois de acordar, o pino de wake fica como RTC IO | chamar `rtc_gpio_deinit` antes de usá-lo como GPIO digital | [IDF — sleep_modes.rst][idf-sleep] | avisos, linhas 217 e 270 | CONFIRMADO na fonte |
| S3-RTC-07 | Duração mínima do sinal de wake | pelo menos 3 ciclos do relógio lento RTC | [IDF — sleep_modes.rst][idf-sleep] | nota, linha 407 | CONFIRMADO na fonte |
| S3-RTC-08 | Estado de GPIO digital durante o deep sleep | `gpio_hold_en` sozinho não mantém um GPIO digital em deep sleep no ESP32-S3 ("will be reset to its default state when the chip wakes up"); é preciso `gpio_deep_sleep_hold_en` | [IDF — driver/gpio.h][idf-gpio-h] | comentário de `gpio_hold_en`, item 3, linhas 441–443 | CONFIRMADO na fonte |
| S3-RTC-09 | USB Serial/JTAG em deep sleep | o dispositivo USB "appears disconnected from the host/PC" | [IDF — usb-serial-jtag-console.rst][idf-usj] | "Sleep Mode Considerations" | CONFIRMADO na fonte |

### 3.6 USB, UART0, JTAG e cristal de 32 kHz

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-USB-01 | Pinos do PHY USB interno | D− = GPIO19; D+ = GPIO20 | [IDF — usb_pins.h (esp32s3)][idf-usb-h]; [HDG — schematic-checklist.rst][hdg-sc] | linhas 10–11; seção "USB", linhas 926–946 | CONFIRMADO na fonte |
| S3-USB-02 | Periféricos USB | USB OTG full-speed com transceptor integrado e controlador USB Serial/JTAG (dispositivo USB 2.0 full-speed) | [HDG — schematic-checklist.rst][hdg-sc] | seção "USB", linhas 930–934 | CONFIRMADO na fonte |
| S3-USB-03 | GPIO19/20 por padrão | "used by USB-JTAG by default. If they are reconfigured to operate as normal GPIOs, USB-JTAG functionality will be disabled" | [IDF — gpio/esp32s3.inc][idf-gpio] | nota "USB-JTAG", linha 223 | CONFIRMADO na fonte |
| S3-USB-04 | USB Serial/JTAG é de função fixa | console serial, gravação e JTAG; "cannot be reconfigured" (diferente do USB OTG) | [IDF — usb-serial-jtag-console.rst][idf-usj] | nota inicial | CONFIRMADO na fonte |
| S3-USB-05 | Recuperação quando o app reconfigura os pinos USB | colocar o chip em modo download manualmente: GPIO0 em nível baixo e reset | [IDF — usb-serial-jtag-console.rst][idf-usj] | "USB Pin Reconfiguration" | CONFIRMADO na fonte |
| S3-USB-06 | Quando o download automático por USB deixa de funcionar | PHY USB desligado pelo app; USB configurado para outra função ("USB host, USB standard device"); pinos USB usados por outro periférico. Nesses casos é preciso entrar em Joint Download Boot pelos straps | [HDG — download-guidelines.rst][hdg-dl] | notas, linhas 49–53 | CONFIRMADO na fonte |
| S3-USB-07 | Recomendações de projeto | manter controle dos pinos de strapping; manter a interface de download por UART (o firmware de teste de RF só suporta UART) | [HDG — download-guidelines.rst][hdg-dl] | linhas 55–56 | CONFIRMADO na fonte |
| S3-USB-08 | Componentes nas linhas USB | reservar resistores em série (22/33 Ω iniciais) e capacitores ao terra, perto do chip | [HDG — schematic-checklist.rst][hdg-sc] | seção "USB", linha 946 | CONFIRMADO na fonte |
| S3-USB-09 | USB_D+ no power-up | oscila entre alto e baixo ao energizar | [HDG — schematic-checklist.rst][hdg-sc] | seção "USB", linha 962 | CONFIRMADO na fonte |
| S3-USB-10 | Pinos acionados pela ROM quando o USB-OTG Download Boot está habilitado | GPIO40 (MTDO): saída em ALTO; GPIO39 (MTCK): saída em BAIXO; GPIO38: saída em BAIXO; GPIO42, GPIO41 e GPIO21: entradas. Evita-se queimando o eFuse `EFUSE_DIS_USB_OTG_DOWNLOAD_MODE` | [HDG — esp32s3-usb.inc][hdg-usb] | tabela "IO Pad Status After Chip Initialization in the USB-OTG Download Boot Mode" | CONFIRMADO na fonte |
| S3-UART-01 | Pinos padrão do UART0 (IO_MUX) | U0TXD = GPIO43; U0RXD = GPIO44; U0RTS = GPIO15; U0CTS = GPIO16 | [IDF — uart_pins.h (esp32s3)][idf-uart-h]; [HDG — schematic-checklist.rst][hdg-sc] | linhas 10–13; seção "UART", linhas 688–695 | CONFIRMADO na fonte |
| S3-UART-02 | Demais UARTs | 3 UARTs no total; UART1 em IO_MUX: TXD = GPIO17, RXD = GPIO18; UART2 só pela matriz de GPIO; os demais sinais podem ir a qualquer GPIO | [IDF — uart_pins.h (esp32s3)][idf-uart-h]; [HDG — schematic-checklist.rst][hdg-sc] | linhas 17–29 e 47–48; linha 695 | CONFIRMADO na fonte |
| S3-UART-03 | Papel do UART0 | "Usually, UART0 is used as the serial port for download and log printing"; resistor de 499 Ω em série em U0TXD; "For application communication, use UART interfaces other than UART0 if possible" | [HDG — schematic-checklist.rst][hdg-sc] | seção "UART", linhas 717–719 | CONFIRMADO na fonte |
| S3-UART-04 | A ROM imprime no UART0 | para confirmar o Joint Download Boot: "check the log via the UART0 serial port" (mensagem "waiting for download") | [HDG — download-guidelines.rst][hdg-dl] | passo 2, linha 18 | CONFIRMADO na fonte |
| S3-UART-05 | U0TXD e U0RXD no reset | entrada habilitada com pull-up fraco | [HDG — esp32s3-table-io-mux.inc][hdg-iomux] | linhas de U0TXD e U0RXD | CONFIRMADO na fonte |
| S3-JTAG-01 | Pinos de JTAG | MTCK = GPIO39; MTDO = GPIO40; MTDI = GPIO41; MTMS = GPIO42 | [IDF — jtag-debugging/esp32s3.inc][idf-jtag-s3] | tabela "ESP32-S3 pins and JTAG signals", linhas 117–130 | CONFIRMADO na fonte |
| S3-JTAG-02 | Uso de GPIO39–42 para outras funções | permitido quando a depuração é feita pelo USB Serial/JTAG (padrão) | [IDF — tips-and-quirks.rst][idf-jtag-tips] | "Can JTAG Pins Be Used for Other Purposes?", linhas 220–224 | CONFIRMADO na fonte |
| S3-XTAL-01 | Pinos do cristal de 32,768 kHz | XTAL_32K_P = GPIO15; XTAL_32K_N = GPIO16 | [IDF — io_mux_reg.h (esp32s3)][idf-iomux-h] | linhas 109–110 e 261–269 | CONFIRMADO na fonte |
| S3-XTAL-02 | O cristal de 32 kHz é opcional | sem ele, os dois pinos podem ser usados como GPIO | [HDG — schematic-checklist.rst][hdg-sc] | "RTC Clock Source (Optional)", linhas 561 e 594 | CONFIRMADO na fonte |

### 3.7 Glitches no power-up

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-GLT-01 | Glitch de nível baixo no power-up | GPIO1–GPIO14, XTAL_32K_P (GPIO15), XTAL_32K_N (GPIO16) e GPIO17: o pino fica em saída de nível baixo por cerca de 60 µs | [HDG — esp32s3-table-glitches.inc][hdg-glitch] | tabela "Power-Up Glitches on Pins" | CONFIRMADO na fonte |
| S3-GLT-02 | GPIO18 | glitch de nível baixo/alto, cerca de 60 µs | [HDG — esp32s3-table-glitches.inc][hdg-glitch] | tabela | CONFIRMADO na fonte |
| S3-GLT-03 | GPIO19 e GPIO20 | dois glitches de nível alto de cerca de 60 µs cada (GPIO19 também nível baixo; GPIO20 também pull-down); duração total de glitches e atraso: 3,2 ms (GPIO19) e 2 ms (GPIO20) | [HDG — esp32s3-table-glitches.inc][hdg-glitch] | tabela e nota 4 | CONFIRMADO na fonte |
| S3-GLT-04 | GPIO0, GPIO21 e GPIO33–48 | não constam da tabela de glitches | [HDG — esp32s3-table-glitches.inc][hdg-glitch] | tabela (ausência) | CONFIRMADO na fonte |

### 3.8 SPI, I2S, RMT e LEDC

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| S3-SPI-01 | Pinos IO_MUX do SPI2 (FSPI) | CS0 = GPIO10; SCLK = GPIO12; MOSI = GPIO11; MISO = GPIO13; QUADWP = GPIO14; QUADHD = GPIO9 | [IDF — spi_master.rst][idf-spi]; [IDF — io_mux_reg.h][idf-iomux-h] | "GPIO Matrix and IO_MUX", linhas 530–560; `FUNC_GPIO10_FSPICS0` etc., linhas 221–255 | CONFIRMADO na fonte |
| S3-SPI-02 | Mistura de IO_MUX e matriz | "If at least one signal is routed through the GPIO matrix, then all signals will be routed through it" | [IDF — spi_master.rst][idf-spi] | linha 537 | CONFIRMADO na fonte |
| S3-SPI-03 | Implicação de velocidade no ESP32-S3 | "When an SPI Host is set to 40 MHz or lower frequencies, routing SPI pins via the GPIO matrix will behave the same compared to routing them via IOMUX" | [IDF — spi_master.rst][idf-spi] | linha 539 (ramo "not esp32") | CONFIRMADO na fonte |
| S3-SPI-04 | Limite acima de 40 MHz pela matriz de GPIO no ESP32-S3 | a fonte não quantifica; a seção "Timing Considerations" (80 MHz por IO_MUX, bits dummy) está em bloco exclusivo do ESP32 original | [IDF — spi_master.rst][idf-spi] | bloco `only:: esp32`, linha 726 em diante | NÃO VERIFICADO |
| S3-SPI-05 | Uso do pino CS0 do IO_MUX | "Only the first Device attached to the bus can use the CS0 pin" | [IDF — spi_master.rst][idf-spi] | nota de rodapé 1, linha 562 | CONFIRMADO na fonte |
| S3-SPI-06 | EMC no barramento SPI | resistor em série (ou ferrite) e capacitor ao terra na linha de clock, perto do chip ou módulo | [HDG — schematic-checklist.rst][hdg-sc] | seção "SPI", linha 734 | CONFIRMADO na fonte |
| S3-SPI-07 | Cartão SD dividindo o barramento SPI: sequência de inicialização | inicializar o barramento; manter em nível alto o CS de todos os outros dispositivos; montar o cartão antes de qualquer outra comunicação SPI ("Otherwise, the card will stay in the SD mode ... may randomly respond") | [IDF — sdspi_share.rst][idf-sdspi] | "Initialization Sequence" | CONFIRMADO na fonte |
| S3-SPI-08 | Cartão SD dividindo o barramento SPI: carga | pull-ups de 10 kΩ a 50 kΩ exigidos pelo SD; a capacitância somada dos dispositivos limita a frequência | [IDF — sdspi_share.rst][idf-sdspi] | "Pin Loading of Other Devices" | CONFIRMADO na fonte |
| S3-I2S-01 | Quantidade de controladores I2S | dois | [IDF — i2s.rst][idf-i2s] | substituição `IDF_TARGET_I2S_NUM`, linha 6 | CONFIRMADO na fonte |
| S3-I2S-02 | Full-duplex em uma única porta | TX e RX registrados na mesma porta I2S ao mesmo tempo, compartilhando BCLK e WS (modos padrão e TDM; PDM full-duplex não é suportado) | [IDF — i2s.rst][idf-i2s] | seção "Full-duplex", linha 1006 | CONFIRMADO na fonte |
| S3-I2S-03 | Em full-duplex, só um canal gera o clock | "only one channel can work as the master that generates BCLK and WS" | [IDF — i2s.rst][idf-i2s] | nota, linha 1012 | CONFIRMADO na fonte |
| S3-I2S-04 | Sinais I2S passam pela matriz de GPIO | `I2S0O_BCK`, `I2S0O_WS`, `I2S0O_SD`, `I2S0I_SD` e `I2S0_MCLK` são sinais da matriz de GPIO (logo, roteáveis a qualquer GPIO, conforme S3-GPIO-07) | [IDF — gpio_sig_map.h (esp32s3)][idf-sig] | linhas 48–71 | CONFIRMADO na fonte |
| S3-RMT-01 | Finalidade do RMT | projetado como transceptor de infravermelho; cada canal é configurado como transmissor ou receptor; modula e demodula portadora | [IDF — rmt.rst][idf-rmt] | introdução, linhas 9–19 | CONFIRMADO na fonte |
| S3-RMT-02 | Sinais RMT na matriz de GPIO do ESP32-S3 | 4 entradas (`RMT_SIG_IN0–3`) e 4 saídas (`RMT_SIG_OUT0–3`); a contagem de canais no TRM não foi lida | [IDF — gpio_sig_map.h (esp32s3)][idf-sig] | linhas 157–164 | CONFIRMADO na fonte |
| S3-LEDC-01 | Canais e modo do LEDC | 8 canais; somente modo "low speed"; resolução de timer de até 14 bits | [IDF — soc_caps.h (esp32s3)][idf-caps]; [IDF — ledc.rst][idf-ledc] | linhas 227–228; linha 28 | CONFIRMADO na fonte |
| S3-LEDC-02 | Saídas LEDC passam pela matriz de GPIO | `LEDC_LS_SIG_OUT0–7` | [IDF — gpio_sig_map.h (esp32s3)][idf-sig] | linhas 149–156 | CONFIRMADO na fonte |

## 4. Módulo ESP32-S3-WROOM-1-N16R8

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| MOD-01 | Pinagem do módulo (41 terminais) | 1 GND, 2 3V3, 3 EN, 4 IO4, 5 IO5, 6 IO6, 7 IO7, 8 IO15, 9 IO16, 10 IO17, 11 IO18, 12 IO8, 13 IO19, 14 IO20, 15 IO3, 16 IO46, 17 IO9, 18 IO10, 19 IO11, 20 IO12, 21 IO13, 22 IO14, 23 IO21, 24 IO47, 25 IO48, 26 IO45, 27 IO0, 28 IO35, 29 IO36, 30 IO37, 31 IO38, 32 IO39, 33 IO40, 34 IO41, 35 IO42, 36 RXD0, 37 TXD0, 38 IO2, 39 IO1, 40 GND, 41 EPAD | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, símbolo U3 "ESP32-S3-WROOM-1 (pin-out)" | CONFIRMADO na fonte |
| MOD-02 | GPIO33 e GPIO34 não saem no módulo | os pinos 38 (GPIO33) e 39 (GPIO34) do chip, e o pino 28 (SPICS1), estão marcados sem conexão | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, símbolo U1 | CONFIRMADO na fonte |
| MOD-03 | GPIO35, GPIO36 e GPIO37 saem fisicamente no módulo | terminais 28, 29 e 30 (IO35, IO36, IO37) | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, símbolos U1 e U3 | CONFIRMADO na fonte |
| MOD-04 | Em módulos com PSRAM octal, GPIO35–37 não estão disponíveis | "used for the internal communication between ESP32-S3 and SPI flash/PSRAM memory, thus not available for external use" | [DKC — User Guide v1.1][dk-11] | nota depois da tabela de componentes, linha 80 | CONFIRMADO na fonte |
| MOD-05 | Flash do módulo | componente externo ao chip (U2, "FLASH-3V3"), alimentado por VDD_SPI, ligado por SPICS0, SPICLK, SPID, SPIQ, SPIWP e SPIHD (linhas de SPI quad) | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, símbolo U2 | CONFIRMADO na fonte |
| MOD-06 | PSRAM do módulo | dentro do chip, opcional, QSPI ou OSPI; variantes de chip previstas: ESP32-S3, ESP32-S3R2, ESP32-S3R8 | [REF — esquemático do módulo V1.3][hdg-ref] | folha 1, "Hardware Block" | CONFIRMADO na fonte |
| MOD-07 | Variante N16R8 | 16 MB de flash e 8 MB de PSRAM (linha da DevKitC-1-N16R8: módulo ESP32-S3-WROOM-1, antena de PCB) | [DKC — Ordering Information][dk-ord] | linha "ESP32-S3-DevKitC-1-N16R8" | CONFIRMADO na fonte |
| MOD-08 | Tipo de interface das memórias na variante com 8 MB de PSRAM | PSRAM de 8 MB do ESP32-S3R8 é SPI octal (S3-GPIO-06). A tabela antiga da DevKitC lista a N8R8 com flash "8 MB QD" (quad), PSRAM "8 MB OT" (octal) e tensão SPI de 3,3 V | [HDG — esp32s3-chip-flash-pin-mapping.inc][hdg-flash]; [DKC — User Guide v1.0][dk-10] | tabela de PSRAM octal; tabela "Ordering Information", linhas 121–148 | CONFIRMADO na fonte |
| MOD-09 | Linha da N16R8 na tabela do datasheet do módulo (flash 16 MB quad, PSRAM 8 MB octal, tensão SPI, chip embutido) | não lida. Evidência do repositório: o README registra a placa de bancada detectada com 16 MB + 8 MB de PSRAM, e `firmware/bruce/bmo.ini` usa `memory_type = qio_opi` | DS-MOD (seção 2.2) | tabela de variantes | NÃO VERIFICADO |
| MOD-10 | Dentro do módulo não há RC em EN | CHIP_PU vai direto ao terminal EN, sem resistor nem capacitor | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, rede CHIP_PU | CONFIRMADO na fonte |
| MOD-11 | Dentro do módulo não há pull-up externo em GPIO0 | a rede GPIO0 vai só ao terminal IO0 (resta o pull-up fraco interno do chip) | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, rede GPIO0 | CONFIRMADO na fonte |
| MOD-12 | GPIO45 dentro do módulo | resistor R1 de 10 kΩ para VDD33 marcado "NC" (não montado) | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, R1 | CONFIRMADO na fonte |
| MOD-13 | U0TXD dentro do módulo | resistor R3 de 499 Ω em série | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, R3 | CONFIRMADO na fonte |
| MOD-14 | Cristal e proteção | cristal de 40 MHz (±10 ppm); diodo ESD em VDD33; desacoplamento de 10 µF, 1 µF e 0,1 µF em VDD33 | [REF — esquemático do módulo V1.3][hdg-ref] | folha 2, Y1, D1, C6–C9 | CONFIRMADO na fonte |
| MOD-15 | Tensão de alimentação e corrente mínima da fonte, na tabela do datasheet do módulo | trecho de busca: VDD33 mín. 3,0 V, típ. 3,3 V, máx. 3,6 V; IVDD mín. 0,5 A. Os mesmos valores estão confirmados no guia de hardware (S3-PWR-01 a 03) | DS-MOD (seção 2.2) | "Recommended Operating Conditions" | NÃO VERIFICADO (trecho de busca) |
| MOD-16 | Correntes de pico de TX/RX do módulo | não obtidas | DS-MOD (seção 2.2) | tabelas de consumo de RF | NÃO VERIFICADO |
| MOD-17 | Dimensões do módulo e footprint recomendado | não lidos | DS-MOD (seção 2.2) | "Module Dimensions" | NÃO VERIFICADO |
| MOD-18 | Posição do módulo na placa-base | antena de PCB para fora da placa-base e ponto de alimentação da antena perto da borda; se não puder ultrapassar a borda, recortar a placa-base dos dois lados e sob a antena | [HDG — pcb-layout-design.rst][hdg-pcb] | "Positioning a Module on a Base Board", linhas 341–367 | CONFIRMADO na fonte |
| MOD-19 | Antena dentro da carcaça | "A clearance of at least 15 mm is recommended in all directions" | [HDG — pcb-layout-design.rst][hdg-pcb] | linha 375 | CONFIRMADO na fonte |
| MOD-20 | Sinais UART e USB longe da antena | porta USB, conversor USB-serial e linhas UART o mais longe possível da antena | [HDG — pcb-layout-design.rst][hdg-pcb] | linha 227 | CONFIRMADO na fonte |

## 5. Placa ESP32-S3-DevKitC-1

| ID | Afirmação | Valor | Fonte (título + URL) | Onde na fonte | Status |
|---|---|---|---|---|---|
| DKC-01 | LED RGB na revisão v1.0 | "Addressable RGB LED, driven by GPIO48" | [DKC — User Guide v1.0][dk-10] | tabela de componentes, linha 74; J3 pino 16 | CONFIRMADO na fonte |
| DKC-02 | LED RGB na revisão v1.1 | "Addressable RGB LED, driven by GPIO38" | [DKC — User Guide v1.1][dk-11] | tabela de componentes, linha 74; J3 pino 10 | CONFIRMADO na fonte |
| DKC-03 | Diferença entre revisões | as duas estão no mercado; a diferença principal é o GPIO do LED RGB (v1.0: GPIO48; v1.1: GPIO38) | [DKC — User Guide v1.1][dk-11] | "Hardware Revision Details", linha 256 | CONFIRMADO na fonte |
| DKC-04 | Como identificar a revisão na figura oficial | a figura de pinagem v1.1 mostra a marcação "RGB@IO38" ao lado do LED | [DKC — figura de pinagem v1.1][dk-pin] | centro da placa | CONFIRMADO na fonte |
| DKC-05 | Ponte USB-UART | chip único, até 3 Mbps; porta "USB-to-UART" (Micro-USB) serve para alimentar, gravar e comunicar | [DKC — User Guide v1.1][dk-11] | tabela de componentes, linhas 63–64 e 71–72 | CONFIRMADO na fonte |
| DKC-06 | A ponte fala com o módulo por TX/RX | diagrama de blocos: "USB-to-UART Bridge" ligado ao módulo por "TX/RX" | [DKC — diagrama de blocos][dk-blk] | figura | CONFIRMADO na fonte |
| DKC-07 | Os pinos TX e RX do conector J3 | pino 2 "TX" = U0TXD, GPIO43; pino 3 "RX" = U0RXD, GPIO44 | [DKC — User Guide v1.1][dk-11] | tabela J3, linhas 208–209 | CONFIRMADO na fonte |
| DKC-08 | Modelo da ponte e detalhe elétrico da ligação a GPIO43/44 (resistores em série, possibilidade de isolar) | a ilustração oficial desenha o chip com a marca "CP2102"; o esquemático não foi aberto | SCH-DKC (seção 2.2); [DKC — figura de pinagem v1.1][dk-pin] | — | NÃO VERIFICADO |
| DKC-09 | Botão BOOT | "Download button. Holding down Boot and then pressing Reset initiates Firmware Download mode"; a figura de pinagem rotula o GPIO0 como "BOOT" | [DKC — User Guide v1.1][dk-11]; [DKC — figura de pinagem v1.1][dk-pin] | linhas 65–66; rótulo de GPIO0 | CONFIRMADO na fonte |
| DKC-10 | Botão RESET e pino RST | reinicia o sistema; pino RST do J1 = EN | [DKC — User Guide v1.1][dk-11] | linhas 67–68; J1 pino 3 | CONFIRMADO na fonte |
| DKC-11 | Regulador de 3,3 V | "5 V to 3.3 V LDO: Power regulator that converts a 5 V supply into a 3.3 V output" | [DKC — User Guide v1.1][dk-11] | tabela de componentes, linhas 59–60 | CONFIRMADO na fonte |
| DKC-12 | Modelo e corrente nominal do LDO | sem evidência obtida | SCH-DKC (seção 2.2) | — | NÃO VERIFICADO |
| DKC-13 | Opções de alimentação da placa | três formas "mutually exclusive": portas USB (padrão, recomendada); pinos 5V e G; pinos 3V3 e G | [DKC — User Guide v1.1][dk-11] | "Power Supply Options", linhas 155–159 | CONFIRMADO na fonte |
| DKC-14 | Comportamento ao alimentar o pino 3V3 com USB ligado, ou proteção do LDO contra tensão aplicada na saída | sem evidência além de "mutually exclusive" | SCH-DKC (seção 2.2) | — | NÃO VERIFICADO |
| DKC-15 | LED de alimentação | "3.3 V Power On LED: Turns on when the USB power is connected to the board"; corrente do LED não informada | [DKC — User Guide v1.1][dk-11] | linhas 75–76 | CONFIRMADO na fonte |
| DKC-16 | Porta USB nativa | USB OTG full-speed ("compliant with the USB 1.1 specification"); serve para alimentar, gravar, comunicar e depurar por JTAG | [DKC — User Guide v1.1][dk-11] | linhas 69–70 | CONFIRMADO na fonte |
| DKC-17 | Conectores | cabo exigido: "USB 2.0 cable (Standard-A to Micro-B)" | [DKC — User Guide v1.1][dk-11] | "Required Hardware", linha 93 | CONFIRMADO na fonte |
| DKC-18 | Pinos expostos | todos os GPIOs disponíveis, exceto o barramento SPI da flash; J1 e J3 com 22 pinos cada; GPIO35/36/37 estão em J3 (pinos 13/12/11), mas não servem em placas com memória octal | [DKC — User Guide v1.1][dk-11] | linhas 61–62, 80 e tabelas J1/J3 | CONFIRMADO na fonte |
| DKC-19 | Variante N16R8 da placa | ESP32-S3-DevKitC-1-N16R8: módulo ESP32-S3-WROOM-1, 16 MB de flash, 8 MB de PSRAM, antena de PCB | [DKC — Ordering Information][dk-ord] | linhas 25–29 | CONFIRMADO na fonte |
| DKC-20 | Dimensões da placa | o guia não traz medidas no texto; remete a um desenho dimensional em PDF/DXF, que não foi aberto | DIM-DKC (seção 2.2) | — | NÃO VERIFICADO |
| DKC-21 | Funções de strapping rotuladas na figura oficial | GPIO0 "BOOT"; GPIO3 "JTAG"; GPIO45 "VSPI"; GPIO46 "LOG" | [DKC — figura de pinagem v1.1][dk-pin] | rótulos de strapping | CONFIRMADO na fonte |

## 6. Avaliação: botão de energia no GPIO0 em um projeto com power-latch

Pergunta: no perfil `target` de `firmware/config/include/dbmo/config/pins.h`, o botão de energia
está no GPIO0 e o `POWER_HOLD` no GPIO48. O que acontece se o botão mantém o GPIO0 em nível
baixo enquanto o chip é energizado?

Fatos confirmados na fonte:

1. GPIO0 = 0 com GPIO46 = 0 na saída do reset seleciona Joint Download Boot (S3-STRAP-05).
2. O GPIO46 tem pull-down interno por padrão (S3-STRAP-03) e está livre no perfil `target`
   (`kFreePins` em `pins.h`), portanto é lido como 0.
3. Os straps são lidos a cada partida ou reset, com janela de hold de pelo menos 3 ms depois de
   CHIP_PU subir (S3-STRAP-02, S3-STRAP-07). Um aperto de botão dura muito mais que isso.
4. O fabricante avisa que até um capacitor grande em GPIO0 pode levar o chip ao modo download
   (S3-STRAP-08).
5. Em Joint Download Boot a ROM fica esperando download ("waiting for download" no UART0,
   S3-UART-04); o aplicativo não é executado.

Consequência (inferência, não medida): em uma topologia em que o mesmo botão liga o regulador e
puxa o GPIO0 ao terra, o chip sai do reset em Joint Download Boot, o firmware não roda, o
`POWER_HOLD` nunca é acionado e a alimentação cai quando o botão é solto. O aparelho não ligaria
em uso normal. **Classificação: PROPOSTO com risco aberto; PENDENTE DE VALIDAÇÃO FÍSICA.**
O risco só não existe se, durante os primeiros milissegundos depois de CHIP_PU subir, o GPIO0
enxergar nível alto mesmo com o botão pressionado.

Agravantes confirmados:

- Um reset por watchdog RTC com o botão pressionado pode levar a um novo modo de boot
  (S3-STRAP-20).
- Inverter a lógica (botão pressionado = GPIO0 alto) deixaria o GPIO0 em nível baixo no repouso,
  e qualquer reset entraria em modo download; contraria também a recomendação de pull-up em GPIO0.
- O Bruce de bancada usa USB como HID (`-DUSB_as_HID=1` em `firmware/bruce/bmo.ini`). Com o USB
  configurado para outra função, o download automático por USB deixa de funcionar e a entrada
  em modo download passa a depender dos straps (S3-USB-06). O acesso ao GPIO0 (função BOOT)
  precisa ser preservado no produto final.

Alternativas (nenhuma decidida aqui):

- **A.** Ler o botão de energia em um GPIO RTC que não seja de strapping (GPIO1, 2 ou 4–21) e
  deixar o GPIO0 só como BOOT (pad de teste ou botão interno). No perfil `target` todos esses
  pinos já estão ocupados, então exige remanejar a pinagem.
- **B.** Desacoplar a leitura do botão do GPIO0 por circuito, de modo que o GPIO0 fique em nível
  alto durante o power-up mesmo com o botão pressionado. Depende da topologia do latch, ainda não
  definida.
- **C.** Não cortar a alimentação e usar deep sleep com wake por ext0/ext1 em GPIO0. Falta
  confirmar se os straps são reamostrados ao acordar do deep sleep (S3-STRAP-21). O consumo do
  módulo em deep sleep, medido pela Espressif, é de 6,8 a 8,14 µA (S3-PWR-14, S3-PWR-15), o que
  sozinho ocupa quase toda a meta de "corrente desligado ≤ ~10 µA" do README.

Evidência que falta:

- Leitura do capítulo "Boot Configurations" do datasheet (DS-SOC).
- Teste de bancada simples, possível hoje na DevKitC: manter o botão BOOT (GPIO0) pressionado ao
  energizar a placa e observar no UART0 se aparece "waiting for download".
- Esquema do circuito de power-latch (ainda não existe no repositório).

## 7. Implicações para a pinagem proposta

Leitura da pinagem do README e de `pins.h` à luz das evidências acima. Nada aqui altera a pinagem.

| Item do projeto | Evidência | Classificação |
|---|---|---|
| SPI em GPIO10/11/12/13 (tela, CC1101, SD, cartucho) | Coincide com os pinos IO_MUX do SPI2 (S3-SPI-01). Até 40 MHz, matriz e IO_MUX se comportam igual (S3-SPI-03). O README registra a tela funcionando nesses pinos a 10 MHz | Mapa IO_MUX: CONFIRMADO (fabricante). Tela a 10 MHz: VALIDADO EM BANCADA. Barramento com CC1101, SD e cartucho: PENDENTE DE VALIDAÇÃO FÍSICA |
| VBAT em GPIO1; analógicos em GPIO2/4/5/6 | São ADC1_CH0, CH1, CH3, CH4 e CH5 (S3-ADC-01) | Canais: CONFIRMADO (fabricante). Uso: PROPOSTO |
| VBAT com divisor 100k/100k | 4,2 V viram 2,1 V, dentro da faixa efetiva de 0–2900 mV em ATTEN3, com erro total de ±50 mV no pino (S3-ADC-09), ou seja, cerca de ±100 mV na bateria depois do ×2. O fabricante pede 0,1 µF no pino (S3-ADC-10) | PROPOSTO; PENDENTE DE VALIDAÇÃO FÍSICA |
| Analógicos alimentados em 3,3 V | O cursor pode ir de 0 a 3,3 V; a faixa efetiva calibrada termina em 2900 mV (S3-ADC-09). O trecho final do curso pode saturar ou perder exatidão. Falta medir o curso elétrico real dos sticks | PROPOSTO; PENDENTE DE VALIDAÇÃO FÍSICA |
| Botão de energia em GPIO0 | Ver seção 6 | PROPOSTO com risco aberto; PENDENTE DE VALIDAÇÃO FÍSICA |
| Shutdown do MAX98357A em GPIO3 | Strap de JTAG só atua com o eFuse `STRAP_JTAG_SEL` queimado (S3-STRAP-13). No reset o pino fica flutuante (S3-STRAP-14) e tem glitch de nível baixo de cerca de 60 µs (S3-GLT-01). Ocupa o canal ADC1_CH2 | PROPOSTO; estado do amplificador durante o boot: PENDENTE DE VALIDAÇÃO FÍSICA |
| UART do cartucho em GPIO43/44 | É o UART0: a ROM imprime o log de boot em U0TXD por padrão (S3-STRAP-16, S3-UART-04), U0RXD é entrada de download em Joint Download Boot, e o fabricante recomenda outro UART para a aplicação (S3-UART-03). Na DevKitC esses pinos também estão na ponte USB-UART (DKC-06, DKC-07) | PROPOSTO com risco aberto |
| POWER_HOLD em GPIO48 | Não é GPIO RTC: domínio VDD_SPI / VDD3P3_CPU (S3-GPIO-09); em deep sleep só se mantém com `gpio_deep_sleep_hold_en` (S3-RTC-08). No reset fica como entrada sem pull (S3-GPIO-08): o latch precisa de estado padrão definido por resistor externo. Na DevKitC v1.0 é o LED RGB (DKC-01) | PROPOSTO; PENDENTE DE VALIDAÇÃO FÍSICA |
| CS do cartucho em GPIO38 | Na DevKitC v1.1 é o LED RGB (DKC-02). Com USB-OTG Download Boot habilitado, a ROM leva o GPIO38 a nível baixo (S3-USB-10); se o CS for ativo em nível baixo, isso selecionaria o cartucho. A fonte não deixa claro se ocorre em toda partida ou só ao entrar no modo download | PROPOSTO; PENDENTE DE VALIDAÇÃO FÍSICA |
| IR RX em GPIO39; I2S em GPIO40/41/42 | São os pinos de JTAG, livres enquanto o JTAG fica no USB (S3-JTAG-02). Com USB-OTG Download Boot, a ROM aciona GPIO39 em baixo e GPIO40 em alto (S3-USB-10), com a mesma ressalva da linha anterior | PROPOSTO; PENDENTE DE VALIDAÇÃO FÍSICA |
| I2S full-duplex (MAX98357A + INMP441 na mesma porta) | Suportado: TX e RX na mesma porta, compartilhando BCLK e WS (S3-I2S-02) | Capacidade: CONFIRMADO (fabricante). Montagem: PENDENTE DE VALIDAÇÃO FÍSICA |
| GDO0 do CC1101 em GPIO15; backlight em GPIO16 | São os pinos do cristal de 32 kHz, utilizáveis como GPIO sem o cristal (S3-XTAL-02). Sem pull no reset (S3-GPIO-08) e com glitch de nível baixo no power-up (S3-GLT-01) | PROPOSTO |
| USB em GPIO19/20 | Fixos no chip (S3-USB-01) | CONFIRMADO (fabricante) |
| GPIO45 e GPIO46 livres | GPIO45 alto no reset leva VDD_SPI a 1,8 V (S3-STRAP-09); GPIO46 participa do modo de boot e do log da ROM (S3-STRAP-05, S3-STRAP-17) | CONFIRMADO (fabricante) |
| GPIO35/36/37 | Indisponíveis no N16R8 (MOD-04) | CONFIRMADO (fabricante) |
| EN (reset) na PCB própria | O módulo não traz o RC (MOD-10); o fabricante recomenda 10 kΩ + 1 µF e, com bateria e liga/desliga frequente, um supervisor de tensão em torno de 3,0 V (S3-PWR-09, S3-PWR-10) | Recomendação: CONFIRMADO (fabricante). Circuito: em aberto |
| Regulador de 3,3 V | Só o ESP32-S3 pede fonte de pelo menos 500 mA (S3-PWR-01) e trilho acima de 3,0 V (S3-PWR-05); tela, CC1101, PN532 e demais cargas somam a isso. Topologia e CI continuam em aberto | Requisito do chip: CONFIRMADO (fabricante). Regulador: em aberto |
| Revisão da DevKitC de bancada (v1.0 ou v1.1) | Não documentada no repositório. Conferir a serigrafia ao lado do LED RGB (DKC-04) | PENDENTE DE VALIDAÇÃO FÍSICA |

## 8. Pontos não verificados

1. **Datasheets não abertos** (hosts da Espressif bloqueados): ESP32-S3 Series Datasheet e
   ESP32-S3-WROOM-1 Datasheet. Tudo o que depende só deles está marcado NÃO VERIFICADO.
2. Corrente de pico de RF: Wi-Fi TX por padrão e potência, Wi-Fi RX, Bluetooth LE TX/RX
   (S3-PWR-16, MOD-16). Nem os trechos de busca trouxeram valores. O único número confirmado
   nessa direção é o máximo medido em modem-sleep, de 235,8 a 267,2 mA (S3-PWR-11), e a
   recomendação de fonte de pelo menos 500 mA (S3-PWR-01).
3. Correntes típicas de datasheet em light-sleep, deep-sleep e power-off (S3-PWR-17). Os valores
   confirmados são medições publicadas no guia do ESP-IDF, não a tabela do datasheet.
4. Corrente por pino por nível de drive, resistência dos pulls internos e limite de corrente
   somada (S3-GPIO-14 a 16).
5. Tabela de variantes do módulo: linha exata da N16R8 (tipo de flash, tipo de PSRAM, tensão SPI,
   chip embutido, faixa de temperatura) (MOD-09); estado de fábrica dos eFuses de VDD_SPI
   (S3-STRAP-11).
6. Dimensões e footprint do módulo (MOD-17).
7. Capítulo "Boot Configurations" do datasheet: tabela completa do JTAG com os três eFuses
   (S3-STRAP-15), tabela de controle do log da ROM com GPIO46 (S3-STRAP-18), comportamento com
   GPIO0 = 0 e GPIO46 = 1 (S3-STRAP-06) e se os straps são reamostrados ao acordar do deep sleep
   (S3-STRAP-21).
8. Tabela "ADC Characteristics" do datasheet (S3-ADC-11) e errata do ESP32-S3 citada para o ADC2.
9. Limite de frequência de SPI acima de 40 MHz pela matriz de GPIO no ESP32-S3 (S3-SPI-04).
10. Esquemático da DevKitC-1: modelo e corrente do LDO (DKC-12), modelo da ponte USB-UART e
    ligação elétrica a GPIO43/44 (DKC-08), existência de diodo ou proteção entre USB, 5V e 3V3
    (DKC-14), ligação elétrica do botão BOOT.
11. Dimensões oficiais da DevKitC-1 (DKC-20). O README já prevê medir a placa com paquímetro.
12. Revisão da DevKitC de bancada e tipo de conector USB dela (o guia oficial descreve
    Micro-USB): não documentados no repositório.
13. Planilha de materiais do projeto de referência do módulo (`BOM(V0.8)_ESP32-S3-WROOM-1.xls`):
    presente no ZIP, mas não foi possível lê-la nesta sessão.
14. Equivalência entre o ESP-IDF `master` lido e a versão embutida no core Arduino usado pelo
    Bruce (disponibilidade das APIs de wake e hold citadas).
15. Momento em que a ROM aciona GPIO38, GPIO39 e GPIO40 por causa do USB-OTG Download Boot
    (S3-USB-10): a frase da fonte ("initializes the IO pad ... in ROM when starts up") não
    esclarece se vale para toda partida ou só para a entrada no modo download. Medível na
    bancada com osciloscópio ou analisador lógico.

## 9. Divergências encontradas em relação ao README

Comparação com `README.md` (seções "Arquitetura", "Pinagem", "Alimentação" e "Configurações
críticas") e, onde indicado, com `firmware/config/include/dbmo/config/pins.h`.

1. **ADC2 e Wi-Fi (README, "Arquitetura": "o ADC2 não funciona com o WiFi ligado").** A fonte é
   menos categórica: no modo oneshot o ADC2 é compartilhado com o Wi-Fi, o driver arbitra o uso e
   a leitura pode falhar com `ESP_ERR_TIMEOUT` (S3-ADC-04, S3-ADC-05); no modo contínuo o ADC2
   deixou de ser suportado no ESP32-S3 (S3-ADC-06). A decisão do README (usar só o ADC1)
   continua coerente com a fonte; a justificativa é que deveria ser reescrita.
2. **Botão de energia no GPIO0 (README, "Pinagem": "strapping; GPIO RTC, acorda do deep sleep").**
   Está correto que GPIO0 é RTC_GPIO0, mas o README não menciona que GPIO0 em nível baixo na
   saída do reset seleciona o modo download (seção 6). O `pins.h` já marca o pino como
   `ProposedAtRisk`; o README não traz o aviso.
3. **UART do cartucho em GPIO43/44 (README, "Pinagem": "UART0; liberado porque o console usa o
   USB CDC").** Incompleto. Mesmo com o console no USB, a ROM imprime o log de boot em U0TXD por
   padrão, e mudar isso é uma gravação permanente de eFuse (S3-STRAP-16); U0RXD é a entrada de
   download por UART; o fabricante recomenda usar outro UART para comunicação da aplicação
   (S3-UART-03). Na DevKitC, GPIO43/44 também estão ligados à ponte USB-UART da placa (DKC-06,
   DKC-07), o que afeta o teste de bancada.
4. **Pino 3V3 da DevKitC (README, "Riscos da bateria", item 6: "o pino 3V3 aceita 3,3 V direto
   (sem passar pelo regulador da placa)").** A fonte confirma que 3V3 + G é uma das três formas de
   alimentar a placa, mas as declara mutuamente exclusivas (DKC-13): não alimentar por 3V3 com o
   USB conectado. Que a corrente "não passa pelo regulador" e que o LDO tolera tensão aplicada na
   saída não foi verificado (DKC-14).
5. **GPIO3 (README, "Pinagem": "strapping (JTAG); seguro como saída depois do boot").** Coerente
   com a fonte, com dois complementos: o strap só tem efeito com o eFuse `STRAP_JTAG_SEL` queimado
   (S3-STRAP-13), e durante o reset o pino fica flutuante (S3-STRAP-14), de modo que o estado do
   amplificador no boot depende do circuito externo.
6. **POWER_HOLD em GPIO48 (README, "Pinagem").** O README avisa sobre o LED RGB apenas na linha do
   GPIO38. Na DevKitC v1.0 o LED RGB está no GPIO48 (DKC-01), então o aviso vale para as duas
   linhas. Além disso, GPIO48 não é GPIO RTC (item correspondente na seção 7).
7. **"GPIO 26–37 (flash/PSRAM octal)" (README, "Pinagem").** Coerente com a fonte (S3-GPIO-03 a
   05). Já o comentário de `pins.h` ("33–37 não saem no módulo") é impreciso: GPIO33 e GPIO34 não
   saem no módulo (MOD-02); GPIO35, 36 e 37 saem fisicamente (MOD-03) e aparecem no conector J3 da
   DevKitC, mas estão ocupados pela PSRAM octal no N16R8 (MOD-04). O efeito prático é o mesmo.
8. **GPIO45 e GPIO46 (README, "Pinagem": "o 45 define a tensão da flash no boot").** Coerente
   (S3-STRAP-09). Complemento: o GPIO46 também é strap ativo (modo de boot e log da ROM) e deve
   permanecer em nível baixo ou livre no reset.
9. **Corrente desligado ≤ ~10 µA (README, "Riscos da bateria", item 4).** Não é divergência de
   fato, mas um limite a registrar: se "desligado" for implementado como deep sleep, o módulo
   sozinho consome de 6,8 a 8,14 µA nas medições da Espressif (S3-PWR-14, S3-PWR-15), sem contar
   regulador, divisor e BMS.
10. **Sem divergência encontrada** nos seguintes pontos do README: módulo N16R8 com 16 MB de
    flash e 8 MB de PSRAM e antena de PCB (MOD-07, DKC-19); USB nativo em GPIO19/20 (S3-USB-01);
    LED RGB em GPIO38 ou GPIO48 conforme a revisão (DKC-03); ADC1 em GPIO1–10 (S3-ADC-01); GPIOs
    RTC 0–21 para wake do deep sleep (S3-RTC-01, S3-RTC-05); MAX98357A e INMP441 na mesma porta
    I2S (S3-I2S-02); SPI em GPIO10–13 com possibilidade de subir a 40 MHz do lado do ESP32-S3
    (S3-SPI-01, S3-SPI-03).

[hdg-sc]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/schematic-checklist.rst
[hdg-boot]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/esp32s3/esp32s3-chip-boot-mode-control.inc
[hdg-core]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/esp32s3/esp32s3-chip-core-schem-note.inc
[hdg-flash]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/esp32s3/esp32s3-chip-flash-pin-mapping.inc
[hdg-iomux]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/esp32s3/esp32s3-table-io-mux.inc
[hdg-glitch]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/esp32s3/esp32s3-table-glitches.inc
[hdg-adc]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/esp32s3/esp32s3-table-adc-gpio.inc
[hdg-usb]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/esp32s3/esp32s3-usb.inc
[hdg-pwr]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/shared/esp32-s-series-digital-power-supply.inc
[hdg-dl]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/download-guidelines.rst
[hdg-pcb]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/en/pcb-layout-design.rst
[hdg-ref]: https://github.com/espressif/esp-hardware-design-guidelines/blob/f6f2603594c2e5f3744da931d605c5f5d4a81772/docs/_static/esp32s3/esp32-s3-wroom-1_reference_design.zip
[dk-11]: https://github.com/espressif/esp-dev-kits/blob/5c06bf1835329d6525f316475f7218aab6acfe79/docs/en/esp32-s3-devkitc-1/user_guide_v1.1.rst
[dk-10]: https://github.com/espressif/esp-dev-kits/blob/5c06bf1835329d6525f316475f7218aab6acfe79/docs/en/esp32-s3-devkitc-1/user_guide_v1.0.rst
[dk-ord]: https://github.com/espressif/esp-dev-kits/blob/5c06bf1835329d6525f316475f7218aab6acfe79/docs/en/esp32-s3-devkitc-1/ordering-info.inc
[dk-pin]: https://github.com/espressif/esp-dev-kits/blob/5c06bf1835329d6525f316475f7218aab6acfe79/docs/_static/esp32-s3-devkitc-1/ESP32-S3_DevKitC-1_pinlayout_v1.1.jpg
[dk-blk]: https://github.com/espressif/esp-dev-kits/blob/5c06bf1835329d6525f316475f7218aab6acfe79/docs/_static/esp32-s3-devkitc-1/ESP32-S3-DevKitC-1_v2-SystemBlock.png
[idf-gpio]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/gpio/esp32s3.inc
[idf-adc1]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/adc/adc_oneshot.rst
[idf-adcc]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/adc/adc_continuous.rst
[idf-adci]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/adc/index.rst
[idf-sleep]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/system/sleep_modes.rst
[idf-lpw]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/low-power-mode/low-power-mode-wifi.rst
[idf-cur-sum]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/sleep-current/esp32s3_summary.inc
[idf-cur-ms]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/sleep-current/esp32s3_modem_sleep.inc
[idf-cur-ls]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/sleep-current/esp32s3_light_sleep.inc
[idf-ccm]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/current-consumption-measurement-modules.rst
[idf-spi]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/spi_master.rst
[idf-sdspi]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/sdspi_share.rst
[idf-i2s]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/i2s.rst
[idf-rmt]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/rmt.rst
[idf-ledc]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-reference/peripherals/ledc.rst
[idf-usj]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/usb-serial-jtag-console.rst
[idf-jtag-other]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/jtag-debugging/configure-other-jtag.rst
[idf-jtag-s3]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/jtag-debugging/esp32s3.inc
[idf-jtag-tips]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/jtag-debugging/tips-and-quirks.rst
[idf-startup]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/docs/en/api-guides/startup.rst
[idf-adc-h]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/soc/esp32s3/include/soc/adc_channel.h
[idf-rtc-h]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/soc/esp32s3/include/soc/rtc_io_channel.h
[idf-uart-h]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/soc/esp32s3/include/soc/uart_pins.h
[idf-usb-h]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/soc/esp32s3/include/soc/usb_pins.h
[idf-iomux-h]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/soc/esp32s3/register/soc/io_mux_reg.h
[idf-caps]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/soc/esp32s3/include/soc/soc_caps.h
[idf-sig]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/soc/esp32s3/include/soc/gpio_sig_map.h
[idf-gpio-h]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/esp_driver_gpio/include/driver/gpio.h
[idf-gpiot]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/esp_hal_gpio/include/hal/gpio_types.h
[idf-romk]: https://github.com/espressif/esp-idf/blob/827cc6229fb01c6ecf6362b4840ef27d62d2138e/components/esp_rom/Kconfig.projbuild
