<!--
SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
SPDX-License-Identifier: Apache-2.0
-->

# d-BMO-body v1 — Registro de dimensões dos componentes

| Campo | Valor |
|---|---|
| Status | Preliminar. Registro, não projeto: uma linha por componente e por dimensão |
| Data | 2026-11-12 |
| Fonte das tabelas | [`mechanical/cad/components.yaml`](../../mechanical/cad/components.yaml). As seções 2 a 5 são geradas por `mechanical/cad/render_register.py`; não editar à mão |
| Referencial | RC-dBMO, definido em [MECHANICAL_SPECIFICATION.md](MECHANICAL_SPECIFICATION.md), seção 3 |

Regra deste registro: **nenhuma dimensão é inventada**. Um valor só entra com fonte. Onde não há
fonte, a linha fica sem valor e com STATUS `PENDENTE`.

## 1. STATUS e regras

| STATUS | Significado | Pode virar geometria no CAD? |
|---|---|---|
| `CONFIRMADO (fabricante)` | Valor lido, na data do registro, em documento dimensional publicado pelo fabricante da peça. A URL do arquivo aberto está na seção 2.1 | Sim |
| `NOMINAL (anúncio/designação)` | Valor de anúncio, de designação da peça (por exemplo o código 4070115 da bateria), de medida de régua anotada no README, ou valor que o README atribui a um documento que não foi possível localizar | Não |
| `PENDENTE (medir com paquímetro)` | Sem valor. A peça precisa ser medida, ou o documento do fabricante precisa ser aberto | Não |

Regras adicionais:

1. **O que conta como documento de fabricante.** Desenho cotado, datasheet ou dado de CAD oficial
   (footprint e modelo 3D publicados pelo próprio fabricante). Nesta sessão, só os dados de CAD da
   Espressif puderam ser abertos; o datasheet em PDF do módulo e o desenho cotado da placa não
   (seção 2.2). Por isso as linhas `CONFIRMADO` trazem valor nominal de projeto, **sem tolerância**:
   a tolerância está no datasheet, que continua por abrir.
2. **Peça em mãos tem precedência.** Um valor `CONFIRMADO` descreve a peça do fabricante. Se a peça
   comprada for de terceiros com o mesmo nome, vale a medida da peça. Conferir sempre uma cota de
   controle com o paquímetro (seção 6).
3. **Módulos genéricos** (placa do CC1101, placas do INMP441 e do MAX98357A, alto-falante, conjunto
   IR, módulo PN532 de anúncio) não têm desenho de fabricante identificável. Ficam `PENDENTE`. As
   medidas de placas de outros fornecedores com o mesmo chip **não** foram usadas.
4. **Resumo de busca não é fonte.** Quando um mecanismo de busca resumiu uma página que não pôde ser
   aberta, o fato está anotado na observação e o STATUS não muda.
5. **Rebaixamento.** Onde o README diz "datasheet" e o documento não foi localizado, o valor ficou
   `NOMINAL` (seção 7).

<!-- BEGIN: REGISTRO-GERADO (não editar à mão; fonte: mechanical/cad/components.yaml) -->

## 2. Fontes

Data do registro: 2026-11-12. Todas as fontes abaixo foram abertas nessa data.

### 2.1 Fontes abertas

| ID | Origem | Tipo | Documento e local | URL | SHA-256 do arquivo lido |
|---|---|---|---|---|---|
| `ESPRESSIF_KICAD_WROOM1_FP` | fabricante | footprint oficial (biblioteca KiCad da Espressif) | espressif/kicad-libraries — footprints/Espressif.pretty/ESP32-S3-WROOM-1.kicad_mod. Local: contorno nas camadas F.SilkS e Eco2.User (x de -9 a 9, y de -15,75 a 9,75); zona 'antenna keepout' (y de -15,75 a -9,8); pads 1 a 40 | <https://raw.githubusercontent.com/espressif/kicad-libraries/dd76561812ab300351234ba6e0ec1295641796f0/footprints/Espressif.pretty/ESP32-S3-WROOM-1.kicad_mod> | `a99796419aec905e6b0a12ddf266a63d546754070c587acf10585d70a161dddc` |
| `ESPRESSIF_KICAD_WROOM1_STEP` | fabricante | modelo 3D oficial (biblioteca KiCad da Espressif) | espressif/kicad-libraries — 3dmodels/espressif.3dshapes/ESP32-S3-WROOM-1.STEP. Local: cabeçalho FILE_NAME 'ESP32-S3-WROOM-1_20211021.STEP' (SolidWorks); caixa envolvente medida com cadquery 2.8.0: 18,0 x 25,5 x 3,1 mm | <https://raw.githubusercontent.com/espressif/kicad-libraries/dd76561812ab300351234ba6e0ec1295641796f0/3dmodels/espressif.3dshapes/ESP32-S3-WROOM-1.STEP> | `02e192087a1a2268ccbe7bf667ef04884370fc76c994484bf85654dfa7407ef6` |
| `ESPRESSIF_KICAD_DEVKITC_FP` | fabricante | footprint oficial (biblioteca KiCad da Espressif) | espressif/kicad-libraries — footprints/Espressif.pretty/ESP32-S3-DevKitC.kicad_mod. Local: 44 pads de furo passante: fileira 1 a 22 em x = 0, fileira 23 a 44 em x = 22,86; y de 0 a 53,34; furo 0,8 | <https://raw.githubusercontent.com/espressif/kicad-libraries/dd76561812ab300351234ba6e0ec1295641796f0/footprints/Espressif.pretty/ESP32-S3-DevKitC.kicad_mod> | `1e033c202e4ae743955d325da9e8e392c9600b51bf67caec56fb7f7eca723c0a` |
| `ESPRESSIF_KICAD_README` | fabricante | documentação da biblioteca | espressif/kicad-libraries — README.md. Local: linha 29: 'All footprints were designed according to the Recommended PCB Land Pattern section present on each module datasheet.' | <https://raw.githubusercontent.com/espressif/kicad-libraries/dd76561812ab300351234ba6e0ec1295641796f0/README.md> | `416c1ccccae91df9122483f3e4c315b7b62f28b5de5aad391138406ec5329ce5` |
| `ESPRESSIF_HDG_PCB_LAYOUT` | fabricante | diretrizes de projeto de hardware (texto-fonte) | espressif/esp-hardware-design-guidelines — docs/en/pcb-layout-design.rst. Local: seção 'General Principles of PCB Layout for Modules (Positioning a Module on a Base Board)', linhas 334 a 377 | <https://raw.githubusercontent.com/espressif/esp-hardware-design-guidelines/12c39ca361650334fbe72fdc13385a6913e75edf/docs/en/pcb-layout-design.rst> | `97b0972dac243e2e691b4b7ca70ea910bbccbc14404ac3772985f45f82653be9` |
| `ESPRESSIF_HDG_CLEARANCE_FIG` | fabricante | figura cotada das diretrizes de projeto de hardware | espressif/esp-hardware-design-guidelines — docs/_static/esp32s3/esp32s3-module-clearance.png. Local: cotas 'Min15', '6', 'Max 2' e 'Max 1'; legenda 'Unit: mm' e 'Clearance Area' | <https://raw.githubusercontent.com/espressif/esp-hardware-design-guidelines/12c39ca361650334fbe72fdc13385a6913e75edf/docs/_static/esp32s3/esp32s3-module-clearance.png> | `ea84d1e439d2818ee4d9dc9a8578c433833409d69f77a24ce4c87b23b2f67d3d` |
| `ESPRESSIF_DEVKITC_GUIDE` | fabricante | guia do usuário (texto-fonte) | espressif/esp-dev-kits — docs/en/esp32-s3-devkitc-1/user_guide_v1.1.rst e ordering-info.inc. Local: tabela de componentes (portas Micro-USB), 'Hardware Revision Details' e 'Related Documents' (links do desenho cotado); ordering-info.inc (variante N16R8), sha256 962e485a50b17e9665660345b0f0cae8d111f1b3350ab0b645dbc2772f5100f4; ramo master, sem commit fixado | <https://raw.githubusercontent.com/espressif/esp-dev-kits/master/docs/en/esp32-s3-devkitc-1/user_guide_v1.1.rst> | `ee0b339051b013bdccbed9134335ee5d78e6564072d636c7127c3495936b116d` |
| `README` | projeto | decisões e anotações do projeto | README.md deste repositório. Local: seções Lista de componentes, Carcaça 3D, Especificação do cartucho, Dimensões conhecidas, Analógicos e gatilhos reaproveitados | — | — |

