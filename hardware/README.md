# Placas do d-BMO (módulos de uma face para o Tiago)

Desde 2026-11-27 o d-BMO é feito de módulos pequenos, ligados por cabo Dupont, porque o Tiago Batista
fabrica placas de até 70 × 70 mm (laser + percloreto, uma face; trilha de 0,8 a 1 mm; brocas de 1,0 e
0,6 mm). A placa única deitada de 170 × 115 mm, desenhada em 2026-11-26, não cabia no processo dele.

| Placa | Pasta | Tamanho | Fios (jumpers) | O que leva |
|---|---|---|---:|---|
| Placa-base (CPU) | [`placa-principal/`](placa-principal/README.md) | 64 × 53 mm | 0 | DevKitC com cada pino em barras de pinos (3 cópias em cima, 2 embaixo), barra de 3V3 e de GND |
| Botões da esquerda | [`placa-botoes-esquerda/`](placa-botoes-esquerda/README.md) | 52 × 64 mm | 4 | Cruz, Select/Start, energia, MCP23017 (0x20), pull-ups do I²C, D1 |
| Botões da direita | [`placa-botoes-direita/`](placa-botoes-direita/README.md) | 39,2 × 39,2 mm | 0 | A, B, X, Y |
| Energia | [`placa-energia/`](placa-energia/README.md) | 60,1 × 39,8 mm | 5 (4 de verdade) | TP4056, trava liga/desliga, divisor da bateria |

Sem placa própria, presos na caixa e ligados por cabo à placa-base: tela MSP3218 (com o SD), CC1101,
INMP441, MAX98357A (alto-falante no borne), IR RX, IR TX e PN532. O mapa de cada fio está na seção 4 de
[`placa-principal/README.md`](placa-principal/README.md).

Em cada pasta: `ligacoes.json` (a fonte), `esquematico.enet` (netlist do EasyEDA, conferida),
`gerber.zip`, `cobre-espelhada.png` (para o Tiago gravar), `cobre-vista-de-cima.png`,
`lista-de-pecas.tsv` e `fios.md`.

Ferramentas (em `placa-principal/`): `conferir_netlist.py` (netlist × `ligacoes.json` × `pins.h`),
`dsn.py` + `rotear.py` (Gerber do EasyEDA → Specctra → Freerouting → SES, que volta ao EasyEDA pelo
MCP), `fios.py` (fios que faltam, isolação, borda e trilhas finas, lidos do Gerber) e `png_tiago.py`
(imagem 1:1 do cobre para o laser).

## Medidas no Gerber (2026-11-27)

Saída do `fios.py` sobre o `gerber.zip` de cada pasta (cada `fios.md` tem o detalhe). Regras pedidas
pelo Tiago: trilha de 0,8 a 1 mm, margem de 2 a 3 mm; a isolação de 0,5 mm é escolha nossa.

| Placa | Trilhas | Isolação mínima | Cobre mais perto da borda | Netlist |
|---|---|---:|---:|---|
| Placa-base | 127, todas com 0,8 mm | 0,60 mm | 2,03 mm | 0 divergências (também contra `pins.h`) |
| Botões da esquerda | 131; 0,8 mm menos `BTN_Y` (0,6 mm × 28,1 mm) e `GND` (0,4 mm × 1,4 mm) | 0,52 mm | 1,94 mm | 0 divergências |
| Botões da direita | 29, todas com 0,8 mm | 0,52 mm | 2,15 mm | 0 divergências |
| Energia | 123, com 0,6 mm e 38,9 mm de 0,45 mm (decisão do desenho, por causa do TP4056) | 0,52 mm | 2,50 mm | 0 divergências |

## Pendências que valem para mais de uma placa

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Trilhas abaixo de 0,8 mm | **Fato:** só na esquerda (2 lugares) e na energia (por desenho). **Hipótese:** 0,6 mm e 0,45 mm saem no laser do Tiago, que engrossa tudo no Paint | Perguntar ao Tiago antes de gravar; se não sair, rerrotear (pode custar fios) |
| 2 | Isolação de 0,5 mm | **Hipótese:** basta para o processo do Tiago (pintura, laser, percloreto) | Primeira placa gravada |
| 3 | Botões 6 × 6 | **Hipótese:** pinos 1 e 4 ficam em lados opostos da chave | Multímetro num botão comprado |
| 4 | Imagem para gravar | **Fato:** o cobre está na camada de baixo em todas; `cobre-espelhada.png` é o lado do cobre visto de frente, 1:1. **Hipótese:** é a imagem que o Tiago quer | Confirmar com o Tiago na primeira placa |
| 5 | Nada comprado nem montado | **Fato:** nenhuma placa foi gravada | — |
