# Audio (áudio)

Reprodução de tons e de amostras em memória pelo amplificador I2S, e gravação em memória
pelo microfone I2S. Formato único: PCM de 16 bits com sinal, mono.

| Peça | Arquivo | Papel |
|---|---|---|
| Contratos e utilitários | `audio_types.h` | `IAudioOutput`, `IAudioInput`, `IAudioSource`, `Volume`, `BufferSource`, taxas aceitas |
| `ToneSource` | `tone.h` | Gerador de tons inteiro, com rampa de entrada e saída |
| `AudioSubsystem` | `audio_subsystem.h` | Reprodução e gravação sem bloqueio, eventos, tratamento de falhas |

## Privacidade

- O microfone vem **desabilitado** (`audio.mic_enabled = false`). Desabilitado, a entrada
  I2S nem é ligada e `startRecording()` devolve `NotPermitted`.
- **Não existe gravação silenciosa**: `kRecordingStarted` é publicado antes da primeira
  amostra e `kRecordingFinished` ao terminar. A aplicação liga esses eventos à expressão
  LISTENING do rosto.
- A gravação vai para um buffer fornecido por quem chamou, tem duração máxima (30 s) e
  **não é gravada em arquivo nem transmitida** por este módulo. A entrada I2S só fica
  ligada durante a gravação.
- **Não há reconhecimento de voz** no firmware. O estado LISTENING significa "gravando",
  não "entendendo".

## Reprodução

- `playTone(frequência, duração)` e `play(origem)`; uma reprodução por vez (`Busy`).
- O amplificador fica **desligado em repouso** (pino SD_MODE) e desliga 50 ms depois da
  última amostra. Mudo ou volume zero: a reprodução corre e os eventos saem, mas o
  amplificador não liga.
- No máximo 4 blocos de 128 amostras por volta do laço principal; se a fila I2S enche, o
  restante segue na volta seguinte, sem perder amostra.
- Volume de 0 a 100 com curva quadrática; 100 é ganho unitário (nunca amplifica).

## Taxas de amostragem

Aceitas: **8000, 16000, 32000, 44100 e 48000 Hz**: as que o MAX98357A aceita e que cabem
na faixa do INMP441, já que os dois dividem os clocks (`PERIPHERALS.md`, AMP-08 e MIC-05).
11025, 12000, 22050 e 24000 Hz **não** são aceitas pelo amplificador; uma taxa inválida na
configuração cai para 16000 Hz com aviso no log.

## Configuração

| Chave | Padrão |
|---|---:|
| `audio.volume` | 60 (0 a 100) |
| `audio.muted` | false |
| `audio.sample_rate` | 16000 |
| `audio.mic_enabled` | false |

## Estado

- Implementado e testado no computador (`tests/unit/audio/`, 39 testes).
- Aguarda hardware: adaptadores I2S do ESP32-S3; módulos MAX98357A e INMP441 ainda não
  recebidos. Pontos a observar na bancada: 64 ciclos de clock por quadro para o microfone
  (MIC-04); os primeiros ~85 ms do microfone vêm zerados (MIC-08, o módulo descarta 100 ms);
  não remover o LRCLK com o BCLK presente (AMP-13); resistores de SD_MODE e de ganho da
  placa comprada não conferidos (AMP-15).
- Não implementado: leitura de arquivos de áudio do cartão, reamostragem, mixagem de mais
  de uma origem.
