<!--
SPDX-FileCopyrightText: 2026 Gabriel Almeida Resende
SPDX-License-Identifier: Apache-2.0
-->

# d-BMO-body v1 — Disposição de montagem (zonas, empilhamento e parâmetros)

| Campo | Valor |
|---|---|
| Status | Preliminar (revisão 0.2). Descreve **zonas**, **restrições** e a nova interface de cartucho protegido; não define dimensões sem medição |
| Data | 2026-11-12 |
| Referencial | RC-dBMO, definido em [MECHANICAL_SPECIFICATION.md](MECHANICAL_SPECIFICATION.md), seção 3. Este documento não redefine eixos nem origem |
| Dimensões das peças | [COMPONENT_DIMENSIONS.md](COMPONENT_DIMENSIONS.md) |

## 1. Regras deste documento

1. Coordenada numérica só aparece quando sai de dimensões `CONFIRMADO`. Hoje isso vale apenas para o
   módulo ESP32-S3-WROOM-1 e para o padrão de pinos da DevKitC oficial (seção 6).
2. Todo o resto é **parâmetro nomeado**, sem valor. A seção 7 lista cada parâmetro, o que ele
   significa e o que precisa ser medido ou decidido para preenchê-lo.
3. Dimensões de peças são citadas pelo ID do registro, em minúsculas (por exemplo
   `display_msp3218.pcb_length`). Parâmetros de posição e de carcaça ficam em maiúsculas (por exemplo
   `DISP_CY`).
4. Nenhuma posição aqui é decisão de projeto. As fontes definem a **ordem relativa** dos elementos
   (o que fica acima, ao lado, entre); este documento registra essa ordem e as restrições.

As marcações `[R: seção]`, `[M: §n]`, `[F: id]`, **(derivado)**, **(interpretação)** e **(hipótese)**
seguem [MECHANICAL_SPECIFICATION.md](MECHANICAL_SPECIFICATION.md), seção 2.2.

## 2. Zonas

Lembrete do referencial: +X para a direita do usuário, +Y para cima (borda do cartucho), +Z saindo
da face frontal; interior em Z < 0.

### 2.1 Planos de divisão (parâmetros)

| Parâmetro | Significado |
|---|---|
| `Y_CTRL_TOP` | Altura Y do limite entre a zona da tela e a zona dos controles |
| `Y_STICK_TOP` | Altura Y do limite entre a zona dos controles e a zona dos analógicos |
| `Y_DOOR_TOP` | Altura Y da linha de divisão da traseira: tampa abaixo, zona fixa acima (decisão D-18) |
| `TOP_BAND_H` | Altura da faixa do topo reservada ao soquete, à riser, ao conector SMA, ao microfone e aos gatilhos |
| `Z_PART` | Plano Z de separação entre a casca frontal e a casca traseira |

Relações de ordem conhecidas: `0 < Y_STICK_TOP < Y_CTRL_TOP < BODY_H − TOP_BAND_H < BODY_H` e
`−BODY_D < Z_PART < 0`. A tela ocupa a metade superior `[M: §2]`, o que situa `Y_CTRL_TOP` perto da
metade de `BODY_H`; o valor não está definido.

### 2.2 Tabela de zonas

| Zona | Face | Faixa em Y | Conteúdo | Fonte |
|---|---|---|---|---|
| `ZF1` Tela | `FR` | de `Y_CTRL_TOP` a `BODY_H − TOP_BAND_H` | Janela da tela com moldura preta | `[M: §2]`, `[R: Carcaça 3D]` |
| `ZF2` Controles | `FR` | de `Y_STICK_TOP` a `Y_CTRL_TOP` | Cruz direcional em X < 0; quatro botões de ação em losango em X > 0; select e start no centro; LED de estado, se mantido (D-17) | `[M: §2]`, `[R: Carcaça 3D]` |
| `ZF3` Analógicos e áudio | `FR` | de 0 a `Y_STICK_TOP` | Um analógico em X < 0 e outro em X > 0; grade do alto-falante entre eles | `[M: §2]`, `[R: Carcaça 3D]` |
| `ZT` Topo | `TP` | de `BODY_H − TOP_BAND_H` a `BODY_H` | Fenda do cartucho no centro; saída SMA; furo do microfone; gatilhos de ombro | `[R: Carcaça 3D]` |
| `ZR1` Traseira fixa | `TR` | de `Y_DOOR_TOP` a `BODY_H` | Eletrônica; parafusada | `[R: Carcaça 3D]` |
| `ZR2` Tampa traseira | `TR` | de 0 a `Y_DOOR_TOP` | Bateria; berços dos cartuchos reserva; slot do microSD da PCB definitiva | `[R: Carcaça 3D]` |
| `ZL` Lateral | `LE` ou `LD` | a definir | Uma ou duas portas USB-C; botão de energia; LED de carga | `[R: Carcaça 3D]`, `[R: Conector de carga USB-C dedicado]` |

