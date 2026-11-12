# Mapa de GPIOs do d-BMO v1

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Fonte de verdade | `firmware/config/include/dbmo/config/pins.h` |
| Evidências | `docs/hardware/sources/ESP32-S3.md` (identificadores `S3-…`, `MOD-…`, `DKC-…`) |
| Estado | Pinagem **proposta**. Só a tela e os três botões da bancada foram validados em hardware |

As tabelas entre os marcadores `BEGIN pinmap` são **geradas** a partir de `pins.h`
(`python3 tools/check_pinmap.py --write`) e conferidas na CI: não editar à mão.

## 1. Classificação usada

| Rótulo | Significado |
|---|---|
| Fixo no chip | Função presa ao pino pelo ESP32-S3 (por exemplo, USB em 19/20). Confirmado em fonte do fabricante |
| Validado em bancada | Já funcionou na protoboard, com o registro no `README.md` |
| Proposto | Escolha do projeto, coerente com as restrições do fabricante, **sem** validação física |
| Proposto, com risco aberto | Proposto, e há um problema conhecido que pode obrigar a mudar o pino |

Nenhum pino é "definitivo": a pinagem só fecha depois dos ensaios de bancada da seção 6.

## 2. Perfil `target` (pinagem proposta do v1)

<!-- BEGIN pinmap:target (gerado por tools/check_pinmap.py --write; não editar) -->

| GPIO | Sinal | Classificação |
|---:|---|---|
| 0 | `POWER_BUTTON` | PROPOSTO, COM RISCO ABERTO |
| 1 | `VBAT_SENSE` | PROPOSTO |
| 2 | `STICK_LEFT_X` | PROPOSTO |
| 3 | `AMP_SHUTDOWN` | PROPOSTO |
| 4 | `STICK_LEFT_Y` | PROPOSTO |
| 5 | `STICK_RIGHT_X` | PROPOSTO |
| 6 | `STICK_RIGHT_Y` | PROPOSTO |
| 7 | `I2S_MIC_DATA` | PROPOSTO |
| 8 | `I2C_SDA` | PROPOSTO |
| 9 | `I2C_SCL` | PROPOSTO |
| 10 | `DISPLAY_CS` | VALIDADO EM BANCADA |
| 11 | `SPI_MOSI` | VALIDADO EM BANCADA |
| 12 | `SPI_SCK` | VALIDADO EM BANCADA |
| 13 | `SPI_MISO` | VALIDADO EM BANCADA |
| 14 | `CC1101_CS` | PROPOSTO |
| 15 | `CC1101_GDO0` | PROPOSTO |
| 16 | `DISPLAY_BACKLIGHT` | PROPOSTO |
| 17 | `DISPLAY_DC` | VALIDADO EM BANCADA |
| 18 | `DISPLAY_RESET` | VALIDADO EM BANCADA |
| 19 | `USB_DM` | CONFIRMADO (fabricante) |
| 20 | `USB_DP` | CONFIRMADO (fabricante) |
| 21 | `SD_CS` | PROPOSTO |
| 38 | `CARTRIDGE_CS` | PROPOSTO |
| 39 | `IR_RX` | PROPOSTO |
| 40 | `I2S_BCLK` | PROPOSTO |
| 41 | `I2S_LRCLK` | PROPOSTO |
| 42 | `I2S_SPEAKER_DATA` | PROPOSTO |
| 43 | `CARTRIDGE_UART_TX` | PROPOSTO, COM RISCO ABERTO |
| 44 | `CARTRIDGE_UART_RX` | PROPOSTO, COM RISCO ABERTO |
| 47 | `IR_TX` | PROPOSTO |
| 48 | `POWER_HOLD` | PROPOSTO |

<!-- END pinmap:target -->

## 3. Perfil `bench` (a protoboard de hoje)

<!-- BEGIN pinmap:bench (gerado por tools/check_pinmap.py --write; não editar) -->

| GPIO | Sinal | Classificação |
|---:|---|---|
| 2 | `BUTTON_OK` | VALIDADO EM BANCADA |
| 4 | `IR_RX` | PROPOSTO |
| 5 | `IR_TX` | PROPOSTO |
| 8 | `I2C_SDA` | PROPOSTO |
| 9 | `I2C_SCL` | PROPOSTO |
| 10 | `DISPLAY_CS` | VALIDADO EM BANCADA |
| 11 | `SPI_MOSI` | VALIDADO EM BANCADA |
| 12 | `SPI_SCK` | VALIDADO EM BANCADA |
| 13 | `SPI_MISO` | VALIDADO EM BANCADA |
| 14 | `CC1101_CS` | PROPOSTO |
| 15 | `CC1101_GDO0` | PROPOSTO |
| 16 | `DISPLAY_BACKLIGHT` | PROPOSTO |
| 17 | `DISPLAY_DC` | VALIDADO EM BANCADA |
| 18 | `DISPLAY_RESET` | VALIDADO EM BANCADA |
| 19 | `USB_DM` | CONFIRMADO (fabricante) |
| 20 | `USB_DP` | CONFIRMADO (fabricante) |
| 21 | `SD_CS` | PROPOSTO |
| 39 | `BUTTON_UP` | VALIDADO EM BANCADA |
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
nenhum pino repetido, nenhum pino inexistente ou de flash/PSRAM, analógicos só no ADC1 e
botão de energia em GPIO de RTC.