### 2.2 Documentos de fabricante que não puderam ser abertos

Nenhum valor destes documentos foi usado. Eles são o caminho para promover linhas a `CONFIRMADO`.

| Documento | URL | Motivo |
|---|---|---|
| ESP32-S3-DevKitC-1 Dimensions (PDF e DXF), v1.1 | <https://dl.espressif.com/dl/schematics/esp_idf/DXF_ESP32-S3-DevKitC-1_V1.1_20220429.pdf> | dl.espressif.com bloqueado (403 no proxy da sessão) |
| ESP32-S3-DevKitC-1 Dimensions (PDF e DXF), versão inicial (link no guia do esp-idf v5.1.6) | <https://dl.espressif.com/dl/DXF_ESP32-S3-DevKitC-1_V1_20210312CB.pdf> | dl.espressif.com bloqueado |
| ESP32-S3-WROOM-1 / WROOM-1U Datasheet (dimensões físicas e tolerâncias) | <https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf> | www.espressif.com bloqueado (403 no proxy da sessão) |
| Guia do usuário publicado da ESP32-S3-DevKitC-1 | <https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide.html> | docs.espressif.com não resolve na sessão; o texto-fonte foi lido no GitHub |
| Página do módulo MSP3218 (LCD Wiki) | <http://www.lcdwiki.com/3.2inch_SPI_Module_ILI9341_SKU:MSP3218> | lcdwiki.com não resolve na sessão |
| Manual do PN532 NFC Module V3 (Elechouse) | — | o repositório elechouse/PN532 no GitHub não traz manual nem medidas; o site do fabricante não foi tentado depois das falhas acima |
| Datasheet do MCP23017 (Microchip) | — | ww1.microchip.com consta como bloqueado (403) no registro do proxy da sessão; não foi tentado de novo |
| Datasheet do soquete DK920A-20M | — | fabricante não identificado; busca pelo código não devolveu datasheet |

## 3. Resumo por componente

"Modelo CAD" indica se `mechanical/cad/generate_models.py` gera envelope para o componente.

