# App (aplicação)

| Arquivo | Papel |
|---|---|
| `include/dbmo/app/application.h`, `src/application.cpp` | `Application`: raiz de composição. Cria os subsistemas, define a ordem de boot, liga os eventos às reações (rosto, temporizadores) e conduz o desligamento. `ConfigPersistence` e `FileIrCodeStorage` ficam aqui porque juntam dois subsistemas |
| `include/dbmo/app/null_hal.h` | Implementações "ausentes" de I2C, SPI e sistema de arquivos, para periféricos que não existem na montagem |
| `include/dbmo/app/console.h`, `src/console.cpp` | Console de linha de comando, independente de porta serial |
| `src/app_commands.cpp` | Comandos do console (`status`, `config`, `face`, `nfc`, `ir`, `radio`...) |
| `include/dbmo/app/menu.h`, `src/menu.cpp` | `Menu`: lógica do menu de 3 botões (tela atual, cursor, o que cada botão faz). Não desenha nem fala com os subsistemas; devolve uma `MenuAction` |
| `src/menu_screens.cpp` | Parte do `Application` que executa as ações do menu (aprender IR, ligar e desligar a recepção do rádio) e monta o texto de cada tela na `TextScreen` do módulo `display` |
| `src/main.cpp` | Ponto de entrada no ESP32-S3: cria os adaptadores do porte e descreve o hardware de cada perfil. **Único arquivo desta pasta que não compila no computador** |

Tudo, menos `main.cpp`, é portável e é exercitado pelos testes de `tests/unit/app` e
`tests/integration` (29 cenários com simuladores de hardware).

Referências: [`docs/architecture/ARCHITECTURE.md`](../../docs/architecture/ARCHITECTURE.md)
(seção 4) e [`docs/firmware/FIRMWARE.md`](../../docs/firmware/FIRMWARE.md) (console e
configuração).
