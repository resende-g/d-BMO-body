# Cartridges (sistema de cartuchos)

Implementa o protocolo lógico descrito em
[`docs/cartridges/CARTRIDGE_SPECIFICATION.md`](../../docs/cartridges/CARTRIDGE_SPECIFICATION.md).
A especificação é a referência; este arquivo só situa o código.

| Peça | Arquivo | Papel |
|---|---|---|
| `Eeprom24c02` | `eeprom_24c02.h` | Leitura e gravação da EEPROM de identificação (página de 8 bytes, conferência por releitura) |
| Cabeçalho | `cartridge_header.h` | Codificação, validação (`HeaderCheck`) e compatibilidade (`checkCompatibility`) |
| Modelo de driver | `cartridge_driver.h` | `ICartridgeDriver`, `CartridgeContext`, `CartridgeRegistry`, `ISerialPort` |
| `CartridgeSubsystem` | `cartridge_subsystem.h` | Detecção, identificação, ciclo de vida e isolamento de falhas |
| Exemplo | `examples/example_sensor_cartridge.h` | Driver de um sensor I2C fictício: modelo para drivers reais |
| Ferramenta | `tools/cartridge_image.cpp` | `dbmo_cartridge_image`: gera e confere a imagem de 256 bytes da EEPROM |

Configuração: `cartridge.max_ma` (padrão 100, provisório).

## Estado

- Implementado e testado no computador: tudo o que está na tabela (`tests/unit/cartridges/`).
- Aguarda hardware: soquete, EEPROM real, barramentos do conector. O porte para o ESP32-S3
  ainda não liga o SPI nem a UART do cartucho ao subsistema.
- Não definido: pinagem do conector, linha de 5 V, proteção de alimentação (ver a
  especificação, seção 11).
