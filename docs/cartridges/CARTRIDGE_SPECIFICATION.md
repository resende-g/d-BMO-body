# Especificação do sistema de cartuchos do d-BMO v1

| Campo | Valor |
|---|---|
| Versão do documento | 0.1 (2026-11-12) |
| Formato do cabeçalho descrito | 1 |
| Estado | Protocolo lógico **implementado e testado no computador**; interface elétrica e mecânica **propostas**, sem validação física |
| Código | `firmware/cartridges/` |
| Testes | `tests/unit/cartridges/` |

Classificação usada neste documento:

- **DECIDIDO**: consta do `README.md` do projeto como decisão.
- **IMPLEMENTADO**: existe no firmware e tem teste automatizado no computador.
- **PROPOSTO**: sugestão deste documento; pode mudar sem quebrar nada já fabricado.
- **PENDENTE DE VALIDAÇÃO FÍSICA**: depende de peça em mãos, medição ou ensaio.

Nenhuma pinagem, tensão ou dimensão deste documento está fechada. O soquete ainda não foi
comprado (`README.md`, "Compras em fases", fase 4).

## 1. Escopo e princípios

O cartucho acrescenta um periférico ao d-BMO pelo conector do topo: outro rádio, um GPS,
um sensor. Ele **não** acrescenta processamento e **não** traz código para o aparelho
executar (DECIDIDO, `README.md`, "Cartuchos de expansão").

Princípios do protocolo:

1. **O cartucho só declara.** Uma EEPROM pequena diz quem ele é e o que usa do conector.
   O firmware decide se aceita.
2. **Conteúdo não confiável.** Tudo o que vem da EEPROM é validado (assinatura, CRC,
   faixas) antes de qualquer uso. Um cartucho malformado é recusado, não derruba o sistema.
3. **Núcleo fechado para alteração.** Um cartucho novo entra como um driver registrado; o
   núcleo e o gerenciador de cartuchos não mudam (seção 9).
4. **Falha contida.** Um driver que falha é parado e o conector marcado como em falha; os
   demais subsistemas seguem funcionando.
5. **Local.** Nada do cartucho sai do aparelho; não há rede envolvida.

## 2. Conector

| Item | Valor | Classificação |
|---|---|---|
| Tipo | Card edge, 2 × 10 contatos, passo de 2,54 mm (soquete DK920A-20M) | DECIDIDO (`README.md`, "Carcaça 3D") |
| Posição | Borda superior do aparelho, em placa riser | DECIDIDO |
| Placa do cartucho | 1,6 mm, 52 × 52 mm, acabamento ENIG, chanfro na língua | DECIDIDO, com pontos em aberto registrados no `README.md` |
| Dimensões do soquete, profundidade da fenda, aceitação de placa de 1,6 mm | — | PENDENTE DE VALIDAÇÃO FÍSICA (medir a peça) |
| Anti-inversão mecânica (língua deslocada) | — | PROPOSTO no `README.md`; PENDENTE |

Dimensões e detalhes mecânicos ficam em `docs/mechanical/`.

### 2.1 Sinais disponíveis

