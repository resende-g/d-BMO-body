# Investigação do power bank AJ-C15

| Campo | Valor |
|---|---|
| Versão | 0.1 (2026-11-20) |
| Estado | **Aberto em 21/11/2026** pelo usuário (etapa 5). Nenhuma medição elétrica ainda |
| Objetivo | Saber se a placa interna tem uma linha regulada de 5 V aproveitável pelo d-BMO e decidir entre os três cenários da seção 7 |
| Regra | Nenhuma ligação elétrica, solda ou fio em ponto cuja função não esteja confirmada por inscrição, datasheet **e** medição |

## 1. Fatos informados (etiqueta do aparelho, relatados pelo usuário)

| # | Dado | Valor na etiqueta | Estado |
|---|---|---|---|
| PB-01 | Modelo | AJ-C15 | Informado; sem datasheet localizado |
| PB-02 | Capacidade nominal | 5.000 mAh | Informado |
| PB-03 | Energia | 11,13 Wh | Informado |
| PB-04 | Química | Lithium-ion | Informado |
| PB-05 | Entrada | 5 V / 9 V, até 3 A | Informado |
| PB-06 | Carregamento magnético | sim | Informado; tipo de transmissão (Qi/MagSafe) não confirmado |
| PB-07 | Conexões físicas | só Lightning; sem USB-A nem USB-C | Informado |
| PB-08 | Lightning fornece energia para fora | **desconhecido** | Pendente |

## 2. Inconsistências e consequências já identificáveis

1. **A energia da etiqueta não corresponde a 5.000 mAh de célula Li-ion.**
   11,13 Wh ÷ 5,0 Ah = 2,23 V, tensão que não existe para Li-ion. Na tensão nominal usual
   de 3,7 V, 11,13 Wh correspondem a ≈ 3.000 mAh; 5.000 mAh a 3,7 V seriam 18,5 Wh.
   **Hipótese:** a célula real tem ≈ 3.000 mAh, ou a etiqueta é inconsistente. Confirmar
   pela etiqueta da célula (Etapa 5) e, no fim, por ensaio de capacidade.
2. **Não há saída com fio.** A alternativa "power bank externo (v0)" do `README.md` previa
   ligar o power bank à USB da DevKitC. Com só uma porta Lightning (provavelmente de
   entrada) e a face magnética (provavelmente transmissor sem fio), **essa ligação não
   existe neste modelo**. A seção do README precisa ser revista.
3. **"5 V dentro da placa" não é garantido.** Se a face magnética for transmissor sem fio,
   a placa tem um conversor que alimenta a ponte da bobina, mas a tensão desse barramento
   depende do projeto (pode ser 5 V, pode ser outra). Só inscrições e medição dizem.
4. **A estimativa de autonomia do README (~8–9 h a 0,35 A) não se sustenta** com 11,13 Wh:
   ela é compatível com ~18,5 Wh. O `POWER_BUDGET.md` já proíbe estimar autonomia antes das
   medições 1 a 3; esta estimativa deve ser retirada ou marcada como inválida.
5. **Conflito com decisão registrada.** O `README.md` diz "não abrir o power bank para
   aproveitar a célula" e, nos critérios de aceitação de bateria (Riscos, item 2), recusa
   "célula tirada de power bank". O cenário 2 (seção 7) já nasce reprovado por esses
   critérios, salvo se a célula for identificável com fabricante, modelo e datasheet.
   Abrir para **investigar a placa** não contraria o objetivo de segurança, desde que a
   célula não seja manipulada.

## 3. Sequência de trabalho e portões

Cada etapa só começa com o portão anterior aprovado. Um "não" num portão de segurança
encerra a investigação invasiva.