## 5. Riscos abertos, por pino

| # | Pino | Uso proposto | Problema | Estado |
|---:|---|---|---|---|
| 1 | GPIO 0 | Botão de energia | GPIO 0 em nível baixo na saída do reset entra em modo de download (S3-STRAP-05). Se o mesmo botão liga a alimentação e puxa o GPIO 0 ao terra, **o aparelho não liga**: o firmware não roda e a trava nunca é segura. Análise completa em `sources/ESP32-S3.md`, seção 6 | **Risco aberto.** Alternativas A, B e C descritas na análise; nenhuma decidida |
| 2 | GPIO 43/44 | UART do cartucho | É o UART0: a ROM imprime o log de boot em GPIO 43 a cada partida, e mudar isso é gravação permanente de eFuse (S3-STRAP-16). O fabricante recomenda outro UART para a aplicação (S3-UART-03). Na DevKitC os pinos estão na ponte USB-UART (DKC-07) | **Risco aberto.** O cartucho receberia o log de boot como dados |
| 3 | GPIO 48 | Trava de alimentação (HOLD) | Não é GPIO de RTC: não mantém o nível em deep sleep sem configuração extra (S3-RTC-08). No reset é entrada sem pull (S3-GPIO-08): a trava precisa de resistor externo que defina o estado. Na DevKitC v1.0 é o LED RGB (DKC-01) | Proposto; depende do circuito da trava, que não existe |
| 4 | GPIO 38 | CS do cartucho | Na DevKitC v1.1 é o LED RGB (DKC-02). Com o USB-OTG Download Boot habilitado, a ROM leva o pino a nível baixo (S3-USB-10), o que selecionaria o cartucho | Proposto; conferir na bancada |
| 5 | GPIO 3 | Desligamento do amplificador | Flutuante no reset (S3-STRAP-14) e com glitch ao energizar (S3-GLT-01): o estado do amplificador durante o boot depende de resistor externo | Proposto; conferir se há estalo no alto-falante ao ligar |
| 6 | GPIO 39–42 | IR RX e I2S | Pinos de JTAG; a ROM aciona 39 e 40 em um dos modos de download (S3-USB-10) | Proposto; conferir na bancada |
| 7 | GPIO 2, 4, 5, 6 | Analógicos em 3,3 V | A faixa calibrada do ADC termina em 2900 mV (S3-ADC-09): o fim do curso pode saturar | Proposto; medir o curso elétrico real dos analógicos |
| 8 | GPIO 16 | Backlight por PWM | Na bancada o LED da tela está direto no 3V3; o controle por PWM nunca foi testado, e a corrente do backlight não é conhecida (`PERIPHERALS.md`, MSP-05) | Proposto; pode exigir transistor |
| 9 | GPIO 47, 48 | IR TX e trava | Dividem o domínio de alimentação da flash/PSRAM (S3-GPIO-09, S3-GPIO-11). O estado de fábrica do eFuse que fixa essa tensão no N16R8 não foi verificado (S3-STRAP-11); o registro de bancada indica 3,3 V | Proposto |
| 10 | GPIO 45, 46 | Livres | GPIO 45 em nível alto no reset leva a alimentação da flash a 1,8 V (S3-STRAP-09); GPIO 46 participa do modo de boot | **Manter livres**; nunca ligar a algo que os puxe no reset |

## 6. Ensaios de bancada que fecham o mapa

Em ordem de custo, todos possíveis com a DevKitC:

1. **GPIO 0 no power-up** (risco 1): segurar o botão BOOT ao energizar e observar no UART0
   se aparece a mensagem de espera de download.
2. **Revisão da DevKitC** (riscos 3 e 4): ler a serigrafia ao lado do LED RGB (DKC-04).
3. **Log de boot em GPIO 43** (risco 2): observar o pino com analisador lógico a cada reset.
4. **Deep sleep com despertar pelo GPIO 0**: gravar o firmware próprio, usar o comando `off`
   e acordar pelo botão BOOT; conferir se o chip volta ao firmware ou ao modo de download
   (a fonte não esclarece se os straps são reamostrados ao acordar: S3-STRAP-21).
5. **Curso dos analógicos** (risco 7): medir a tensão do cursor nos extremos.
6. **Estalo do amplificador ao ligar** (risco 5).

## 7. Divergências em relação ao `README.md`

Registradas em `sources/ESP32-S3.md`, seção 9. As que afetam o mapa:

- O README trata o GPIO 0 só como "strapping; GPIO RTC" e não avisa do modo de download.
- O README dá a UART em 43/44 como "liberada porque o console usa o USB CDC": a ROM
  continua imprimindo o log de boot nesses pinos.
- O README conta 16 botões no MCP23017; a revisão D do datasheet restringe GPA7 e GPB7 a
  saída, o que deixa **14 entradas** (`PERIPHERALS.md`, MCP-03 a MCP-05). `pins.h` já usa
  14 e reserva os dois cliques dos analógicos para um segundo expansor ou outra solução.
