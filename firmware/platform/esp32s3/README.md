# Porte para o ESP32-S3 (Arduino-ESP32 3.x)

Implementa os contratos de hardware do firmware próprio sobre o Arduino-ESP32. Junto com
`firmware/app/src/main.cpp`, é o único código do firmware próprio que inclui cabeçalhos do
Arduino; por isso não compila no computador e **não tem testes automatizados**.

| Arquivo | Adaptadores | Contrato |
|---|---|---|
| `hal.h` | `ArduinoClock`, `ArduinoGpio`, `ArduinoSpiDevice`, `ArduinoI2cBus`, `ArduinoAdcChannel`, `LedcPwmOutput`, `PortCriticalSection`, `SerialLogSink`, `NvsConfigBackend` | HAL do núcleo, log e configuração |
| `ili9341_display.h` | `Ili9341Display` (sobre a biblioteca Adafruit ILI9341) | `display::IDisplay` |
| `peripherals.h` | `SdFileSystem` | `storage::IFileSystem` |
| | `I2sAudio` (amplificador e microfone na mesma porta) | `audio::IAudioOutput`, `audio::IAudioInput` |
| | `RmtIrReceiver`, `RmtIrTransmitter` | `infrared::IIrReceiver`, `infrared::IIrTransmitter` |
| | `DeepSleepControl` | `power::ISleepControl` |

## Estado

| Item | Situação |
|---|---|
| Compilação | Aprovada na CI (PlatformIO, pioarduino 55.03.39, perfis `dbmo-bench` e `dbmo-target`) |
| `hal.h` e `ili9341_display.h` em hardware | Não executados com o firmware próprio. A tela funcionou com outro firmware nos mesmos pinos |
| `peripherals.h` em hardware | **Nunca executado.** Escrito a partir dos cabeçalhos do núcleo Arduino-ESP32 3.3.9 |

Pontos a conferir na primeira execução em bancada:

- **I2S**: quadros estéreo de 32 bits; a escrita usa tempo de espera zero e conta com a
  fila aceitar múltiplos de um quadro (8 bytes). A recepção é desligada logo depois do
  `begin()` e religada só durante a gravação.
- **RMT**: 1 µs por tique; fim de rajada depois de 12 ms sem borda; cada metade de símbolo
  tem 15 bits, então durações acima de 32767 µs são recusadas no envio.
- **SD**: a biblioteca não diferencia cartão cheio de erro de escrita; zero bytes escritos
  é tratado como erro.
- **Deep sleep**: o pino de despertar precisa ser GPIO de RTC (0 a 21) e ter pull-up
  externo; sem pino de despertar a chamada é recusada.

Ainda não existe adaptador para a UART e para os GPIOs do cartucho.
