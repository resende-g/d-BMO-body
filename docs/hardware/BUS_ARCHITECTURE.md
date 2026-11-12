# Arquitetura de barramentos do d-BMO v1

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Pinos e endereços | `firmware/config/include/dbmo/config/pins.h` ([`GPIO_MAP.md`](GPIO_MAP.md)) |
| Evidências | [`sources/ESP32-S3.md`](sources/ESP32-S3.md) e [`sources/PERIPHERALS.md`](sources/PERIPHERALS.md) |
| Estado | Proposta. Em hardware, só a tela no SPI a 10 MHz foi validada |

## 1. Visão geral

```
                       ┌──────────────── ESP32-S3-WROOM-1-N16R8 ────────────────┐
  SPI2 (compartilhado) │ SCK 12 · MOSI 11 · MISO 13                             │
     ├─ Tela ILI9341        CS 10, DC 17, RST 18, BL 16                         │
     ├─ CC1101              CS 14, GDO0 15                                      │
     ├─ microSD             CS 21                                               │
     └─ Cartucho            CS 38                                               │
  I2C0 (compartilhado) │ SDA 8 · SCL 9 · 100 kHz                                │
     ├─ MCP23017 (botões)   0x20                                                │
     ├─ MCP23017 (auxiliar) 0x21   (opcional, em decisão)                       │
     ├─ PN532               0x24                                                │
     └─ Cartucho            EEPROM 0x50–0x57 + endereços declarados pelo cartucho│
  I2S0 (full-duplex)   │ BCLK 40 · LRCLK 41 · DOUT 42 → MAX98357A · DIN 7 ← INMP441 │
  UART0                │ TX 43 · RX 44 → cartucho (risco aberto)                │
  RMT                  │ IR RX 39 · IR TX 47                                    │
  ADC1                 │ VBAT 1 · analógicos 2, 4, 5, 6                         │
  USB                  │ D− 19 · D+ 20 (console, gravação)                      │
                       └────────────────────────────────────────────────────────┘
```

## 2. SPI compartilhado

| Dispositivo | CS | Clock no firmware | Limite do dispositivo | Evidência do limite |
|---|---:|---|---|---|
| Tela ILI9341 | 10 | 10 MHz (`spi::kDisplayHz`) | Escrita: ciclo mínimo de 100 ns (10 MHz). Leitura: 150 ns (≈6,67 MHz) | ILI-07, ILI-08 |
| CC1101 | 14 | 1 MHz (`spi::kCc1101Hz`) | 10 MHz com atraso entre bytes; 6,5 MHz em rajada sem atraso | CC-06 |
| microSD | 21 | 400 kHz na identificação; 4 MHz depois (padrão da biblioteca) | Identificação até 400 kHz; por SPI no ESP-IDF, até 20 MHz | SD-03, SD-13 |
| Cartucho | 38 | Declarado pelo cartucho (`spiMaxHz`) | Depende do cartucho | Especificação de cartuchos |

Regras de projeto e de firmware:

1. **Todos os CS em nível alto antes de qualquer tráfego.** Confiar só em pull-up não é
   recomendado (SD-08). O firmware faz isso em `parkChipSelects()` antes de iniciar o SPI.
2. **O cartão é o primeiro dispositivo a ser iniciado** (SD-06, SD-07). Um cartão que
   ainda não entrou em modo SPI pode responder a tráfego destinado a outro dispositivo.
   `main.cpp` monta o cartão antes de iniciar a tela.
3. **Uma transação por vez.** Cada dispositivo abre a transação com o seu clock e modo, e
   a biblioteca SPI do Arduino-ESP32 trava o barramento entre abrir e fechar.
4. **Leitura da tela é mais lenta que a escrita.** O firmware só lê o identificador da
   tela (uma vez, no boot); o clock de leitura acima de 6,67 MHz está **fora do
   datasheet** (ILI-08) e, na bancada, a leitura funcionou a 10 MHz (MSP-09). Margem a
   confirmar.
