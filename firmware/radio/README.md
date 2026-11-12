# Radio (CC1101, sub-GHz)

Driver do CC1101 em modo de pacote, recepção por consulta e transmissão **condicionada a
uma política explícita, desabilitada por padrão**.

| Peça | Arquivo | Papel |
|---|---|---|
| Endereços | `cc1101_regs.h` | Registradores, comandos e estados do chip |
| `RadioConfig` e fórmulas | `radio_config.h` | Frequência, taxa, banda e desvio → campos do chip; tempo no ar |
| `TxPolicy`, `TxGrant` | `tx_policy.h` | Única origem de autorização de transmissão |
| `Cc1101` | `cc1101.h` | Reset, identificação, configuração conferida por releitura, RX/TX |
| `RadioSubsystem` | `radio_subsystem.h` | Ciclo de vida, fila de pacotes, eventos, recuperação de falha |

## Política de uso do espectro

1. **Receber** é permitido em qualquer faixa do chip (300–348, 387–464, 779–928 MHz).
2. **Transmitir** exige as três condições:
   - `radio.tx_enabled = true` (padrão: `false`);
   - faixa informada pelo usuário em `radio.tx_min_khz` / `radio.tx_max_khz`
     (padrão: nenhuma), e a portadora **mais metade da banda ocupada** dentro dela;
   - tempo no ar dentro de `radio.tx_ms_per_min` (padrão: 600 ms por minuto, 1 %).
3. O firmware **não assume nenhuma faixa como permitida**. Nenhuma norma pôde ser
   verificada nesta etapa (`docs/hardware/RF_REGULATORY_NOTES.md`); a responsabilidade de
   informar a faixa e a potência autorizadas é de quem habilita a transmissão.
4. **Não existe** no código: portadora contínua, transmissão assíncrona sem pacote, modo
   de teste de PA, repetição automática sem limite, ruído ou qualquer função de
   interferência. Os registradores que definem o que vai ao ar (frequência, taxa, banda,
   desvio, formato e tamanho do pacote, comportamento depois de transmitir, seleção de
   potência) **só são escritos por `configure()`**, a partir da configuração que a política
   avaliou: `applyOverrides()` (ajuste fino de bancada) e `writeRegister()` (acesso de
   baixo nível) os recusam com `NotPermitted`. Isso inclui o modo de dados aleatórios e o
   modo assíncrono do chip (`PKTCTRL0`) e o "permanecer transmitindo" (`MCSM1`).
5. O driver só transmite com um `TxGrant` válido emitido para a frequência configurada;
   uma transmissão que passe do tempo previsto é cortada.

O limite de tempo no ar é uma **salvaguarda do firmware** contra transmissão contínua, não
uma declaração de conformidade com norma alguma.

## Configuração

| Chave | Padrão | Observação |
|---|---:|---|
| `radio.tx_enabled` | false | |
| `radio.tx_min_khz`, `radio.tx_max_khz` | 0 | Sem faixa, não transmite |
| `radio.tx_ms_per_min` | 600 | 0 a 6000 |
| `radio.tx_patable` | 18 (0x12) | Byte de potência (PATABLE). A correspondência com dBm depende da faixa (tabela de potência do datasheet) |
| `radio.xtal_khz` | 26000 | Cristal do módulo: **não conferido** na peça (CC-21) |

## O que está verificado e o que não está

| Item | Estado |
|---|---|
| Cabeçalho SPI, bit de rajada para registradores de status, PARTNUM/VERSION, sinal "pronto" no MISO, tempo de saída do sono, faixas de frequência | Conferido no datasheet: `docs/hardware/sources/PERIPHERALS.md`, CC-04 a CC-14 |
| Demais endereços de registradores, comandos, estados de `MARCSTATE`, campos de `PKTCTRL`/`MDMCFG`, fórmulas de taxa/banda/desvio, deslocamento de RSSI (74 dB), valor 0x12 de PATABLE | Escritos a partir das tabelas do mesmo datasheet, **não reconferidos nesta sessão**. As fórmulas reproduzem valores conhecidos (433,92 MHz → `0x10B071`; 1200 baud → E=5, M=131), o que é verificação de coerência, não de fonte |
| Registradores de ajuste de RF (AGC, FOCCFG, BSCFG, FREND1, FSCAL, TEST) | Ficam no valor de reset. A sensibilidade real depende de ajuste (por exemplo, exportado do SmartRF Studio) aplicado com `applyOverrides()` na bancada |
| Funcionamento com o módulo real | **Não testado.** Primeiro teste de bancada: ler PARTNUM/VERSION; depois reler os registradores escritos |

Não implementado: detecção de canal livre antes de transmitir (CCA), uso do pino GDO0
(a recepção é por consulta), pacotes maiores que 61 bytes.

## Estado

- Implementado e testado no computador com um simulador do chip no nível do SPI
  (`tests/unit/radio/`, 61 testes).
- Aguarda hardware: módulo CC1101 na bancada, cristal e casamento de antena do módulo
  comprado, adaptador SPI do porte.