Observações:

- `Y_DOOR_TOP` e `Y_CTRL_TOP` são independentes: nenhuma fonte diz que a linha da tampa coincide com
  o limite inferior da tela.
- Select e start: `[M: §2]` diz "centro inferior" e `[R: Carcaça 3D]` diz "no centro", depois da cruz
  e dos botões de ação. A zona exata (`ZF2` ou `ZF3`) fica em aberto junto com `SEL_CY`.
- A janela do IR não tem zona atribuída enquanto a decisão D-9 estiver aberta.

## 3. Colunas de empilhamento

A espessura `BODY_D` é ditada pela coluna mais alta. As colunas abaixo listam, da frente (Z = 0) para
trás (Z = −`BODY_D`), o que se empilha em cada região. A ordem vem das fontes; as espessuras são
parâmetros ou linhas do registro.

### 3.1 Coluna A — tela e eletrônica (regiões `ZF1` e `ZR1`)

| Ordem | Camada | Espessura | Situação |
|---|---|---|---|
| A1 | Parede frontal com a moldura da tela | `T_WALL_FR` | Parâmetro (D-15) |
| A2 | Tela, do vidro à face traseira da placa | `display_msp3218.total_thickness` | `PENDENTE` |
| A3 | Atrás da placa da tela: barra de pinos, slot SD e, na v0, o cartão com adaptador | `display_msp3218.pin_header_envelope`, `display_msp3218.sd_slot_envelope` | `PENDENTE` |
| A4 | Folga para fios (v0) | `GAP_WIRING` | Parâmetro |
| A5 | v0: DevKitC e módulos. v1: PCB definitiva com o módulo WROOM-1 | `esp32_s3_devkitc_1.height_with_pins` (v0); na v1, depende da PCB | `PENDENTE` (v0). No módulo WROOM-1, a espessura de 3,1 mm é `CONFIRMADO` |
| A6 | Antena do NFC, se ficar nesta coluna (opção NFC-A, seção 5.4) | `NFC_METAL_GAP` + `pn532_nfc_v3.total_thickness` + `NFC_WALL_GAP` | Parâmetros e `PENDENTE` |
| A7 | Parede traseira fixa | `T_WALL_TR` | Parâmetro (D-15) |

### 3.2 Coluna B — controles e bateria (regiões `ZF2`, `ZF3` e `ZR2`)

| Ordem | Camada | Espessura | Situação |
|---|---|---|---|
| B1 | Parede frontal, atravessada pelas tampas dos botões e pelos analógicos | `T_WALL_FR` | Parâmetro |
| B2 | Tampa do botão, espaçador impresso e botão táctil | `button_caps_ps3.total_height`, `tactile_switch.total_height` | `PENDENTE` |
| B2' | Na região dos analógicos: altura do stick abaixo da face frontal | `analog_stick_ps3.height_with_cap − STK_PROTRUSION` | `PENDENTE` e parâmetro |
| B2'' | Na região do alto-falante: alto-falante e cavidade acústica | `speaker_2030.thickness` + `SPK_CAVITY` | `PENDENTE` e parâmetro |
| B3 | Suporte dos botões e dos sticks (placa ou peça impressa) | `T_CTRL_CARRIER` | Parâmetro; a v0 liga os módulos por fio e não define esse suporte |
| B4 | Divisória entre os controles e o berço da bateria | `T_PARTITION` | Parâmetro |
| B5 | Berço da bateria | `BAT_BAY_T` = (1 + `battery_bay_thickness_margin`) × espessura **medida** da célula | Regra de `[R: Riscos da bateria, item 7]`; espessura `NOMINAL` |
| B6 | Tampa traseira | `T_DOOR` | Parâmetro |

**(derivado)** A divisória B4 existe porque nenhuma aresta pode encostar na célula e a bateria não
pode ser pressionada `[R: Riscos da bateria]`: pinos de solda dos botões e dos sticks não podem ficar
voltados para a célula sem uma parede lisa entre eles.

### 3.3 Coluna C — topo (região `ZT`), ao longo de Y

O cartucho entra em −Y. O corpo plástico é guiado pela abertura superior; somente a
língua card edge sem a guarda entra na fenda elétrica. De cima para baixo:

