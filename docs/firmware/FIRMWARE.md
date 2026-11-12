# Firmware próprio do d-BMO v1: guia de uso e referência

| Campo | Valor |
|---|---|
| Versão do firmware | 0.1.0 (`firmware/core/include/dbmo/core/version.h`) |
| Versão do documento | 0.1 (2026-11-12) |
| Estado | Testado no computador; compila para o ESP32-S3; **não testado em hardware** |

A arquitetura está em [`../architecture/ARCHITECTURE.md`](../architecture/ARCHITECTURE.md);
os testes, em [`../testing/TESTING.md`](../testing/TESTING.md).

## 1. Módulos

| Módulo | Pasta | Faz | Estado |
|---|---|---|---|
| Núcleo | `firmware/core` | Erros, HAL, eventos, log, sistema, configuração, recursos | Testado no computador |
| Pinos | `firmware/config` | Mapa de pinos e endereços, com verificações em tempo de compilação | Testado no computador |
| Tela | [`firmware/display`](../../firmware/display/README.md) | Contrato de tela, brilho, apagamento por inatividade | Tela validada na bancada com outro firmware |
| Rosto | [`firmware/rosto`](../../firmware/rosto/README.md) | 17 expressões, máquina de estados, renderização | Testado no computador |
| Controles | [`firmware/input`](../../firmware/input/README.md) | Botões (debounce, toque longo, repetição, combinações), analógicos, MCP23017 | Testado no computador |
| Áudio | [`firmware/audio`](../../firmware/audio/README.md) | Tons, reprodução, gravação em memória, volume | Aguarda hardware |
| Rádio | [`firmware/radio`](../../firmware/radio/README.md) | CC1101: recepção de pacotes; transmissão condicionada | Aguarda hardware |
| NFC | [`firmware/nfc`](../../firmware/nfc/README.md) | PN532: identificação do chip e de etiquetas | Aguarda hardware |
| Infravermelho | [`firmware/infrared`](../../firmware/infrared/README.md) | Recepção, NEC e Sony, biblioteca de códigos, reenvio condicionado | Aguarda hardware |
| Armazenamento | [`firmware/storage`](../../firmware/storage/README.md) | Cartão, configuração atômica, log com rotação | Aguarda hardware |
| Energia | [`firmware/power`](../../firmware/power/README.md) | Bateria, botão, trava, desligamento | Aguarda hardware e decisão de circuito |
| Cartuchos | [`firmware/cartridges`](../../firmware/cartridges/README.md) | Identificação, compatibilidade, drivers | Aguarda hardware |
| Aplicação | `firmware/app` | Liga tudo; console | Testado no computador |
| Porte | `firmware/platform/esp32s3` | HAL sobre o Arduino-ESP32 | Só compilado |

## 2. Compilar, testar e gravar

```bash
# Testes no computador (CMake, Ninja, compilador C++17)
cmake -S . -B build/host -G Ninja -DDBMO_SANITIZE=ON
cmake --build build/host && ctest --test-dir build/host

# Firmware para o ESP32-S3 (PlatformIO Core 6.1.19 ou mais novo)
pio run -e dbmo-bench                # protoboard de hoje
pio run -e dbmo-target               # pinagem proposta do v1
pio run -e dbmo-bench -t upload      # gravar
pio device monitor                   # console (USB nativa, 115200)
```

| Perfil | Hardware assumido |
|---|---|
| `dbmo-bench` | Tela; 3 botões em GPIO 39, 2 e 47; IR em GPIO 4 e 5; cartão, CC1101 e PN532 se estiverem ligados |
| `dbmo-target` | Pinagem de `pins.h` (namespace `target`): MCP23017, analógicos, áudio I2S, trava de alimentação, backlight por PWM |

Um periférico que não está ligado não impede o boot: aparece como `Absent` em `status`.

## 3. Console serial

| Comando | Efeito |
|---|---|
| `help` | Lista os comandos |
| `version` | Versão do firmware |
| `status` | Estado do sistema e de cada subsistema; bateria (ou "sem leitura"); cartão; NFC; rádio |
| `log` | Últimas mensagens de log |
| `config` | Lista todas as chaves; `config <chave>` mostra; `config <chave> <valor>` altera |
| `save` | Grava a configuração agora |
| `face [nome\|número]` | Mostra uma expressão; sem argumento, lista |
| `tone [hz] [ms]` | Toca um tom |
| `nfc [on\|off]` | Liga ou desliga a busca; sem argumento, mostra a etiqueta presente |
| `ir learn` / `ir save <nome>` / `ir send <nome>` / `ir last` | Infravermelho |
| `radio [rx on\|off]` | Recepção; sem argumento, mostra frequência e contadores |
| `cart` | Cartucho identificado |
| `sd` | Tenta montar o cartão de novo |
| `off` | Desliga em ordem |

## 4. Configuração

Guardada em `/dbmo/config/config.txt` no cartão (texto `chave=valor`, editável à mão) ou,
sem cartão, na memória interna. Valores fora da faixa são recusados; uma linha inválida no
arquivo não impede as demais.

**Tudo o que emite, escuta ou identifica vem desligado.**

