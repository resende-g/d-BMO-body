<!--
SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
SPDX-License-Identifier: CERN-OHL-P-2.0
-->

# mechanical/ — CAD paramétrico preliminar do d-BMO-body v1

Esta pasta guarda os arquivos de projeto mecânico. Hoje ela contém só **envelopes** de componentes
gerados por script, para servir de referência de volume ao modelar a carcaça no FreeCAD. Não há
carcaça aqui, nem dimensões externas do aparelho: elas ainda não existem (ver
[`docs/mechanical/MECHANICAL_SPECIFICATION.md`](../docs/mechanical/MECHANICAL_SPECIFICATION.md)).

## Conteúdo

| Arquivo | O que é |
|---|---|
| `cad/components.yaml` | Registro de dimensões em formato legível por máquina: valor, STATUS, fonte e observação de cada dimensão. É a fonte das tabelas de [`docs/mechanical/COMPONENT_DIMENSIONS.md`](../docs/mechanical/COMPONENT_DIMENSIONS.md) |
| `cad/generate_models.py` | Lê o registro e gera os envelopes em STEP. Recusa qualquer componente que tenha dimensão não confirmada |
| `cad/render_register.py` | Reescreve as tabelas do Markdown a partir do registro |
| `cad/test_generate_models.py` | Autoteste, roda com Python puro |
| `cad/out/*.step` | Envelopes gerados |
| `cad/out/manifest.json` | O que foi gerado (dimensões usadas, fontes, SHA-256) e o que foi recusado, com o motivo |

## O que os modelos são

São **caixas envolventes**: o menor paralelepípedo alinhado aos eixos que contém a peça, com as
medidas nominais publicadas pelo fabricante. Não têm pads, blindagem, furos, chanfros nem
tolerâncias. Servem para reservar volume e para posicionar zonas de exclusão, não para verificar
encaixe fino.

| Arquivo | Medidas (mm) | O que representa |
|---|---|---|
| `esp32_s3_wroom_1__envelope.step` | 18 × 25,5 × 3,1 | Envelope do módulo ESP32-S3-WROOM-1 |
| `esp32_s3_wroom_1__antenna_zone.step` | 18 × 5,95 × 3,1 | Parte do envelope ocupada pela antena de trilha, na ponta +Y |
| `esp32_s3_wroom_1__antenna_keepout.step` | 48 × 35,95 × 33,1 | Zona de exclusão `KO-ESP`: a região da antena acrescida de 15 mm em todas as direções |

Os três arquivos usam o **mesmo referencial local**: origem no centro da face de montagem do módulo;
+X ao longo da largura; +Y ao longo do comprimento, apontando para a ponta da antena; +Z da face de
montagem para a blindagem. Importados no mesmo documento, eles já saem alinhados entre si. As
coordenadas estão em
[`docs/mechanical/ASSEMBLY_LAYOUT.md`](../docs/mechanical/ASSEMBLY_LAYOUT.md), seção 6.1.

### Por que só o ESP32-S3-WROOM-1 tem modelo

É o único componente cujas dimensões de envelope estão todas `CONFIRMADO`, isto é, lidas em
documento do fabricante aberto na data do registro: o footprint e o modelo 3D oficiais da biblioteca
KiCad da Espressif, e as diretrizes de hardware da Espressif para a folga da antena. As URLs e os
SHA-256 dos arquivos lidos estão no registro.

Os outros 21 componentes foram recusados. Os motivos, por componente, estão em `cad/out/manifest.json`
e na seção 3 de `COMPONENT_DIMENSIONS.md`. Em resumo:

- **DevKitC:** o desenho cotado da Espressif existe, mas o host dele estava bloqueado na sessão em
  que o registro foi feito. Contorno e altura ficaram `PENDENTE`.
- **Tela, bateria, soquete do cartucho, alto-falante, botões tácteis:** só há valores `NOMINAL`
  (README, anúncio, designação) e faltam cotas.
- **Módulos genéricos, analógicos, conectores e fixadores:** sem desenho de fabricante; `PENDENTE`.

O módulo WROOM-1 só entra na carcaça v1 (PCB definitiva). Na v0 ele está soldado na DevKitC, que
ainda não tem modelo.

## Como abrir no FreeCAD