| Etapa | O que | Saída | Portão para seguir |
|---|---|---|---|
| 0 | Inventário dos componentes ([`INVENTORY.md`](INVENTORY.md)) | Tabela preenchida | Independente desta investigação |
| 1 | Ferramentas e local (seção 4) | Lista conferida | Itens de segurança presentes |
| 2 | Inspeção externa, sem abrir (seção 5) | Fotos + registro | Sem inchaço, calor, cheiro, dano ou ruído interno |
| 3 | Testes funcionais sem abrir (seção 5.3) | Comportamento da saída sem fio e da Lightning | Resultado registrado; decidir se a abertura é necessária |
| 4 | Preparação: reduzir a carga para ~30–50 % | Estado de carga registrado | Carga reduzida, aparelho frio |
| 5 | Abertura (seção 6) | Placa visível sem tocar a célula | Célula intacta; parar se ela estiver colada à tampa |
| 6 | Fotos da placa e da célula | Marcações legíveis de cada CI | Fotos aprovadas para identificação |
| 7 | Identificação por datasheet | Tabela de componentes com evidência | Função de cada ponto candidato a 5 V sustentada por datasheet |
| 8 | Medições de tensão, sem carga | Tabela de medições | Polaridade e tensão confirmadas |
| 9 | Medições com carga resistiva | Tensão × corrente, aquecimento | Saída estável sob a carga prevista |
| 10 | Decisão (seção 7) | Cenário escolhido e registrado | — |

O roteiro detalhado das etapas 8 e 9 (onde pôr as pontas) **só será escrito depois das
fotos da etapa 6**: sem saber quais CIs existem, qualquer ponto indicado seria palpite.

## 4. Ferramentas e local

### Segurança (obrigatório)

- Óculos de proteção e luvas nitrílicas.
- Superfície não inflamável (azulejo, bandeja metálica), sem papel ou tecido em volta.
- Recipiente metálico com tampa e **areia seca** (ou bolsa anti-chamas para LiPo) ao alcance,
  para isolar a célula se ela aquecer, inchar ou soltar fumaça.
- Local ventilado, de preferência perto de uma área externa. Não trabalhar sobre a cama, o
  sofá ou a mesa de madeira sem proteção.
- Nunca deixar a célula carregando ou aberta sem supervisão.

### Abertura

- Palhetas plásticas finas (de violão) e espátulas de nylon (*spudger*). **Nenhuma ferramenta
  metálica de alavanca** perto da célula.
- Secador de cabelo (preferível a soprador térmico) para amolecer adesivo, com aquecimento
  breve, sem passar de morno ao toque.
- Ventosa pequena, se a tampa for lisa.
- Chaves de precisão: Phillips PH000/PH00, Torx T5/T6, tri-wing Y00 (conferir os parafusos
  antes).
- Álcool isopropílico 99 % com conta-gotas: opcional, em pouca quantidade, só na junta.
- Fita kapton e fita isolante para cobrir partes metálicas expostas.
- Pinça plástica ou cerâmica.

### Documentação

- Celular com modo macro, ou lupa de 10×, e luz lateral (lanterna inclinada) para ler
  marcações a laser em CIs foscos.
- Régua ou papel milimetrado como escala nas fotos.
- Paquímetro (também necessário para o CAD da carcaça).
- Balança de cozinha de 0,1 g ou 1 g (opcional: a massa da célula ajuda a avaliar se a
  capacidade declarada é plausível).

### Medição (só nas etapas 8 e 9)

- Multímetro digital com fusível na entrada de corrente; se possível com seleção manual de
  faixa.
- Pontas finas de agulha com termorretrátil ou capa deixando só 1–2 mm de metal exposto,
  para não encostar em dois pads vizinhos.
- Garras tipo gancho (*test hooks*): só em pontos já confirmados.
- Carga de teste: resistores de potência (por exemplo 10 Ω / 5 W para ~0,5 A a 5 V; 5 Ω /
  10 W para ~1 A). **Os resistores de 1/4 W do kit não servem** como carga: 0,5 A em 10 Ω
  dissipam 2,5 W.
- Termômetro (infravermelho ou de contato) para conferir aquecimento sob carga.
- Opcional, com valor alto também para o `POWER_BUDGET.md`: fonte de bancada com limite de
  corrente e medidor USB em série.