| Componente | Peça | Qtd | CONFIRMADO | NOMINAL | PENDENTE | Modelo CAD |
|---|---|---|---:|---:|---:|---|
| `esp32_s3_wroom_1` | Módulo ESP32-S3-WROOM-1 (N16R8) | 1 | 10 | 0 | 0 | sim: envelope, antenna_zone, antenna_keepout |
| `esp32_s3_devkitc_1` | Placa ESP32-S3-DevKitC-1 N16R8 | 1 | 4 | 0 | 5 | não: pcb_width: status PENDENTE; pcb_length: status PENDENTE; height_with_pins: status PENDENTE |
| `display_msp3218` | Módulo TFT 3,2 pol. ILI9341 320 x 240, MSP3218 (com toque) | 1 | 0 | 6 | 7 | não: pcb_length: status NOMINAL; pcb_width: status NOMINAL; total_thickness: status PENDENTE |
| `battery_4070115` | Bateria LiPo 1S, designação 4070115 (4000 mAh no anúncio) | 1 | 0 | 3 | 2 | não: width: status NOMINAL; length: status NOMINAL; thickness: status NOMINAL |
| `card_edge_socket_dk920a_20m` | Soquete card edge DK920A-20M (2 x 10 contatos, 2,54 mm) | 1 | 0 | 8 | 1 | não: body_length: status NOMINAL; body_width: status NOMINAL; height_above_board: status NOMINAL |
| `pn532_nfc_v3` | Módulo NFC PN532 V3 | 1 | 0 | 0 | 5 | não: pcb_length: status PENDENTE; pcb_width: status PENDENTE; total_thickness: status PENDENTE |
| `cc1101_module` | Módulo CC1101 433 MHz com conector SMA | 1 | 0 | 0 | 5 | não: pcb_length: status PENDENTE; pcb_width: status PENDENTE; total_thickness: status PENDENTE |
| `mcp23017` | Expansor de I/O MCP23017 | 1 a 2 | 0 | 0 | 4 | não: body_length: status PENDENTE; body_width: status PENDENTE; body_height: status PENDENTE |
| `inmp441_breakout` | Placa do microfone INMP441 (I2S) | 1 | 0 | 0 | 4 | não: pcb_length: status PENDENTE; pcb_width: status PENDENTE; total_thickness: status PENDENTE |
| `max98357a_breakout` | Placa do amplificador MAX98357A (I2S) | 1 | 0 | 0 | 3 | não: pcb_length: status PENDENTE; pcb_width: status PENDENTE; total_thickness: status PENDENTE |
| `speaker_2030` | Alto-falante 2030, 8 ohms, 2 W | 1 | 0 | 2 | 2 | não: length: status NOMINAL; width: status NOMINAL; thickness: status PENDENTE |
| `ir_kit` | Conjunto IR: emissor e receptor de 38 kHz | 1 | 0 | 0 | 4 | não: length: status PENDENTE; width: status PENDENTE; height: status PENDENTE |
| `analog_stick_ps3` | Analógico reaproveitado de controle genérico de PS3 | 2 | 0 | 0 | 6 | não: base_length: status PENDENTE; base_width: status PENDENTE; height_with_cap: status PENDENTE |
| `tactile_switch` | Botão táctil (cruz, ações, select/start) | 10 | 0 | 2 | 2 | não: body_length: status NOMINAL; body_width: status NOMINAL; total_height: status PENDENTE |
| `tactile_switch_right_angle` | Botão táctil de ângulo reto (gatilhos de ombro) | 2 a 4 | 0 | 0 | 3 | não: body_length: status PENDENTE; body_depth: status PENDENTE; body_height: status PENDENTE |
| `power_button` | Botão momentâneo de energia | 1 | 0 | 0 | 3 | não: body_length: status PENDENTE; body_width: status PENDENTE; total_height: status PENDENTE |
| `button_caps_ps3` | Tampas reaproveitadas do controle de PS3 | 1 conjunto | 0 | 0 | 5 | não: body_diameter: status PENDENTE; total_height: status PENDENTE |
| `sd_card_with_adapter` | Cartão microSD com adaptador para SD de tamanho normal | 1 | 0 | 0 | 3 | não: width: status PENDENTE; length: status PENDENTE; thickness: status PENDENTE |
| `charger_module` | Módulo de carga com power-path (BQ24074 ou MCP73871) | 1 | 0 | 0 | 4 | não: pcb_length: status PENDENTE; pcb_width: status PENDENTE; total_thickness: status PENDENTE |
| `usb_c_receptacle` | Conector USB-C da PCB definitiva | 1 a 2 | 0 | 0 | 3 | não: shell_width: status PENDENTE; shell_depth: status PENDENTE; shell_height: status PENDENTE |
| `hex_standoff` | Standoff hexagonal de metal reaproveitado (de placa-mãe) | 1 | 0 | 0 | 3 | não: across_flats: status PENDENTE; body_length: status PENDENTE |
| `heat_set_insert` | Inserto roscado heat-set (plano B e demais parafusos) | — | 0 | 0 | 2 | não: outer_diameter: status PENDENTE; length: status PENDENTE |
| **Total** | 22 componentes | | 14 | 21 | 76 | 1 com modelo, 21 recusados |

## 4. Registro por componente

### `esp32_s3_wroom_1` — Módulo ESP32-S3-WROOM-1 (N16R8)

