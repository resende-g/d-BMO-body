# Continuidade do d-BMO: correções do firmware e menu (sessão sem placa)

| Campo | Valor |
|---|---|
| Versão | 2026-11-21 |
| Uso | Primeira mensagem de uma sessão do Claude Code na nuvem, que roda sozinha enquanto meu notebook está desligado |
| Branch de partida | `claude/adoring-heisenberg-b9zyh3` |
| Branch de trabalho | `claude/firmware-menu` |

Você vai trabalhar sozinho, sem acesso à placa e sem poder me perguntar nada até eu voltar. Nada do que você fizer será testado em hardware nesta sessão: o teste na bancada fica comigo, depois. Seu trabalho é deixar o código pronto, testado no computador e com um roteiro claro para eu validar na placa.

## 1. Antes de qualquer coisa

1. Crie a branch `claude/firmware-menu` a partir de `origin/claude/adoring-heisenberg-b9zyh3`. Faça commit e push **só nela**. Não abra PR. Não faça push em nenhuma outra branch (outra sessão trabalha na `claude/adoring-heisenberg-b9zyh3`).
2. Leia:
   - `docs/testing/TESTING.md`, seção "Evidência de execução", em especial as entradas de 2026-11-21: é o que funcionou na placa hoje e os problemas encontrados.
   - `docs/firmware/FIRMWARE.md` e `docs/architecture/ARCHITECTURE.md`.
   - `firmware/config/include/dbmo/config/pins.h` (perfis `bench` e `target`).
   - `docs/hardware/RF_REGULATORY_NOTES.md`, antes de mexer em qualquer coisa do rádio.
3. Rode os testes do computador antes de mudar qualquer coisa, para ter a linha de base:
   `cmake -S . -B build/host -G Ninja -DDBMO_SANITIZE=ON && cmake --build build/host && ctest --test-dir build/host`

## 2. Regras

- Português nos textos, nas mensagens de log e nos documentos. Mensagens de commit em inglês, no estilo dos commits anteriores.
- Separe sempre **fato confirmado** (com fonte, teste ou saída), **hipótese** e **pendente de bancada**. "Compilou" e "passou no teste do computador" não são "funcionou na placa".
- **Rádio: nunca habilitar transmissão**, nem por padrão, nem por menu, nem em teste. O infravermelho continua com o envio bloqueado (`infrared.tx_enabled` = 0); o menu não oferece envio.
- Não mudar pinagem (`pins.h`), nem documentação de hardware, nem nada do power bank AJ-C15.
- Cada correção começa por um teste no computador que falha e depois passa (`tests/unit/...` ou `tests/integration/test_application.cpp`, com os mocks de `tests/mocks/`). Lógica nova (o menu) é testável no computador, sem Arduino.
- Reaproveite o que existe antes de escrever código novo: procure no repositório (rosto, display, input, console) antes de criar uma abstração.
- Escopo fechado: só as correções da seção 3 e o menu da seção 4. Outros problemas que você encontrar vão para o relatório final, sem correção.
- Commits pequenos, um por item, cada um com os testes passando.

## 3. Correções (problemas vistos na bancada em 2026-11-21)

**F1. Rádio classificado como falha com o barramento solto.** Com o CC1101 desligado ou com o MISO solto, a leitura de VERSION devolveu valores sem sentido (0xE0, 0x0B). O `Cc1101::identify()` (`firmware/radio/src/cc1101.cpp`) só rejeita 0x00 e 0xFF; o `RadioSubsystem::bringUp()` (`firmware/radio/src/radio_subsystem.cpp`) só avisa da VERSION fora das documentadas e segue, e a configuração falha depois com `Failed (IoError)`. Esperado: barramento sem chip vira `Absent (HardwareAbsent)`, nunca `Failed`. Aceitar o chip quando VERSION for 0x14 ou 0x04 (CC-13 em `docs/hardware/sources/PERIPHERALS.md`); para outra VERSION, fazer um teste de escrita e leitura num registrador sem efeito no rádio antes de declarar o chip presente (o datasheet avisa que VERSION pode mudar, então não rejeitar só pela VERSION). Testes: VERSION 0xE0 e 0x0B com o teste de escrita falhando → `HardwareAbsent`; 0x14 → presente; VERSION desconhecida com o teste passando → presente, com aviso no log.

**F2. Tela declarada pronta com ID errado.** Sem o SDO da tela ligado, o boot leu `id 0xFFFF` e `0x0000` e o `status` mostrou `display: Ready (Ok)`. O ID só é lido para o log, em `firmware/app/src/main.cpp`. Esperado: um ID diferente de 0x9341 aparece como aviso no log e fica visível no `status`. Não impedir a tela de desenhar por causa disso: ela pode funcionar sem o MISO. Escolha a forma dentro do modelo de estados que já existe e explique a escolha no relatório.