5. **Pinos IO_MUX.** SCK 12, MOSI 11 e MISO 13 são os pinos nativos do SPI2 (S3-SPI-01).
   Até 40 MHz não há diferença entre pinos nativos e matriz de GPIO (S3-SPI-03).

Riscos elétricos em aberto:

| Risco | Evidência | Como fechar |
|---|---|---|
| Pull-ups do cartão (10 kΩ a 50 kΩ) podem ser fortes para os outros dispositivos | SD-09, SD-10 | Medir as bordas com os quatro dispositivos ligados |
| Módulo microSD com conversor de nível que não solta o MISO | Afirmação do `README.md`, sem fonte primária (SD-16) | Com o CS do cartão em alto, conferir se a tela e o rádio ainda respondem |
| Carga capacitiva de quatro dispositivos e fios de protoboard | SD-12 | Baixar o clock para isolar; definir o clock final na PCB |
| A tela solta o SDO com CS em alto? | O datasheet não afirma explicitamente (ILI-10) | Conferir com o CC1101 e o cartão no mesmo MISO |
| SO do CC1101 fica em alta impedância com CS em alto | Confirmado no datasheet (CC-08) | — |
| O alvo do Retro-Go usa a tela a 40 MHz fora do perfil de bancada | O datasheet limita a escrita a 10 MHz (ILI-07); 40 MHz é prática comum com este controlador, sem validação aqui. A 10 MHz um quadro inteiro leva cerca de 120 ms | Validar na placa antes de adotar; o Bruce e o firmware próprio ficam em 10 MHz |

## 3. I2C compartilhado

Barramento único, 100 kHz (`i2c::kBusHz`), em GPIO 8 (SDA) e 9 (SCL).

| Endereço (7 bits) | Dispositivo | Evidência do endereço | Estado |
|---|---|---|---|
| 0x20 | MCP23017 dos botões | Faixa 0x20–0x27 pelos pinos A2..A0 (MCP-11) | Proposto |
| 0x21 | MCP23017 auxiliar | Idem | Opcional; decisão em aberto |
| 0x24 | PN532 | Código da biblioteca da Elechouse (PN-08); não conferido em fonte da NXP (PN-09) | Proposto |
| 0x50–0x57 | EEPROM do cartucho | A 24LC02B responde na faixa inteira (EE-03) | Reservado: ninguém mais pode usar |
| 0x08–0x77, exceto os acima | Periféricos do cartucho | Declarados no cabeçalho do cartucho | O firmware recusa conflito |

Regras:

- **100 kHz** serve a todos: o MCP23017 aceita 100 kHz de 1,8 V a 5,5 V (MCP-10) e as
  EEPROMs aceitam 400 kHz a partir de 2,5/2,7 V (EE-01, EE-06). A velocidade máxima do
  PN532 em I2C não foi verificada (PN-13).
- **Pull-ups**: um único par no barramento. O valor de 4,7 kΩ do `README.md` é proposta;
  conferir se os módulos (PN532, MCP23017 em placa) já trazem pull-ups, para não somar.
- **Pinos de endereço e de reset do MCP23017 precisam de nível definido** (MCP-12).
- **GPA7 e GPB7 do MCP23017 são só saída** (MCP-03 a MCP-05): sobram 14 entradas.
- **O cartucho é o ponto fraco do barramento**: um cartucho com defeito que prenda SDA ou
  SCL derruba botões e NFC. Mitigações propostas (sem decisão): resistores em série,
  isolador ou multiplexador no ramo do cartucho (`docs/cartridges/CARTRIDGE_SPECIFICATION.md`,
  seção 7).

Tratamento no firmware: cada subsistema conta erros seguidos e passa a "em falha" sem
derrubar os demais; a aplicação mantém um registro de endereços ocupados e recusa um
cartucho que declare um endereço já em uso.

## 4. I2S (áudio)