MCU da PCB definitiva (carcaça v1). Na v0 ele está soldado na DevKitC. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `esp32_s3_wroom_1.length` | Comprimento do módulo (inclui a região da antena) | 25,5 | mm | não publicada | CONFIRMADO (fabricante) | footprint oficial e modelo 3D oficial | `ESPRESSIF_KICAD_WROOM1_FP`, `ESPRESSIF_KICAD_WROOM1_STEP` | As duas fontes coincidem. Tolerância: consta do datasheet, não aberto nesta sessão. |
| `esp32_s3_wroom_1.width` | Largura do módulo | 18,0 | mm | não publicada | CONFIRMADO (fabricante) | footprint oficial e modelo 3D oficial | `ESPRESSIF_KICAD_WROOM1_FP`, `ESPRESSIF_KICAD_WROOM1_STEP` | As duas fontes coincidem. Tolerância não publicada nas fontes abertas. |
| `esp32_s3_wroom_1.thickness` | Espessura total (placa do módulo mais blindagem) | 3,1 | mm | não publicada | CONFIRMADO (fabricante) | modelo 3D oficial (caixa envolvente em Z) | `ESPRESSIF_KICAD_WROOM1_STEP` | Valor nominal do modelo. Tolerância não publicada nas fontes abertas; conferir no datasheet quando acessível. |
| `esp32_s3_wroom_1.pcb_thickness` | Espessura da placa do módulo (com cobre) | 0,8 | mm | não publicada | CONFIRMADO (fabricante) | modelo 3D oficial | `ESPRESSIF_KICAD_WROOM1_STEP` | No modelo, a base da blindagem está em z = 0,8 mm. |
| `esp32_s3_wroom_1.shield_length` | Blindagem: comprimento | 17,6 | mm | não publicada | CONFIRMADO (fabricante) | modelo 3D oficial | `ESPRESSIF_KICAD_WROOM1_STEP` | No modelo, começa a 1,05 mm da borda oposta à antena. |
| `esp32_s3_wroom_1.shield_width` | Blindagem: largura | 15,8 | mm | não publicada | CONFIRMADO (fabricante) | modelo 3D oficial | `ESPRESSIF_KICAD_WROOM1_STEP` | Centrada na largura (1,1 mm de cada lado). |
| `esp32_s3_wroom_1.shield_height` | Blindagem: altura acima da placa do módulo | 2,3 | mm | não publicada | CONFIRMADO (fabricante) | modelo 3D oficial | `ESPRESSIF_KICAD_WROOM1_STEP` | — |
| `esp32_s3_wroom_1.antenna_zone_length` | Região da antena: extensão ao longo do comprimento, a partir da ponta | 5,95 | mm | não publicada | CONFIRMADO (fabricante) | footprint oficial (zona 'antenna keepout') | `ESPRESSIF_KICAD_WROOM1_FP`, `ESPRESSIF_HDG_CLEARANCE_FIG` | A figura das diretrizes de hardware cota a mesma região como 6 mm. A região ocupa toda a largura do módulo. No modelo 3D, o cobre da antena vai de 19,42 a 25,07 mm a partir da borda oposta; o limite da zona no footprint fica a 19,55 mm. |
| `esp32_s3_wroom_1.antenna_clearance_min` | Folga mínima recomendada em volta da antena, dentro da carcaça | 15,0 | mm | não publicada | CONFIRMADO (fabricante) | diretrizes de hardware do fabricante | `ESPRESSIF_HDG_PCB_LAYOUT`, `ESPRESSIF_HDG_CLEARANCE_FIG` | Texto: 'A clearance of at least 15 mm is recommended in all directions.' É recomendação, não dimensão da peça. |
| `esp32_s3_wroom_1.pad_pitch` | Passo dos pads de borda | 1,27 | mm | não publicada | CONFIRMADO (fabricante) | footprint oficial | `ESPRESSIF_KICAD_WROOM1_FP` | 14 pads em cada lado maior e 12 no lado oposto à antena. |

### `esp32_s3_devkitc_1` — Placa ESP32-S3-DevKitC-1 N16R8

Placa principal da bancada e da carcaça v0. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `esp32_s3_devkitc_1.pcb_length` | Comprimento da placa (com a ponta da antena) | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O desenho cotado da Espressif existe (ver unreachable_sources) e não pôde ser aberto. Referência não cotada: a serigrafia do footprint oficial delimita 63,90 mm mais 5,89 mm da caixa da antena; serve de ordem de grandeza, não de cota. |
| `esp32_s3_devkitc_1.pcb_width` | Largura da placa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Referência não cotada: a serigrafia do footprint oficial delimita 25,86 mm. |
| `esp32_s3_devkitc_1.height_with_pins` | Altura total, da ponta dos pinos ao componente mais alto | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Pedido pelo README (Dimensões conhecidas). Medir também a altura sem os pinos. |
| `esp32_s3_devkitc_1.header_pitch` | Passo dos pinos | 2,54 | mm | não publicada | CONFIRMADO (fabricante) | footprint oficial | `ESPRESSIF_KICAD_DEVKITC_FP` | Vale para a placa oficial da Espressif. Conferir na placa em mãos. |
| `esp32_s3_devkitc_1.header_row_spacing` | Distância entre as duas fileiras de pinos | 22,86 | mm | não publicada | CONFIRMADO (fabricante) | footprint oficial | `ESPRESSIF_KICAD_DEVKITC_FP` | Vale para a placa oficial. Placas de terceiros com o mesmo nome podem ter outra distância: conferir na peça. |
| `esp32_s3_devkitc_1.header_pins_per_row` | Pinos por fileira | 22 | un | não publicada | CONFIRMADO (fabricante) | footprint oficial | `ESPRESSIF_KICAD_DEVKITC_FP` | Duas fileiras, 44 pinos. |
| `esp32_s3_devkitc_1.header_span` | Distância do primeiro ao último pino de uma fileira | 53,34 | mm | não publicada | CONFIRMADO (fabricante) | footprint oficial | `ESPRESSIF_KICAD_DEVKITC_FP` | Igual a 21 x 2,54 mm. |
| `esp32_s3_devkitc_1.usb_connector_positions` | Posição, tipo e saliência dos dois conectores USB | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O guia oficial descreve duas portas Micro-USB. Se a placa em mãos tiver USB-C, não é a revisão documentada. |
| `esp32_s3_devkitc_1.button_positions` | Posição e altura dos botões Boot e Reset | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `display_msp3218` — Módulo TFT 3,2 pol. ILI9341 320 x 240, MSP3218 (com toque)