## 5. Inspeção externa e testes sem abrir

### 5.1 Fotos (todas com escala)

1. As seis faces, inteiras.
2. Etiqueta inteira e legível (todas as linhas: modelo, saída sem fio, fabricante, CNPJ ou
   importador, certificação Anatel, data/lote).
3. A porta Lightning de frente, com luz.
4. As juntas da carcaça (onde as tampas se encontram), de perto, em cada lado.
5. Parafusos, tampas de borracha ou adesivos que possam esconder parafusos.

### 5.2 Critérios de recusa (qualquer um encerra a investigação invasiva)

- Carcaça abaulada, juntas abrindo, tampa que "afunda e volta" ao pressionar levemente.
- Aquecimento em repouso, cheiro adocicado ou de solvente, manchas.
- Queda forte registrada, amassado ou perfuração.
- Ruído de peça solta por dentro.
- Não carrega, ou aquece ao carregar.

Se recusado: não abrir, não carregar, guardar no recipiente com areia e descartar em ponto de
coleta de pilhas e baterias.

### 5.3 Testes funcionais (sem ferramentas, sem fios)

| # | Teste | Como | O que registrar |
|---|---|---|---|
| F-1 | Indicador de carga | Pressionar o botão (se houver) | Quantos LEDs ou percentual |
| F-2 | Recarga do power bank | Cabo Lightning original, carregador conhecido | Se carrega, se aquece |
| F-3 | Saída sem fio | Encostar um celular com carregamento sem fio | Se o celular carrega; se o power bank liga sozinho ou só pelo botão |
| F-4 | Lightning como saída | Só com um dispositivo/cabo projetado para isso; **não improvisar adaptador** | Se não houver como testar com acessório adequado, registrar "não testável sem abrir" |

F-3 positivo indica que existe um conversor alimentando a bobina; **não** indica que existe
5 V acessível.

## 6. Abertura

1. Reduzir a carga para ~30–50 % (usando a saída sem fio num celular). Menos energia
   armazenada reduz a gravidade de um curto acidental. Não descarregar a zero.
2. Identificar a construção pelas fotos da 5.1 antes de escolher o ponto de entrada.
   Em power banks magnéticos é comum a bobina, o ferrite e o anel de ímãs ficarem
   **colados diretamente sobre a célula**: entrar pela face magnética é o caminho de maior
   risco de perfurar a pouch. Preferir a face ou a lateral da porta, onde costuma estar a
   placa. Confirmar com as fotos antes.
3. Aquecer a junta de leve e inserir a palheta **no máximo 3–5 mm**, deslizando ao longo da
   junta, sem alavancar contra o interior.
4. Ao primeiro sinal de resistência anormal, deformação da célula, calor ou cheiro: parar,
   afastar as ferramentas e observar por 15 min no recipiente metálico.
5. Ao ver a placa: **parar.** Não descolar a célula, não dobrá-la, não cortar nem dessoldar
   as abas, não puxar fios. Cobrir com kapton qualquer metal exposto que possa encostar na
   placa.

## 7. Fotos para identificação (etapa 6)

Enviar, com escala e luz lateral:

1. Placa inteira, face de componentes, de cima, sem ângulo.
2. Verso da placa, se visível sem soltar nada.
3. Cada CI de perto, até a marcação ficar legível (código, logotipo, data). Se não ler,
   mais duas fotos com a luz vindo de lados diferentes.
4. Indutores (marcação como "1R0", "2R2", "4R7") e os componentes ao redor de cada indutor.
5. Onde as abas ou fios da célula chegam à placa (sem mexer neles), com as marcações de
   serigrafia próximas (B+, B−, BAT, VOUT, 5V, GND, P+, P−).
6. A ligação da bobina à placa.
7. A etiqueta da célula, se estiver visível sem descolar nada.
8. Qualquer placa pequena separada perto da célula (pode ser o PCM).

