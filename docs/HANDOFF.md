# Continuidade do d-BMO: bancada com o firmware próprio (sessão local)

| Campo | Valor |
|---|---|
| Versão | 2026-11-21 |
| Uso | Primeira mensagem de uma sessão local do Claude Code, aberta na pasta do repositório |
| Branch | `claude/adoring-heisenberg-b9zyh3` |

Você está numa sessão local, no meu Mac, dentro do clone do repositório `resende-g/d-BMO-body`. Esta sessão continua o trabalho de uma sessão na nuvem que não conseguia acessar a placa. Agora você consegue compilar, gravar, ler o console e rodar testes de bancada direto pela USB. As tarefas físicas (mexer em fios, apertar botões e RST, olhar a tela, usar o multímetro) continuam comigo.

## 1. Antes de qualquer coisa

1. Confirme que estamos na branch `claude/adoring-heisenberg-b9zyh3` e atualizada (`git fetch origin claude/adoring-heisenberg-b9zyh3`, `git checkout`, `git pull`). Todo o trabalho desta fase está nela. Faça commit e push só nela. Não abra PR sem eu pedir.
2. Leia, nesta ordem:
   - `docs/testing/TESTING.md`, seção "Evidência de execução": o que já foi testado no hardware e com qual resultado.
   - `docs/hardware/BREADBOARD_MAP.md`: mapa de ligação da bancada.
   - `docs/hardware/INVENTORY.md`: o que eu tenho de cada peça e em que estado.
   - `docs/hardware/GPIO_MAP.md`, `docs/hardware/ASSEMBLY_GUIDE.md`, `docs/hardware/POWER_BUDGET.md` e `docs/hardware/sources/PERIPHERALS.md`: pinagem, roteiro de bancada, consumo e fontes (identificadores CC-xx, MSP-xx, DKC-xx etc.).
   - `firmware/config/include/dbmo/config/pins.h`: fonte de verdade dos pinos (perfis `bench` e `target`).
3. Me diga em poucas linhas o que entendeu do estado atual antes de propor o próximo passo.

## 2. Como eu trabalho

- Responda em português, de forma objetiva e técnica.
- Separe sempre **fato confirmado** (com fonte ou medição), **hipótese** e **medição pendente**.
- Não invente especificações nem pinagens. Pinagem só com datasheet ou documentação confiável, e conferida pela serigrafia da placa.
- Nenhuma ligação elétrica antes de confirmar tensão, polaridade e função do pino.
- Observar não é alterar: identificar um problema não autoriza mudar código ou documentação sem eu pedir.
- Registre os resultados de bancada em `docs/testing/TESTING.md`: data, montagem, comando, saída, leitura e limites da evidência. "Compilou" não é "funcionou".

## 3. Hardware e ferramentas

- **Placa:** ESP32-S3-DevKitC-1 N16R8. No Mac aparece como `/dev/cu.usbmodem1101` (VID:PID 303A:1001, "USB JTAG/serial debug unit"). Uso só a porta marcada **USB** (USB nativa). O console do firmware está nela (`ARDUINO_USB_MODE=1`, `ARDUINO_USB_CDC_ON_BOOT=1`). A porta COM/UART não é usada.
- **Multímetro:** digital simples, faixas DCV 1000/200/20/2000m/200m. Para 3,3 V uso a faixa **20**. Não sei se tem bip de continuidade; se não tiver, Ω na escala 200 ("1" = aberto).
- **Ainda sem ferro de solda**, malha e fluxo (a caminho). Tenho jumpers F-F, M-M e M-F, uma protoboard grande MB-102 e protoboards menores.
- **Não tenho MCP23017.** O README dizia o contrário por erro; já foi corrigido. Os botões ficam direto nos GPIO (perfil `dbmo-bench`).

## 4. Montagem atual (montagem mínima de diagnóstico)

- **Tela MSP3218 (ILI9341):** ligada direto na DevKitC com 9 jumpers F-F, sem protoboard:
  - VCC → 3V3 (pino 1 do J1); LED → 3V3 (pino 2 do J1); GND → GND (pino 22 do J1, ao lado do 5V);
  - CS → 10, RESET → 18, DC → 17, SDI → 11, SCK → 12, SDO → 13; pinos T_* sem ligação.
