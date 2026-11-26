# Mapa de GPIOs do d-BMO v1

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Fonte de verdade | `firmware/config/include/dbmo/config/pins.h` |
| Evidências | `docs/hardware/sources/ESP32-S3.md` (identificadores `S3-…`, `MOD-…`, `DKC-…`) |
| Estado | Pinagem **proposta**. Validados em hardware na bancada (21 a 25/11/2026): tela, três botões, receptor de IR (GPIO 4), CS do CC1101, CS do SD e os três pinos I2S do microfone |

As tabelas entre os marcadores `BEGIN pinmap` são **geradas** a partir de `pins.h`
(`python3 tools/check_pinmap.py --write`) e conferidas na CI: não editar à mão.

## 1. Classificação usada

| Rótulo | Significado |
|---|---|
| Fixo no chip | Função presa ao pino pelo ESP32-S3 (por exemplo, USB em 19/20). Confirmado em fonte do fabricante |
| Validado em bancada | Já funcionou na protoboard, com o registro em `docs/testing/TESTING.md` |
| Proposto | Escolha do projeto, coerente com as restrições do fabricante, **sem** validação física |
| Proposto, com risco aberto | Proposto, e há um problema conhecido que pode obrigar a mudar o pino |

Nenhum pino é "definitivo": a pinagem só fecha depois dos ensaios de bancada da seção 6.

## 2. Perfil `target` (pinagem proposta do v1)

<!-- BEGIN pinmap:target (gerado por tools/check_pinmap.py --write; não editar) -->

| GPIO | Sinal | Classificação |
|---:|---|---|
| 1 | `VBAT_SENSE` | PROPOSTO |
| 2 | `POWER_BUTTON` | PROPOSTO |
| 3 | `AMP_SHUTDOWN` | PROPOSTO |
| 6 | `CHARGER_STATUS` | PROPOSTO |
| 7 | `I2S_MIC_DATA` | VALIDADO EM BANCADA |
| 8 | `I2C_SDA` | PROPOSTO |
| 9 | `I2C_SCL` | PROPOSTO |
| 10 | `DISPLAY_CS` | VALIDADO EM BANCADA |
| 11 | `SPI_MOSI` | VALIDADO EM BANCADA |
| 12 | `SPI_SCK` | VALIDADO EM BANCADA |
| 13 | `SPI_MISO` | VALIDADO EM BANCADA |
| 14 | `CC1101_CS` | VALIDADO EM BANCADA |
| 15 | `CC1101_GDO0` | PROPOSTO |
| 16 | `DISPLAY_BACKLIGHT` | PROPOSTO |
| 17 | `DISPLAY_DC` | VALIDADO EM BANCADA |
| 18 | `DISPLAY_RESET` | VALIDADO EM BANCADA |
| 19 | `USB_DM` | CONFIRMADO (fabricante) |
| 20 | `USB_DP` | CONFIRMADO (fabricante) |
| 21 | `SD_CS` | VALIDADO EM BANCADA |
| 39 | `IR_RX` | PROPOSTO |
| 40 | `I2S_BCLK` | VALIDADO EM BANCADA |
| 41 | `I2S_LRCLK` | VALIDADO EM BANCADA |
| 42 | `I2S_SPEAKER_DATA` | PROPOSTO |
| 47 | `IR_TX` | PROPOSTO |
| 48 | `POWER_HOLD` | PROPOSTO |

<!-- END pinmap:target -->

## 3. Perfil `bench` (a protoboard de hoje)

<!-- BEGIN pinmap:bench (gerado por tools/check_pinmap.py --write; não editar) -->

| GPIO | Sinal | Classificação |
|---:|---|---|
| 0 | `POWER_BUTTON` | PROPOSTO |
| 2 | `BUTTON_OK` | VALIDADO EM BANCADA |
| 4 | `IR_RX` | VALIDADO EM BANCADA |
| 5 | `IR_TX` | PROPOSTO |
| 7 | `I2S_MIC_DATA` | VALIDADO EM BANCADA |
| 8 | `I2C_SDA` | PROPOSTO |
| 9 | `I2C_SCL` | PROPOSTO |
| 10 | `DISPLAY_CS` | VALIDADO EM BANCADA |
| 11 | `SPI_MOSI` | VALIDADO EM BANCADA |
| 12 | `SPI_SCK` | VALIDADO EM BANCADA |
| 13 | `SPI_MISO` | VALIDADO EM BANCADA |
| 14 | `CC1101_CS` | VALIDADO EM BANCADA |
| 15 | `CC1101_GDO0` | PROPOSTO |
| 16 | `DISPLAY_BACKLIGHT` | PROPOSTO |
| 17 | `DISPLAY_DC` | VALIDADO EM BANCADA |
| 18 | `DISPLAY_RESET` | VALIDADO EM BANCADA |
| 19 | `USB_DM` | CONFIRMADO (fabricante) |
| 20 | `USB_DP` | CONFIRMADO (fabricante) |
| 21 | `SD_CS` | VALIDADO EM BANCADA |
| 39 | `BUTTON_UP` | VALIDADO EM BANCADA |
| 40 | `I2S_BCLK` | VALIDADO EM BANCADA |
| 41 | `I2S_LRCLK` | VALIDADO EM BANCADA |
| 42 | `I2S_SPEAKER_DATA` | PROPOSTO |
| 47 | `BUTTON_DOWN` | VALIDADO EM BANCADA |