## 8. Registro de identificação (preencher na etapa 7)

| Ref. | Marcação lida | Foto | Fabricante/peça (hipótese) | Datasheet | Função confirmada | Estado |
|---|---|---|---|---|---|---|
| | | | | | | |

## 9. Registro de medições (preencher nas etapas 8 e 9)

| # | Data | Ponto + | Ponto − | Modo/faixa | Condição (repouso, saída sem fio ativa, carga X Ω) | Leitura | Observação |
|---|---|---|---|---|---|---|---|
| | | | | | | | |

## 10. Cenários de decisão (avaliar só com as etapas 6 a 9 feitas)

| Critério | 1. Placa original + 5 V interno | 2. Célula + circuito novo | 3. Conjunto novo (célula com PCM/NTC + carregador com power-path + regulador) |
|---|---|---|---|
| Segurança | Depende da qualidade desconhecida da placa; proteção da célula a confirmar | **Reprovado pelos critérios do README** salvo célula identificável com datasheet; manipular a célula é o passo de maior risco | Peças com datasheet; alinhado à arquitetura documentada |
| Custo | Zero | Baixo | ~R$ 80–100 (README, Fase 2) |
| Autonomia | ≤ 11,13 Wh nominais, menos as perdas do conversor | Idem | Escolhida pela célula (anúncio de 4000 mAh) |
| Complexidade | Solda em placa desconhecida; botão e corte por baixo consumo da placa original | Alta | Média, com módulos de bancada; depois SMD na PCB |
| Manutenção e reprodutibilidade | Baixa: peça sem documentação, difícil repetir | Baixa | Alta |
| Pontos a confirmar | Existência, estabilidade e capacidade da linha de 5 V; liga/desliga automático da saída | Origem e especificação da célula | Compra da célula e do carregador |

## 11. Registro de execução

### 21/11/2026: abertura (foto do usuário)

Não registrado: estado de carga antes da abertura, ferramentas usadas, se houve aquecimento.

| Observação | Estado |
|---|---|
| Uma célula tipo pouch (prateada, bordas com fita amarela), com isolante verde (fishpaper) sobre a extremidade da placa e um adesivo amarelo no centro | Observado na foto |
| Placa com serigrafia "C302-V8.8J6P-KCF" e "BE0514"; face visível com áreas de cobre estanhado; componentes provavelmente na outra face | Observado; face dos componentes não vista |
| Fio vermelho (à esquerda) e fio preto (à direita) da célula soldados em pads da placa; marcação "B" perto do fio preto (provavelmente "B−", não legível por inteiro) | Observado; polaridade **não medida** |
| Moldura cinza ao redor (material não confirmado; se for metálica, há risco de curto com soldas expostas) | Pendente |
| Bobina/ímãs do carregamento sem fio | Não visíveis; provavelmente embaixo da célula |
| Inchaço da célula | Não avaliável pela foto de cima; falta foto lateral |

### 21/11/2026: placa e bobina expostas (segunda foto do usuário)

| Observação | Estado |
|---|---|
| Placa retirada da moldura, face dos componentes visível; ainda ligada à célula pelos fios | Observado |
| Bobina de cobre sobre ferrite, colada na outra face da célula com um tecido/fita preta (rasgado em vários pontos); dois fios da bobina soldados nos pads marcados **"TX" "TX"** | Observado: a face magnética é um **transmissor** sem fio |
| Um CI de 16 pinos (SOP-16) à esquerda; vários CIs pequenos de 6 e 8 pinos; quatro capacitores grandes em fileira ao lado dos pads TX; um indutor blindado grande à direita | Observado; **nenhuma marcação lida** |
| Conector no centro da borda | Tipo não confirmado pela foto (o usuário informou Lightning) |
| Hipótese de arquitetura: um bloco de gerenciamento da bateria e conversor (indutor grande à direita) e um bloco do transmissor sem fio (CI de 16 pinos, transistores e capacitores de ressonância perto dos pads TX) | **Hipótese**, depende das marcações dos CIs |