Tela única: rosto, menu e status. O slot SD do verso recebe o cartão na v0. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `display_msp3218.pcb_length` | Placa: lado maior | 89,3 | mm | não publicada | NOMINAL (anúncio/designação) | README, que atribui o valor a um 'datasheet' não identificado | `README` | A régua do README deu 88 mm. A página do fabricante não pôde ser aberta; um resumo de busca cita o mesmo valor, sem valor de prova. |
| `display_msp3218.pcb_width` | Placa: lado menor | 55,04 | mm | não publicada | NOMINAL (anúncio/designação) | README, que atribui o valor a um 'datasheet' não identificado | `README` | A régua do README deu 54 mm. |
| `display_msp3218.active_area_length` | Área ativa: lado maior | 64,8 | mm | não publicada | NOMINAL (anúncio/designação) | README, que atribui o valor a um 'datasheet' não identificado | `README` | Coerente com a designação: diagonal de 81,0 mm (3,19 pol.) e proporção 4:3. |
| `display_msp3218.active_area_width` | Área ativa: lado menor | 48,6 | mm | não publicada | NOMINAL (anúncio/designação) | README, que atribui o valor a um 'datasheet' não identificado | `README` | — |
| `display_msp3218.glass_visible_length` | Vidro visível: lado maior | 70,0 | mm | não publicada | NOMINAL (anúncio/designação) | régua (README) | `README` | O README registra '~70'. |
| `display_msp3218.glass_visible_width` | Vidro visível: lado menor | 51,0 | mm | não publicada | NOMINAL (anúncio/designação) | régua (README) | `README` | O README registra '~51'. |
| `display_msp3218.total_thickness` | Espessura total com o painel de toque (sem os pinos) | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Pedido pelo README (Dimensões conhecidas). |
| `display_msp3218.pcb_thickness` | Espessura da placa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `display_msp3218.glass_outline` | Contorno externo do vidro e do painel de toque (lado maior e lado menor) | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Pedido pelo README (Dimensões conhecidas). |
| `display_msp3218.active_area_offset` | Posição do centro da área ativa em relação ao centro da placa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Define o centro da janela da tela. |
| `display_msp3218.mounting_holes` | Furos de fixação: diâmetro e posição | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `display_msp3218.sd_slot_envelope` | Slot SD do verso: posição, altura e saliência do cartão inserido | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O slot é de SD de tamanho normal (README); o microSD entra com adaptador. |
| `display_msp3218.pin_header_envelope` | Barra de pinos: posição, comprimento e altura abaixo da placa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `battery_4070115` — Bateria LiPo 1S, designação 4070115 (4000 mAh no anúncio)

Fonte de energia, na zona da tampa traseira. Compra não confirmada no README. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `battery_4070115.thickness` | Espessura da célula | 4,0 | mm | não publicada | NOMINAL (anúncio/designação) | designação 4070115 | `README` | O anúncio informa 0,4 mm; o README considera 4,0 mm provável. Medir a célula real, no ponto mais espesso. |
| `battery_4070115.width` | Largura da célula | 70,0 | mm | não publicada | NOMINAL (anúncio/designação) | anúncio e designação 4070115 | `README` | — |
| `battery_4070115.length` | Comprimento da célula | 115,0 | mm | não publicada | NOMINAL (anúncio/designação) | anúncio e designação 4070115 | `README` | Não se sabe se inclui a placa de proteção (PCM) e as abas. |
| `battery_4070115.length_with_pcm` | Comprimento total com PCM, abas e saída dos fios | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `battery_4070115.wire_exit` | Posição da saída dos fios e comprimento do chicote com o conector | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Dois ou três fios, conforme haja NTC. |

### `card_edge_socket_dk920a_20m` — Soquete card edge DK920A-20M (2 x 10 contatos, 2,54 mm)

Recebe o cartucho, numa riser no topo. Compra prevista para a Fase 4. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `card_edge_socket_dk920a_20m.body_length` | Corpo: comprimento | 31,7 | mm | não publicada | NOMINAL (anúncio/designação) | README, sem fonte citada | `README` | Datasheet não localizado. O README manda medir a peça real antes de desenhar a pegada. |
| `card_edge_socket_dk920a_20m.body_width` | Corpo: largura | 9,2 | mm | não publicada | NOMINAL (anúncio/designação) | README, sem fonte citada | `README` | — |
| `card_edge_socket_dk920a_20m.height_above_board` | Altura acima da placa | 10,2 | mm | não publicada | NOMINAL (anúncio/designação) | README, sem fonte citada | `README` | O README registra '~10,2'. |
| `card_edge_socket_dk920a_20m.row_spacing` | Distância entre as duas fileiras de terminais | 5,08 | mm | não publicada | NOMINAL (anúncio/designação) | README, sem fonte citada | `README` | — |
| `card_edge_socket_dk920a_20m.contact_pitch` | Passo dos contatos | 2,54 | mm | não publicada | NOMINAL (anúncio/designação) | designação do anúncio | `README` | 10 contatos por lado: 22,86 mm do primeiro ao último (9 x 2,54). |
| `card_edge_socket_dk920a_20m.slot_width` | Fenda: largura (na direção da espessura do cartucho) | 1,9 | mm | não publicada | NOMINAL (anúncio/designação) | README, sem fonte citada | `README` | Para placa de 1,6 mm, segundo o README. Conferir com uma sobra de PCB. |
| `card_edge_socket_dk920a_20m.slot_opening_length` | Fenda: comprimento da abertura | 28,3 | mm | não publicada | NOMINAL (anúncio/designação) | README, sem fonte citada | `README` | O README registra '~28,3'. |
| `card_edge_socket_dk920a_20m.pcb_hole_diameter` | Furo dos terminais na placa | 1,0 | mm | não publicada | NOMINAL (anúncio/designação) | README, sem fonte citada | `README` | O README registra '~1,0'. |
| `card_edge_socket_dk920a_20m.slot_depth` | Fenda: profundidade | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O README estima ~8 mm e manda medir na peça. Define o comprimento da língua do cartucho. |

### `pn532_nfc_v3` — Módulo NFC PN532 V3

NFC de 13,56 MHz. A antena é a própria placa. Aguardando entrega. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `pn532_nfc_v3.pcb_length` | Placa: lado maior | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O manual da Elechouse não pôde ser aberto. O item comprado é um 'PN532 Set' de anúncio; não assumir que tem as medidas do módulo da Elechouse. |
| `pn532_nfc_v3.pcb_width` | Placa: lado menor | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `pn532_nfc_v3.total_thickness` | Espessura total com componentes, chaves DIP e barra de pinos | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `pn532_nfc_v3.antenna_outline` | Contorno da bobina da antena na placa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Define a zona KO-NFC. |
| `pn532_nfc_v3.mounting_holes` | Furos de fixação: diâmetro e posição | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `cc1101_module` — Módulo CC1101 433 MHz com conector SMA

