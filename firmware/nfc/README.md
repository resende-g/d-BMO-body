# NFC (PN532)

Identifica o PN532 por I2C e informa a chegada e a saída de etiquetas ISO 14443-A pelo
identificador (UID).

| Peça | Arquivo | Papel |
|---|---|---|
| Quadros | `pn532_frame.h` | Montagem e validação de quadros (tamanho, complemento, soma de verificação) |
| `Pn532` | `pn532.h` | Driver sem bloqueio: comando → ACK → resposta, com prazos e desistência |
| `NfcSubsystem` | `nfc_subsystem.h` | Identificação no boot, busca periódica, eventos, recuperação de falha |

## Escopo e limites deliberados

O módulo faz **só identificação**. O chip recebe apenas quatro comandos:
`GetFirmwareVersion`, `SAMConfiguration`, `RFConfiguration` e `InListPassiveTarget`.
A restrição é estrutural: `Pn532::start()` recusa (`NotPermitted`) qualquer código de
comando fora dessa lista, antes de tocar o barramento. Os testes
`test_only_identification_commands_can_be_sent` (driver) e
`test_only_identification_commands_are_ever_sent` (subsistema) conferem isso.

**Não existe** no código: autenticação (MIFARE ou outra), leitura ou gravação da memória
de etiquetas, troca de dados com o alvo, emulação de cartão, cópia de credenciais.
Acrescentar qualquer uma dessas funções é decisão de projeto a ser tomada caso a caso,
com a autorização de uso bem definida.

Privacidade:

- A busca vem **desligada** (`nfc.scan_enabled = false`): o campo de RF fica desligado e
  nenhuma etiqueta é energizada sem o usuário pedir.
- O UID fica só na memória (`currentTag()`). Não vai para o log (só o tamanho é
  registrado) nem para arquivo; os eventos levam um CRC-32 do UID.

## Configuração

| Chave | Padrão | Faixa |
|---|---:|---|
| `nfc.scan_enabled` | false | Liga a busca já no boot |
| `nfc.scan_period_ms` | 250 | 100 a 5000 |

## Comportamento

- `init()` confere o endereço, lê a versão do firmware (o campo IC precisa ser `0x32`),
  configura o chip como leitor e desliga o campo. É o único trecho que espera, com limite
  de 150 ms por comando.
- Em busca, um `InListPassiveTarget` por período. A mesma etiqueta parada no campo gera
  **um** evento; a saída exige duas buscas vazias seguidas.
- Se o chip não responder a uma busca, o driver desiste no prazo e envia o quadro de ACK,
  que faz o PN532 abandonar o comando.
- Três erros seguidos de barramento ou de protocolo levam a `Fault`; o subsistema tenta
  reiniciar o chip a cada 5 s e retoma a busca se ela estava ligada.

Limitação conhecida: em `Fault`, a tentativa de reiniciar o chip (a cada 5 s) envia quatro
comandos e espera cada um dentro de um único `tick()`. Com um chip que responde devagar,
essa volta do laço principal pode levar cerca de meio segundo. Não foi corrigido.

## O que está verificado e o que não está

| Item | Estado |
|---|---|
| Endereço I2C 0x24, formato do quadro, byte de estado (bit 0 = pronto), ACK depois do comando | Conferido no código da biblioteca da Elechouse: `docs/hardware/sources/PERIPHERALS.md`, PN-08, PN-10, PN-11 |
| Códigos e parâmetros dos comandos, formato das respostas, quadro de ACK como "abortar", item 5 (`MaxRetries`) e item 1 (campo de RF) de `RFConfiguration` | Escritos a partir do manual do usuário da NXP (UM0701-02), que **não foi aberto nesta sessão** (PN-09, PN-13). Os quadros gerados coincidem com os exemplos amplamente publicados (`00 00 FF 02 FE D4 02 2A 00`), o que é verificação de coerência, não de fonte |
| Velocidade máxima de I2C, esticamento de clock, linhas IRQ e H_REQ | Não verificado (PN-13). O módulo não usa IRQ: consulta o byte de estado |
| Funcionamento com o módulo real | **Não testado.** O módulo PN532 V3 precisa estar com as chaves em modo I2C |

## Estado

- Implementado e testado no computador com um simulador do chip no nível do I2C
  (`tests/unit/nfc/`, 44 testes).
- Aguarda hardware: módulo PN532 na bancada. Risco conhecido do README do projeto: I2C do
  PN532 com o ESP32 pode exigir o plano B (HSU), que disputa pinos com a UART do cartucho.