- **Botões** na protoboard pequena, do lado do GND, ligados pelo J3: CIMA → 39 (`up` no log), OK → 2 (`a`), BAIXO → 47 (`down`). GND do J3 (pino 22) no trilho (−) de baixo. Cada botão atravessa o canal central; o fio do GPIO fica na linha a e o do GND na linha j, em pernas diagonais.
- **Desligados agora:** receptor e transmissor de IR, CC1101.
- **Desenhos de ligação** (artifacts privados):
  - montagem mínima atual: https://claude.ai/artifact/Q43ZVYJK8utdA8Yqw1Cx9F
  - bancada completa: https://claude.ai/artifact/Jcf6cXU6TdTkLStP5fy38u

## 5. O que já está comprovado no hardware (21/11/2026)

- O firmware próprio grava e roda. O rosto do d-BMO aparece na tela.
- `pio test -e dbmo-bench -f test_display`: **4/4 aprovados** (ID 0x9341, resolução, comandos de preencher e dormir, backlight sem controle). A primeira tentativa falhou no computador ("Implicit dependency ... unity.h not found"); resolveu com `rm -rf .pio/build/dbmo-bench`.
- `status`: display, rosto, input, infrared, power, config e cartridges em Ready; storage, nfc, audio e radio em Absent (esperado nesta montagem).
- O console só respondeu a comandos com o monitor aberto assim:
  `pio device monitor -p /dev/cu.usbmodem1101 -b 115200 --echo --filter send_on_enter --eol CRLF`
- **Log de botões** (commit `56cd461`, nesta branch): o botão **BAIXO (47)** gera "apertou / soltou / clique" corretamente, sem leituras duplicadas.

## 6. Pendências, em ordem

1. **Botões CIMA (39) e OK (2):** só apareceu `botao down` no log. Ainda não sei se eu apertei os três. Se CIMA e OK não aparecerem, diagnosticar com continuidade (USB desligado): pino do GPIO até o furo do botão; pino do GPIO até o GND do J3, que só deve ligar com o botão apertado.
2. **Registrar** o resultado dos botões em `docs/testing/TESTING.md`.
3. **Religar a protoboard grande com cuidado.** Antes, a tela ficou apagada quando era alimentada por ela: medi **2,65 V** no 3,3 V (ponto exato não registrado), e o GND do receptor de IR estava no lugar errado (já corrigido). A causa exata não foi isolada; suspeitas: trilho dividido no meio da MB-102, mau contato de jumper, ou a carga do receptor ligado errado. Antes de religar:
   - continuidade de ponta a ponta em cada trilho, com ponte M-M no meio se precisar;
   - os dois 3V3 da DevKitC estão ocupados pela tela: definir de onde o trilho grande recebe o 3,3 V e o GND;
   - atualizar o desenho/mapa para a nova montagem (tela direto na DevKitC; IR e CC1101 na protoboard grande; SCK/MOSI/MISO compartilhados com a tela).