| Ordem | Camada | Extensão em Y | Situação |
|---|---|---|---|
| C1 | Parede do topo, boca chanfrada e entrada dos canais laterais | T_WALL_TP + parâmetros da abertura | Parâmetro; cotas pendentes |
| C2 | Corpo plástico do cartucho, guardas laterais e guias do alojamento | Envelope externo do cartucho e GUIDE_LEN | Ainda sem dimensões |
| C3 | Soquete DK920A-20M, com a língua card edge engatada | card_edge_socket_dk920a_20m.height_above_board | NOMINAL; medir peça real |
| C4 | Riser perpendicular à PCB principal, terminais e apoio estrutural | T_RISER + RISER_LINK_H | Parâmetros de projeto da PCB |

TOP_BAND_H precisa acomodar o corpo plástico inserido, a abertura/guia, o soquete, a riser
e o apoio, sem colisão com SMA, microfone e gatilhos. O batente do cartucho deve encostar na
estrutura do d-BMO na posição de engate completo; o corpo do soquete recebe apoio no lado
oposto à boca. A força não deve ficar concentrada nas 20 soldas.

### 3.4 Condição de espessura

`BODY_D` ≥ maior soma entre a coluna A e as variantes da coluna B. Nenhuma das somas pode ser
calculada hoje: cada uma tem pelo menos uma linha `PENDENTE`.

## 4. Restrições de vizinhança e zonas de exclusão

| ID | Restrição | Fonte |
|---|---|---|
| AD-01 | A ponta da DevKitC com a antena de trilha fica perto de uma borda da carcaça, longe da bateria, do alto-falante e de metal | `[R: Carcaça v0]`, `[R: Lista de componentes]` |
| AD-02 | `KO-ESP`: 15 mm livres em volta da região da antena, em todas as direções | `[F: ESPRESSIF_HDG_PCB_LAYOUT]` |
| AD-03 | A antena do PN532 fica virada para fora, sem metal atrás | `[R: Carcaça 3D]` |
| AD-04 | A bateria fica longe do regulador, do amplificador e da S3 | `[R: Riscos da bateria, item 7]` |
| AD-05 | O alto-falante fica entre os analógicos | `[M: §2]`, `[R: Carcaça 3D]` |
| AD-06 | No topo, cartucho no centro; saída SMA e furo do microfone distribuídos para não colidir com os gatilhos | `[R: Carcaça 3D]` |
| AD-07 | O corpo do soquete encosta num apoio da carcaça | `[R: Especificação do cartucho]` |
| AD-08 | A fenda da carcaça tem o mesmo deslocamento da língua do cartucho (anti-inversão) | `[R: Especificação do cartucho]` |
| AD-09 | Portas USB-C e botão de energia numa lateral; a porta de carga e a porta de dados em posições distintas | `[R: Carcaça 3D]`, `[R: Conector de carga USB-C dedicado]` |
| AD-10 | O LED de carga fica ao lado da porta de carga | `[R: Conector de carga USB-C dedicado]` |
| AD-11 | v0: o cartão fica no slot SD atrás da tela. v1: slot de microSD dentro da zona da tampa | `[R: Lista de componentes]`, `[R: Carcaça 3D]` |
| AD-12 | Os berços dos cartuchos reserva ficam na zona da tampa e não tocam a célula | `[R: Carcaça 3D]`; (derivado de BT-3) |
| AD-13 | Parafusos, standoffs e insertos ficam fora de `KO-ESP` e de `KO-NFC` e não alcançam a célula | (derivado; ver especificação, seções 7 e 8) |
| AD-14 | A riser do soquete é perpendicular à PCB principal e fica no topo | `[R: Carcaça 3D]` |
| AD-15 | Os gatilhos L1/R1 usam botão táctil de ângulo reto; as alavancas L2/R2 precisam de apoio do pivô impresso e de um botão táctil embaixo | `[R: Analógicos e gatilhos reaproveitados]` |
| AD-16 | A cruz de peça única precisa de apoio do pino central e de quatro botões tácteis sob as pontas | `[R: Analógicos e gatilhos reaproveitados]` |

Conflitos já visíveis **(derivado)**; nenhum deles está resolvido:

1. **Antena do ESP32 e a tela.** Na coluna A, a DevKitC fica atrás da tela. Se a tela tiver moldura
   metálica (conferir na peça), a região da antena só fica a 15 mm dela se a ponta da placa
   ultrapassar a borda da tela. Isso pode não caber em `BODY_W`. A recomendação do fabricante é
   recomendação, com teste de alcance no produto final `[F: ESPRESSIF_HDG_PCB_LAYOUT]`.
2. **Antena do ESP32 e o topo.** O topo concentra metal: conector SMA, placa do CC1101, soquete do
   cartucho e, com cartucho de rádio, outra antena. Orientar a ponta da antena para o topo tende a
   violar `KO-ESP`.
3. **NFC e metal.** Atrás da traseira ficam a eletrônica (acima) e a bateria (abaixo). Nas duas
   posições há metal atrás da antena (seção 5.4).