1. Abrir o FreeCAD e criar um documento (**File > New**).
2. **File > Import...** e escolher um ou mais arquivos de `mechanical/cad/out/*.step`.
3. Cada arquivo traz um produto de topo chamado `dBMO_envelope` com um sólido que leva o nome do
   arquivo. A unidade é o milímetro. A forma como o FreeCAD mostra essa hierarquia depende das
   preferências de importação STEP.
4. O objeto entra na origem do documento, no referencial local descrito acima. Para levá-lo ao
   referencial do aparelho (RC-dBMO), ajustar o campo **Placement** do objeto com os parâmetros
   `MCU_X`, `MCU_Y`, `MCU_Z` e a orientação escolhida. Esses parâmetros ainda não têm valor (ver
   `ASSEMBLY_LAYOUT.md`, seção 7.3).
5. Para conferir, selecionar o objeto e usar a ferramenta de medição: a caixa envolvente deve dar as
   medidas da tabela acima.

No RC-dBMO, a vista frontal do aparelho é a vista **Top** do FreeCAD.

Limite: a importação no FreeCAD **não foi executada** na sessão em que estes arquivos foram gerados
(o FreeCAD não estava instalado). Os arquivos foram relidos com o cadquery (núcleo OpenCASCADE) e a
caixa envolvente de cada um foi conferida contra o registro.

## Como regenerar

Requisitos: Python 3 com `pyyaml` e `cadquery`. Versões usadas na geração registrada: Python 3.13.18,
cadquery 2.8.0, cadquery-ocp 7.9.3.1.1, PyYAML 6.0.3.

```bash
python3 -m venv .venv            # .venv/ já está no .gitignore
.venv/bin/pip install cadquery pyyaml

cd mechanical/cad
../../.venv/bin/python generate_models.py --check   # mostra o plano, sem escrever nada
../../.venv/bin/python generate_models.py           # gera out/*.step e out/manifest.json
../../.venv/bin/python render_register.py           # atualiza docs/mechanical/COMPONENT_DIMENSIONS.md
../../.venv/bin/python test_generate_models.py      # autoteste
```

A saída é reprodutível: com o mesmo registro e a mesma versão do cadquery, os arquivos STEP saem
com os mesmos bytes. A data no cabeçalho do STEP é a data do registro (`register_date`), não a da
execução.

Sem o cadquery, `generate_models.py --check`, `render_register.py` e a parte lógica do autoteste
continuam funcionando; os testes de geometria aparecem como `IGNORADO`.

## Regra de recusa

Aplicada em `generate_models.py` e coberta pelo autoteste:

- um componente só é modelado se **todas** as dimensões usadas pelos sólidos dele tiverem STATUS
  `CONFIRMADO`, valor numérico positivo em milímetros e ao menos uma fonte de fabricante com URL e
  data de acesso;
- basta uma dimensão `NOMINAL` ou `PENDENTE` para recusar o componente inteiro;
- ao gerar, o script apaga de `out/` qualquer STEP de componente que tenha perdido o direito a
  modelo (por exemplo, depois de um rebaixamento no registro);
- depois de escrever cada STEP, o script relê o arquivo e compara a caixa envolvente com a
  especificada.

Para um componente ganhar modelo: abrir o documento do fabricante, registrar a fonte em `sources`
(URL, data de acesso e, de preferência, SHA-256 do arquivo lido), preencher as dimensões com STATUS
`CONFIRMADO` e rodar o gerador. Os sólidos de cada componente já estão declarados em `cad.solids`,
de modo que nenhuma mudança de código é necessária.

Medição com paquímetro **não** é `CONFIRMADO`. O registro ainda não tem STATUS para medição própria;
essa decisão está em aberto (`COMPONENT_DIMENSIONS.md`, seção 6). Não ampliar `MODELABLE_STATUSES`
no script sem essa decisão.

## Licença e origem dos dados

Os arquivos desta pasta são arquivos de projeto de hardware e estão sob
[CERN-OHL-P-2.0](../LICENSE-HARDWARE). Os arquivos de texto trazem o cabeçalho SPDX; os STEP trazem
o identificador SPDX no campo `FILE_DESCRIPTION`.

As medidas do módulo ESP32-S3-WROOM-1 foram lidas da biblioteca `espressif/kicad-libraries`
(CC-BY-SA-4.0, com exceção para projetos que a usam) e das diretrizes de hardware da Espressif. Os
arquivos da Espressif **não** estão copiados aqui: os envelopes foram construídos do zero a partir
das medidas.