Rádio sub-GHz. Módulo genérico, sem desenho de fabricante. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `cc1101_module.pcb_length` | Placa: lado maior (sem o conector SMA) | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `cc1101_module.pcb_width` | Placa: lado menor | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `cc1101_module.total_thickness` | Espessura total com blindagem e barra de pinos | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `cc1101_module.sma_connector` | Conector SMA: posição na placa, saliência, diâmetro e comprimento da rosca | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Define o furo AB-10 e a espessura máxima da parede do topo. |
| `cc1101_module.antenna` | Antena: comprimento e diâmetro da base | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `mcp23017` — Expansor de I/O MCP23017

Lê os 16 botões por I2C. O README não informa o encapsulamento. Quantidade: 1 a 2.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `mcp23017.package` | Encapsulamento (identificar pela marcação da peça) | — | - | — | PENDENTE (medir com paquímetro) | — | — | Identificado o encapsulamento, as cotas saem do datasheet da Microchip, que não pôde ser aberto nesta sessão. |
| `mcp23017.body_length` | Corpo: comprimento | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `mcp23017.body_width` | Largura com os terminais | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `mcp23017.body_height` | Altura acima da placa (com soquete, se houver) | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `inmp441_breakout` — Placa do microfone INMP441 (I2S)

Captação de voz, junto ao furo do topo. Módulo genérico. Aguardando entrega. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `inmp441_breakout.pcb_length` | Placa: lado maior ou diâmetro | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `inmp441_breakout.pcb_width` | Placa: lado menor | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `inmp441_breakout.total_thickness` | Espessura total com pinos | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `inmp441_breakout.acoustic_port` | Furo acústico: posição na placa, face em que fica e diâmetro | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Define o furo AB-11. |

### `max98357a_breakout` — Placa do amplificador MAX98357A (I2S)

Amplificador do alto-falante. Módulo genérico. Aguardando entrega. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `max98357a_breakout.pcb_length` | Placa: lado maior | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `max98357a_breakout.pcb_width` | Placa: lado menor | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `max98357a_breakout.total_thickness` | Espessura total com pinos e borne | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `speaker_2030` — Alto-falante 2030, 8 ohms, 2 W

Saída de áudio, atrás da grade frontal. Aguardando entrega. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `speaker_2030.length` | Lado maior | 30,0 | mm | não publicada | NOMINAL (anúncio/designação) | designação '2030' | `README` | Leitura usual da designação (20 x 30 mm). O README não traz medidas e pede conferir. |
| `speaker_2030.width` | Lado menor | 20,0 | mm | não publicada | NOMINAL (anúncio/designação) | designação '2030' | `README` | — |
| `speaker_2030.thickness` | Espessura total | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `speaker_2030.terminals` | Posição dos terminais e dos fios | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `ir_kit` — Conjunto IR: emissor e receptor de 38 kHz

Infravermelho, atrás da janela AB-07. Aguardando entrega; conferir se o conjunto traz as duas peças. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `ir_kit.length` | Maior dimensão de cada peça (emissor e receptor) | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `ir_kit.width` | Largura de cada peça | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `ir_kit.height` | Altura de cada peça | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `ir_kit.optical_axis` | Diâmetro do LED, face sensível do receptor e posição do eixo óptico | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `analog_stick_ps3` — Analógico reaproveitado de controle genérico de PS3

Dois sticks, um de cada lado, na parte inferior da frente. Peça não identificada. Quantidade: 2.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `analog_stick_ps3.base_length` | Base: comprimento, com os potenciômetros | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O README pede medir área e altura com a capa, com o stick já fora do controle. |
| `analog_stick_ps3.base_width` | Base: largura, com o botão de clique | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `analog_stick_ps3.height_with_cap` | Altura total, da face de assentamento ao topo da capa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Entra na espessura da carcaça v0 (README). |
| `analog_stick_ps3.cap_diameter` | Diâmetro da capa e do colar | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Define o furo AB-05. |
| `analog_stick_ps3.tilt_envelope` | Ângulo máximo de inclinação e deslocamento lateral da capa | — | grau | — | PENDENTE (medir com paquímetro) | — | — | — |
| `analog_stick_ps3.carrier_outline` | Contorno do pedaço de placa, se o stick for cortado com a placa do controle | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Alternativa de retirada descrita no README; fica um pouco maior. |

### `tactile_switch` — Botão táctil (cruz, ações, select/start)

Estoque próprio; modelo não identificado. Quantidade: 10.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `tactile_switch.body_length` | Corpo: comprimento | 6,0 | mm | não publicada | NOMINAL (anúncio/designação) | designação '6 x 6 mm' citada no README | `README` | O README manda confirmar dimensões e quantidade na montagem. |
| `tactile_switch.body_width` | Corpo: largura | 6,0 | mm | não publicada | NOMINAL (anúncio/designação) | designação '6 x 6 mm' citada no README | `README` | — |
| `tactile_switch.total_height` | Altura total, da placa ao topo do atuador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Define o espaçador sob cada tampa. |
| `tactile_switch.travel` | Curso do atuador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `tactile_switch_right_angle` — Botão táctil de ângulo reto (gatilhos de ombro)

Estoque próprio, se houver; senão, comprar. Quantidade: 2 a 4.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `tactile_switch_right_angle.body_length` | Corpo: comprimento | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `tactile_switch_right_angle.body_depth` | Profundidade com o atuador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `tactile_switch_right_angle.body_height` | Altura acima da placa e altura do eixo do atuador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `power_button` — Botão momentâneo de energia

Numa lateral. Peça a definir com o esquema elétrico. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `power_button.body_length` | Corpo: comprimento | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `power_button.body_width` | Corpo: largura | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `power_button.total_height` | Altura total com o atuador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `button_caps_ps3` — Tampas reaproveitadas do controle de PS3

