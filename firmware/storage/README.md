# Storage (armazenamento)

Cartão microSD: configuração, log e arquivos de dados. O aparelho funciona **sem cartão**
(configuração nos valores padrão, log só na memória), e nada do que é gravado sai dele.

| Peça | Arquivo | Papel |
|---|---|---|
| `IFileSystem` | `filesystem.h` | Contrato de sistema de arquivos, sem alocação dinâmica |
| Caminhos | `path.h` | Validação de caminhos e nomes; estrutura de pastas |
| `readFile`, `writeFileAtomic`, `ensureLayout` | `file_utils.h` | Operações de arquivo inteiro |
| `FileConfigBackend` | `file_config_backend.h` | Persistência do `ConfigStore` em texto editável |
| `FileLogSink` | `file_log_sink.h` | Log em arquivo, com rotação |
| `StorageSubsystem` | `storage_subsystem.h` | Monta o cartão, grava o log em segundo plano, acompanha a saúde |

## Estrutura de pastas

```
/dbmo/config/config.txt       configuração (chave=valor), .bak = versão anterior
/dbmo/logs/log.txt            log atual; log.1.txt = anterior
/dbmo/cartridges/<vvvv>-<pppp>/   dados de cada cartucho (fornecedor-produto em hexadecimal)
/dbmo/ir/                     códigos de infravermelho gravados
```

## Regras

- **Caminhos**: só absolutos, com componentes de `[A-Za-z0-9._-]`; `..`, componente vazio,
  espaço e barra invertida são recusados. Todo nome que vem de fora passa por `isValidName`.
- **Gravação atômica**: escreve em `.tmp`, sincroniza e troca. Uma queda de energia deixa o
  arquivo antigo ou o novo, nunca um pela metade. Se só o `.bak` sobrar, a leitura o usa.
- **Arquivo maior que o buffer é erro** (`NoSpace`), não truncamento.
- **Log**: `write()` só copia para a memória (2 KB); a gravação é feita a cada 2 s. Se a
  memória encher, linhas são descartadas e o próprio arquivo registra quantas.
- **Cartão retirado em uso**: detectado em até 5 s; novas tentativas de montar a cada 5 s.
  Um cartão que monta mas não grava é tentado com espera crescente (até 5 min).

## Configuração

| Chave | Padrão | Faixa |
|---|---:|---|
| `storage.log_enabled` | true | |
| `storage.log_max_kb` | 256 | 16 a 4096 (o cartão guarda até cerca do dobro) |

## Estado

- Implementado e testado no computador com um sistema de arquivos em memória que simula
  cartão cheio, cartão retirado e falhas de leitura/escrita (`tests/unit/storage/`, 44 testes).
- Aguarda hardware: adaptador de `IFileSystem` sobre a biblioteca SD do ESP32 e o módulo
  microSD. Pontos a observar na bancada (`docs/hardware/sources/PERIPHERALS.md`): o cartão
  deve ser o primeiro dispositivo iniciado no SPI compartilhado; módulos com conversor de
  nível podem não soltar o MISO.