4. **Bateria e eletrônica.** Com o lado maior da célula em Y, a zona da tampa precisa conter o
   comprimento da bateria (`NOMINAL`) mais as folgas. Isso empurra `Y_DOOR_TOP` para cima e reduz a
   zona fixa da eletrônica, ou deixa parte da célula atrás da eletrônica, o que contraria AD-04.
   Depende de `BODY_H` e da decisão D-3.
5. **Topo.** Cartucho de 52 mm de largura (parâmetro de projeto), conector SMA, microfone e dois ou
   quatro gatilhos disputam a largura `BODY_W` (EC-4).

## 5. Posicionamento por componente

Cada tabela indica a zona, a referência de posição, os parâmetros a preencher e o que medir.

### 5.1 Tela (`display_msp3218`)

| Item | Definição |
|---|---|
| Zona | `ZF1` |
| Orientação | Lado maior em X (320 × 240, horizontal), conforme `[M: §2]` e o firmware do rosto; formalmente em aberto (D-2) |
| Referência | Centro da **área ativa**, não da placa |
| Parâmetros | `DISP_CX`, `DISP_CY`, `DISP_ROT`, `DISP_Z_GLASS`, `BEZEL_X`, `BEZEL_Y` |
| Medir | `total_thickness`, `glass_outline`, `active_area_offset`, `mounting_holes`, `sd_slot_envelope`, `pin_header_envelope` |
| Nota | `DISP_CX` = 0 é a posição coerente com a silhueta simétrica de `[M: §2]` **(interpretação)**. A janela tem de mostrar a área ativa inteira; a moldura preta cobre o resto do vidro |

### 5.2 Placa principal: DevKitC na v0, PCB definitiva na v1

| Item | Definição |
|---|---|
| Zona | `ZR1`, coluna A |
| Orientação | Em aberto: eixo maior da placa em X ou em Y (`MCU_AXIS`), e para que lado fica a ponta da antena (`MCU_ANT_DIR`) |
| Referência | v0: centro do padrão de pinos (seção 6.2). v1: centro do módulo WROOM-1 (seção 6.1) |
| Parâmetros | `MCU_X`, `MCU_Y`, `MCU_Z`, `MCU_AXIS`, `MCU_ANT_DIR`, `MCU_FACE` |
| Medir | `esp32_s3_devkitc_1.pcb_length`, `.pcb_width`, `.height_with_pins`, `.usb_connector_positions`, `.button_positions`; posição do padrão de pinos em relação ao contorno da placa |
| Restrições | AD-01, AD-02, AD-04. Na v0, os conectores USB da placa precisam de abertura ou de extensão até a lateral |
| Nota | A posição dos conectores USB em relação à antena, na placa oficial, **não foi verificada nesta sessão**; conferir na peça |

### 5.3 Bateria (`battery_4070115`)

| Item | Definição |
|---|---|
| Zona | `ZR2`, coluna B, atrás da divisória |
| Orientação | `BAT_AXIS`: lado maior em Y ou em X (D-3) |
| Parâmetros | `BAT_CX`, `BAT_CY`, `BAT_AXIS`, `BAT_BAY_T`, `BAT_GAP_XY`, `BAT_WIRE_SIDE` |
| Medir | Espessura, largura e comprimento reais; `length_with_pcm`; `wire_exit` |
| Restrições | BT-1 a BT-9 da especificação; AD-04, AD-12, AD-13 |

### 5.4 NFC (`pn532_nfc_v3`)

| Item | Definição |
|---|---|
| Zona | Em aberto (D-6) |
| Opção NFC-A | Na parede traseira fixa (`ZR1`). Metal atrás: eletrônica e tela |
| Opção NFC-B | Na tampa traseira (`ZR2`). Metal atrás: bateria. Contraria BT-9 se a antena ficar sobre a célula |
| Parâmetros | `NFC_ZONE`, `NFC_CX`, `NFC_CY`, `NFC_WALL_GAP`, `NFC_METAL_GAP` |
| Medir | Contorno da placa, `antenna_outline`, `total_thickness`, `mounting_holes` |
| Como fechar | Teste de leitura com as tags reais, com tela e bateria montadas, variando `NFC_METAL_GAP` |
| Nota | **(hipótese)** Folha de ferrite entre a antena e o metal é uma mitigação conhecida; não foi consultada fonte sobre isso nesta sessão |

### 5.5 Rádio sub-GHz (`cc1101_module`)

| Item | Definição |
|---|---|
| Zona | `ZT` ou `ZR1`, junto à parede do topo |
| Parâmetros | `CC_X`, `CC_Y`, `CC_Z`, `CC_LINK` (conector direto na parede ou cabo), `SMA_X`, `SMA_Z` |
| Medir | Contorno da placa, `sma_connector`, `total_thickness`, `antenna` |
| Restrições | AD-06; a haste da antena não pode cruzar o curso dos gatilhos nem o cartucho |