| Chave | Padrão | Faixa | Observação |
|---|---:|---|---|
| `display.brightness` | 80 | 0–100 | % (só com controle de backlight) |
| `display.dim_after_s` | 30 | 0–3600 | 0 = não escurece |
| `display.off_after_s` | 120 | 0–3600 | 0 = não apaga |
| `input.debounce_ms` | 20 | 1–200 | |
| `input.long_press_ms` | 600 | 200–5000 | |
| `input.repeat_delay_ms` | 400 | 100–3000 | |
| `input.repeat_ms` | 120 | 30–1000 | |
| `input.deadzone` | 120 | 0–900 | Milésimos do curso do analógico |
| `audio.volume` | 60 | 0–100 | Curva quadrática |
| `audio.muted` | false | | |
| `audio.sample_rate` | 16000 | 8000, 16000, 32000, 44100, 48000 | Outras caem para 16000 |
| `audio.mic_enabled` | **false** | | Sem isto não há gravação |
| `nfc.scan_enabled` | **false** | | Campo de RF desligado |
| `nfc.scan_period_ms` | 250 | 100–5000 | |
| `radio.tx_enabled` | **false** | | Transmissão desabilitada |
| `radio.tx_min_khz`, `radio.tx_max_khz` | 0 | 0–928000 | Sem faixa informada, não transmite |
| `radio.tx_ms_per_min` | 600 | 0–6000 | Tempo no ar por minuto |
| `radio.tx_patable` | 18 | 0–255 | Byte de potência do CC1101 |
| `radio.xtal_khz` | 26000 | 26000–27000 | Cristal do módulo |
| `infrared.rx_enabled` | true | | |
| `infrared.tx_enabled` | **false** | | Envio desabilitado |
| `infrared.tx_ms_per_min` | 6000 | 0–30000 | |
| `storage.log_enabled` | true | | Log em arquivo, só no cartão do aparelho |
| `storage.log_max_kb` | 256 | 16–4096 | |
| `power.vbat_enabled` | **false** | | Ligar só com o divisor montado |
| `power.vbat_ratio_permille` | 2000 | 1000–10000 | Divisor |
| `power.low_mv`, `power.critical_mv` | 3500, 3300 | | Provisórios |
| `power.hysteresis_mv` | 100 | 0–500 | |
| `power.off_hold_ms` | 1500 | 500–10000 | Toque longo para desligar |
| `power.auto_off_critical` | true | | |
| `power.sleep_after_s` | 0 | 0–86400 | 0 = não desliga por inatividade |
| `cartridge.max_ma` | 100 | 0–500 | Provisório |

## 5. Eventos

Cada evento tem um identificador (domínio + código) e dois argumentos. Os identificadores
e o significado dos argumentos estão no cabeçalho do subsistema, no namespace `*_event`.

| Domínio | Eventos |
|---|---|
| `System` | Mudança de estado do sistema, de um subsistema, falha |
| `Input` | Botão (apertou, soltou, toque longo, repetição), combinação, analógico, falha e recuperação de origem |
| `Power` | Nível da bateria, estado de carga, toque no botão, pedido de desligamento, falha de leitura, inatividade |
| `Audio` | Reprodução iniciada e terminada, gravação iniciada e terminada, falha |
| `Radio` | Estado, pacote recebido, transmissão concluída, falhou ou foi negada, falha do chip |
| `Nfc` | Etiqueta detectada e retirada, estado, falha |
| `Infrared` | Código recebido, repetição, sinal bruto, aprendido, tempo esgotado, envio concluído ou negado, falha |
| `Storage` | Cartão disponível, indisponível |
| `Cartridge` | Estado do conector, recusa, falha; eventos próprios de cada driver |
| `Face` | Pedido de expressão, expressão alterada |
| `Display` | Tela acesa, escurecida, apagada |

## 6. Limites de uso embutidos

| Função | Limite | Onde |
|---|---|---|
| Transmissão por rádio | Desabilitada; exige faixa informada pelo usuário; banda ocupada dentro da faixa; 600 ms por minuto; corta transmissão que passe do tempo | `firmware/radio/README.md` |
| Infravermelho | Envio desabilitado; 6 s por minuto; 500 ms por envio; sem emissão contínua | `firmware/infrared/README.md` |
| NFC | Só identificação: sem autenticação, leitura ou gravação de memória, emulação ou cópia | `firmware/nfc/README.md` |
| Áudio | Microfone desabilitado; gravação sempre anunciada; só em memória; sem reconhecimento de voz | `firmware/audio/README.md` |
| Cartuchos | O cartucho declara, o aparelho decide; nenhum código é carregado do cartucho | `docs/cartridges/CARTRIDGE_SPECIFICATION.md` |

Não existem no firmware próprio: interferência deliberada (jamming) de rádio ou de
infravermelho, portadora contínua, varredura automática de códigos, cópia de credenciais,
telemetria ou qualquer envio de dados para fora do aparelho.

## 7. O que ainda não existe

- Menu e navegação na tela (o rosto e o console são a interface de hoje).
- Calibração dos analógicos e gravação de áudio pelo console.
- Leitura de arquivos de áudio do cartão.
- UART e GPIOs do cartucho no porte (o I2C e o SPI estão ligados).
- Wi-Fi, Bluetooth e o "modo agente" descrito no `README.md`.
- Validação em hardware de tudo o que está acima.
