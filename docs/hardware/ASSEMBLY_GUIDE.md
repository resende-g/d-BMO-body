# Guia de montagem e validação em bancada

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-12) |
| Escopo | Protoboard com a ESP32-S3-DevKitC-1 N16R8. A PCB própria e a carcaça ficam para depois dos ensaios deste guia |
| Estado | Roteiro **ainda não executado** com o firmware próprio. O que já foi validado, com outro firmware, está marcado |

Este guia não substitui a seção "Montagem passo a passo" do `README.md`: ele acrescenta, a
cada etapa, o que conferir antes de energizar, qual teste rodar e o que registrar. As
ligações usam os pinos de [`GPIO_MAP.md`](GPIO_MAP.md); os motivos estão em
[`BUS_ARCHITECTURE.md`](BUS_ARCHITECTURE.md).

## Regras gerais

1. **Um periférico novo por vez.** Ligar, testar, registrar; só então o próximo. Com dois
   módulos novos ao mesmo tempo não dá para saber qual derrubou o barramento.
2. **Desenergizar para mexer em fio.** USB desconectado ao ligar ou retirar qualquer módulo.
3. **Só 3,3 V nos sinais.** O CC1101 não tolera mais que 3,9 V em nenhum pino (CC-02) e o
   INMP441, 3,63 V (MIC-02). Nunca ligar esses módulos ao pino 5V.
4. **Uma fonte por vez na DevKitC.** USB, pino 5V e pino 3V3 são mutuamente exclusivos
   (DKC-13): não alimentar pelo 3V3 com o USB conectado.
5. **Não deixar GPIO 45 e 46 ligados a nada**: são pinos de strapping (S3-STRAP-09,
   S3-STRAP-05).
6. **Nada de transmissão por rádio nesta fase.** O firmware próprio não transmite sem
   habilitação explícita, e este guia não pede para habilitar. Antes de qualquer
   transmissão, ler [`RF_REGULATORY_NOTES.md`](RF_REGULATORY_NOTES.md).
7. **Registrar o resultado** na seção "Evidência de execução" de
   [`../testing/TESTING.md`](../testing/TESTING.md): data, o que foi ligado, o que apareceu.

## Como gravar e observar

```bash
pio run -e dbmo-bench -t upload      # firmware próprio, perfil da protoboard
pio device monitor                   # console pela USB nativa, 115200
pio test -e dbmo-bench -f test_display   # um teste de bancada por vez
```

No console, `status` mostra cada subsistema (Ready, Absent, Failed) e `help` lista os
comandos. Um periférico que não está ligado aparece como `Absent`: isso é o esperado, não
um defeito.

## Etapa 0: antes de ligar qualquer coisa

| Conferir | Como | Por quê |
|---|---|---|
| Revisão da DevKitC | Serigrafia ao lado do LED RGB: "RGB@IO38" indica v1.1; sem ela, v1.0 (LED no GPIO 48) | O LED disputa o GPIO 38 ou o 48 (DKC-01 a DKC-04) |
| GPIO 0 no power-up | Segurar BOOT, conectar o USB, observar o UART0 (porta "UART" da placa) | Fecha o risco 1 de [`GPIO_MAP.md`](GPIO_MAP.md): se aparecer a espera de download, o botão de energia não pode ficar no GPIO 0 como proposto |
| Log de boot no GPIO 43 | Analisador lógico ou a própria porta "UART" a cada reset | Fecha o risco 2 (UART do cartucho) |

## Etapa 1: tela (já validada com outro firmware)

Ligações (iguais nos dois perfis): SCK 12, MOSI 11, MISO 13, CS 10, DC 17, RST 18; VCC e
LED em 3,3 V.

| Passo | Esperado |
|---|---|
| `pio test -e dbmo-bench -f test_display` | Identificador `0x9341`; a tela pisca vermelho e volta ao preto (conferir a cor a olho) |
| Gravar o firmware próprio | O rosto IDLE aparece; `status` mostra `display` e `rosto` em `Ready` e o sistema em `Degraded` (faltam os outros periféricos) |
| `face 4`, `face happy` | A expressão muda |

Já validado no projeto: ID `0x9341` a 10 MHz com o sketch `firmware/teste_tela` (MSP-09).
Não validado: o firmware próprio na placa.

## Etapa 2: botões da protoboard (já validados com outro firmware)

Perfil `dbmo-bench`: botões entre o pino e o GND em GPIO 39 (cima), 2 (OK) e 47 (baixo).

| Passo | Esperado |
|---|---|
| `status` | `input` em `Ready` |
| Apertar cada botão | Sem efeito visível ainda (não há menu); conferir com o log em nível de depuração, quando houver |

## Etapa 3: cartão SD no slot da tela

Ligar o CS do slot ao GPIO 21; os demais sinais são os do SPI da tela. Cartão em FAT32.

| Conferir antes | Por quê |
|---|---|
| Pinos do slot no módulo da tela e se há resistores em série | Não verificados (MSP-06) |

| Passo | Esperado |
|---|---|
| Reiniciar com o cartão | `status`: `storage` em `Ready`; no cartão surgem `/dbmo/config`, `/dbmo/logs`, `/dbmo/cartridges`, `/dbmo/ir` |
| `config audio.volume 30`, esperar 3 s, `off`, tirar o cartão | `/dbmo/config/config.txt` contém `audio.volume=30`; `/dbmo/logs/log.txt` tem o log |
| **A tela continua funcionando com o cartão montado** | Se a tela travar ou mostrar lixo, o cartão não está soltando o barramento (SD-11, SD-16) |
| Reiniciar sem o cartão | `storage` em `Absent`; o resto funciona |