4. **Receptor de IR** (OUT → GPIO 4, VCC 3,3 V, GND): ligar sozinho, medir ~3,3 V entre VCC e GND dele, testar com um controle remoto (`ir last`). Ele pode ter sido danificado pela ligação errada do GND.
5. **Transmissor de IR** (DAT → GPIO 5): pode ser ligado, mas o envio continua **bloqueado** (`infrared.tx_enabled` = 0) até identificar se a plaquinha tem transistor. Falta uma foto de perto dos componentes dela.
6. **CC1101 (Ebyte E07-M1101D V2.0, 433M, conector 2×4, nunca na protoboard):**
   - Pinagem: 1 GND, 2 VCC, 3 GDO0, 4 CSN (manual Ebyte); 5 SCK, 6 MOSI, 7 MISO/GDO1, 8 GDO2 (só fonte secundária: ficha LCSC do E07-M1101D-TH).
   - Ligação: CSN → 14 (**ao lado do 5V**), GDO0 → 15, SPI compartilhado com a tela, VCC só 3,3 V (máximo absoluto 3,9 V).
   - Na primeira montagem ficou `Absent (HardwareAbsent)`; refiz a ligação, mas ainda não houve teste com a alimentação confirmada.
   - Diagnóstico com USB ligado, nas soldas do verso do módulo: V1 = pino 2 contra pino 1, ~3,3 V; V2 = pino 3 (GDO0) contra pino 1, ~1,5 a 1,8 V (sinal de clock de reset, CC-12, indica cristal oscilando); V3 = pino 4 (CSN), ~3,3 V em repouso.
   - Antes de ligar: continuidade do pino 1 com a rosca do SMA.
   - Teste: `pio test -e dbmo-bench -f test_cc1101` (esperado PARTNUM 0x00, VERSION 0x14 ou 0x04; não transmite). Nunca habilitar transmissão sem eu pedir e sem ler `docs/hardware/RF_REGULATORY_NOTES.md`.
7. **Imprecisão conhecida no firmware (não corrigida):** com o CC1101 desligado, a primeira tentativa leu VERSION 0xE0 (MISO solto), relatou `Failed (IoError)` e só depois passou a `Absent`. `Cc1101::identify()` (`firmware/radio/src/cc1101.cpp`) só rejeita 0x00/0xFF. Possível correção: exigir VERSION documentada ou um teste de escrita e leitura antes de declarar o chip presente. **Só mexer se eu pedir.**

## 7. Bloqueado ou fora desta fase

- **SD da tela:** o conector de 4 pinos só tem furos; precisa de solda.
- **PN532:** pinos não soldados.
- **Áudio (MAX98357A e INMP441, com pinos soldados):** o perfil `dbmo-bench` não tem pinos de áudio, e a ligação do alto-falante não foi confirmada. Os pinos 40, 41, 42, 3 e 7 estão livres na bancada; acrescentá-los ao perfil `bench` só com meu pedido.
- **Analógicos:** placa do controle de PS3 BM-901 VER1.1 (chip ShanWan, sem datasheet). Recomendação registrada: dessoldar os sticks quando chegarem as ferramentas. Medições sem energia pendentes: resistência dos potenciômetros, curso e ligação com V3V/GND. Conflito: no perfil de bancada, os pinos de ADC previstos para os sticks (2, 4, 5, 6) estão ocupados.
- **Power bank AJ-C15:** investigação pausada (`docs/hardware/POWERBANK_AJ-C15.md`). Não abrir nem ligar nada a ele.

## 8. Regras de segurança da bancada

- USB desligado para mexer em qualquer fio. Um módulo novo por vez.
- Nada no pino 5V. CC1101 e INMP441 só em 3,3 V.
- Antes de cada energização: sem continuidade entre os trilhos (+) e (−).
- Algo esquentou, cheirou ou a tensão caiu? Desligar o USB e parar.

## 9. Comandos

```bash
pio run -e dbmo-bench -t upload --upload-port /dev/cu.usbmodem1101
pio device monitor -p /dev/cu.usbmodem1101 -b 115200 --echo --filter send_on_enter --eol CRLF
pio test -e dbmo-bench -f test_display --upload-port /dev/cu.usbmodem1101
pio test -e dbmo-bench -f test_cc1101 --upload-port /dev/cu.usbmodem1101
cmake -S . -B build/host -G Ninja -DDBMO_SANITIZE=ON && cmake --build build/host && ctest --test-dir build/host
```

Os `pio test` substituem o firmware na placa; depois de cada um, regravar com `pio run -e dbmo-bench -t upload`. Para ler o console sem travar a sessão, capture a saída com tempo-limite e me peça para apertar RST quando precisar do boot desde o início.

## 10. Primeiro passo agora

Leia os arquivos da seção 1, confirme o entendimento e comece pela pendência 1 (botões CIMA e OK). Grave o firmware atual, abra o console e me diga quando posso apertar os três botões, um de cada vez.