<!-- END pinmap:bench -->

## 4. Restrições do ESP32-S3 que moldam o mapa

Confirmadas em fonte do fabricante (documentação do ESP-IDF, guia de hardware e guia da
DevKitC, nas cópias identificadas em `sources/ESP32-S3.md`):

| Restrição | Pinos | Evidência |
|---|---|---|
| Não existem | GPIO 22–25 | S3-GPIO-01, S3-GPIO-02 |
| Flash e PSRAM: não usar | GPIO 26–32; e 33–37 no N16R8 (PSRAM octal) | S3-GPIO-03 a S3-GPIO-06, MOD-04 |
| Pinos de strapping (lidos a cada partida ou reset) | GPIO 0, 3, 45, 46 | S3-STRAP-01, S3-STRAP-02 |
| USB nativo | GPIO 19 (D−), 20 (D+) | S3-USB-01 |
| UART0: log da ROM e download | GPIO 43 (TX), 44 (RX) | S3-UART-01, S3-UART-04, S3-STRAP-16 |
| ADC1 (o ADC2 disputa com o Wi-Fi) | GPIO 1–10 | S3-ADC-01, S3-ADC-04 |
| GPIOs de RTC (acordam do deep sleep; mantêm estado dormindo) | GPIO 0–21 | S3-RTC-01, S3-RTC-05, S3-GPIO-12 |
| JTAG (livres enquanto a depuração fica no USB) | GPIO 39–42 | S3-JTAG-01, S3-JTAG-02 |
| Cristal de 32 kHz (livres sem o cristal) | GPIO 15, 16 | S3-XTAL-01, S3-XTAL-02 |
| Glitch de nível baixo de ~60 µs ao energizar | GPIO 1–17 | S3-GLT-01 |
| LED RGB da DevKitC | GPIO 48 (placa v1.0) ou 38 (v1.1) | DKC-01, DKC-02 |

As verificações estáticas de `pins.h` (`static_assert`) já garantem em tempo de compilação:
nenhum pino repetido, nenhum pino inexistente ou de flash/PSRAM, leitura da bateria no ADC1 e
botão de energia em GPIO de RTC.

## 5. Riscos abertos, por pino