### 5.6 Cartucho protegido, soquete e riser

| Item | Definição |
|---|---|
| Zona | ZT, centro |
| Referência | Plano médio da língua card edge, paralelo ao datum A; envelope externo da carcaça como referência de manuseio |
| Parâmetros | CART_X0, CART_OFFSET, CART_Z, CART_BODY_W, CART_BODY_H, CART_BODY_D, CART_WALL_T, CART_CAVITY_W, CART_CAVITY_H, CART_TONGUE_W, CART_TONGUE_T, CART_TONGUE_LEN, CONTACT_RECESS, SLOT_W, SLOT_T, SLOT_GUIDE_W, SLOT_GUIDE_T, SLOT_STOP_Z, SLOT_KEY_X, CART_RETENTION, CART_GRIP, SOCK_Y_SEAT, T_RISER, RISER_LINK_H, GUIDE_LEN |
| Medir | Todas as linhas de card_edge_socket_dk920a_20m, especialmente abertura, profundidade e altura; PCB-cartucho; módulo funcional; amostra impressa da guarda e do canal |
| Restrições | AD-06, AD-07, AD-08, AD-14; RM-13 a RM-18; CT-1 a CT-13 |
| Nota | A fenda externa acomoda o corpo plástico, não apenas a espessura de 1,6 mm da PCB. A guarda lateral enquadra a língua, mas não pode cobrir pads nem invadir a área de varredura do soquete. O batente é no corpo plástico; as guias e o apoio da riser descarregam esforços na carcaça principal. A estimativa anterior de ~40 mm salientes é apenas referência histórica, não cota do cartucho pronto. |

### 5.7 Controles frontais

| Item | Zona | Parâmetros | Medir |
|---|---|---|---|
| Cruz direcional | `ZF2`, X < 0 | `DPAD_CX`, `DPAD_CY`, `DPAD_HOLE` | `button_caps_ps3.body_diameter`, `.flange`, `.total_height`, `.dpad_pivot` |
| Botões de ação | `ZF2`, X > 0 | `ABXY_CX`, `ABXY_CY`, `ABXY_PITCH`, `ABXY_HOLE` | Idem, por tampa |
| Select e start | Centro | `SEL_CX`, `START_CX`, `SEL_CY` | Botão e tampa escolhidos |
| Analógicos | `ZF3`, um em X < 0 e outro em X > 0 | `STK_L_CX`, `STK_R_CX`, `STK_CY`, `STK_PROTRUSION`, `STK_HOLE` | Todas as linhas de `analog_stick_ps3` |
| Alto-falante e grade | `ZF3`, entre os analógicos | `SPK_CX`, `SPK_CY`, `SPK_CAVITY`, `GRILLE_PATTERN` | Todas as linhas de `speaker_2030` |
| LED de estado | `ZF2`, entre a cruz e os botões | `LED_STATE_POS` | Só se a decisão D-17 mantiver o LED |

Nenhuma fonte exige que `ABXY_CX` = −`DPAD_CX` nem que `STK_R_CX` = −`STK_L_CX`. A simetria é
coerente com a silhueta, mas é escolha de projeto.

### 5.8 Topo e laterais

| Item | Zona | Parâmetros | Medir |
|---|---|---|---|
| Gatilhos L1/R1 (e L2/R2) | `ZT`, nos cantos | `TRIG_X`, `TRIG_Z`, `TRIG_TRAVEL`, `TRIG_PIVOT` | `tactile_switch_right_angle`, `button_caps_ps3.trigger_lever` |
| Microfone | `ZT` | `MIC_X`, `MIC_Z`, `MIC_HOLE` | `inmp441_breakout.acoustic_port` e contorno |
| Janela do IR | `FR` ou `TP` (D-9) | `IR_FACE`, `IR_TX_POS`, `IR_RX_POS`, `IR_WINDOW` | Todas as linhas de `ir_kit` |
| Portas USB-C | `ZL` | `PORT_SIDE`, `USB_DATA_Y`, `USB_DATA_Z`, `USB_CHG_Y`, `USB_CHG_Z` | v0: conectores da DevKitC e do `charger_module`. v1: `usb_c_receptacle` |
| Botão de energia | `ZL` | `PWR_Y`, `PWR_Z` | `power_button` |
| LED de carga | `ZL`, ao lado da porta de carga | `LED_CHG_POS` | Componente a definir no esquema |

### 5.9 Demais módulos e fixadores