Cruz, quatro botões de ação, L1/R1, alavancas L2/R2 e botão redondo, se forem usados (decisões D-10 e D-11). Quantidade: 1 conjunto.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `button_caps_ps3.body_diameter` | Diâmetro ou contorno do corpo de cada tampa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O furo da carcaça sai desta medida, com folga de ~0,2 a 0,3 mm (README). |
| `button_caps_ps3.flange` | Diâmetro ou contorno da aba de cada tampa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `button_caps_ps3.total_height` | Altura total de cada tampa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Define o espaçador impresso. |
| `button_caps_ps3.dpad_pivot` | Cruz de peça única: posição e altura do pino central | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `button_caps_ps3.trigger_lever` | Alavancas L2/R2: comprimento, largura, eixo do pivô e curso | — | mm | — | PENDENTE (medir com paquímetro) | — | — | O README alerta que podem ser grandes demais para o topo do corpo vertical. |

### `sd_card_with_adapter` — Cartão microSD com adaptador para SD de tamanho normal

Armazenamento na v0, no slot do verso da tela. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `sd_card_with_adapter.width` | Largura do adaptador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | A especificação física da SD Association não foi aberta nesta sessão. Medir a peça. |
| `sd_card_with_adapter.length` | Comprimento do adaptador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `sd_card_with_adapter.thickness` | Espessura do adaptador | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `charger_module` — Módulo de carga com power-path (BQ24074 ou MCP73871)

Fase 2 de compras. Valida a energia na bancada e entra na carcaça v0. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `charger_module.pcb_length` | Placa: lado maior | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Peça ainda não escolhida nem comprada. |
| `charger_module.pcb_width` | Placa: lado menor | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `charger_module.total_thickness` | Espessura total com conectores | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `charger_module.usb_connector_position` | Posição e saliência do conector USB do módulo | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Na v0, define a abertura AB-14. |

### `usb_c_receptacle` — Conector USB-C da PCB definitiva

Dados e carga (uma ou duas portas, decisão D-4). Peça a definir no esquema. Quantidade: 1 a 2.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `usb_c_receptacle.shell_width` | Carcaça metálica: largura | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Sai do datasheet do conector escolhido. |
| `usb_c_receptacle.shell_depth` | Carcaça metálica: profundidade e saliência além da borda da placa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `usb_c_receptacle.shell_height` | Carcaça metálica: altura e altura do eixo acima da placa | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

### `hex_standoff` — Standoff hexagonal de metal reaproveitado (de placa-mãe)

Rosca do parafuso da tampa traseira, embutido em furo hexagonal. Quantidade: 1.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `hex_standoff.across_flats` | Distância entre faces do sextavado | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Medir também a distância entre vértices. |
| `hex_standoff.body_length` | Comprimento do corpo sextavado | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |
| `hex_standoff.thread` | Rosca interna e rosca do pino, se houver | — | - | — | PENDENTE (medir com paquímetro) | — | — | Identificar com um parafuso conhecido. Não assumir M3. |

### `heat_set_insert` — Inserto roscado heat-set (plano B e demais parafusos)

M2 ou M3 (decisão D-16). Peça a definir. Quantidade: —.

| ID | Dimensão | Valor | Un. | Tolerância | STATUS | Origem do valor | Fonte | Observação |
|---|---|---:|---|---|---|---|---|---|
| `heat_set_insert.outer_diameter` | Diâmetro externo e diâmetro do furo recomendado | — | mm | — | PENDENTE (medir com paquímetro) | — | — | Sai do datasheet do inserto escolhido. Quantidade não definida no README. |
| `heat_set_insert.length` | Comprimento | — | mm | — | PENDENTE (medir com paquímetro) | — | — | — |

## 5. Parâmetros definidos pelo projeto

Não são medidas de peça: são decisões do README. Não têm STATUS e não viram geometria no gerador.

| ID | Parâmetro | Valor | Un. | Fonte | Observação |
|---|---|---:|---|---|---|
| `cartridge_pcb_length` | Cartucho: comprimento da placa | 52,0 | mm | README, Especificação do cartucho | Em aberto: cartucho curto de 52 x 25 mm (decisão D-8). |
| `cartridge_pcb_width` | Cartucho: largura da placa | 52,0 | mm | README, Especificação do cartucho | — |
| `cartridge_pcb_thickness` | Cartucho: espessura da placa | 1,6 | mm | README, Especificação do cartucho | — |
| `cartridge_tongue_width` | Cartucho: largura da língua | 27,8 | mm | README, Especificação do cartucho | O README registra '~27,8', derivado da abertura do soquete (~28,3 mm), que é NOMINAL. |
| `cartridge_contact_span` | Cartucho: distância do primeiro ao último contato de um lado | 22,86 | mm | README, Especificação do cartucho | 9 x 2,54 mm. |
| `cartridge_contact_edge_offset` | Cartucho: distância dos contatos à borda | 0,6 | mm | README, Especificação do cartucho | — |
| `cartridge_protrusion` | Estimativa histórica de saliência do cartucho (não usar como cota final) | 40,0 | mm | README, Especificação do cartucho (estimativa anterior) | Valor histórico de ~40 mm; não representa o envelope da carcaça plástica e não deve ser usado no CAD. Recalcular após medir o soquete, definir batente, guarda e envelope externo. |
| `battery_bay_thickness_margin` | Berço da bateria: folga na espessura, como fração da espessura medida | 0,1 | fração | README, Riscos da bateria e recomendações, item 7 | O README registra '~10%'. |
| `button_cap_hole_clearance_min` | Furo das tampas de botão: folga mínima | 0,2 | mm | README, Analógicos e gatilhos reaproveitados | O README registra '~0,2 a 0,3 mm'. |
| `button_cap_hole_clearance_max` | Furo das tampas de botão: folga máxima | 0,3 | mm | README, Analógicos e gatilhos reaproveitados | — |

<!-- END: REGISTRO-GERADO -->

## 6. Como medir e como registrar

Procedimento proposto por este documento (não vem do README). Instrumento: paquímetro com resolução de 0,05 mm ou melhor. Medida de régua não promove uma linha.

1. Medir cada cota três vezes, em pontos diferentes da peça, e anotar as três leituras.
2. Para envelope (o que ocupa espaço na carcaça), registrar a **maior** leitura. Para encaixe (o que
   precisa entrar em algo), registrar também a menor.