**F3. Infravermelho sem registro no log.** Para ver um código recebido foi preciso mandar `ir last` à mão. Registrar cada código recebido no log, em nível INFO, como já se faz com os botões em `Application::handle` (`firmware/app/src/application.cpp`), sem registrar as repetições.

**F4. Console sem resposta no `pio device monitor` padrão.** O console só respondeu com `--echo --filter send_on_enter --eol CRLF`; enviando a linha inteira com CRLF por um script, funcionou. O `Console` (`firmware/app/src/console.cpp`) já trata CR, LF e CRLF, e o eco começa desligado. Investigue pelo código a causa provável. Corrija só o que o código mostrar com clareza (por exemplo, eco ligado por padrão, se for o caso); o que depender da placa vai como hipótese no roteiro de bancada.

**F5 (só se for simples).** Ruído no boot: erros `nvs_open failed: NOT_FOUND` na primeira inicialização e `sdCommand(): Card Failed!` repetidos sem cartão. Se der para evitar sem mexer no comportamento, faça; se não, registre no relatório.

Não corrigir, só registrar se encontrar algo: `infrared: Ready` aparece mesmo sem os módulos de IR (não há como detectá-los); eventos de botão vistos uma vez sem confirmação de que foram apertados.

## 4. Menu

Hoje a tela só mostra o rosto. Já funcionam na placa: tela, 3 botões (perfil `bench`: CIMA, OK, BAIXO), receptor de IR e CC1101 em recepção.

- O rosto continua sendo a tela inicial. OK abre o menu.
- Navegação no perfil `bench`, que só tem 3 botões: CIMA e BAIXO mudam o item, OK escolhe, **toque longo no OK** volta. No perfil `target`, B também volta. Use os eventos de botão que já existem (`ButtonAction::Click`, `LongPress`).
- Itens:
  1. **Estado**: lista dos subsistemas com o estado de cada um, como o `status` do console.
  2. **Infravermelho**: último código recebido; aprender um código (`ir learn`) mostrando o resultado. Sem envio.
  3. **Rádio**: ligar e desligar a recepção, frequência, estado e contadores (o que o comando `radio` mostra). Sem transmissão.
  4. **Botões**: mostra o último botão e a ação, para testar a fiação.
  5. **Sobre**: versão do firmware, perfil (`bench` ou `target`) e nome do hardware.
  6. **Voltar ao rosto**.
- Itens de subsistema ausente (cartão, NFC, áudio) aparecem como "ausente" em Estado, e não como entrada do menu.
- A lógica do menu (estado, navegação, ações) fica separada do desenho e é testada no computador. O desenho passa pela interface de tela que já existe (`firmware/display/include/dbmo/display/display.h`), testável com o `FramebufferDisplay`. Se não houver fonte de texto no projeto, use uma fonte de bitmap pequena desenhada com `fillRect`; não traga uma biblioteca nova para isso.
- Nada pode travar o laço principal: o menu reage a eventos e redesenha só o que mudou.
- Qualquer botão conta como atividade, como hoje (escurecer e dormir continuam funcionando). Depois de um tempo sem uso no menu, volta ao rosto.

## 5. Verificação antes de terminar

1. Testes do computador: o comando da seção 1, com sanitizers, todos passando.
2. Compilação do firmware nos dois perfis: `pio run -e dbmo-bench` e `pio run -e dbmo-target`, se o PlatformIO estiver disponível. Se não estiver, faça push e acompanhe o job "Firmware próprio (ESP32-S3)" da CI da branch com `gh run list --branch claude/firmware-menu` e `gh run view`.
3. `python3 tools/check_pinmap.py`, `python3 tools/check_links.py`, `python3 tools/check_repo.py` e `tools/check_format.sh`.
4. Atualize `docs/firmware/FIRMWARE.md` (como usar o menu) e acrescente em `docs/testing/TESTING.md` uma entrada com o que foi executado no computador, os resultados e o que **não** foi testado na placa.

## 6. O que deixar para mim

Termine com um relatório curto, em português:

- O que mudou, item por item (F1 a F5 e o menu), com os commits.
- O resultado dos testes e da compilação, com a saída resumida.
- O que ficou de fora e por quê.
- Um **roteiro de bancada** numerado para eu validar na placa: o que ligar, o comando, o que apertar e o que deve aparecer na tela e no console, um passo por item. Inclua como regravar o firmware (`pio run -e dbmo-bench -t upload --upload-port /dev/cu.usbmodem1101`) e como voltar à versão anterior se algo der errado.