| Item | Zona | Parâmetros | Medir |
|---|---|---|---|
| MCP23017 | `ZR1` | `IOX_POS` | `mcp23017.package` e corpo |
| Amplificador MAX98357A | `ZR1`, longe da bateria (AD-04) | `AMP_POS` | `max98357a_breakout` |
| Módulo de carga (v0) | `ZR1`, longe da bateria (AD-04) | `CHG_POS` | `charger_module` |
| Parafuso e lábio da tampa | `ZR2` | `DOOR_SCREW_POS`, `DOOR_LIP_EDGE` | `hex_standoff` ou `heat_set_insert` |
| Parafusos de fechamento | `TR` | `FST_N`, `FST_POS` | Idem |
| Cartuchos reserva | `ZR2` | `SPARE_N`, `SPARE_POS` | Cartucho final (D-8, D-19) |
| Suportes da DevKitC (v0) | `ZR1` | `MCU_CLIP_POS` | Contorno e espessura da placa |

## 6. Coordenadas numéricas disponíveis hoje

Só as que saem de linhas `CONFIRMADO`. Estão em referencial **local** da peça. A passagem para o
RC-dBMO depende dos parâmetros `MCU_X`, `MCU_Y`, `MCU_Z`, `MCU_AXIS`, `MCU_ANT_DIR` e `MCU_FACE`,
ainda sem valor.

### 6.1 Módulo ESP32-S3-WROOM-1 (PCB definitiva)

Referencial local: origem no centro da face de montagem; +X ao longo da largura; +Y ao longo do
comprimento, apontando para a ponta da antena; +Z da face de montagem para a blindagem. É o mesmo
dos arquivos `mechanical/cad/out/esp32_s3_wroom_1__*.step`.

| Volume | X (mm) | Y (mm) | Z (mm) | Origem dos números |
|---|---|---|---|---|
| Envelope do módulo | −9,00 a +9,00 | −12,75 a +12,75 | 0 a 3,10 | `width` 18,0; `length` 25,5; `thickness` 3,1 |
| Blindagem | −7,90 a +7,90 | −11,70 a +5,90 | 0,80 a 3,10 | `shield_width` 15,8; `shield_length` 17,6; `shield_height` 2,3; `pcb_thickness` 0,8; início a 1,05 mm da borda oposta à antena (modelo 3D oficial) |
| Região da antena | −9,00 a +9,00 | +6,80 a +12,75 | 0 a 3,10 (envelope) | `antenna_zone_length` 5,95 |
| `KO-ESP` (região da antena mais 15 mm) | −24,00 a +24,00 | −8,20 a +27,75 | −15,00 a +18,10 | `antenna_clearance_min` 15,0 |

Contas: 12,75 − 5,95 = 6,80; 6,80 − 15 = −8,20; 12,75 + 15 = 27,75; 9 + 15 = 24; 3,10 + 15 = 18,10.

O volume `KO-ESP` é conservador: usa a espessura inteira do módulo na região da antena, embora ali
só exista a placa de 0,8 mm.

### 6.2 Padrão de pinos da DevKitC oficial (carcaça v0)

Referencial local do padrão: origem no centro do retângulo formado pelas duas fileiras; +X de uma
fileira para a outra; +Y ao longo das fileiras.

| Elemento | Coordenada | Origem dos números |
|---|---|---|
| Fileiras | x = −11,43 e x = +11,43 | `header_row_spacing` 22,86 ÷ 2 |
| Pinos de cada fileira | y = −26,67 + 2,54 × k, com k de 0 a 21 | `header_span` 53,34 ÷ 2; `header_pitch` 2,54; `header_pins_per_row` 22 |

Limites:

- vale para a placa oficial da Espressif. Conferir na placa em mãos a distância entre fileiras;
- a posição desse padrão em relação ao contorno da placa e à ponta da antena é `PENDENTE`. Sem ela,
  o padrão não localiza a placa na carcaça;
- na v0, a placa é presa pelas bordas `[R: Carcaça v0]`, não pelos pinos. O padrão serve para prever
  o espaço dos conectores de fio, não para fixação.

## 7. Parâmetros a preencher

A coluna "Fecha com" diz o que destrava o valor: medição (linhas do registro), decisão (IDs `D-nn`
da especificação) ou regra de processo.

### 7.1 Carcaça

| Parâmetro | Significado | Fecha com |
|---|---|---|
| `BODY_W`, `BODY_H`, `BODY_D` | Envelope externo | D-1; todas as medições da seção 16 da especificação |
| `R_CORNER`, `R_EDGE_LOWER` | Raio dos cantos; chanfro ou raio das arestas inferiores | Mantenedor; `[M: §2]`, `[R: Carcaça 3D]` |
| `T_WALL_FR`, `T_WALL_TR`, `T_WALL_TP`, `T_WALL_SIDE`, `T_DOOR`, `T_PARTITION` | Espessuras de parede | D-15 |
| `Z_PART` | Plano de separação das cascas | D-15; coluna A |
| `Y_CTRL_TOP`, `Y_STICK_TOP`, `Y_DOOR_TOP`, `TOP_BAND_H` | Planos de divisão das zonas | Medições; D-18 |
| `GAP_WIRING` | Folga para fios na v0 | Montagem de bancada |
| `T_CTRL_CARRIER` | Espessura do suporte dos botões e sticks | Definição do suporte |