3. Em placas com componentes, medir separadamente: contorno da placa, espessura só da placa e altura
   total em cada face (componente mais alto, pinos, conectores).
4. Em peças com parte funcional (antena, conector, furo acústico, eixo óptico, atuador), medir a
   posição dessa parte em relação a duas bordas da peça.
5. Bateria: medir a espessura no ponto mais espesso, sem apertar o paquímetro contra a célula. Medir
   largura e comprimento com e sem a placa de proteção.
6. Cota de controle das linhas `CONFIRMADO`: na DevKitC, a distância entre as duas fileiras de pinos
   (22,86 mm na placa oficial); no módulo WROOM-1, largura e comprimento da blindagem ou do módulo.
   Se a leitura divergir, a peça não é a do desenho oficial: rebaixar as linhas e medir tudo.
7. Registrar em `mechanical/cad/components.yaml` e rodar `render_register.py` para atualizar este
   documento.

Segundo o README, já estão em mãos e podem ser medidos agora: a DevKitC, a tela, o módulo CC1101, os
MCP23017, os botões tácteis, o controle de PS3 (analógicos e tampas), o cartão microSD e os
standoffs. Os itens comprados em 02/11/2026 aguardavam entrega; a compra da bateria não estava
confirmada e o soquete do cartucho estava fora do carrinho (Fase 4).

Ponto em aberto: este registro ainda não tem STATUS para "medido com paquímetro". Os três STATUS
acima são os definidos para esta etapa, e o gerador de CAD só aceita `CONFIRMADO`. Antes de registrar
as primeiras medições, o mantenedor precisa decidir como elas entram (por exemplo, um quarto STATUS
com data, instrumento e autor da medição) e se o gerador pode modelar a partir delas. Até lá, uma
medição não deve ser gravada como `CONFIRMADO`, que significa "documento de fabricante".

## 7. Rastreio das medidas que o README já traz

O README, na seção *Dimensões conhecidas* e em outras, já cita medidas. Esta tabela mostra o que foi
feito com cada uma.

| Medida no README | O que o README diz da origem | O que foi encontrado nesta sessão | Resultado |
|---|---|---|---|
| Placa da tela 89,3 × 55,04 mm | "datasheet", sem identificar o documento | A página do módulo na LCD Wiki não pôde ser aberta (seção 2.2). Um resumo de busca atribui a ela 55,04 × 89,3 mm; resumo não é fonte | `NOMINAL` |
| Placa da tela 88 × 54 mm | Régua | — | Anotado na observação das linhas acima; diverge do valor anterior em 1,3 e 1,04 mm |
| Vidro visível ~70 × 51 mm | Régua | — | `NOMINAL` |
| Área ativa 64,8 × 48,6 mm | "datasheet", sem identificar o documento | Mesma situação da placa. O resumo de busca cita 48,6 × 64,8 mm. Coerente por aritmética com 3,2 polegadas e 4:3 | `NOMINAL` |
| Bateria 0,4 × 70 × 115 mm | Anúncio; o README já aponta que 4,0 mm é o provável | Nenhum datasheet da célula | `NOMINAL`, com 4,0 mm vindo da designação 4070115 |
| DevKitC: "medir com paquímetro" | Sem medida | O desenho cotado da Espressif existe e não pôde ser aberto. O footprint oficial deu o padrão de pinos | Contorno e altura `PENDENTE`; passo, fileiras e número de pinos `CONFIRMADO` |
| Cartucho 52 × 52 mm, 1,6 mm, língua ~27,8 mm | Decisão do projeto | — | Parâmetro de projeto (seção 5), sem STATUS |
| Soquete DK920A-20M: corpo 31,7 × 9,2 mm, altura ~10,2 mm, fileiras a 5,08 mm, fenda 1,9 mm, abertura ~28,3 mm, furos ~1,0 mm | Sem fonte citada | Fabricante não identificado; a busca pelo código não devolveu datasheet | `NOMINAL` |
| Soquete: profundidade da fenda ~8 mm | Estimativa, "a medir na peça" | — | `PENDENTE` |
| Botões tácteis de 6 × 6 mm | Designação, em *Analógicos e gatilhos reaproveitados* | Modelo do estoque não identificado | Corpo `NOMINAL`, altura `PENDENTE` |
| Alto-falante "2030" | Designação | O README não traz medidas | 20 × 30 mm `NOMINAL` pela leitura usual da designação; espessura `PENDENTE` |

## 8. Notas sobre as fontes da Espressif

- A biblioteca `espressif/kicad-libraries` é mantida pela Espressif. O README dela declara que os
  footprints seguem o padrão de pads recomendado no datasheet de cada módulo (`ESPRESSIF_KICAD_README`).
- O contorno do módulo no footprint (18 × 25,5 mm) e a caixa envolvente do modelo 3D
  (18,0 × 25,5 × 3,1 mm) são duas fontes do mesmo fabricante que coincidem. A caixa envolvente foi
  medida nesta sessão com cadquery 2.8.0 sobre o arquivo STEP, cujo cabeçalho traz o nome
  `ESP32-S3-WROOM-1_20211021.STEP`.
- A biblioteca é licenciada em CC-BY-SA-4.0 com exceção para projetos que a usam. Este repositório
  **não** redistribui os arquivos dela: registra medidas lidas e cita a origem.
- No footprint da DevKitC, a serigrafia delimita 25,86 × 63,90 mm, mais uma caixa de
  17,98 × 5,89 mm para a ponta do módulo. Serigrafia não é contorno de placa: esses números ficam
  como ordem de grandeza na observação e **não** entram como dimensão.
- O guia oficial lista a variante `ESP32-S3-DevKitC-1-N16R8` (módulo ESP32-S3-WROOM-1, 16 MB de
  flash, 8 MB de PSRAM, antena de trilha) e descreve duas portas Micro-USB (`ESPRESSIF_DEVKITC_GUIDE`).