| Sinal | GPIO | Dispositivo |
|---|---:|---|
| BCLK | 40 | MAX98357A e INMP441 |
| LRCLK (WS) | 41 | MAX98357A e INMP441 |
| DOUT | 42 | MAX98357A (DIN) |
| DIN | 7 | INMP441 (SD) |
| Desligamento do amplificador | 3 | MAX98357A (SD_MODE) |

- Uma única porta I2S em full-duplex: transmissão e recepção dividem BCLK e LRCLK
  (S3-I2S-02). O ESP32-S3 é o mestre dos clocks.
- **Quadro de 64 ciclos de BCLK** (duas palavras de 32 bits): exigido pelo microfone
  (MIC-04) e aceito pelo amplificador (AMP-09).
- **Taxas aceitas pelos dois**: 8, 16, 32, 44,1 e 48 kHz (AMP-08, MIC-05). O firmware
  recusa as demais.
- Sem MCLK: o amplificador não precisa (AMP-10).
- Cuidados: não remover o LRCLK com o BCLK presente (AMP-13); não dar clocks ao microfone
  sem alimentação (MIC-10); resistor de pull-down de 100 kΩ na linha de dados do
  microfone, recomendado pelo fabricante (MIC-07).

## 5. UART

| Uso | Pinos | Estado |
|---|---|---|
| Console e gravação | USB nativo (GPIO 19/20), USB-Serial-JTAG | Em uso na bancada |
| Cartucho | UART0 em GPIO 43/44 | **Risco aberto**: a ROM imprime o log de boot em GPIO 43 a cada partida (S3-STRAP-16); o fabricante recomenda outro UART para a aplicação (S3-UART-03) |
| Plano B do PN532 (HSU) | Disputaria os mesmos pinos | Só se o I2C do PN532 falhar |

O ESP32-S3 tem três UARTs e os sinais das outras duas podem ir a qualquer GPIO
(S3-UART-02); o que falta são pinos livres. Remanejar a UART do cartucho é uma das
decisões pendentes de pinagem. O firmware ainda **não liga** a UART do cartucho.

## 6. RMT (infravermelho)

| Sinal | GPIO (alvo) | GPIO (bancada) |
|---|---:|---:|
| IR RX (receptor demodulado, ativo em nível baixo) | 39 | 4 |
| IR TX (LED, portadora gerada pelo RMT) | 47 | 5 |

O RMT foi projetado como transceptor de infravermelho e modula a portadora (S3-RMT-01).
O firmware usa 1 µs por tique. A peça real do receptor e a necessidade de transistor no
LED não foram verificadas (IR-08 a IR-10).

## 7. ADC

| Sinal | GPIO | Canal | Observação |
|---|---:|---|---|
| Tensão da bateria | 1 | ADC1_CH0 | Divisor 100k/100k proposto; 0,1 µF no pino (S3-ADC-10) |
| Analógico esquerdo X / Y | 2 / 4 | ADC1_CH1 / CH3 | Faixa calibrada até 2900 mV (S3-ADC-09) |
| Analógico direito X / Y | 5 / 6 | ADC1_CH4 / CH5 | Idem |

Só o ADC1 é usado: o ADC2 é compartilhado com o Wi-Fi e a leitura pode falhar
(S3-ADC-04, S3-ADC-05).

## 8. Conflitos conhecidos entre barramentos e pinos

| Conflito | Situação |
|---|---|
| Bancada × alvo: GPIO 2, 4, 5 e 39/47 | Na protoboard são botão e IR; no alvo, analógicos e IR em outros pinos. Os dois perfis de compilação (`dbmo-bench`, `dbmo-target`) resolvem isso; não dá para montar os dois ao mesmo tempo |
| UART do cartucho × log da ROM × ponte USB-UART da DevKitC | Risco aberto (seção 5) |
| PN532 em HSU × UART do cartucho | Só no plano B |
| LED RGB da DevKitC × GPIO 48 (trava) ou 38 (CS do cartucho) | Depende da revisão da placa (DKC-01, DKC-02) |
