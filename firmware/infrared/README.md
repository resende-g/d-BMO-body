# Infrared (infravermelho)

Recebe e identifica códigos de controle remoto, aprende um código a pedido, guarda códigos
por nome e os reenvia quando o envio está habilitado.

| Peça | Arquivo | Papel |
|---|---|---|
| Tipos e contratos | `ir_types.h` | `RawTiming`, `IrCode`, `IIrReceiver`, `IIrTransmitter` |
| Codec | `ir_codec.h` | NEC e Sony SIRC (12, 15 e 20 bits); o resto é tratado como sinal bruto |
| Biblioteca | `ir_library.h` | Formato de texto de um código e guarda por nome (`IIrCodeStorage`) |
| `InfraredSubsystem` | `infrared_subsystem.h` | Recepção, repetição, aprendizado, envio com limites |

## Uso autorizado e limites

O módulo existe para repetir códigos de aparelhos do próprio usuário.

- **Envio desabilitado por padrão** (`infrared.tx_enabled = false`).
- **Orçamento de emissão**: `infrared.tx_ms_per_min` (padrão 6000 ms por minuto, 10 %).
  Dá folga para uso normal de controle remoto e não permite emissão contínua.
- Um único envio tem no máximo 500 ms e 128 durações; portadora só entre 30 e 60 kHz;
  duração zero é recusada.
- **Não existe** modo de emissão contínua, varredura automática de códigos de desligar
  ("TV-B-Gone") nem emissão de ruído para bloquear receptores.

## Recepção

- O sinal bruto é uma lista de durações (marca, espaço, marca...) em microssegundos.
- NEC: confere o comando com o seu inverso; quadro corrompido é descartado. Endereço de
  8 bits ou estendido de 16. O código de repetição só vale até 200 ms depois de um quadro.
- Sony: os quadros repetidos da mesma pressão viram um `kCodeReceived` e depois `kRepeat`.
- Protocolo desconhecido: `kRawReceived`, com o sinal disponível em `lastReceived()`.
- O que chega durante o próprio envio é ignorado (reflexo do emissor).

## Biblioteca de códigos

Um código por arquivo de texto de uma linha (a aplicação grava em `/dbmo/ir/<nome>.ir`):

```
nec:0004:08
sony:12:0001:15
raw:36000:889,889,1778,889
```

Nomes: 1 a 24 caracteres de `[a-z0-9_-]`. O texto lido do cartão é validado campo a campo;
qualquer coisa fora do formato devolve `Corrupt`.

## Configuração

| Chave | Padrão |
|---|---:|
| `infrared.rx_enabled` | true |
| `infrared.tx_enabled` | false |
| `infrared.tx_ms_per_min` | 6000 (0 a 30000) |

## O que está verificado e o que não está

| Item | Estado |
|---|---|
| Receptor (família TSOP48xx de 38 kHz): alimentação, saída, pinagem por série | Conferido no datasheet da Vishay (`docs/hardware/sources/PERIPHERALS.md`, IR-01 a IR-07) |
| Peça real do receptor comprado, LED emissor e necessidade de transistor | Não verificado (IR-08 a IR-10) |
| Tempos dos protocolos NEC e Sony SIRC | Valores amplamente publicados; **nenhuma especificação do fabricante foi aberta nesta sessão**. Validar com um controle real na bancada |
| Um receptor de 38 kHz recebendo Sony (40 kHz) | Funciona na prática com alcance menor; não verificado aqui |

Não implementado: códigos de repetição do NEC no envio (envia um quadro por chamada),
RC-5/RC-6 e outros protocolos (ficam como sinal bruto), sinais com mais de 128 durações
(ar-condicionado).

## Estado

- Implementado e testado no computador (`tests/unit/infrared/`, 48 testes).
- Aguarda hardware: adaptadores de `IIrReceiver` e `IIrTransmitter` sobre o RMT do
  ESP32-S3 e validação com controles reais. O emissor e o receptor já foram ligados na
  bancada com outro firmware (`README.md` do projeto).
