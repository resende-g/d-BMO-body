# Display

Camada entre os módulos que desenham e a tela física.

| Peça | Arquivo | Papel |
|---|---|---|
| `IDisplay` e paleta | `include/dbmo/display/display.h` | Contrato mínimo de uma tela (retângulo preenchido, backlight, repouso) e as cores do d-BMO em RGB565 |
| `FramebufferDisplay` | `framebuffer_display.h` | Tela em memória para testes, pré-visualização e integração; também exporta PPM |
| `DisplaySubsystem` | `display_subsystem.h` | Brilho configurado e economia por inatividade |

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

## Configuração

| Chave | Padrão | Faixa | Significado |
|---|---:|---|---|
| `display.brightness` | 80 | 0–100 | Brilho em % |
| `display.dim_after_s` | 30 | 0–3600 | Segundos sem atividade até esmaecer; 0 desliga |
| `display.off_after_s` | 120 | 0–3600 | Segundos sem atividade até apagar; 0 nunca apaga |

## Evento (domínio `Display`)

`display_event::kPowerChanged`: `a` = estado novo, `b` = estado anterior (`ScreenPower`).

## Estado

- Implementado e testado no computador (`tests/unit/display/`).
- Aguarda hardware: controle do backlight pelo GPIO 16 (corrente e circuito do LED do
  módulo MSP3218 não foram verificados; ver `docs/hardware/sources/PERIPHERALS.md`,
  MSP-05), e o comportamento do comando de repouso na tela real.
- Não implementado: texto e indicadores de status (bateria, sinal). O contrato `IDisplay`
  só tem retângulos.