## Etapa 4: I2C (varredura) e MCP23017

SDA 8, SCL 9, pull-ups de 4,7 kΩ ao 3,3 V (um par só no barramento). MCP23017 com A0, A1
e A2 ao GND (endereço 0x20) e RESET ao 3,3 V (MCP-12).

| Passo | Esperado |
|---|---|
| `pio test -e dbmo-bench -f test_i2c_scan` | Lista os endereços que respondem; sem nada ligado, nenhum |
| Com o MCP23017 | Aparece `0x20` |
| Perfil `dbmo-target` gravado, botão entre GPA0 e GND | `input` em `Ready` |

Lembrete: **GPA7 e GPB7 não servem para botão** (só saída, MCP-03).

## Etapa 5: PN532

Chaves do módulo em I2C. VCC em 3,3 V, SDA e SCL no barramento.

| Passo | Esperado |
|---|---|
| Varredura | Aparece `0x24` |
| `status` | `nfc` em `Ready` |
| `nfc on`, aproximar uma etiqueta | Rosto SCANNING, depois SUCCESS; `nfc` mostra o identificador |
| `nfc off` | Campo desligado; rosto volta ao IDLE |

Se o PN532 não responder em I2C: o `README.md` prevê o modo HSU como plano B, que disputa
os pinos da UART do cartucho. Registrar o sintoma antes de mudar qualquer coisa.

## Etapa 6: CC1101 (só recepção)

SCK, MOSI e MISO do barramento; CS 14; GDO0 15; VCC em 3,3 V; antena conectada.

| Passo | Esperado |
|---|---|
| `status` | `radio` em `Ready` (o firmware leu PARTNUM e VERSION) |
| `radio` | Mostra a frequência configurada e os contadores |
| `radio rx on` | Rosto SCANNING |
| **A tela e o cartão continuam funcionando** | Confirma o SPI com três dispositivos |

Não habilitar transmissão nesta etapa (regra 6).

## Etapa 7: infravermelho

Receptor: saída no GPIO 4 (perfil de bancada), VCC e GND conforme a peça. **Conferir a
pinagem do receptor antes de ligar**: ela muda entre séries (IR-05) e a peça do conjunto
comprado não foi identificada (IR-08). LED emissor no GPIO 5, com o resistor ou transistor
que o conjunto pedir (IR-10).

| Passo | Esperado |
|---|---|
| `status` | `infrared` em `Ready` |
| Apontar um controle remoto e apertar uma tecla | Rosto SUCCESS; `ir last` mostra o código (`nec:…`, `sony:…` ou `raw:…`) |
| `ir learn`, apertar a tecla, `ir save tv_power` | Arquivo `/dbmo/ir/tv_power.ir` no cartão |

O envio (`ir send`) fica negado até `config infrared.tx_enabled 1`; usar só com aparelho
próprio.

## Etapa 8: áudio (perfil `dbmo-target`)

MAX98357A: BCLK 40, LRCLK 41, DIN 42, SD_MODE 3; alimentação conforme a etapa (3,3 V na
bancada). INMP441: mesmos BCLK e LRCLK, SD no GPIO 7, L/R ao GND, VDD em 3,3 V.

| Conferir antes | Por quê |
|---|---|
| Resistores de SD_MODE e de ganho da placa do amplificador | Não verificados (AMP-15) |
| Ligação de CHIPEN e de L/R na placa do microfone | Não verificada (MIC-12) |

| Passo | Esperado |
|---|---|
| `tone 1000 300` | Um bipe; sem estalo forte no começo e no fim |
| Ligar e desligar a placa | Observar se há estalo no alto-falante (risco 5 de `GPIO_MAP.md`) |
| `config audio.mic_enabled 1` e gravação | Ainda não há comando de console para gravar: validar quando houver |

## Etapa 9: analógicos (perfil `dbmo-target`)

Pontas em 3,3 V e GND, cursor no ADC (GPIO 2, 4, 5, 6). **Nunca 5 V** (README).

| Passo | Esperado |
|---|---|
| Medir a tensão do cursor nos extremos do curso | Registrar; acima de 2,9 V o ADC perde exatidão (S3-ADC-09) |
| Calibração de centro e de curso | Ainda sem comando de console: validar quando houver |

## Etapa 10: desligamento

| Passo | Esperado |
|---|---|
| `off` (perfil de bancada, sem trava) | Os subsistemas encerram, a configuração e o log são gravados, o chip entra em deep sleep |
| Apertar BOOT (GPIO 0) | O chip acorda. **Registrar se volta ao firmware ou ao modo de download**: é o ensaio 4 de `GPIO_MAP.md` |
| Segurar BOOT por 1,5 s com o aparelho ligado | Mesmo desligamento |

## O que este guia não cobre

- Bateria, carregador, regulador e trava: não há circuito definido
  ([`POWER_ARCHITECTURE.md`](POWER_ARCHITECTURE.md)). **Não ligar bateria de lítio à
  protoboard sem proteção e sem o circuito de carga definido.**
- Cartuchos: o soquete não foi comprado
  ([`../cartridges/CARTRIDGE_SPECIFICATION.md`](../cartridges/CARTRIDGE_SPECIFICATION.md)).
- Medição de consumo: roteiro em [`POWER_BUDGET.md`](POWER_BUDGET.md), seção 5.
- Bruce e Retro-Go: `firmware/bruce/README.md` e `firmware/retro-go/README.md`.