### 7.2 Tela

| Parâmetro | Significado | Fecha com |
|---|---|---|
| `DISP_CX`, `DISP_CY` | Centro da área ativa no RC-dBMO | `active_area_offset`; D-2 |
| `DISP_ROT` | Rotação da tela no plano (0 ou 180 graus): define para que lado ficam os pinos e o slot SD | `pin_header_envelope`, `sd_slot_envelope` |
| `DISP_Z_GLASS` | Profundidade da face do vidro abaixo do datum A | `total_thickness`, `T_WALL_FR` |
| `BEZEL_X`, `BEZEL_Y` | Largura da moldura preta em cada direção | `glass_outline`; `[M: §2]` |

### 7.3 Placa principal e antena

| Parâmetro | Significado | Fecha com |
|---|---|---|
| `MCU_X`, `MCU_Y`, `MCU_Z` | Posição da referência da placa (seção 5.2) | Medições da DevKitC; PCB definitiva |
| `MCU_AXIS` | Eixo do RC-dBMO alinhado ao comprimento da placa | AD-01, AD-02 |
| `MCU_ANT_DIR` | Sentido para o qual aponta a ponta da antena | AD-01, AD-02; teste de alcance |
| `MCU_FACE` | Face para a qual ficam os componentes | Coluna A |
| `MCU_CLIP_POS` | Posição dos suportes impressos (v0) | Contorno da placa |

### 7.4 Bateria

| Parâmetro | Significado | Fecha com |
|---|---|---|
| `BAT_AXIS` | Eixo do lado maior da célula | D-3 |
| `BAT_CX`, `BAT_CY` | Centro do berço | Medições; `Y_DOOR_TOP` |
| `BAT_BAY_T` | Profundidade do berço | Espessura medida × (1 + `battery_bay_thickness_margin`) |
| `BAT_GAP_XY` | Folga lateral do berço | Medição; D-15. Nenhuma fonte dá valor |
| `BAT_WIRE_SIDE` | Lado da saída dos fios | `wire_exit` |

### 7.5 NFC, rádio e IR

| Parâmetro | Significado | Fecha com |
|---|---|---|
| `NFC_ZONE` | `ZR1` ou `ZR2` | D-6 |
| `NFC_CX`, `NFC_CY` | Centro da bobina | `antenna_outline` |
| `NFC_WALL_GAP` | Distância da antena à face externa | `T_WALL_TR` ou `T_DOOR`; teste de leitura |
| `NFC_METAL_GAP` | Distância da antena ao metal mais próximo atrás dela | Teste de leitura |
| `CC_X`, `CC_Y`, `CC_Z`, `CC_LINK` | Posição do módulo CC1101 e forma de ligação ao conector | `cc1101_module`; D-7 |
| `SMA_X`, `SMA_Z` | Centro do furo do conector SMA na face `TP` | `sma_connector`; D-7 |
| `IR_FACE`, `IR_TX_POS`, `IR_RX_POS`, `IR_WINDOW` | Face, posições e janela do IR | `ir_kit`; D-9 |

### 7.6 Cartucho protegido