DECIDIDO no `README.md`: o conector expõe I2C, SPI com chip-select dedicado, UART, 3,3 V,
GND e dois GPIOs. A linha de 5 V é **decisão em aberto** ("manter ou não a linha de 5 V no
conector do cartucho").

| Grupo | Sinais | Origem no aparelho (`pins.h`, perfil target) | Observação |
|---|---|---|---|
| Alimentação | 3V3, GND | Trilho de 3,3 V | Proteção por limitador de corrente ou polyfuse: DECIDIDO que existe, valor em aberto |
| I2C | SDA, SCL | GPIO 8 / 9, barramento compartilhado | Mesmo barramento do MCP23017 e do PN532 |
| SPI | SCK, MOSI, MISO, CS | GPIO 12 / 11 / 13 e CS no GPIO 38 | Barramento compartilhado com tela, CC1101 e cartão; CS exclusivo do cartucho |
| UART | TX, RX | GPIO 43 / 44 (UART0) | Risco aberto: a ROM do ESP32-S3 imprime o log de boot em U0TXD (`docs/hardware/sources/ESP32-S3.md`) |
| GPIO | GPIO0, GPIO1 | Segundo MCP23017 (0x21), opcional | Sem pino direto do ESP32-S3 disponível |
| 5 V | — | — | Indefinido; o firmware trata como **não oferecido** |

### 2.2 Atribuição de contatos (PROPOSTA, não definitiva)

Numeração: lado A = face frontal do cartucho, lado B = face traseira; posição 1 à esquerda
de quem olha a face A. A numeração física real depende da peça e do desenho da riser
(PENDENTE DE VALIDAÇÃO FÍSICA).

| Posição | Lado A | Lado B |
|---:|---|---|
| 1 | GND | GND |
| 2 | 3V3 | 5V (reservado; indefinido) |
| 3 | I2C_SDA | SPI_SCK |
| 4 | I2C_SCL | SPI_MOSI |
| 5 | UART_TX (aparelho → cartucho) | SPI_MISO |
| 6 | UART_RX (cartucho → aparelho) | SPI_CS |
| 7 | GPIO0 | reservado (não ligar) |
| 8 | GPIO1 | reservado (não ligar) |
| 9 | 5V (reservado; indefinido) | 3V3 |
| 10 | GND | GND |

Dezoito contatos usados (dez sinais, quatro de GND, dois de 3V3, dois reservados para 5 V)
e dois livres.

**Raciocínio.** O soquete é simétrico (`README.md`: "o soquete é simétrico"). Supondo que
um cartucho virado ao contrário encoste o contato A*n* do cartucho no contato B(11 − *n*)
do aparelho (a face troca e a posição espelha; hipótese geométrica a conferir na peça), a
tabela foi montada para que a inversão não ligue um trilho a outro:

| Par trocado na inversão | Consequência |
|---|---|
| A1 ↔ B10, A10 ↔ B1 (GND ↔ GND) | Nenhuma |
| A2 ↔ B9 (3V3 ↔ 3V3) | Nenhuma: o cartucho continua alimentado em 3,3 V |
| A9 ↔ B2 (5 V ↔ 5 V) | Nenhuma; se a linha de 5 V não existir, os dois ficam sem ligação |
| A3 ↔ B8, A4 ↔ B7 (I2C ↔ reservado) | O barramento I2C do aparelho não é tocado. A EEPROM não responde e o conector aparece como vazio: falha segura |
| A5 ↔ B6 (UART_TX ↔ SPI_CS) | Saída do aparelho em entrada do cartucho: sem disputa de saídas |
| A6 ↔ B5 (UART_RX ↔ SPI_MISO) | Entrada do aparelho em saída do cartucho: sem disputa |
| A7 ↔ B4, A8 ↔ B3 (GPIO ↔ SPI_MOSI/SCK) | Pode haver disputa se o cartucho aciona o GPIO como saída. Mitigação proposta: resistor em série em cada sinal do lado do aparelho |

Pendências desta tabela: confirmar a hipótese de espelhamento na peça real; definir o
valor do resistor em série e o limitador da linha de 3V3; decidir a linha de 5 V. A
anti-inversão mecânica do `README.md` continua sendo a primeira barreira; esta disposição
é a segunda.

## 3. Identificação por EEPROM

### 3.1 Avaliação da 24C02

O `README.md` sugere uma EEPROM I2C pequena, "ex. 24C02", em cada cartucho. Avaliação com
base em `docs/hardware/sources/PERIPHERALS.md` (identificadores EE-01 a EE-12):

| Critério | 24C02 | Fonte |
|---|---|---|
| Capacidade | 256 bytes; o cabeçalho usa 64 | — |
| Alimentação | 2,5 V a 5,5 V (Microchip 24LC02B); versões Atmel de 1,8 V e 2,7 V | EE-01, EE-06 |
| Velocidade | 400 kHz a partir de 2,5/2,7 V; o barramento do d-BMO roda a 100 kHz | EE-01, EE-06 |
| Endereçamento interno | Um byte: protocolo simples | — |
| Tamanho de página | **Varia:** 8 bytes (Microchip, Atmel), 16 bytes (ST, segundo código da ST) | EE-02, EE-07, EE-11 |
| Tempo de gravação | 5 ms no máximo (Microchip, Atmel) | EE-02, EE-07 |
| Proteção de escrita | Pino WP: em VCC, a escrita é inibida | EE-04, EE-09 |
| Endereço I2C | **Varia:** a Microchip 24LC02B ignora A0–A2 e responde em toda a faixa 0x50–0x57; a Atmel AT24C02 responde só no endereço dado por A2..A0 | EE-03, EE-08 |

Consequências adotadas (IMPLEMENTADO):

- O driver grava com página de **8 bytes**, que serve para os três fabricantes, e confere
  cada gravação lendo de volta.
- O aparelho consulta o endereço **0x50**. Num cartucho com AT24C02, os pinos A2..A0 vão
  ao GND.
- A faixa **0x50–0x57 inteira é reservada** à identificação: nenhum outro dispositivo do
  cartucho ou do aparelho pode usá-la, porque uma 24LC02B a ocupa por inteiro.
- No cartucho pronto, o pino **WP vai a VCC** (com jumper ou ponte de solda para
  programar), para que o cabeçalho não possa ser alterado em uso (PROPOSTO).

Pontos não verificados: datasheet do AT24C02C atual e do ST M24C02 (EE-10, EE-12).

### 3.2 Alternativas consideradas

| Alternativa | Vantagem | Por que não é a proposta |
|---|---|---|
| Resistor de identificação lido por ADC | Uma peça, sem protocolo | Não há canal de ADC1 livre; identifica poucos valores; não carrega versão nem capacidades |
| EEPROM maior (24C32 ou acima) | Espaço para dados do cartucho | Endereçamento interno de dois bytes: outro protocolo; 256 bytes bastam para identificar |
| Sem identificação | Cartucho mais simples | O aparelho não saberia o que foi inserido nem poderia recusar um cartucho incompatível |
| Identificação pelo próprio periférico (ler o ID do chip do cartucho) | Nenhuma peça a mais | Cada periférico tem o seu método; não informa consumo, endereços nem requisitos antes de ligar os barramentos |

## 4. Formato do cabeçalho (formato 1)

Fica nos primeiros 64 bytes da EEPROM. Inteiros em **little-endian**. O restante da EEPROM
(bytes 64 a 255) é livre para o cartucho e não é interpretado pelo núcleo.

| Deslocamento | Tamanho | Campo | Regra |
|---:|---:|---|---|
| 0 | 4 | Assinatura | `D` `B` `M` `C` (0x44 0x42 0x4D 0x43) |
| 4 | 1 | Versão do formato | 1 |
| 5 | 1 | Tamanho do cabeçalho | 64 |
| 6 | 2 | Identificador do fornecedor | Seção 4.2 |
| 8 | 2 | Identificador do produto | Atribuído pelo fornecedor |
| 10 | 1 | Revisão de hardware do cartucho | Livre |
| 11 | 3 | Versão do cartucho | maior, menor, correção |
| 14 | 3 | Firmware mínimo do aparelho | maior, menor, correção |
| 17 | 1 | Reservado | 0 |
| 18 | 2 | Capacidades | Bits da seção 4.1 |
| 20 | 2 | Corrente máxima em 3,3 V | mA |
| 22 | 1 | Quantidade de endereços I2C | 0 a 4 |
| 23 | 4 | Endereços I2C ocupados | 7 bits, de 0x08 a 0x77; posições não usadas em 0 |
| 27 | 1 | Modo SPI | 0 a 3 |
| 28 | 4 | Clock máximo de SPI | Hz; obrigatório se usa SPI |
| 32 | 4 | Taxa da UART | baud; obrigatória se usa UART |
| 36 | 16 | Nome | ASCII imprimível (0x20–0x7E), 1 a 16 caracteres, completado com zeros |
| 52 | 10 | Reservado | 0 |
| 62 | 2 | CRC | CRC-16/CCITT-FALSE (polinômio 0x1021, inicial 0xFFFF) dos bytes 0 a 61 |

O teste `test_byte_layout_is_the_documented_one` confere esta tabela byte a byte contra o
codificador.

### 4.1 Capacidades

| Bit | Nome | Significado |
|---:|---|---|
| 0 | I2C | Usa o barramento I2C (e declara os endereços que ocupa) |
| 1 | SPI | Usa o barramento SPI com o chip-select do cartucho |
| 2 | UART | Usa a UART do conector |
| 3 | GPIO0 | Usa a linha GPIO0 |
| 4 | GPIO1 | Usa a linha GPIO1 |
| 5 | 5V | Precisa da linha de 5 V |
| 6–15 | — | Reservados; um cartucho que marque um deles é recusado por um firmware que não o conheça |

### 4.2 Identificadores de fornecedor (PROPOSTO)

Não existe registro externo. Convenção interna:

| Faixa | Uso |
|---|---|
| 0x0000 | Inválido (EEPROM zerada) |
| 0x0001–0xFFFE | A atribuir pelo mantenedor do projeto, conforme os cartuchos forem criados |
| 0xFFFF | Experimentos e exemplos; nunca em cartucho "de produção" |

O driver de exemplo usa 0xFFFF:0x0001.

### 4.3 Resultados da validação

| Resultado | Quando | Efeito |
|---|---|---|
| `Valid` | Tudo confere | Segue para a compatibilidade |
| `Blank` | Os 64 bytes são 0xFF (EEPROM nunca gravada) | Conector em `Unknown`; não é erro |
| `BadMagic` | Assinatura diferente, ou menos de 64 bytes | `Unknown` |
| `UnsupportedFormat` | Versão de formato ou tamanho que este firmware não conhece | `Unknown` |
| `BadCrc` | CRC não confere | `Unknown` |
| `BadField` | CRC confere, mas um campo está fora da faixa | `Unknown` |

A posição da assinatura, da versão, do tamanho e do CRC é fixa para qualquer formato
futuro de 64 bytes, de modo que um firmware antigo distingue "formato novo" de "dado
corrompido".

## 5. Versionamento e compatibilidade

Três números independentes:

| Número | Onde | Para quê |
|---|---|---|
| Versão do formato | Byte 4 | Muda quando a disposição dos campos muda. Um firmware só aceita os formatos que conhece |
| Firmware mínimo | Bytes 14–16 | O cartucho declara a menor versão do firmware do d-BMO que sabe usá-lo |
| Versão do cartucho | Bytes 11–13 | Versão do projeto do próprio cartucho; informativa para o driver |

A verificação de compatibilidade é uma função pura (`checkCompatibility`), nesta ordem:

| Ordem | Verificação | Resultado se falhar |
|---:|---|---|
| 1 | Firmware do aparelho ≥ firmware mínimo do cartucho | `FirmwareTooOld` |
| 2 | Toda capacidade pedida é oferecida pelo aparelho (a linha de 5 V **não** é oferecida) | `CapabilityUnavailable` |
| 3 | Nenhum endereço I2C do cartucho coincide com os do aparelho: 0x20, 0x21, 0x24 e 0x50–0x57 | `I2cAddressConflict` |
| 4 | Corrente declarada ≤ orçamento do conector (`cartridge.max_ma`) | `OverCurrentBudget` |

`cartridge.max_ma` tem padrão de **100 mA**. Esse número é PROVISÓRIO: o circuito de
proteção do conector e o regulador de 3,3 V ainda não foram definidos
(`README.md`, "Riscos da bateria").

## 6. Ciclo de vida

```
            EEPROM responde                cabeçalho válido            compatível
  Empty ───────────────────► (leitura) ─────────────────► (análise) ────────────► driver?
    ▲                            │                            │                    │
    │                            │ inválido                   │ incompatível       ├─ não há ──► Identified
    │                            ▼                            ▼                    │
    │                         Unknown                      Rejected                └─ há: start()
    │                                                                                   │
    │   EEPROM deixa de responder (2 consultas seguidas), em qualquer estado            ├─ Ok ───► Active
    └───────────────────────────────────────────────────────────────────────────────    └─ erro ─► Faulted
                                                                          Active ── tick() devolve erro ──► Faulted
```

| Estado | Significado |
|---|---|
| `Empty` | Nada responde em 0x50. Estado normal no boot |
| `Unknown` | Há uma EEPROM, mas sem cabeçalho válido |
| `Rejected` | Cabeçalho válido, cartucho incompatível (motivo em `lastCompatibility()`) |
| `Identified` | Válido e compatível, sem driver registrado: não é erro |
| `Active` | Driver em execução |
| `Faulted` | O driver falhou ao iniciar ou em operação e foi parado |

Regras (IMPLEMENTADO):

- **Detecção** por consulta periódica do endereço da EEPROM (a cada 500 ms). Não há linha
  de detecção no conector.
- **Leitura com falha** durante a inserção (mau contato) é repetida até três vezes antes
  de o conector ir para `Unknown`.
- **Remoção** exige duas consultas seguidas sem resposta. O driver é parado e tudo o que
  ele reservou no registro de recursos é liberado.
- **Troca rápida** (um cartucho sai e outro entra entre duas consultas) é percebida porque
  a identidade gravada muda: o anterior é removido e o novo, identificado.
- **Falha do driver** não é tentada de novo sozinha: o cartucho fica em `Faulted` até ser
  retirado.

### Eventos (domínio `Cartridge`)

| Evento | `a` | `b` |
|---|---|---|
| `kStateChanged` | (fornecedor << 16) \| produto; 0 se desconhecido | estado novo |
| `kRejected` | idem | motivo (`Compatibility`) |
| `kFault` | idem | `Status` da falha |

## 7. Riscos de inserção a quente

O conector de hoje não tem linha de detecção nem alimentação chaveada para o cartucho.
Enquanto isso valer:

- Inserir ou retirar um cartucho com o aparelho ligado pode perturbar o barramento I2C
  compartilhado (botões e NFC) e o SPI compartilhado (tela, rádio, cartão).
- Um cartucho com defeito que prenda SDA ou SCL em nível baixo para todo o barramento.

**Procedimento recomendado até existir hardware de proteção: inserir e retirar o cartucho
com o aparelho desligado.** A detecção por consulta existe para o firmware reagir bem, não
para autorizar a troca a quente.

Melhorias de hardware a avaliar (PROPOSTO, sem decisão): linha de presença, chave de
alimentação do cartucho comandada pelo firmware só depois da identificação, isolador ou
multiplexador de I2C no ramo do cartucho, resistores em série nos sinais.

## 8. Segurança

- O cabeçalho é dado, nunca código. Não há carregamento de firmware, script ou plugin a
  partir do cartucho.
- Todos os campos são limitados: nome só com ASCII imprimível, endereços em faixa, no
  máximo quatro endereços, modo SPI de 0 a 3.
- Um driver só recebe os barramentos que o cabeçalho declarou. Um cartucho que declara só
  GPIO não recebe o I2C.
- O cartucho não altera a política de rádio do aparelho: um cartucho de rádio continua
  sujeito à regra de transmissão desabilitada por padrão
  (`docs/hardware/RF_REGULATORY_NOTES.md`).
- A EEPROM de identificação deve ficar protegida contra escrita no cartucho pronto (3.1).

Limite conhecido: o cabeçalho não é assinado. Qualquer pessoa pode gravar uma EEPROM com
o identificador de outro cartucho. Para um aparelho pessoal isso foi considerado
aceitável; se cartuchos de terceiros forem um risco, a assinatura do cabeçalho passa a ser
requisito de um formato futuro.

## 9. Como criar um cartucho novo

1. **Hardware:** PCB-cartucho com o módulo da função e uma 24C02 em 0x50
   (`README.md`, "Anatomia de um cartucho").
2. **Identidade:** escolher fornecedor e produto e gerar a imagem da EEPROM:

   ```bash
   build/host/firmware/cartridges/dbmo_cartridge_image make lora.bin \
       vendor=0xFFFF product=2 name="LORA 915" caps=spi,gpio0 ma=60 spi_mode=0 spi_hz=8000000
   build/host/firmware/cartridges/dbmo_cartridge_image check lora.bin
   ```

   Gravar os 256 bytes na EEPROM com um gravador de 24C02 e proteger contra escrita.
3. **Driver:** uma classe que implementa `ICartridgeDriver` (`matches`, `start`, `tick`,
   `stop`). O modelo é `firmware/cartridges/examples/example_sensor_cartridge.h`.
4. **Registro:** `registry.add(driver)` na inicialização da aplicação.

Nenhum arquivo do núcleo (`firmware/core`) nem do gerenciador (`CartridgeSubsystem`) é
alterado. Sem o passo 3, o cartucho ainda é reconhecido e aparece como `Identified`.

## 10. Cartuchos simulados e testes

Sem hardware, o sistema é exercitado por:

- um simulador da 24C02 no nível do protocolo (`tests/unit/cartridges/sim/`): ponteiro de
  endereço, escrita que volta ao início da página, ciclo de gravação sem resposta,
  proteção de escrita;
- o cartucho de exemplo (sensor I2C fictício) com o seu driver;
- cartuchos de teste para cada caminho: EEPROM em branco, CRC corrompido, firmware antigo
  demais, linha de 5 V exigida, conflito de endereço, corrente acima do orçamento, sem
  driver, driver que falha ao iniciar e driver que falha em operação.

## 11. Pendências e decisões em aberto

| # | Item | Do que depende |
|---:|---|---|
| 1 | Atribuição definitiva dos contatos | Soquete em mãos; confirmação da geometria de inversão; desenho da riser |
| 2 | Linha de 5 V no conector | Decisão de alimentação (`README.md`, "Riscos da bateria", item 6) |
| 3 | Proteção da linha de 3V3 e orçamento de corrente real | Escolha do regulador e do limitador |
| 4 | Resistores em série e isolamento do I2C do cartucho | Ensaio de bancada |
| 5 | Origem dos GPIO0/GPIO1 | Uso ou não do segundo MCP23017 |
| 6 | UART do cartucho em GPIO 43/44 | Avaliar o log de boot da ROM em U0TXD e a ponte USB-UART da DevKitC |
| 7 | Linha de presença e alimentação chaveada | Decisão de hardware da PCB |
| 8 | Identificadores de fornecedor | Convenção a confirmar pelo mantenedor |
| 9 | Comportamento com EEPROM real (24LC02B, AT24C02, M24C02) | Peça em mãos |
| 10 | Assinatura do cabeçalho | Só se cartuchos de terceiros entrarem no escopo |

## Fontes

- `README.md`: seções "Carcaça 3D", "Cartuchos de expansão", "Anatomia de um cartucho",
  "Especificação do cartucho (card edge)", "Riscos da bateria".
- `docs/hardware/sources/PERIPHERALS.md`: EE-01 a EE-12 (EEPROM 24C02).
- `docs/hardware/sources/ESP32-S3.md`: UART0 e log de boot da ROM.
- `firmware/config/include/dbmo/config/pins.h`: pinos e endereços.
- Código e testes em `firmware/cartridges/` e `tests/unit/cartridges/`.
