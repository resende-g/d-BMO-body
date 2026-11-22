# Display

Camada entre os módulos que desenham e a tela física.

| Peça | Arquivo | Papel |
|---|---|---|
| `IDisplay` e paleta | `include/dbmo/display/display.h` | Contrato mínimo de uma tela (retângulo preenchido, backlight, repouso) e as cores do d-BMO em RGB565 |
| `FramebufferDisplay` | `framebuffer_display.h` | Tela em memória para testes, pré-visualização e integração; também exporta PPM |
| `DisplaySubsystem` | `display_subsystem.h` | Brilho configurado, economia por inatividade e conferência do ID do controlador |
| Texto | `text.h`, `src/text.cpp`, `src/font5x7.cpp` | Fonte de bitmap 5x7 própria (ASCII 0x20 a 0x7E), `drawText()` sobre `fillRect` e `TextScreen`, a grade de 26 x 15 caracteres do menu, que só redesenha as linhas que mudaram |

O controlador ILI9341 não é programado aqui. No ESP32-S3, o porte
(`firmware/platform/esp32s3`) adapta a biblioteca Adafruit_ILI9341, a mesma dos sketches
de bancada, ao contrato `IDisplay`.

## Economia por inatividade

`On` → `Dimmed` (25 % do brilho) → `Off` (backlight em zero e painel em repouso), pelos
tempos configurados. `notifyActivity()` acorda a tela. O módulo não conhece o módulo de
entrada: quem liga os dois é a aplicação.

Se o hardware não controla o backlight (na bancada de hoje o LED da tela está direto no
3V3), `setBacklight` devolve `Unsupported`; o módulo registra isso uma vez, pula o
esmaecimento e mantém o repouso do painel.

## ID do controlador

No init, `DisplaySubsystem` lê `IDisplay::controllerId()` (o porte lê o comando 0xD3 da
ILI9341). Se o ID não é 0x9341, registra um aviso e guarda o resultado em
`controllerCheck()`: `IoError` para 0x0000 ou 0xFFFF (MISO da tela sem resposta) e
`Incompatible` para outro valor. O init continua `Ok`, porque a tela desenha sem o MISO; a
aplicação marca a tela como `Degraded`, o que aparece no `status`. Telas sem ID (o
`FramebufferDisplay`, por padrão) não são conferidas.

## Configuração

| Chave | Padrão | Faixa | Significado |
|---|---:|---|---|
| `display.brightness` | 80 | 0–100 | Brilho em % |
| `display.dim_after_s` | 30 | 0–3600 | Segundos sem atividade até esmaecer; 0 desliga |
| `display.off_after_s` | 120 | 0–3600 | Segundos sem atividade até apagar; 0 nunca apaga |

Sem controle do backlight (`setBacklight` devolve `Unsupported`, como na bancada, com o LED
no 3,3 V), a tela não esmaece nem apaga: com o LED aceso, o painel em repouso fica branco
(visto na bancada em 2026-11-22).

## Evento (domínio `Display`)

`display_event::kPowerChanged`: `a` = estado novo, `b` = estado anterior (`ScreenPower`).

## Estado

- Implementado e testado no computador (`tests/unit/display/`).
- Aguarda hardware: controle do backlight pelo GPIO 16 (corrente e circuito do LED do
  módulo MSP3218 não foram verificados; ver `docs/hardware/sources/PERIPHERALS.md`,
  MSP-05), e o comportamento do comando de repouso na tela real com a luz apagada. Com a
  luz acesa, o repouso deixa o painel branco (bancada, 2026-11-22).
- Texto: só ASCII, sem acentos, um tamanho de letra (escala 2). Não implementado:
  indicadores de status (bateria, sinal).
