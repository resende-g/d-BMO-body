# Placa de botões da direita (A, B, X, Y)

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-27) |
| Para quem | Tiago Batista (laser + percloreto, uma face) |
| Ferramenta | EasyEDA Pro, projeto `dbmo`, placa `botoes-direita` (Schematic4 + PCB4) |
| Tamanho | 39,2 × 39,2 mm (contorno do Gerber) |
| Estado | **Desenhada e roteada: 0 fio, sem curto** (`fios.py` sobre o Gerber exportado do EasyEDA). Nenhuma peça comprada, nada montado |

Quatro botões táteis 6 × 6 mm em cruz (X em cima, B embaixo, Y à esquerda, A à direita, 10 mm do
centro) e o conector JD na borda esquerda. O MCP23017 fica na placa de botões da esquerda: o cabo do
JD vai para o JD de lá com 5 fios Dupont soltos, pino a pino (as ordens dos dois lados são
diferentes, ver a tabela).

| JD (daqui) | Net | JD da placa da esquerda |
|---:|---|---:|
| 1 | `BTN_A` | 5 |
| 2 | `BTN_B` | 1 |
| 3 | `BTN_X` | 3 |
| 4 | `BTN_Y` | 4 |
| 5 | `GND` | 2 |

Cada botão liga o pino 1 ao seu `BTN_*` e o pino 4 ao GND; os pinos 2 e 3 ficam sem trilha.

Arquivos: `ligacoes.json`, `esquematico.enet` (conferida: 13 pinos, 0 divergências),
`gerber.zip`, `cobre-espelhada.png` (para gravar), `cobre-vista-de-cima.png`, `lista-de-pecas.tsv`
(5 peças, códigos LCSC), `fios.md` (fios, isolação, borda e trilhas finas, de `../placa-principal/fios.py`).

## Pendente antes de fabricar

| # | Item | Situação | Como fechar |
|---|---|---|---|
| 1 | Botões 6 × 6 | **Hipótese:** pinos 1 e 4 em lados opostos da chave (diagonal), seja o par interno 1-2/3-4 ou 1-3/2-4 | Multímetro num botão comprado |
| 2 | Conector na frente | **Hipótese:** a barra JD fica do lado das peças (frente); o cabo sai pela borda esquerda, por trás da tampa | Conferir na caixa; se atrapalhar, barra em 90° |
| 3 | Regras do Tiago | **Fato:** toda trilha com 0,8 mm, isolação mínima de 0,52 mm, cobre a 2,15 mm da borda no ponto mais perto (JD.1). **Hipótese:** isolação de 0,5 mm basta para o laser dele | Primeira placa gravada |