### 21/11/2026: fotos de perto da placa

| Observação | Estado |
|---|---|
| Dois resistores marcados **"R025"** (0,025 Ω pela notação usual), entre os pads TX e o CI de 16 pinos | Marcação lida. Função provável: medição de corrente (shunt). **Hipótese** |
| Dois LEDs pequenos ao lado da serigrafia "KCF"; três pads marcados **"G C Q"** perto do indutor | Observado; função dos pads desconhecida, **não usar** |
| Marcações do CI de 16 pinos, dos CIs de 6 e 8 pinos e do indutor | **Ainda ilegíveis** |
| Conector: carcaça retangular com cerca de 10 contatos visíveis | Tipo ainda não confirmado |

### 21/11/2026: primeira marcação lida

| Componente | Marcação | Identificação | Estado |
|---|---|---|---|
| Dois CIs de 6 pinos (SOT-23-6) ao lado dos pads TX | **"4608B"** | Candidato: BORN Semiconductor **BM4608B**, par de MOSFETs complementares (canal N + canal P) de 20 V em SOT-23-6, cuja ficha técnica usa a marcação "4608B" (atta.szlcsc.com, PDF BFCDB57A9EEF074ABEE4353B62206472) | **Hipótese provável.** Códigos de marcação podem se repetir entre fabricantes; a função (dois pares formando uma ponte H que aciona a bobina) é inferência pela posição na placa |
| CI de 6 pinos à esquerda, abaixo do CI de 16 pinos | Começo parecido com "XB8…"/"YB8…", resto ilegível | Não identificado | Pendente |

### 21/11/2026: bloco do conversor (lado direito)

| Componente | Marcação | Identificação | Estado |
|---|---|---|---|
| CI de 6 pinos ao lado do indutor grande | **"4608B"** e "MR" | Mesma marcação dos dois CIs da ponte da bobina: par de MOSFETs complementares (candidato BM4608B). **Não é um CI controlador** | Marcação lida; identificação provável |
| Indutor blindado grande | **"4R7"** | 4,7 µH pela notação usual | Marcação lida |
| Diodo em encapsulamento SMA/SOD perto de "G C Q" | **"S4"** | Não identificado com segurança (marcação curta, usada por mais de uma peça) | Pendente |
| CI de 16 pinos | Sem marcação visível em várias fotos | Provável controlador único da placa (carga, conversor e transmissor), com a marcação apagada | **Hipótese** |

Leitura: não há CI conversor dedicado com datasheet. O conversor parece montado com um par de MOSFETs (4608B) e o indutor de 4,7 µH, comandado por um controlador sem marcação. Topologia (elevador ou abaixador), tensão de saída e quando ele liga (sempre, ou só durante a transmissão sem fio) **não podem ser determinadas por datasheet**: só por medição.

### 21/11/2026: medição M1

| # | Data | Ponto + | Ponto − | Modo/faixa | Condição | Leitura | Observação |
|---|---|---|---|---|---|---|---|
| M1 | 21/11 | Solda do fio vermelho da célula | Solda do fio preto da célula | DCV 20 | Parado, placa fora da carcaça | **3,9 V** (positivo) | Polaridade confirmada: vermelho = positivo. Tensão dentro da faixa normal de uma célula Li-ion (3,0 a 4,2 V); o estado de carga exato não se deduz só da tensão em repouso |
| M2 | 21/11 | Lado direito dos 3 capacitores empilhados à direita do indutor "4R7" | Solda do fio preto da célula | DCV 20 | Parado, nada sobre a bobina | Lado esquerdo: **0 V**; lado direito: **3,86 V** (nos três) | Os três capacitores estão no mesmo nó, cerca de 0,04 V abaixo da célula (3,9 V). Em repouso **não há 5 V** nesse ponto: ele segue a tensão da bateria. Se é a entrada ou a saída do conversor parado, ainda não se sabe |