| Parâmetro | Significado | Fecha com |
|---|---|---|
| CART_X0 | X do centro da entrada do cartucho | AD-06; EC-4 |
| CART_OFFSET | Deslocamento anti-inversão da língua e da chave externa | Projeto da PCB e da guarda; D-21 |
| CART_Z | Profundidade do plano médio do cartucho | Posição da riser; coluna A |
| CART_BODY_W, CART_BODY_H, CART_BODY_D | Envelope externo comum da carcaça plástica (largura, altura e profundidade) | Medição dos módulos; padrão de família; D-8 |
| CART_WALL_T | Espessura das paredes da carcaça do cartucho | Material, impressora e ensaio de resistência; D-15, D-20 |
| CART_CAVITY_W, CART_CAVITY_H | Envelope interno disponível à PCB e ao módulo funcional | Medições de cada módulo; varia por família |
| CART_TONGUE_W, CART_TONGUE_T, CART_TONGUE_LEN | Largura, espessura e comprimento de engate da língua card edge | Medição do DK920A-20M e desenho da PCB |
| CONTACT_RECESS | Profundidade/contorno do rebaixo de proteção ao redor dos pads | Área de varredura dos contatos do soquete; corpo de prova |
| SLOT_W, SLOT_T | Abertura externa da boca guiada (não confundir com a fenda elétrica do soquete) | Envelope da carcaça; guarda e tolerâncias de impressão |
| SLOT_GUIDE_W, SLOT_GUIDE_T | Largura e espessura dos canais laterais que orientam a guarda plástica | Medição da guarda e corpo de prova |
| SLOT_STOP_Z | Posição do batente que determina o engate completo sem sobrecurso | Profundidade real do soquete e riser |
| SLOT_KEY_X | Deslocamento da chave mecânica que impede inserção invertida | CART_OFFSET; D-21 |
| CART_RETENTION | Tipo/posição da retenção secundária (detente ou lingueta flexível) | Corpo de prova; D-20, D-21 |
| CART_GRIP | Rebaixo ou área externa para os dedos, sem tocar nos contatos | Ergonomia e envelope final |
| SOCK_Y_SEAT | Y do plano de assentamento do soquete | height_above_board, T_WALL_TP |
| T_RISER, RISER_LINK_H | Espessura da riser e altura da ligação à PCB principal | Projeto da PCB |
| GUIDE_LEN | Comprimento efetivo das guias laterais | Profundidade de engate, apoio e corpo de prova |
| SPARE_N, SPARE_POS | Quantidade e posição dos berços de cartucho reserva (acomodam o corpo plástico final) | Envelope externo comum; D-8, D-19 |

### 7.7 Controles, áudio, topo e laterais

| Parâmetro | Significado | Fecha com |
|---|---|---|
| `DPAD_CX`, `DPAD_CY`, `DPAD_HOLE` | Centro e contorno do furo da cruz | `button_caps_ps3`; D-10 |
| `ABXY_CX`, `ABXY_CY`, `ABXY_PITCH`, `ABXY_HOLE` | Centro do losango, distância do centro a cada botão e furo | `button_caps_ps3`; D-10 |
| `SEL_CX`, `START_CX`, `SEL_CY` | Posição de select e start | Botões escolhidos |
| `STK_L_CX`, `STK_R_CX`, `STK_CY` | Centros dos analógicos | `analog_stick_ps3` |
| `STK_PROTRUSION` | Quanto a capa sobressai do datum A | `height_with_cap`; ergonomia |
| `STK_HOLE` | Diâmetro do furo, com a inclinação máxima | `cap_diameter`, `tilt_envelope` |
| `SPK_CX`, `SPK_CY`, `SPK_CAVITY`, `GRILLE_PATTERN` | Posição do alto-falante, cavidade e desenho da grade | `speaker_2030`; D-15 |
| `LED_STATE_POS` | Posição do LED de estado | D-17 |
| `TRIG_X`, `TRIG_Z`, `TRIG_TRAVEL`, `TRIG_PIVOT` | Posição, curso e pivô dos gatilhos | D-11 |
| `MIC_X`, `MIC_Z`, `MIC_HOLE` | Furo do microfone | `inmp441_breakout.acoustic_port`; D-7 |
| `PORT_SIDE` | Lateral das portas (`LE` ou `LD`) | D-4 |
| `USB_DATA_Y`, `USB_DATA_Z`, `USB_CHG_Y`, `USB_CHG_Z` | Centros das portas USB-C | Conectores; D-4 |
| `PWR_Y`, `PWR_Z` | Centro do botão de energia | `power_button` |
| `LED_CHG_POS` | LED de carga | Esquema elétrico |
| `IOX_POS`, `AMP_POS`, `CHG_POS` | Posição do MCP23017, do amplificador e do módulo de carga | Medições; AD-04 |
| `DOOR_SCREW_POS`, `DOOR_LIP_EDGE` | Parafuso da tampa e aresta do lábio | D-16, D-18 |
| `FST_N`, `FST_POS` | Quantidade e posição dos parafusos de fechamento | D-15, D-16 |

## 8. Diferenças entre a v0 e a v1

| Aspecto | Carcaça v0 | Carcaça v1 |
|---|---|---|
| Placa principal | DevKitC inteira, presa por suportes impressos | PCB própria com o módulo ESP32-S3-WROOM-1-N16R8 |
| Ligações | Módulos ligados por fio | Periféricos na PCB |
| microSD | Slot SD do verso da tela, com adaptador | Slot próprio na zona da tampa traseira |
| Cartucho | Fenda impressa, com tampa cega até a Fase 4 | Soquete numa riser no topo |
| Energia | Módulo de carga com power-path avulso | Carga, proteção e regulador na PCB |
| Forma externa | A da variação 1 | A mesma; espessura, parafusos e encaixe interno podem mudar |

Fonte de todas as linhas: `[R: Carcaça v0]`, `[R: Compras em fases]`, `[R: Lista de componentes]`,
`[R: Carcaça 3D]` e `[R: Alimentação]`.