| # | Pino | Uso proposto | Problema | Estado |
|---:|---|---|---|---|
| 1 | GPIO 0 | Botão de energia (até 2026-11-25) | GPIO 0 em nível baixo na saída do reset entra em modo de download (S3-STRAP-05). Se o mesmo botão liga a alimentação e puxa o GPIO 0 ao terra, **o aparelho não liga**: o firmware não roda e a trava nunca é segura. Análise completa em `sources/ESP32-S3.md`, seção 6 | **Resolvido por projeto em 2026-11-25** (alternativa A): o botão foi para o GPIO 2, RTC e fora dos pinos de strapping (S3-RTC-01, S3-STRAP-01); o GPIO 0 fica só como BOOT. Um `static_assert` em `pins.h` impede a volta a um pino de strapping. Na bancada, o botão de energia continua sendo o BOOT (GPIO 0), porque o 2 é o OK |
| 2 | GPIO 43/44 | UART do cartucho | É o UART0: a ROM imprime o log de boot em GPIO 43 a cada partida, e mudar isso é gravação permanente de eFuse (S3-STRAP-16). O fabricante recomenda outro UART para a aplicação (S3-UART-03). Na DevKitC os pinos estão na ponte USB-UART (DKC-07) | Fechado em 2026-11-25: o cartucho saiu do v1 e 43/44 ficaram livres. Quem usar esses pinos recebe o log de boot da ROM |
| 3 | GPIO 48 | Trava de alimentação (HOLD) | Não é GPIO de RTC: não mantém o nível em deep sleep sem configuração extra (S3-RTC-08). No reset é entrada sem pull (S3-GPIO-08): a trava precisa de resistor externo que defina o estado. Na DevKitC v1.0 é o LED RGB (DKC-01) | Proposto; depende do circuito da trava, que não existe |
| 4 | GPIO 38 | CS do cartucho | Na DevKitC v1.1 é o LED RGB (DKC-02). Com o USB-OTG Download Boot habilitado, a ROM leva o pino a nível baixo (S3-USB-10), o que selecionaria o cartucho | Fechado em 2026-11-25: o cartucho saiu do v1 e o 38 ficou livre |
| 5 | GPIO 3 | Desligamento do amplificador | Flutuante no reset (S3-STRAP-14) e com glitch ao energizar (S3-GLT-01): o estado do amplificador durante o boot depende de resistor externo | Proposto; conferir se há estalo no alto-falante ao ligar |
| 6 | GPIO 39–42 | IR RX e I2S | Pinos de JTAG; a ROM aciona 39 e 40 em um dos modos de download (S3-USB-10) | Proposto; conferir na bancada |
| 7 | GPIO 4, 5, 6 | Livres (4, 5) e status do carregador (6) | Eram os analógicos, retirados do v1 em 2026-11-25; o GPIO 2 virou o botão de energia. O 6 recebe o status do carregador da placa de energia (decisão de 2026-11-25): GPIO RTC, acorda o chip quando o carregador é ligado. O 4 e o 5 ficam livres porque o IR da bancada usa os dois | Removido em 2026-11-25: v1 sem analógicos nem gatilhos |
| 8 | GPIO 16 | Backlight por PWM | Na bancada o LED da tela está direto no 3V3; o controle por PWM nunca foi testado, e a corrente do backlight não é conhecida (`PERIPHERALS.md`, MSP-05) | Proposto; pode exigir transistor |
| 9 | GPIO 47, 48 | IR TX e trava | Dividem o domínio de alimentação da flash/PSRAM (S3-GPIO-09, S3-GPIO-11). O estado de fábrica do eFuse que fixa essa tensão no N16R8 não foi verificado (S3-STRAP-11); o registro de bancada indica 3,3 V | Proposto |
| 10 | GPIO 45, 46 | Livres | GPIO 45 em nível alto no reset leva a alimentação da flash a 1,8 V (S3-STRAP-09); GPIO 46 participa do modo de boot | **Manter livres**; nunca ligar a algo que os puxe no reset |

## 6. Ensaios de bancada que fecham o mapa

Em ordem de custo, todos possíveis com a DevKitC:

1. **GPIO 0 no power-up**: segurar o botão BOOT ao energizar e observar no UART0 se aparece a
   mensagem de espera de download. Não decide mais o risco 1 (botão no GPIO 2 desde
   2026-11-25); só confirma na peça o que a fonte diz.
2. **Revisão da DevKitC** (riscos 3 e 4): ler a serigrafia ao lado do LED RGB (DKC-04).
3. Removido em 2026-11-25 (log de boot no UART do cartucho; o v1 não tem mais cartucho).
4. **Deep sleep com despertar pelo botão de energia**: gravar o firmware próprio, usar o
   comando `off` e acordar pelo botão. Na bancada é o BOOT (GPIO 0): conferir se o chip volta ao
   firmware ou ao modo de download (a fonte não esclarece se os straps são reamostrados ao
   acordar: S3-STRAP-21). No target o botão é o GPIO 2, que não é strap.
5. Removido em 2026-11-25 (curso dos analógicos; o v1 não tem mais analógicos).
6. **Estalo do amplificador ao ligar** (risco 5).

## 7. Divergências em relação ao `README.md`

Registradas em `sources/ESP32-S3.md`, seção 9. As que afetam o mapa:

- O README tratava o GPIO 0 só como "strapping; GPIO RTC" e não avisava do modo de download.
  Resolvido em 2026-11-25: o botão de energia foi para o GPIO 2.
- O README dá a UART em 43/44 como "liberada porque o console usa o USB CDC": a ROM
  continua imprimindo o log de boot nesses pinos.
- O README conta 16 botões no MCP23017; a revisão D do datasheet restringe GPA7 e GPB7 a
  saída, o que deixa **14 entradas** (`PERIPHERALS.md`, MCP-03 a MCP-05). Resolvido em
  2026-11-25: sem analógicos nem gatilhos, o v1 tem 10 botões e o README já conta 10.
