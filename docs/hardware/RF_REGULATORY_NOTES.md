# Notas regulatórias de RF do d-BMO v1

| Campo | Valor |
|---|---|
| Situação do documento | Rascunho de engenharia. **Nenhuma disposição normativa foi conferida em fonte primária nesta sessão** (ver seção 3) |
| Revisão adversarial | 2026-11-12 (mesma data). As fontes normativas continuaram inacessíveis; foram abertas apenas duas fontes não normativas (arquivos do Bruce 1.16.1 e datasheet do CC1101). Registro na seção 11 |
| Data de elaboração e de todas as consultas | 2026-11-12 |
| Data de referência das regras | 2026-11-12 |
| Escopo | Funções de rádio do d-BMO v1, uso pessoal e não comercial, no Brasil |
| Arquivo | `docs/hardware/RF_REGULATORY_NOTES.md` |

## 1. Natureza e alcance

Esta é uma **nota de engenharia, não um parecer jurídico**. Ela organiza o que foi possível apurar
sobre as regras brasileiras aplicáveis aos rádios do aparelho e traduz isso em decisões de
firmware. Ela não atesta conformidade de nenhum equipamento. **Quem monta e opera o aparelho
continua responsável pelo cumprimento da legislação**, como já registra o `README.md` (seção
"Segurança e ética").

A nota não analisa: sigilo das comunicações e interceptação, crimes informáticos, proteção de
dados, importação e tributação, exposição humana a campos eletromagnéticos, nem regras de outros
países. A recepção de sinais é tratada aqui apenas sob o ângulo da emissão de radiofrequência
(receber não emite); a licitude de capturar comunicações de terceiros depende de normas que esta
nota não examinou.

## 2. Resumo

1. **Os sítios oficiais não puderam ser abertos.** Os cinco hosts primários tentados
   (`informacoes.anatel.gov.br`, `sistemas.anatel.gov.br`, `www.gov.br`, `www.planalto.gov.br`,
   `www.in.gov.br`) falharam por resolução de nome no ambiente desta sessão. Por isso, pela regra
   de evidência do projeto, **todo ponto normativo desta nota está "não verificado nesta
   sessão"**. O que existe são pistas obtidas por mecanismo de busca, identificadas como tal. A
   revisão do mesmo dia repetiu parte das tentativas, com o mesmo resultado (seções 3.1 e 11).
2. **Existência e identificação das normas de referência** (pistas consistentes, não conferidas):
   Resolução Anatel nº 680, de 27/06/2017 (Regulamento sobre Equipamentos de Radiocomunicação de
   Radiação Restrita); Ato Anatel nº 14.448, de 04/12/2017 (requisitos técnicos); Resolução Anatel
   nº 715, de 23/10/2019 (avaliação da conformidade e homologação); Resolução Anatel nº 760, de
   06/02/2023 (bloqueadores de sinais); Lei nº 9.472, de 16/07/1997.
3. **Situação em 2026-11-12** (pistas): nenhuma das normas acima aparece como revogada por
   inteiro; todas aparecem como alteradas. Há três atos de 2026 cujo efeito sobre a Resolução 680
   não foi apurado: Resolução 789/2026 (novo PDFF), Resolução 792/2026 (datada de 15/10/2026, com
   vacância de 30 dias a partir da publicação) e Resolução 793/2026 (novo Regulamento de Uso do
   Espectro, noticiada como publicada em 09/11/2026). As buscas da revisão repetiram a notícia da
   Res. 793 e **não localizaram** as Res. 789 e 792 (ausência de evidência, não de existência).
   Ver seções 5 e 7.
4. **Consequência para o firmware:** recepção permitida; **transmissão desabilitada por padrão**
   em todos os rádios; tabela de faixas em que a transmissão só pode ser habilitada por
   configuração local explícita; **todas as linhas da tabela estão marcadas como não
   verificadas**; nenhuma função de bloqueio de sinais (jamming). Ver seção 10.
5. **O firmware não garante conformidade de potência irradiada.** Isso depende de módulo, antena,
   casamento e tabela de PA, e só se demonstra por medição (seção 10.4).
6. **O build do Bruce em `firmware/bruce/` não cumpre essa política por inteiro.** Conferido na
   revisão, no código do Bruce na ref `1.16.1`: o Bruce traz bloqueadores de sinal ("Jammer" de
   RF, "IR Jammer" e "NRF Jammer"), "BLE Spam" e ataques de WiFi. Na versão registrada em
   commit, o `bmo.ini` não retirava nenhuma dessas funções. Durante a revisão, outra frente de
   trabalho alterou a árvore de trabalho para excluir do build os três bloqueadores (alteração
   sem commit, não compilada nem testada); "BLE Spam", os ataques de WiFi e as funções de
   transmissão sem tabela de faixas permanecem (bloco 6.10). Os achados sobre o Bruce e sobre o
   datasheet do CC1101 são os únicos da nota apoiados em fonte aberta, e não são normativos.

## 3. Método, data de referência e limites da evidência

### 3.1 Tentativas de acesso a fontes primárias

Ferramenta: busca de página por URL (WebFetch) do ambiente da sessão, em 2026-11-12.

| Host | URL tentada | Resultado |
|---|---|---|
| `informacoes.anatel.gov.br` | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2017/936-resolucao-680` | Falha: `getaddrinfo ENOTFOUND` |
| `www.planalto.gov.br` | `https://www.planalto.gov.br/ccivil_03/leis/l9472.htm` | Falha: `getaddrinfo ENOTFOUND` |
| `www.gov.br` | `https://www.gov.br/anatel/pt-br/regulado/espectro` | Falha: `getaddrinfo ENOTFOUND` |
| `sistemas.anatel.gov.br` | endereço de anexo devolvido por uma busca (`.../anexar-api/publico/anexos/download/95540d66...`) | Falha: `getaddrinfo ENOTFOUND` |
| `www.in.gov.br` | endereço de teste do host (não é referência desta nota) | Falha: `getaddrinfo ENOTFOUND` |
| GitHub `pr3y/Bruce` (código do Bruce) | leitura por API | Negada: repositório fora do escopo da sessão |

A mensagem do ambiente diz que um host falha desse modo quando está fora dos domínios de rede
permitidos à sessão ou quando não existe. Como os hosts acima são sítios oficiais conhecidos, a
primeira hipótese é a provável, mas a mensagem não a confirma. Não se tentou contornar a
restrição. O banco público de homologação da Anatel não foi consultado (mesmo impedimento).

Tentativas da revisão adversarial (2026-11-12, mesma ferramenta, uma tentativa por endereço):

| Host | Endereço tentado | Resultado |
|---|---|---|
| `informacoes.anatel.gov.br` | F03 (Res. 680) e F05 (Ato 14.448) | Falha: `getaddrinfo ENOTFOUND` |
| `www.planalto.gov.br` | F01 (Lei 9.472) | Falha: `getaddrinfo ENOTFOUND` |
| `apps.anatel.gov.br` | F25 (`ConsultaId=20181`) | Falha: `getaddrinfo ENOTFOUND` |
| `documentation.espressif.com` | S01 | Falha: `getaddrinfo ENOTFOUND` |
| `www.legisweb.com.br` | S04 | Falha: `getaddrinfo ENOTFOUND` |
| `teletime.com.br` | S05 | Falha: `getaddrinfo ENOTFOUND` |
| `www.gmalabs.com` | S08 | Falha: `getaddrinfo ENOTFOUND` |
| `github.com` e `raw.githubusercontent.com` | S13 (Bruce, ref `1.16.1`) e S14 (cópia do datasheet do CC1101) | **Abertos** |

Depois de sete hosts com a mesma falha, a revisão deixou de tentar outros, como orienta a
própria mensagem do ambiente. Não foram tentados na revisão: `www.gov.br`,
`sistemas.anatel.gov.br` e `www.in.gov.br` (vale o registro da primeira tabela),
`pesquisa.in.gov.br` (F13), `www.espressif.com` (S02, S03), `www.mobiletime.com.br` (S06),
`atlaspublico.com.br` (S07), `c-prav.com` (S09), `nccgroup.com.br` (S10) e
`www.brics-ocp.com.br` (S11). Para os que não constam de nenhuma das duas tabelas, o que existe
é **ausência de tentativa**, não falha registrada.

### 3.2 Níveis de evidência usados nesta nota

| Nível | Significado | Itens neste nível |
|---|---|---|
| **P** | Fonte primária aberta e lida nesta sessão | **Nenhuma fonte normativa.** Na revisão, duas fontes não normativas: arquivos do Bruce na ref `1.16.1` (S13) e datasheet do CC1101 (S14). Elas sustentam fatos sobre o software e sobre o chip, não sobre a regra |
| **B** | Busca indexada: a ferramenta de busca devolveu um resumo, redigido por modelo, do conteúdo indexado de uma página em domínio oficial. Os trechos entre aspas são os fragmentos que a ferramenta apresentou; a página não foi aberta e a fidelidade literal não foi conferida. A ferramenta pode truncar, misturar versões ou atribuir o trecho ao dispositivo errado | Todas as pistas normativas |
| **S** | Fonte secundária (consultoria, imprensa, base jurídica privada, documento de fabricante), também vista só pelo resumo da busca | Marcadas como "secundária" |
| **R** | Arquivo deste repositório, lido nesta sessão | Citado com caminho |
| **C** | Cálculo do autor desta nota a partir de valores de nível B | Marcado como "cálculo" |
| **E** | Conhecimento geral de engenharia de rádio, sem fonte aberta nesta sessão (por exemplo: um leitor NFC gera campo para ler; a varredura ativa de WiFi transmite; o ganho da antena altera a potência irradiada). Sujeito a conferência nos datasheets | Afirmações de funcionamento nas partes (ii) e (iii) e na seção 10.3 |

Regra de leitura: **níveis B e S não são verificação**. Onde a nota diz "não verificado nesta
sessão", o valor não deve ser tratado como regra, apenas como o que conferir.

Risco de viés a registrar: várias consultas de busca continham expressões e valores candidatos
fornecidos pelo autor (por exemplo "8 dBm", "3 kHz", "125 mW", "35 canais", "0,4 segundos",
"3.750", "12.500", "5 segundos", "50" mV/m, o texto do § 2º do art. 162 e a expressão do art. 183
da Lei 9.472). Nesses casos o resumo da busca pode ter apenas ecoado a consulta. As cadeias de
busca estão na seção 9.2 para que isso possa ser auditado.

A lista acima é exemplificativa. A revisão comparou as cadeias da seção 9.2 com os fragmentos da
seção 6 (conferência feita só com o texto desta nota). As pistas abaixo tinham o valor ou a
expressão central **já presente na consulta** e são as mais expostas a eco:

| Pista | Consulta (seção 9.2) | O que já estava na consulta |
|---|---|---|
| A1 (433–435 MHz) | 12 | "item 4.1.5", "433", "435 MHz", "10 mW", "e.i.r.p.", "250 nW" (a consulta 11, anterior, não continha o item nem os valores) |
| B1, B4 (operação periódica) | 19, 20 | "260-470", "3.750", "12.500", "5 segundos" |
| C1 (864–868 MHz) | 21 | "864-868", "944-948" |
| D4 a D7 e H2 a H4 (item 10) | 15, 16, 17 | "35", "0,25 Watt", "0,4 segundos", "20 dB", "250 kHz", "500 kHz", "75", "125 mW", "6 dB", "8 dBm", "3 kHz", "6 dBi", "1 W" |
| D8 (item 10.3.4.4) | 26, 27 | "item 10.3.4.4", "6 dB", "500 kHz", "6 de outubro de 2026" |
| D9 (intensidade de campo) | 18 | a frase com a lista de faixas, "3 metros", "50", "500" |
| E1 (13,56 MHz) | 13, 14 | "13,553", "13,567", "30 m", "item 13.1" (os valores 15.848, 334 e 106 não estavam) |
| G3 a G6 (Res. 680) | 22, 25 | "caráter secundário", "não têm direito à proteção contra interferências prejudiciais", "não podem causar interferência", "somente a antena", "art. 4º art. 5º art. 6º" |
| I1 (Glossário) | 41 | "3.000 GHz" |
| G1, G2, J1, K8 (Lei 9.472) | 36, 37, 38 | o texto do § 2º do art. 162, "Independerá de outorga", "equipamentos de radiação restrita definidos pela Agência", "Desenvolver clandestinamente atividades de telecomunicação" |
| J2, J4 a J7 (Res. 715 e páginas da Anatel) | 30, 31, 32, 34 | "homologação é pré-requisito obrigatório", "construção artesanal", "uso do próprio Requerente", "qualquer pessoa física ou jurídica", "fabricação artesanal para uso próprio", "Declaração de Conformidade para uso próprio", "art. 13 art. 22" |
| Atos de 2025 e 2026 (seção 5) | 3, 4, 5, 7, 9, 10 | número e data de cada ato; na consulta 7, a palavra "REVOGADA" |

As buscas feitas na revisão (seção 9.2, R1 a R13) têm o mesmo nível B. Quando reproduzem uma
pista, isso é repetição de conteúdo indexado, não confirmação em fonte primária.

### 3.3 Estrutura dos blocos

Cada tema da seção 6 tem três partes separadas: **(i) texto normativo** (o que a norma diria,
com identificação, dispositivo, fragmento, fonte, data e situação), **(ii) interpretação** (leitura
do autor, que pode estar errada) e **(iii) consequência de engenharia** para o firmware do d-BMO.

## 4. Rádios do aparelho

Fonte: `README.md` deste repositório (seções "Recursos", "Lista de componentes (BOM)", "Cartuchos
de expansão") e, onde indicado, datasheet do CC1101 (S14). Nível R, salvo indicação.

| Id | Função | Hardware | Frequência | Observação |
|---|---|---|---|---|
| (a) | Sub-GHz | Transceptor CC1101, módulo vendido para 433 MHz, antena em conector SMA | Módulo: 433 MHz. Cobertura do chip: 300–348, 387–464 e 779–928 MHz (datasheet SWRS061I, Table 3, p. 8; cópia aberta na revisão, S14) | Módulo genérico de marketplace; não há no repositório evidência de homologação |
| (b) | NFC | PN532 V3 | 13,56 MHz | Leitor: para ler uma etiqueta, o módulo gera o campo de 13,56 MHz, isto é, transmite (nível E) |
| (c) | WiFi e Bluetooth LE | ESP32-S3 (DevKitC-1 N16R8 na bancada; ESP32-S3-WROOM-1-N16R8 na PCB final) | 2,4 GHz | Antena de trilha do próprio módulo |
| (d) | Infravermelho | LED emissor e receptor de 38 kHz | Óptico (não é rádio) | 38 kHz é a frequência de modulação da luz (nível E) |
| (e) | LoRa (cartucho planejado, não construído) | Módulo LoRa em cartucho (o `README.md` cita SX1276/1278 como exemplo) | 915 MHz | O `README.md` afirma "Anatel: 902-907,5 e 915-928"; a afirmação não está verificada em fonte primária |

Outros fatos do repositório relevantes para a política:

- O `README.md` prevê cartuchos de rádio em "868, 315" MHz (seção "Cartuchos de expansão").
- O `README.md` prevê teste de "captura/reenvio apenas em equipamento próprio/autorizado" com o
  CC1101 (seção "Montagem passo a passo"). Reenvio é transmissão e fica sujeito a esta nota.
- O `README.md` estabelece que o assistente remoto "não aciona" as ferramentas de RF, NFC e IR
  (seção "Sobre" e tabela de firmwares).
- `firmware/core/include/dbmo/core/status.h` já define `Status::NotPermitted` como "bloqueado por
  política (ex.: transmissão fora da faixa permitida)".
- `firmware/teste_cc1101/teste_cc1101.ino` declara no cabeçalho que não transmite.
- `firmware/bruce/bmo.ini`, na versão registrada em commit (lida por inteiro nesta sessão e no
  início da revisão), não continha nenhuma opção que desabilitasse transmissão ou funções de
  interferência do Bruce. Durante a revisão, outra frente de trabalho alterou o arquivo na
  árvore de trabalho, ainda sem commit: o build passou a excluir os três arquivos de bloqueador
  de sinal do Bruce (RF, IR e NRF24), substituídos por um aviso em
  `firmware/bruce/disabled_features.cpp`. Nenhuma das duas versões define `LITE_VERSION`. Estado
  lido às 03:21 UTC de 2026-11-12; detalhes e alcance no bloco 6.10.
- `firmware/bruce/build.sh` fixa a versão do Bruce em `1.16.1` e clona essa ref de
  `https://github.com/pr3y/Bruce.git` (lido na revisão, nas duas versões do arquivo).

## 5. Mapa das normas e situação apurada

Todas as linhas: consulta em 2026-11-12; fonte não aberta; situação **não verificada nesta
sessão**. As siglas F e S remetem à lista da seção 9.1.

| Norma | Identificação | Situação apurada (pistas) | Nível | Fonte |
|---|---|---|---|---|
| Lei Geral de Telecomunicações | Lei nº 9.472, de 16/07/1997 | Em vigor. Alterações não mapeadas, exceto a notícia de inclusão do § 4º do art. 162 pela Lei nº 14.108/2020 (cópia em base de terceiros) | B/S | F01, F02 |
| Regulamento de radiação restrita | Resolução Anatel nº 680, de 27/06/2017, DOU de 29/06/2017; aprova o Regulamento sobre Equipamentos de Radiocomunicação de Radiação Restrita e revoga a Resolução nº 506/2008 | **Vigente e alterada.** Alterações apontadas: Res. 718/2020 (atribuição incerta), Res. 726/2020 (revoga o art. 9º e redefine o Anexo I), Res. 760/2023 (arts. 3º, 7º e 10, bloqueadores), Res. 772/2025 (nova tabela do Anexo I; a própria 772 foi revogada pela Res. 789/2026), Res. 779/2025 (revoga o art. 2º, definições, que passam ao Glossário). Res. 769/2024 teria revogado, a partir de 01/01/2026, dispositivos da Res. 680 (arts. 2º e 4º, segundo o trecho indexado; não se apurou se são artigos da Resolução ou do Regulamento anexo). Não se encontrou indício de revogação integral. Divergência vista na revisão: outro resumo de busca descreve a Res. 726/2020 como revogadora de partes do art. 8º e de dispositivos sobre faixas de 5 GHz (seção 7, item 14) | B | F03, F04, F14, F15, F17 |
| Atos de 2026 com efeito possível sobre a Res. 680 | Res. Anatel nº 789, de 06/07/2026 (PDFF); Res. Anatel nº 792 (datada de 15/10/2026 na base LegisWeb, "guilhotina regulatória"; data de publicação no DOU não confirmada); Res. Anatel nº 793 (assinada em 05/11/2026, DOU de 09/11/2026, novo Regulamento de Uso do Espectro) | 789: revoga a 772; efeito sobre a tabela do Anexo I da 680 não apurado. 792: o trecho indexado cita o inciso VII do art. 2º e os arts. 3º, 5º e 6º da Res. 680, sem deixar claro se são revogados ou alterados nem se pertencem à Resolução ou ao Regulamento; vacância de 30 dias a partir da publicação, de modo que pode **não estar em vigor na data de referência**. 793: a imprensa relata que altera o regulamento de radiação restrita e o Glossário; uma busca restrita ao portal da Anatel não localizou o ato; data de vigência com relatos divergentes. Na revisão, a Res. 793 foi reproduzida por busca independente (imprensa e agregador); as Res. 789 e 792 **não foram reproduzidas** (seção 7, itens 6, 11 e 13) | B/S | F16, S04, S05, S06, S07, S17 |
| Requisitos técnicos de radiação restrita | Ato Anatel nº 14.448, de 04/12/2017; teria revogado os Atos nº 11.542/2017 e nº 12.927 (cada um apontado por um resumo de busca diferente) | **Vigente e alterado.** Alterações apontadas: Ato 1379/2019 (itens 6.2.5, 13.1 e 13.1.1), Ato 4776/2020, Ato 1306/2021, Ato 423/2022, Ato 14.158/2025 (inclui, altera e revoga itens, entre eles o 10.3.4.4; aplicação relatada como obrigatória a partir de 06/04/2026), Ato 4746/2026 (altera o 14.158; adia um inciso para 06/10/2026), Ato 10.400/2026 (faixa de 6 GHz; fonte secundária). Não se encontrou indício de revogação | B/S | F05 a F10, S08 a S11 |
| Avaliação da conformidade e homologação | Resolução Anatel nº 715, de 23/10/2019 | **Vigente e alterada** pela Res. 780, de 01/08/2025 (revogações pontuais, entre elas o § 2º do art. 20 e o parágrafo único do art. 63). Texto consolidado não obtido | B | F11 |
| Bloqueadores de sinais | Resolução Anatel nº 760, de 06/02/2023, DOU de 08/02/2023; aprova o Regulamento sobre Bloqueador de Sinais de Radiocomunicações (BSR); em vigor desde 01/03/2023; revogou a Res. 308/2002 | **Vigente** (marcação da própria página, segundo a busca). Capítulo de definições revogado pela Res. 779/2025. Requisitos técnicos no Ato 10.988/2024, alterado pelo Ato 9207/2025 | B | F12, F13, F23 |
| Normas históricas (não usar como regra) | Res. 506/2008; Res. 365/2004; Res. 305/2002; Res. 282/2001; Res. 209/2000; Res. 308/2002; Ato 11.542/2017; Res. 759/2023; Res. 772/2025 | Revogadas, segundo as páginas indexadas. Na revisão, os títulos das páginas do portal da Anatel devolvidos pela busca traziam a marca "(REVOGADA)" ou "(REVOGADO)" para as Res. 209/2000, 305/2002 e 365/2004 e para o Ato 11.542/2017; para as demais a marca não foi vista pela revisão. A revogação da Res. 772/2025 depende da Res. 789/2026, que a revisão não conseguiu reproduzir (seção 7, item 13) | B | F24 (só a Res. 506/2008), F14 (Res. 772/2025); as demais sem endereço registrado |

A Anatel mantém na Agenda Regulatória 2025-2026 um item de reavaliação da Resolução 680 (item 23,
fonte F18); o estágio atual não foi apurado. Um resumo de busca da revisão (R13, nível B) diz que
a agenda de 2026 previa essa reavaliação para o segundo semestre de 2027; não conferido.

## 6. Blocos por tema

Convenção das tabelas "(i)": coluna "Fragmento" entre aspas = trecho devolvido pela ferramenta de
busca, **não conferido na fonte**; "teor relatado" = paráfrase do resumo da busca, sem trecho
literal. Data de consulta de todas as linhas: 2026-11-12.

### 6.1 Regime geral de radiação restrita

**(i) Texto normativo (nível B, não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| G1 | Lei 9.472/1997, art. 163, caput | "O uso de radiofreqüência, tendo ou não caráter de exclusividade, dependerá de prévia outorga da Agência, mediante autorização, nos termos da regulamentação." | F01, F02 | Em vigor (não conferido) |
| G2 | Lei 9.472/1997, art. 163, § 2º, I | "I - o uso de radiofrequências por meio de equipamentos de radiação restrita definidos pela Agência;" (hipótese em que a outorga é dispensada) | F01, F02 | Em vigor (não conferido) |
| G3 | Res. 680/2017, Regulamento, art. 3º (numeração conforme a busca) | Teor relatado: as estações de radiação restrita não têm direito a proteção contra interferência prejudicial e não podem causar interferência em sistemas que operem em caráter primário ou secundário; se causarem, devem cessar o funcionamento até a remoção da causa. A Res. 760/2023 teria incluído um § 2º com exceção ligada às áreas de bloqueio de BSR | F03 | Vigente, alterado (não conferido) |
| G4 | Res. 680/2017, Regulamento, art. 4º | Teor relatado: os equipamentos de radiação restrita devem possuir certificação expedida ou aceita pela Anatel. Fragmento devolvido à busca da revisão (R3): "Art. 4º Os equipamentos de radiação restrita operando conforme o estabelecido neste Regulamento devem possuir certificação emitida ou aceita pela Anatel" | F03 | Vigente (não conferido); ver seção 7, item 10 |
| G5 | Res. 680/2017, Regulamento, art. 5º | Teor relatado: o produto ou o manual deve trazer aviso de que o equipamento não tem direito a proteção contra interferência prejudicial e não pode causar interferência em sistemas autorizados. Fragmento devolvido à busca da revisão (R3), incompleto: "Os equipamentos de radiação restrita devem conter no produto, em lugar facilmente visível, ou no manual de instruções fornecido pelo fabricante" | F03 | Vigente (não conferido); ver seção 7, item 10 |
| G6 | Res. 680/2017, Regulamento, art. 6º | Teor relatado: o equipamento deve ser projetado para usar somente a antena que o acompanha, salvo exceções previstas nos requisitos técnicos. Fragmento devolvido à busca da revisão (R3): "Todo equipamento de radiação restrita deve ser projetado para assegurar que seja utilizada apenas a antena comercializada com o equipamento" | F03 | Vigente (não conferido); ver seção 7, item 10 |
| G7 | Res. 680/2017, Regulamento, dispositivo sobre a Tabela I (art. 7º, a confirmar) | "Não é admitida a operação de equipamentos de radiação restrita nas faixas de radiofrequência indicadas na Tabela I. § 1º Nas faixas indicadas na Tabela I, admitem-se somente emissões espúrias provenientes dos mencionados equipamentos que estejam operando em outra faixa." Título da tabela: "Tabela I Faixas de radiofrequências com restrições de uso" | F03, F04 | Vigente, alterado (não conferido). **Conteúdo da Tabela I não obtido** |
| G8 | Res. 680/2017, Regulamento, art. 8º | "Art. 8º As emissões de um equipamento de radiação restrita não devem ser superiores aos níveis de intensidade de campo especificados na Tabela II." | F03 | Vigente (não conferido). **Linhas de VHF e UHF da Tabela II não obtidas** |
| G9 | Res. 680/2017, Regulamento, art. 10 (a confirmar) | "Alternativamente aos limites de emissão constantes do art. 8º, a Agência estabelecerá especificações mínimas para que equipamentos de radiocomunicação operando em faixas de frequências específicas, conforme indicadas no Anexo I, sejam classificados como sendo de radiação restrita." | F03 | Vigente, alterado (não conferido). **Tabela vigente do Anexo I não obtida** |

**(ii) Interpretação**

- A estrutura que resulta das pistas é: a lei dispensa de outorga o uso de radiofrequência por
  equipamento de radiação restrita (G2); o Regulamento fixa limites gerais de emissão (G8) e
  proíbe certas faixas (G7); em faixas específicas listadas no Anexo I, valem limites alternativos
  definidos em requisitos técnicos (G9), que são os do Ato 14.448.
- A dispensa de outorga não é dispensa de certificação (G4) nem autoriza interferência (G3).
- A regra de antena (G6), se confirmada, é relevante para o CC1101 com conector SMA: o campo
  irradiado depende da antena instalada, e o regime parte de um conjunto transmissor-antena
  definido.

**(iii) Consequência de engenharia**

- Toda transmissão do firmware fica restrita a faixas com linha na tabela da seção 10.2; fora
  delas a requisição é recusada com `Status::NotPermitted`.
- A largura ocupada entra na conta: a emissão inteira, não só a portadora, deve caber entre
  f_min e f_max.
- O firmware deve poder cessar qualquer transmissão de imediato por comando local, sem depender
  de rede.
- Enquanto a Tabela I (faixas proibidas) não for obtida, nenhuma faixa nova deve ser acrescentada
  à tabela de transmissão por analogia.

### 6.2 Cerca de 433 MHz (CC1101)

**(i) Texto normativo (nível B, não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| A1 | Ato 14.448/2017, Anexo I, item 4.1.5 | "4.1.5. A utilização da faixa 433-435 MHz por equipamentos de radiação restrita poderá ser feita com potência irradiada limitada ao valor máximo de 10 mW (EIRP), devendo as emissões fora das faixas de radiofrequência especificada ser inferiores a 250 nW (EIRP)" (fragmento truncado; a busca relata que o texto segue com "até 1000 MHz" e um limite de 1 µW acima disso) | F05 | Vigente; a busca não achou indício de alteração deste item, mas a versão consolidada não foi conferida. **Risco de eco:** a consulta 12 já continha o número do item, "10 mW", "e.i.r.p." e "250 nW", e uma busca independente da revisão (R4) não devolveu o item (seção 7, item 12) |
| A2 | Ato 14.448/2017, Anexo I, item 13.1 (RFID) | Teor relatado: a seção de RFID lista a faixa 433,5–434,5 MHz, com limites próprios não recuperados | F05 | Não conferido |

Histórico (não é regra em vigor; nível B, **não verificado nesta sessão**): segundo os resumos
de busca, as Resoluções 305/2002 e 365/2004 traziam o mesmo limite de 10 mW para 433–435 MHz
restrito a áreas internas de edificações, e a Resolução 506/2008 o trazia sem essa restrição.
Todas aparecem como revogadas (seção 5). O endereço F24 é só o da Res. 506/2008; as páginas das
outras duas não têm endereço registrado nesta nota.

**(ii) Interpretação**

- Bordas candidatas: 433,000 a 435,000 MHz. A frequência central usual de 433,92 MHz está dentro
  da faixa, mas a emissão inteira precisa caber nela.
- 10 mW e.i.r.p. equivalem a +10 dBm e.i.r.p.; 250 nW equivalem a cerca de −36 dBm (cálculo). O
  limite é de potência **irradiada**: potência conduzida mais ganho de antena.
- O item não menciona, no fragmento obtido, ciclo de trabalho nem restrição a ambiente interno.
  A ausência no fragmento não prova ausência na norma.

**(iii) Consequência de engenharia**

- Linha `SUBGHZ_433` na tabela (433 000 000 a 435 000 000 Hz), desabilitada por padrão.
- Teto de potência configurado como índice conservador da tabela de PA do CC1101, a ser
  substituído por valor medido. O firmware não conhece o ganho da antena ligada ao SMA.
- O ajuste mais alto do CC1101 em 433 MHz é de +10 dBm conduzidos, e o mais baixo de −30 dBm,
  medidos no projeto de referência do fabricante em carga de 50 Ω (datasheet SWRS061I, Table 10,
  p. 16 e 17; S14, fonte aberta na revisão). Se o limite candidato de 10 mW e.i.r.p. se
  confirmar, o ajuste mais alto já o iguala antes de somar o ganho da antena: não há margem, e o
  teto de firmware deve ficar abaixo do ajuste máximo. Os valores do módulo comprado podem
  diferir dos do projeto de referência (seção 10.3).
- O segundo harmônico de 433,92 MHz cai em cerca de 867,84 MHz e o terceiro em cerca de
  1301,76 MHz (cálculo); o limite de emissão fora da faixa (250 nW e.i.r.p.) torna a medição de
  harmônicos obrigatória antes de qualquer afirmação de conformidade.

### 6.3 Cerca de 315 MHz (CC1101)

**(i) Texto normativo (nível B, não verificado nesta sessão)**

Não se localizou dispositivo específico para "315 MHz". As pistas apontam para o regime de
dispositivos de operação periódica do Ato 14.448, cuja linha de tabela cobre 260–470 MHz.

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| B1 | Ato 14.448/2017, Anexo I, item 6.2 (sinais de controle: alarmes, abridores de portão, chaves remotas) | Linha de tabela: "260-470 · 3.750 a 12.500 (interpolação linear) · 375 a 1.250 (interpolação linear)" (faixa em MHz; intensidade de campo da fundamental e das espúrias em µV/m; distância de medida relatada: 3 m) | F05 | Vigente; item 6.2.5 alterado pelo Ato 1379/2019 (não conferido) |
| B2 | Ato 14.448/2017, Anexo I, item 6.1 (demais dispositivos de operação periódica) | Linha de tabela: "260-470 · 1.500 a 5.000 (interpolação linear) · 150 a 500 (interpolação linear)" | F05 | Não conferido |
| B3 | Ato 14.448/2017, Anexo I, item 6 (largura de faixa) | "A largura de faixa da emissão, determinada pelos pontos de 20 dB abaixo da portadora modulada, deve estar limitada a 0,25% da radiofrequência central, para dispositivos operando acima de 70 MHz e abaixo de 900 MHz." | F05 | Não conferido |
| B4 | Ato 14.448/2017, Anexo I, itens 6.2.3, 6.2.4 e 6.2.5 | Teor relatado: acionamento manual, com desligamento automático do transmissor em até 5 segundos após o fim do acionamento (6.2.3); acionamento automático, com fim da transmissão em até 5 segundos e transmissão periódica limitada a 1 segundo por hora por transmissor (6.2.4); transmissões periódicas em intervalos predeterminados só para supervisão de integridade em aplicações de segurança, limitadas a 2 segundos por hora por transmissor (6.2.5) | F05 | Não conferido |

**(ii) Interpretação**

- Por interpolação linear da linha B1, o limite da fundamental em 315 MHz seria de cerca de
  6.042 µV/m a 3 m, o que corresponde a cerca de 11 µW e.i.r.p. (cerca de −19,6 dBm), pela relação
  de espaço livre e.i.r.p. = (E·d)²/30 (cálculo; supõe campo distante e não considera o detector
  de medida exigido, que não foi apurado). É um nível muito inferior ao ajuste mais alto do
  CC1101 em 315 MHz (+10 dBm conduzidos; o mais baixo é −30 dBm; datasheet SWRS061I, Table 10,
  p. 16 e 17, S14).
- O regime é o de **sinais de controle com condições de tempo** (B4). Não é uma autorização
  genérica para transmitir dados em 315 MHz.
- A faixa 260–470 MHz é a linha da tabela de limites, não uma permissão contínua: as faixas
  proibidas da Tabela I da Res. 680 (G7), não obtidas, podem excluir trechos dela.
- O módulo do projeto é vendido como de 433 MHz (`README.md`); presume-se que a rede de
  casamento seja a dessa faixa (o circuito do módulo comprado não foi inspecionado). Fora dela,
  potência e harmônicos são desconhecidos.

**(iii) Consequência de engenharia**

- **Sem linha habilitável em 315 MHz** enquanto a Tabela I e o item 6 não forem conferidos. A
  linha consta da seção 10.2 como "bloqueada até verificação".
- Se vier a ser habilitada, o firmware deve impor: acionamento manual, corte automático em até
  5 s, largura a 20 dB limitada a 0,25% da frequência central (787,5 kHz em 315 MHz, cálculo) e
  o menor ajuste de potência disponível, com medição prévia.

### 6.4 Cerca de 868 MHz (CC1101 e cartuchos)

**(i) Texto normativo (nível B, não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| C1 | Ato 14.448/2017, Anexo I, tabela com a linha 864–868 MHz (item não identificado) | Teor relatado: largura de faixa de até 100 kHz, canalização em passos de 0,1 MHz a partir de 864,05 MHz, seleção dinâmica de canal, duplexação TDD, 250 mW de pico na saída do transmissor com redução para antena de ganho acima de 2 dBi. A aplicação a que a linha se destina não foi recuperada | F05 | Não conferido |

Histórico (não é regra em vigor; nível B, **não verificado nesta sessão**): segundo o resumo de
busca, a Res. 506/2008 tratava 864–868 MHz como faixa de sistemas de ramal sem fio de CPCT (F24).
Se a linha C1 herdou essa destinação, a aplicação seria essa; é hipótese, não apurada.

**(ii) Interpretação**

- Não se localizou disposição que admita uso genérico de 868 MHz por equipamento de radiação
  restrita, no sentido em que a faixa é usada na Europa (868,0–868,6 MHz, referência de memória do
  autor, não verificada). A linha C1 termina em 868 MHz e parece destinada a uma aplicação
  específica. **Isto é ausência de evidência, não evidência de proibição.**

**(iii) Consequência de engenharia**

- **Sem linha de transmissão em 868 MHz.** Recepção permitida. Um cartucho de rádio de 868 MHz
  (previsto no `README.md`) só deve transmitir depois de localizada e conferida a base normativa.

### 6.5 902–907,5 MHz e 915–928 MHz (CC1101 e cartucho LoRa)

**(i) Texto normativo (nível B, salvo indicação; não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| D1 | Ato 14.448/2017, Anexo I, item 10 (abertura) | "Equipamentos Utilizando Tecnologia de Espalhamento Espectral ou outras Tecnologias de Modulação Digital operando nas faixas 902-907,5 MHz, 915-928 MHz, 2.400-2.483,5 MHz e 5.725-5.850 MHz devem atender às condições estabelecidas neste item." | F05 | Vigente, alterado (não conferido) |
| D2 | Ato 14.448/2017, Anexo I, item 10.2.1 (salto em frequência) | "As radiofrequências portadoras dos canais de salto devem estar separadas por um mínimo de 25 kHz ou pela largura de faixa do canal de salto a 20 dB, devendo ser considerado o maior valor" | F05 | Não conferido |
| D3 | Ato 14.448/2017, Anexo I, item 10.2.5 | "Em adição ao estabelecido nos subitens anteriores, os requisitos a seguir se aplicam aos sistemas de salto em radiofrequência operando nas faixas 902-907,5 MHz e 915-928 MHz." | F05 | Não conferido |
| D4 | Ato 14.448/2017, Anexo I, item 10.2.5.2 (largura a 20 dB menor que 250 kHz) | "no mínimo, 35 radiofrequências de salto e o tempo médio de ocupação de qualquer radiofrequência não deve ser superior a 0,4 segundos num intervalo de 14 segundos" | F05 | Não conferido |
| D5 | Ato 14.448/2017, Anexo I, item 10.2.5.3 (largura a 20 dB de 250 kHz ou mais) | "no mínimo, 17 radiofrequências de salto e o tempo médio de ocupação de qualquer radiofrequência não deve ser superior a 0,4 segundos num intervalo de 7 segundos" | F05 | Não conferido |
| D6 | Ato 14.448/2017, Anexo I, item 10.2.5.4 (atribuição incerta) | "A máxima largura de faixa ocupada do canal de salto a 20 dB deve estar limitada a 500 kHz." | F05 | Não conferido |
| D7 | Ato 14.448/2017, Anexo I, itens 10.3.1, 10.3.2 e 10.3.3 (sequência direta e outras modulações digitais) | Teor relatado: largura de faixa a 6 dB de no mínimo 500 kHz (10.3.1, fonte secundária S08); potência de pico máxima de saída de 1 W (10.3.2); densidade espectral de potência de pico de até 8 dBm em qualquer faixa de 3 kHz (10.3.3) | F05, S08 | Não conferido |
| D8 | Ato 14.158/2025, artigo de revogações, e Ato 4746/2026 | "item III - o item 10.3.4.4 do Anexo I ao Ato nº 14.448, de 4 de dezembro de 2017" (lista de itens revogados). Fontes secundárias descrevem o item revogado como uma exceção à largura mínima de 500 kHz a 6 dB e dão 06/10/2026 como data a partir da qual novas certificações devem cumpri-la | F09, F10, S08, S09, S15 | Conteúdo do item revogado não conferido. Divergências entre fontes secundárias vistas na revisão: seção 7, item 18 |
| D9 | Ato 14.448/2017, Anexo I, item sobre intensidade de campo (número não recuperado) | "Os limites de intensidade de campo média, medida a uma distância de 3 metros, de um equipamento de radiação restrita operando nas faixas 902-907,5 MHz, 915-928 MHz, 2.400-2.483,5 MHz, 5.725-5.875 MHz e 24,00-24,25 GHz" (frase truncada). Valores relatados para 902–907,5 e 915–928 MHz: 50 mV/m na fundamental e 500 µV/m nos harmônicos; a intensidade de campo de pico não deve exceder o valor médio em mais de 20 dB | F05 | Não conferido. A mesma frase consta da Res. 506/2008 (revogada); a atribuição ao Ato 14.448 é do resumo da busca. Na revisão (R9), um resumo apontou dois itens para essa frase, de forma conflitante: 4.1.4 (com remissão a uma "Tabela I") e 9.1.1 (com remissão a uma "Tabela IV"); ver seção 7, item 4 |
| D10 | Ato 14.448/2017, Anexo I, linha de RFID | Teor relatado: 50 mV/m a 3 m em 915–928 MHz para sistemas de identificação por radiofrequência | F05 | Não conferido |

Limite de potência para salto em frequência em 902–907,5 e 915–928 MHz: **não verificado nesta
sessão**. A busca devolveu informações conflitantes (ver seção 7, item 2).

**(ii) Interpretação**

- Bordas candidatas: 902,000 a 907,500 MHz e 915,000 a 928,000 MHz. O intervalo de 907,5 a
  915 MHz fica fora das duas faixas. O `README.md` traz as mesmas bordas, sem fonte.
- Há, nas pistas, três regimes distintos nessas faixas: salto em frequência (D2 a D6), modulação
  digital de faixa larga (D7) e limite de intensidade de campo (D9). Um transmissor simples de
  portadora única e faixa estreita, como o CC1101 em OOK ou FSK sem salto, não atende aos dois
  primeiros; restaria o regime D9.
- 50 mV/m a 3 m corresponde a cerca de 0,75 mW e.i.r.p., isto é, cerca de −1,25 dBm (cálculo, com
  as mesmas ressalvas do bloco 6.3). 500 µV/m a 3 m corresponde a cerca de 75 nW e.i.r.p. Para
  comparação, o ajuste mais alto do CC1101 em 915 MHz é de +11 dBm conduzidos no projeto de
  referência (datasheet SWRS061I, Table 10, p. 16 e 17, S14).
- Para o cartucho LoRa: larguras de 125 kHz e 250 kHz não atendem ao mínimo de 500 kHz a 6 dB do
  regime D7; as alternativas seriam largura de 500 kHz, salto em frequência nos termos de D4 e
  D5, ou operar no regime D9 em potência baixa. A notícia D8 indica que a exceção que acomodava
  faixa estreita deixou de valer para novas certificações em outubro de 2026. Tudo isto é leitura
  de pistas e de fontes secundárias, que divergem entre si quanto à data e quanto a o LoRa ser
  ou não atingido (seção 7, item 18).
- O módulo CC1101 do projeto é vendido para 433 MHz; presume-se que em 915 MHz a rede de
  casamento e o filtro não sejam os da faixa (o circuito do módulo comprado não foi
  inspecionado).

**(iii) Consequência de engenharia**

- Linhas `SUBGHZ_902` (902 000 000 a 907 500 000 Hz) e `SUBGHZ_915` (915 000 000 a
  928 000 000 Hz), desabilitadas por padrão. Transmissão entre 907 500 000 e 915 000 000 Hz
  sempre recusada.
- O texto de limite adotado nas linhas é o mais restritivo dos regimes apurados (D9), até que a
  aplicação dos demais seja conferida.
- O firmware do cartucho LoRa deve tratar largura de banda, número de canais de salto e tempo de
  ocupação como parâmetros de política, não só de desempenho, e não deve oferecer configuração de
  canal único com 125 kHz em potência alta como padrão.

### 6.6 13,56 MHz (PN532)

**(i) Texto normativo (nível B, não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| E1 | Ato 14.448/2017, Anexo I, item 13.1 (sistemas de identificação por radiofrequência), tabela | Linha: "13,553 - 13,567 · 15.848 · 30" (faixa em MHz; intensidade de campo em µV/m; distância de medida em metros). Teor relatado: subfaixas adjacentes 13,410–13,553 e 13,567–13,710 MHz com 334 µV/m a 30 m; demais subfaixas do item com 106 µV/m a 30 m | F05, F06 | Vigente; itens 13.1 e 13.1.1 com redação do Ato 1379/2019 (não conferido) |

Tolerância de frequência e procedimento de extrapolação de distância: **não verificado nesta
sessão**.

**(ii) Interpretação**

- Bordas candidatas da subfaixa principal: 13,553 a 13,567 MHz. 15.848 µV/m equivalem a
  84 dBµV/m (cálculo).
- O PN532 opera com portadora fixa de 13,56 MHz; o firmware não escolhe a frequência, só liga e
  desliga o campo e define o protocolo. As bandas laterais de modulação caem fora da subfaixa
  principal de 14 kHz: nas subfaixas adjacentes e, conforme a taxa de dados e a subportadora do
  protocolo, além delas, onde os limites relatados são mais baixos (nível E; a ocupação espectral
  de cada protocolo não foi conferida em norma nem em datasheet nesta sessão).
- Ler uma etiqueta exige gerar o campo. Em termos regulatórios, a leitura NFC é emissão
  intencional, não recepção.

**(iii) Consequência de engenharia**

- Linha `NFC_13M56` (13 553 000 a 13 567 000 Hz). Campo de RF desligado por padrão; ligado só
  durante uma operação iniciada localmente pelo usuário e desligado ao final ou por tempo limite.
- Ponto para decisão do projeto: se a "transmissão desabilitada por padrão" inclui o campo NFC, o
  leitor fica inoperante até a habilitação local explícita. Esta nota recomenda que inclua, por
  coerência com a definição de emissão intencional.
- O campo a 30 m depende da antena do módulo e da carcaça; não há como inferi-lo do firmware.

### 6.7 2400–2483,5 MHz (WiFi e Bluetooth LE do ESP32-S3)

**(i) Texto normativo (nível B, não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| H1 | Ato 14.448/2017, Anexo I, item 10 (abertura) | Mesmo fragmento D1, que inclui a faixa "2.400-2.483,5 MHz" | F05 | Não conferido |
| H2 | Ato 14.448/2017, Anexo I, item 10.2.6 (salto em frequência em 2400–2483,5 MHz) | Teor relatado: no mínimo 15 radiofrequências de salto não coincidentes; potência de pico limitada a 125 mW com menos de 75 frequências de salto e a 1 W com 75 ou mais | F05 | Não conferido |
| H3 | Ato 14.448/2017, Anexo I, itens 10.3.2 e 10.3.3 | Teor relatado: potência de pico máxima de saída de 1 W; densidade espectral de potência de até 8 dBm em qualquer faixa de 3 kHz | F05 | Não conferido |
| H4 | Ato 14.448/2017, Anexo I, item 10.5.1 | Teor relatado: antenas com ganho acima de 6 dBi admitidas apenas em aplicações ponto a ponto do serviço fixo, com redução de 1 dB na potência de pico para cada 3 dB de ganho excedente | F05 | Não conferido |
| H5 | Ato 14.448/2017, Anexo I, item 10.6 | Teor relatado: item alterado pelo Ato 14.158/2025; uma fonte secundária menciona limite de emissões espúrias de 30 dBc | F09, S08 | Não conferido |

Plano de canais de WiFi admitido no Brasil e requisitos específicos de Bluetooth LE: **não
verificado nesta sessão**.

**(ii) Interpretação**

- Bordas candidatas: 2400,0 a 2483,5 MHz.
- Para o rádio do ESP32-S3, o que vale na prática é a configuração com que o módulo foi
  certificado (potências, modos, antena). O firmware do projeto não deve alterar tabelas de
  potência, dados de calibração de PHY nem antena.
- Funções de injeção de quadros 802.11 destinadas a derrubar ou confundir redes e dispositivos de
  terceiros (desautenticação, inundação de beacons, inundação de anúncios BLE) são emissão dentro
  da faixa, mas com finalidade de interferir; a análise delas está no bloco 6.10.

**(iii) Consequência de engenharia**

- Linha `WIFI_BLE_2G4` (2 400 000 000 a 2 483 500 000 Hz). Rádio de 2,4 GHz desligado por
  padrão; ligado por configuração local (por exemplo, para o modo agente na rede do próprio
  usuário).
- Usar apenas as APIs e os limites de potência do fabricante; código de país configurado para o
  Brasil; nenhuma elevação de potência acima do padrão do SDK.
- Varredura ativa de WiFi e anúncio BLE são transmissões; varredura passiva não é.

### 6.8 Infravermelho

**(i) Texto normativo (nível B, não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| I1 | Glossário de telecomunicações, aprovado pela Res. Anatel nº 779/2025, verbete de radiofrequência | Teor relatado: radiofrequência como a parte do espectro eletromagnético abaixo de 3.000 GHz, cujas ondas se propagam no espaço sem guia artificial | F15 | Vigente; noticiada alteração do Glossário pela Res. 793/2026 (não conferido) |

**(ii) Interpretação**

- Um LED de infravermelho de controle remoto emite luz com comprimento de onda da ordem de
  940 nm (valor típico, não verificado no componente comprado), o que corresponde a cerca de
  319.000 GHz (cálculo: c/λ), duas ordens de grandeza acima de 3.000 GHz. Nessa leitura, o
  emissor de IR não é equipamento de radiocomunicação.
- Os 38 kHz citados no `README.md` são a frequência de modulação da luz, não uma portadora de
  rádio.

**(iii) Consequência de engenharia**

- O IR fica fora da tabela de faixas de rádio. A mesma regra de produto vale para ele por decisão
  do projeto: emissão só por ação local do usuário e uso em equipamento próprio ou autorizado.

### 6.9 Avaliação da conformidade e homologação

**(i) Texto normativo (nível B, salvo indicação; não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| J1 | Lei 9.472/1997, art. 162, § 2º | Fragmento de texto de consulta pública da Anatel que descreve o dispositivo: "veda a utilização, no país, de equipamentos emissores de radiofrequência sem certificação expedida ou aceita pela Agência (§2º do Art. 162)." O texto do próprio parágrafo não foi obtido de forma independente | F01, F02, F25 | Em vigor (não conferido) |
| J2 | Res. 715/2019, Regulamento, art. 55 (numeração segundo texto de consulta pública) | "A homologação é pré-requisito obrigatório para a utilização e a comercialização, no País, dos produtos abrangidos por este Regulamento." | F11, F25 | Vigente; partes do art. 55 teriam sido revogadas ou reescritas pela Res. 780/2025 (não conferido). Na revisão (R10), o resumo de busca atribuiu a mesma frase ao art. 55 e disse que a Res. 780/2025 transformou o parágrafo único em § 1º e acrescentou um § 2º, de texto não obtido |
| J3 | Res. 715/2019, Regulamento, art. 83 (inciso incerto; a numeração "I" vem de texto de consulta pública) | "uso ou emprego de produto não homologado; comercialização de produto não homologado" (itens de lista de condutas vedadas) | F11, F25, F27 | Não conferido. Na revisão (R10), o resumo de busca disse que a redação original do inciso I era "comercialização e uso de produtos não homologados ou em condições diversas das estabelecidas nos respectivos Requisitos Técnicos" e que a Res. 780/2025 desdobrou as condutas em incisos separados (uso, comercialização e importação). A redação e a numeração vigentes ficam em aberto |
| J4 | Res. 715/2019, Regulamento, seção "Dos Requerentes" (artigo não identificado) | Teor relatado: podem requerer a homologação o fabricante, o representante de pessoa jurídica estrangeira e, quando se tratar de uso próprio, qualquer pessoa física ou jurídica | F11, F26 | Não conferido |
| J5 | Res. 715/2019, Regulamento, art. 33, I e II (numeração segundo texto de consulta pública) | "produtos de construção artesanal ou importados para uso do próprio Requerente" (uso preferencial do modelo de Declaração de Conformidade) | F11, F25 | Não conferido |
| J6 | Formulário de homologação da Anatel | "Declaração de Conformidade... quando o produto for de fabricação artesanal para uso próprio. (art. 22)" | F21, F26 | Documento possivelmente anterior às alterações de 2025 (não conferido). Não se apurou a que regulamento o "art. 22" se refere; a consulta 32 já continha "art. 22" (risco de eco), e o regulamento anterior de certificação (Res. 242/2000) aparece com a marca "(REVOGADA)" no título da página da Anatel devolvido à busca da revisão |
| J7 | Orientação da Anatel sobre Declaração de Conformidade para uso próprio | "A Declaração de Conformidade para uso próprio é aplicável apenas à unidade objeto da homologação (identificado pelo seu número serial de fabricação) e não ao modelo do produto" | F20, F26 | Não conferido |
| J8 | Procedimento de homologação (documento antigo, vigência incerta) | "Para equipamentos comprovadamente produzidos de forma artesanal e sem propósito comercial..." (avaliação por declaração, sem relatório de ensaios, segundo o resumo). O procedimento localizado trata de radioamador, rádio do cidadão e serviços móveis marítimo e aeronáutico | F26 | Vigência e aplicação a radiação restrita não apuradas |
| J9 | Página de formulários e manuais da Anatel | Título listado: "Instruções para Homologação de equipamentos artesanais por Declaração de Conformidade para uso próprio" (conteúdo não obtido) | F21 | Não conferido |
| J10 | Certificado do módulo ESP32-S3-WROOM-1 (documento do fabricante; **fonte secundária** em relação ao banco da Anatel) | Teor relatado: certificado de conformidade técnica emitido por organismo designado, código de rastreabilidade "ICC 06.083/2023.1", certificação em 11/07/2023, validade até 11/07/2027, emissão atualizada em 02/07/2025; o certificado registraria que produto não acabado exige nova avaliação quando integrado a outro equipamento | S01, S02, S03 | **Número de homologação Anatel não obtido; banco da Anatel não consultado.** Na revisão (R6), o resumo de busca repetiu o código "ICC 06.083/2023.1" com emissão inicial em 11/07/2023 e uma primeira manutenção "ICC 06.083/2023.1_M1" de 02/07/2025, mas informou que o campo de validade aparecia sem preenchimento no trecho recuperado: **a validade até 11/07/2027 não foi reproduzida**. O PDF não pôde ser aberto (seção 3.1) |

**(ii) Interpretação**

- Pelas pistas J1 a J3, a regra geral é que equipamento emissor de radiofrequência precisa de
  certificação ou homologação para ser **utilizado**, e não só para ser vendido. Não se localizou
  disposição que dispense dessa exigência o aparelho montado para uso próprio sem comercialização.
- O que as pistas J4 a J9 indicam é um **caminho simplificado**, não uma dispensa: homologação por
  Declaração de Conformidade para produto de fabricação artesanal de uso próprio, requerida pela
  própria pessoa física, válida para a unidade e sem direito a comercialização. Uma página da
  Receita Federal (fonte secundária, S12) afirmaria, segundo o resumo da busca, que "uso próprio"
  não cria dispensa geral de homologação; a página não foi aberta.
- Não foi possível confirmar se esse caminho se aplica a um aparelho de radiação restrita montado
  com módulos de rádio comprados prontos, nem quais ensaios seriam exigidos. É o ponto que mais
  pede consulta direta à Anatel.
- A eventual homologação do módulo ESP32-S3-WROOM-1 cobre o módulo. Ela não se estende ao CC1101,
  ao PN532 nem ao módulo LoRa, e a pista J10 sugere que nem ao produto final de forma automática.
  A placa de bancada é uma DevKitC-1, não o módulo isolado; a cobertura da variante N16R8 não foi
  apurada.
- Sobre os módulos CC1101, PN532 e LoRa: não há no repositório nem nas buscas evidência de
  homologação. É ausência de evidência; o banco da Anatel não foi consultado.

**(iii) Consequência de engenharia**

- O firmware não pode suprir homologação. A política de transmissão desabilitada por padrão é a
  resposta de engenharia à incerteza: o aparelho, como sai da bancada, recebe e não emite pelo
  CC1101 nem pelo cartucho LoRa.
- A tela de habilitação de cada faixa deve informar que a habilitação é de responsabilidade do
  operador e que o projeto não atesta homologação.
- Na PCB final, manter o módulo ESP32-S3-WROOM-1 sem alteração de antena, blindagem ou tabela de
  potência, para não invalidar o que o certificado do módulo cobrir.
- Registrar, em documento do projeto, o número de série ou identificador de cada unidade montada,
  porque o caminho de uso próprio, se aplicável, é por unidade.

### 6.10 Interferência prejudicial e bloqueadores de sinais

**(i) Texto normativo (nível B, não verificado nesta sessão)**

| Ref. | Norma e dispositivo | Fragmento ou teor relatado | Fonte | Situação |
|---|---|---|---|---|
| K1 | Res. 680/2017, Regulamento, art. 3º | Ver G3: vedação de causar interferência e dever de cessar o funcionamento | F03 | Vigente, alterado (não conferido) |
| K2 | Res. 760/2023, Regulamento sobre BSR (artigo não identificado) | "São vedados em todo o país a comercialização, a posse e o emprego de BSR, salvo nas circunstâncias excepcionais delimitadas por este Regulamento" (fragmento sem o final da frase). Na revisão (R8), o resumo de busca, apoiado em cópia de base privada (S18), deu a continuação ", as quais estão ainda condicionadas à anuência da Anatel" e situou a frase no art. 2º do Regulamento anexo | F12, F13, S18 | Vigente (não conferido); artigo e final da frase vindos de fonte secundária |
| K3 | Res. 760/2023, Regulamento sobre BSR (artigo não identificado) | "Em hipótese alguma a Anatel anuirá ao uso de BSR por pessoa natural ou por pessoa jurídica de direito privado" | F12 | Vigente (não conferido). A busca da revisão (R8) **não devolveu essa frase literal**; devolveu uma paráfrase de imprensa (S19) que a situa no § 1º do art. 4º |
| K4 | Res. 760/2023, Regulamento sobre BSR, art. 7º (numeração conforme a busca) | "É vedado portar, transportar, conduzir, ocultar, ter em depósito, deter, ceder e emprestar" (BSR, por quem não seja usuário autorizado nem integre a cadeia de fabricação e comercialização); teor relatado: os equipamentos são passíveis de apreensão, homologados ou não | F12 | Vigente (não conferido) |
| K5 | Res. 760/2023, Regulamento sobre BSR, art. 6º e art. 4º (numeração conforme a busca) | Teor relatado: uso admitido só em áreas listadas, como estabelecimentos penitenciários, portos e aeroportos, por órgãos públicos listados e com anuência da Anatel | F12 | Vigente (não conferido) |
| K6 | Res. 760/2023, Regulamento sobre BSR, dispositivo de sanções | Teor relatado: sanções administrativas da Lei 9.472/1997 e do regulamento de sanções da Anatel, "sem prejuízo das sanções de natureza civil e penal" | F12 | Vigente (não conferido) |
| K7 | Lei 9.472/1997, art. 173 | "A infração desta Lei ou das demais normas aplicáveis... sujeitar os infratores às seguintes sanções, aplicáveis pela Agência, sem prejuízo das de natureza civil e penal" (fragmento com supressão; lista de sanções não obtida) | F01, F02 | Em vigor (não conferido) |
| K8 | Lei 9.472/1997, arts. 183, 184 e 185 | Teor relatado: o art. 183 tipifica como crime desenvolver clandestinamente atividades de telecomunicação, com pena de detenção e multa (valores não obtidos), alcançando quem concorre para o crime; considera-se clandestina a atividade sem a competente concessão, permissão ou autorização de serviço, de uso de radiofrequência e de exploração de satélite; o art. 185 define ação penal pública | F01, F02 | Em vigor (não conferido) |
| K9 | Portaria Anatel nº 3102, de 22/12/2025 (instrução de fiscalização sobre estações rádio base falsas) | Teor relatado: trata o uso de radiofrequência sem autorização como infração ao art. 163 da Lei 9.472 e como atividade clandestina sujeita aos arts. 183 a 185; trata a interferência em faixas de prestadoras como agravante; afirma que o uso de produto não certificado é infração administrativa | F22 | Não conferido. Norma de fiscalização de tema específico; citada só como indício de como a Anatel aplica a lei |

Definição normativa de BSR (hoje no Glossário, após a Res. 779/2025): **não verificado nesta
sessão**.

**(ii) Interpretação**

- As pistas K2 a K4 descrevem uma proibição ampla, que alcança posse e porte, sem hipótese de
  anuência para pessoa natural. Um aparelho de mão com uma função destinada a impedir a recepção
  de sinais de terceiros funcionaria como bloqueador, qualquer que seja o nome da função no menu.
  Se a definição normativa de BSR alcança um aparelho multifunção com essa função em software é
  questão jurídica que esta nota não resolve; a leitura prudente é que sim.
- Independentemente do enquadramento como BSR, na leitura do autor, emitir com a finalidade de
  interferir contrariaria o dever de não causar interferência (K1) e sairia da moldura de
  radiação restrita, o que afastaria a dispensa de outorga (G2) e aproximaria a conduta do uso
  não autorizado de radiofrequência (K8, K9). Nenhuma das pistas obtidas diz isso com essas
  palavras: é inferência do autor, não texto normativo.
- Na faixa de 2,4 GHz (funções que derrubam ou saturam redes e dispositivos de terceiros), o
  argumento de radiofrequência é mais fraco, e a revisão o registra como tal. Pelo teor relatado
  em G3, o dever de não interferir protege sistemas que operam em caráter primário ou secundário;
  as redes WiFi e os dispositivos BLE de terceiros são, em princípio, eles próprios equipamentos
  de radiação restrita, sem direito a proteção contra interferência (leitura da revisão, não
  conferida). Não se apurou se a definição de BSR alcança ataques de protocolo como a
  desautenticação. A exclusão dessas funções apoia-se, portanto, em decisão do projeto (seção
  "Segurança e ética" do `README.md`: não interferir em sistemas de terceiros sem autorização) e
  em normas que esta nota não examinou (seção 1), e não em disposição de radiofrequência
  localizada.

**(iii) Consequência de engenharia**

- **Decisão de projeto fundamentada: o firmware do d-BMO nunca implementa bloqueio de sinais.**
  Não há modo de portadora contínua, ruído, varredura de emissão nem repetição ininterrupta
  destinados a impedir recepção; não há desautenticação, inundação de beacons nem inundação de
  anúncios BLE.
- A ausência dessas funções deve ser verificável: sem símbolo, menu ou comando correspondente no
  firmware próprio, e com teste automatizado que falhe se uma API de emissão contínua for exposta
  fora de um modo de ensaio.
- Limitador de tempo de transmissão no nível mais baixo do driver (duração máxima por emissão e
  intervalo mínimo entre emissões), independente da interface de usuário.
- Modo de ensaio para medição (portadora contínua em carga de 50 Ω): se existir, só em build de
  desenvolvimento, com potência mínima e aviso de que se destina a medição conduzida.
- **Build do Bruce (`firmware/bruce/`)**: conferido na revisão no repositório do Bruce, na ref
  `1.16.1`, a mesma que `firmware/bruce/build.sh` clona (S13, fonte aberta em 2026-11-12).
  - O `README.md` do Bruce lista, na seção RF, "Jammer Full (sends a full squared wave into
    output)" e "Jammer Intermittent (sends PWM signal into output)" (linhas 104 e 105); na seção
    BLE, "iOS Spam", "Windows Spam", "Samsung Spam", "Android Spam" e "Spam All" (linhas 91 a
    95); na seção WiFi, "Beacon Spam", "Target Deauth" e "Deauth Flood (More than one target)"
    (linhas 59 a 64); e "NRF24 Jammer" (linha 159).
  - No código, `src/core/menu_items/RFMenu.cpp` traz a entrada
    `{"Jammer", [=]() { RFJammer(true); } }` (linha 31) dentro de um bloco
    `#if !defined(LITE_VERSION)` (linhas 23 a 32). `src/modules/rf/rf_jammer.cpp` define os modos
    "FULL POWER", "INTERMITTENT", "NOISE STORM" e "FREQ SWEEP" (linha 13) e um corte de
    30 segundos por execução (`#define MAX_JAMMER_RUNTIME 30000`, linha 8).
    `src/core/menu_items/BleMenu.cpp` traz "BLE Spam" (linha 36) e
    `src/core/menu_items/WifiMenu.cpp` traz "Wifi Atks" (linha 62), ambos fora de bloco
    condicional de compilação. Há ainda "IR Jammer" (`src/core/menu_items/IRMenu.cpp`, linha 20,
    dentro de `#if !defined(LITE_VERSION)`) e "NRF Jammer" (`src/core/menu_items/NRF24.cpp`,
    linha 16, fora de bloco condicional).
  - Situação do repositório do d-BMO em dois momentos:
    - (a) Versão registrada em commit, lida no início da revisão: o `bmo.ini` não definia
      `LITE_VERSION` nem outra opção que retirasse essas funções. Com ela, **o build do Bruce
      não cumpria a política desta nota**.
    - (b) Árvore de trabalho às 03:21 UTC de 2026-11-12, alterada por outra frente de trabalho
      durante a revisão e ainda sem commit (`bmo.ini` com SHA-256 `2d7d8813…2dacb`,
      `disabled_features.cpp` com SHA-256 `4c29a644…2d97`): o `build_src_filter` do `bmo.ini`
      exclui `modules/rf/rf_jammer.cpp`, `modules/ir/ir_jammer.cpp` e
      `modules/NRF24/nrf_jammer.cpp`, e `firmware/bruce/disabled_features.cpp` define
      `RFJammer::RFJammer`, `startIrJammer` e `nrf_jammer` de modo que só mostrem o aviso
      "Disabled on d-BMO". A revisão conferiu no Bruce 1.16.1 que esses três arquivos existem,
      que definem esses símbolos (`rf_jammer.cpp`, linha 15; `ir_jammer.cpp`, linha 698;
      `nrf_jammer.cpp`, linha 16) e que são esses os pontos chamados pelos menus. O próprio
      `firmware/bruce/README.md` declara que nada foi compilado nem executado: a eficácia da
      exclusão depende de compilação e de teste na bancada.
  - Mesmo na versão (b), **o build do Bruce não cumpre a política por inteiro**: "BLE Spam" e
    "Wifi Atks" (com "Beacon Spam" e desautenticação, segundo o README do Bruce) continuam no
    build, e a decisão de projeto deste bloco também os exclui. O `firmware/bruce/README.md`
    reconhece que a exclusão dos três bloqueadores não torna o build conforme e remete a decisão
    a esta nota. Isso deve constar da documentação do firmware até que o projeto decida.
  - `LITE_VERSION` é opção do próprio Bruce e o projeto não a usa. Ela retiraria "Jammer" e
    "IR Jammer" dos menus, junto com outras entradas dos mesmos blocos, mas não retira
    "NRF Jammer", "BLE Spam" nem "Wifi Atks"; o efeito completo dessa opção não foi apurado.
  - O corte de 30 segundos do `rf_jammer.cpp` não muda a análise: a função é destinada a impedir
    recepção.
  - O menu de RF do Bruce também oferece funções de transmissão ("Custom SubGhz", linha 20, e
    "Bruteforce", linha 30, do mesmo arquivo). Não se apurou se o Bruce aplica alguma restrição
    de faixa a elas; se não aplicar, o build também não cumpre as regras 2 a 5 da seção 10.1.

## 7. Divergências e alertas de coerência

1. **Numeração dos artigos do Regulamento da Res. 680.** A numeração usada (arts. 3º a 10) é a do
   resumo da busca. Um dos resumos atribui o dispositivo da Tabela I ao art. 7º sem transcrevê-lo
   com o número. Conferir.
2. **Potência no regime de salto em frequência em 902–928 MHz.** Um resumo atribuiu ao Ato 14.448
   o par "1 W com no mínimo 35 canais e 0,25 W com menos"; outro resumo, mais específico, indica
   que esse par é da Res. 506/2008 (revogada) e que o item 10.2.6 do Ato, com 125 mW e 1 W, é da
   faixa de 2,4 GHz. O limite de potência vigente para salto em 902–907,5 e 915–928 MHz ficou sem
   apuração. Na revisão, uma busca restrita a domínios oficiais (R5) devolveu um terceiro resumo:
   o par "1 W com pelo menos 35 canais de salto e 0,25 W com menos de 35" estaria no item
   10.2.5.1 do Ato 14.448, e os valores de 125 mW e 1 W da faixa de 2,4 GHz, nos subitens
   10.2.6.4 e 10.2.6.5; o mesmo resumo adverte que a página indexada parece misturar versões do
   ato. Continua sem apuração.
3. **Item 10.2.5.4** ("500 kHz" de largura máxima do canal de salto): atribuição ao bloco de
   902–928 MHz feita por um único resumo; outro resumo não o encontrou.
4. **Regime de intensidade de campo (D9).** O número do item não foi recuperado e os valores de
   50 mV/m e 500 µV/m só foram relatados pelo resumo para 902–907,5 e 915–928 MHz; os valores
   constavam da cadeia de busca. Na revisão (R9, consulta sem os valores), o resumo apontou o
   item 4.1.4, com remissão a uma "Tabela I" e com a regra de pico 20 dB acima da média, e também
   o item 9.1.1, com remissão a uma "Tabela IV" que traria 50 mV/m na fundamental para
   902–907,5, 915–928, 2.400–2.483,5 e 5.725–5.875 MHz e 250 mV/m para 24,00–24,25 GHz. As duas
   atribuições são incompatíveis entre si, o resumo adverte que os trechos misturam versões, e o
   valor dos harmônicos não foi repetido. O número do item continua em aberto.
5. **Resolução 718/2020.** Um resumo a lista como alteradora da Res. 680 ("femtocélulas"); a
   pertinência não foi confirmada por outro resultado.
6. **Resolução 793/2026.** Existência relatada por imprensa especializada; uma busca restrita aos
   domínios oficiais não a localizou e apontou a Res. 671/2016 como regulamento de uso do espectro
   ainda citado em ato de março de 2026. As fontes divergem quanto à vigência (na publicação ou em
   30 dias). Na revisão (R1, R13), a existência do ato, a assinatura em 05/11/2026 e a publicação
   no DOU de 09/11/2026 foram repetidas por resumos de imprensa e de um agregador que reproduz o
   DOU (S07, S17). A divergência de vigência também se repetiu: um resumo diz que a resolução
   entra em vigor na data de publicação; outro, que a data de publicação não é a de vigência. O
   portal da Anatel e o DOU continuaram inacessíveis, e nenhum resumo da revisão informou o que o
   ato altera no regulamento de radiação restrita.
7. **Ato 14.158/2025.** Data de aplicação obrigatória (06/04/2026) vinda de fontes secundárias e
   de resumo; o artigo de vigência não foi recuperado.
8. **Faixa de 433 MHz.** O Ato usaria 433–435 MHz no item 4.1.5 e 433,5–434,5 MHz na seção de
   RFID. A faixa aplicável depende do enquadramento do equipamento.
9. **Res. 715/2019.** A numeração dos arts. 22, 33, 55 e 83 vem de textos de consulta pública e
   de formulário, não do texto consolidado, e a Res. 780/2025 alterou o regulamento.
10. **Res. 769/2024 e Res. 792/2026 frente ao bloco 6.1.** Os trechos indexados citam os arts.
    2º e 4º (Res. 769) e o inciso VII do art. 2º e os arts. 3º, 5º e 6º (Res. 792) da Res. 680.
    Se esses números se referirem ao Regulamento anexo, e não aos artigos da própria Resolução
    que alteravam outros regulamentos, as pistas G3 a G6 (interferência, certificação, aviso e
    antena) podem estar revogadas ou alteradas. Ponto de verificação prioritário.
11. **Res. 792/2026.** A data de 15/10/2026 vem do título na base LegisWeb; um resumo a dá como
    data de publicação no DOU. Com vacância de 30 dias, o início de vigência fica em torno de
    meados de novembro de 2026 e não foi determinado. Na revisão, duas buscas (R2, R11) **não
    localizaram** a Res. 792 nem a página do LegisWeb (S04); apareceram só notícias de contexto
    sobre a "guilhotina regulatória" (S20, título de 08/10/2026, e S21, que fala em 12 normas e
    em aprovação pelo Conselho Diretor em 4 de novembro, sem ano claro no resumo). É ausência de
    evidência: a existência, a data e o conteúdo do ato ficam sem reprodução independente.
12. **Item 4.1.5 (433–435 MHz).** A pista A1 é a base da linha `SUBGHZ_433`. O fragmento veio da
    consulta 12, que já continha o número do item e os valores "10 mW", "e.i.r.p." e "250 nW". A
    grafia "(EIRP)" do fragmento difere da grafia da consulta, o que sugere texto indexado e não
    simples eco, mas isso não é verificação. Uma busca independente da revisão (R4), com a frase
    "potência irradiada limitada ao valor máximo de 10 mW" entre aspas, não devolveu o item.
13. **Resolução 789/2026.** Uma busca da revisão (R7) não encontrou fonte que confirme a
    existência de uma Res. 789 de julho de 2026 nem a revogação da Res. 772/2025; os resultados
    tratavam a Res. 772/2025 como o PDFF de referência. A consulta 7 do autor continha a palavra
    "REVOGADA". O endereço F16 foi devolvido por busca e não foi aberto. Ausência de evidência,
    não evidência de inexistência.
14. **Resolução 726/2020.** A seção 5 a descreve como revogadora do art. 9º e redefinidora do
    Anexo I da Res. 680; um resumo da revisão (R3) a descreve como revogadora de partes do art.
    8º e de dispositivos sobre faixas de 5 GHz. Conferir.
15. **Res. 715/2019, arts. 55 e 83, depois da Res. 780/2025.** Segundo um resumo da revisão
    (R10), a Res. 780/2025 acrescentou um § 2º ao art. 55 (texto não obtido) e reescreveu o art.
    83 em incisos separados para uso, comercialização e importação de produto não homologado. A
    numeração "art. 83, I" da pista J3 pode ser a da redação original. O mesmo resumo não
    encontrou disposição sobre construção artesanal ou uso próprio.
16. **Res. 760/2023, numeração.** As pistas da revisão situam a vedação geral (K2) no art. 2º do
    Regulamento anexo, com base em cópia de base privada (S18), e a vedação a pessoa natural e a
    pessoa jurídica de direito privado (K3) no § 1º do art. 4º, com base em paráfrase de imprensa
    (S19). Isso é compatível com a referência de K5 ao art. 4º, mas vem de fontes secundárias.
17. **Certificado do ESP32-S3-WROOM-1 (J10).** A validade até 11/07/2027 não foi repetida pela
    busca da revisão (R6), que relata o campo de validade sem preenchimento no trecho recuperado
    e uma data de 09/07/2025 na página de certificados da Espressif, diferente da data de
    manutenção de 02/07/2025. A grafia do endereço S02 também diverge (seção 9.1).
18. **Fim da exceção do item 10.3.4.4.** As fontes secundárias vistas na revisão (R12; S15, S16)
    repetem a revogação da exceção e a aplicação da largura mínima de 500 kHz a 6 dB, mas referem
    a exigência a equipamentos "do item 10.3.4" (a pista D7 a põe no item 10.3.1), dão ora
    05/10/2026 ora 06/10/2026 como marco, e divergem quanto ao LoRa: uma diz que a mudança atinge
    em especial tecnologias como o LoRa; um boletim de outra consultoria (TÜV SÜD, segundo o
    resumo, que não individualizou o endereço) diz que o LoRa não é afetado, aparentemente a
    propósito de outra alteração. Uma delas relata que o limite de
    espúrias desses equipamentos passou de 20 dB para 30 dB, o que é compatível com a pista H5.
19. **Endereços S05 e S06.** A busca da revisão (R13) não devolveu esses dois endereços; devolveu
    outros endereços dos mesmos veículos sobre o mesmo assunto (seção 9.1). Não foi possível
    abrir nenhum deles, de modo que não se sabe se S05 e S06 existem com essa grafia.

## 8. Pontos não verificados e roteiro de verificação

### 8.1 Lista de pontos não verificados nesta sessão

1. Texto literal e vigência de **todos** os dispositivos citados da Lei 9.472/1997, da Res.
   680/2017, do Ato 14.448/2017, da Res. 715/2019 e da Res. 760/2023 (nenhuma fonte primária
   aberta).
2. Texto consolidado da Res. 680 em 2026-11-12 e efeito das Res. 769/2024, 789/2026, 792/2026
   (inclusive a data de início de vigência) e 793/2026, em especial sobre os arts. 3º a 6º do
   Regulamento.
3. Conteúdo vigente da Tabela I (faixas proibidas), da Tabela II (limites gerais, em especial as
   linhas de VHF e UHF) e do Anexo I da Res. 680.
4. Texto consolidado do Ato 14.448 nos itens 4.1.5, 6.1, 6.2 e subitens, 10 e subitens, 13.1, no
   item de intensidade de campo de 902–928 MHz e nas linhas de 864–868 MHz, com as alterações dos
   Atos 1379/2019, 4776/2020, 1306/2021, 423/2022, 14.158/2025, 4746/2026 e 10.400/2026.
5. Limite de potência para salto em frequência em 902–907,5 e 915–928 MHz; admissibilidade de
   transmissor de faixa estreita sem salto; situação do LoRa após a revogação do item 10.3.4.4.
6. Base normativa para 315 MHz (regime de operação periódica e faixas proibidas) e existência de
   qualquer base para 868 MHz.
7. Para 13,56 MHz: distância e método de medida, tolerância de frequência, máscara das subfaixas.
8. Para 2,4 GHz: requisitos aplicados a WiFi e a Bluetooth LE e plano de canais no Brasil.
9. Regra de antena (art. 6º da Res. 680) e sua aplicação a módulo com conector SMA.
10. Res. 715: numeração e redação vigentes dos dispositivos sobre obrigatoriedade de homologação,
    uso próprio e fabricação artesanal; aplicação a aparelho de radiação restrita montado com
    módulos; existência ou não de qualquer dispensa.
11. Homologação do ESP32-S3-WROOM-1 no banco da Anatel (número, titular, validade, variantes
    cobertas) e situação da placa DevKitC-1 e dos módulos CC1101, PN532 e LoRa.
12. Definição normativa de BSR e numeração dos artigos da Res. 760.
13. Penas dos arts. 183 e 184 e lista de sanções do art. 173 da Lei 9.472.
14. Limite de 3.000 GHz no Glossário vigente e comprimento de onda do LED de IR comprado.
15. Funções de interferência no Bruce 1.16.1: **conferidas na revisão** no README e em oito
    arquivos-fonte (bloco 6.10 e seção 11.2). Continuam não apurados o código das funções de
    WiFi e de BLE, o efeito completo de `LITE_VERSION`, a existência de restrição de faixa nas
    funções de transmissão do Bruce e a eficácia (compilação e teste) da exclusão dos três
    bloqueadores feita na árvore de trabalho durante a revisão.
16. CC1101: faixas de frequência e potências máxima e mínima de saída **conferidas na revisão**
    no datasheet (S14). Continua não verificado o comportamento do módulo comprado (rede de
    casamento, potência e harmônicos reais), que só se apura por inspeção e medição.
17. Licitude da recepção e do armazenamento de sinais de terceiros (fora do escopo, não analisada).

### 8.2 Roteiro mínimo para fechar a verificação

Em ambiente com acesso aos domínios `informacoes.anatel.gov.br`, `sistemas.anatel.gov.br`,
`apps.anatel.gov.br`, `www.gov.br`, `www.planalto.gov.br`, `www.in.gov.br` e
`pesquisa.in.gov.br` (e, para as fontes secundárias, aos hosts da seção 9.1):

1. Abrir F03 e registrar, artigo por artigo, a redação e as notas de alteração do Regulamento da
   Res. 680, com as Tabelas I e II e o Anexo I. Abrir F16 e os textos das Res. 792 e 793 de 2026 e
   registrar o que alteram e desde quando.
2. Abrir F05 e copiar os itens listados no ponto 4 da seção 8.1, com as notas de redação.
3. Abrir F11 e localizar os dispositivos sobre obrigatoriedade de homologação e sobre uso próprio
   e fabricação artesanal; abrir F20 e F21 e o manual de equipamentos artesanais.
4. Abrir F12 e o Glossário (F15) para a definição de BSR e os artigos de vedação.
5. Abrir F01 e copiar os arts. 162, 163, 173 e 183 a 185.
6. Consultar o banco público de homologação da Anatel pelo modelo "ESP32-S3-WROOM-1" e pelos
   modelos dos demais módulos.
7. Atualizar a coluna "Verificada" da seção 10.2 linha a linha, com a data e o endereço da fonte.
8. Em caso de dúvida sobre o enquadramento do aparelho montado para uso próprio, formular
   consulta à Anatel pelos canais de atendimento e guardar a resposta junto a esta nota.

## 9. Fontes e registro de consultas

### 9.1 Endereços

Dos endereços abaixo, só S13 e S14 foram abertos (na revisão; seção 3.1). Os demais foram
devolvidos pela ferramenta de busca em 2026-11-12 e servem de ponto de partida para a verificação. A atribuição de cada
fragmento a um endereço é a indicada pelo resumo da busca e pode estar imprecisa, sobretudo
quando um mesmo resumo reuniu várias páginas (casos de F20, F21, F25 e F26).

Domínios oficiais (fonte primária ou página institucional):

| Id | Conteúdo | Endereço |
|---|---|---|
| F01 | Lei 9.472/1997 (Planalto) | `https://www.planalto.gov.br/ccivil_03/leis/l9472.htm` |
| F02 | Lei 9.472/1997 (cópia no portal de legislação da Anatel) | `https://informacoes.anatel.gov.br/legislacao/leis/2-lei-9472` |
| F03 | Res. 680/2017, texto com notas de alteração | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2017/936-resolucao-680` |
| F04 | Res. 680/2017 no DOU | `https://www.in.gov.br/materia/-/asset_publisher/Kujrw0TZC2Mb/content/id/19145767/do1-2017-06-29-resolucao-n-680-de-27-de-junho-de-2017-19145667` |
| F05 | Ato 14.448/2017, texto com notas de alteração | `https://informacoes.anatel.gov.br/legislacao/atos-de-certificacao-de-produtos/2017/1139-ato-14448` |
| F06 | Ato 1379/2019 | `https://informacoes.anatel.gov.br/legislacao/atos-de-certificacao-de-produtos/2019/1244-ato-1379` |
| F07 | Ato 4776/2020 | `https://informacoes.anatel.gov.br/legislacao/index.php/component/content/article?id=1467` |
| F08 | Ato 423/2022 | `https://informacoes.anatel.gov.br/legislacao/component/content/article/155-atos-de-certificacao-de-produtos/2022/1633-ato-423` |
| F09 | Ato 14.158/2025 | `https://informacoes.anatel.gov.br/legislacao/atos-de-certificacao-de-produtos/2025/2070-ato-14158` |
| F10 | Ato 4746/2026 | `https://informacoes.anatel.gov.br/legislacao/atos-de-certificacao-de-produtos/2026/2132-ato-4746` |
| F11 | Res. 715/2019 | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2019/1350-resolucao-715` |
| F12 | Res. 760/2023 | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2023/1842-resolucao-760` |
| F13 | Res. 760/2023 no DOU de 08/02/2023 | `https://pesquisa.in.gov.br/imprensa/servlet/INPDFViewer?jornal=515&pagina=42&data=08/02/2023&captchafield=firstAccess` |
| F14 | Res. 772/2025 (revogada) | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2025/2001-resolucao-772` |
| F15 | Res. 779/2025 (Glossário) | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2025/2029-resolucao-779` |
| F16 | Res. 789/2026 (PDFF) | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2026/2157-resolucao-789` |
| F17 | Res. 769/2024 | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2024/1979-resolucao-769` |
| F18 | Agenda Regulatória 2025-2026, item 23 | `https://www.gov.br/anatel/pt-br/regulado/agenda-regulatoria/agenda-regulatoria-2025-2026-item-23` |
| F19 | Página "Importação para Uso Próprio" | `https://www.gov.br/anatel/pt-br/regulado/certificacao-de-produtos/importacao-para-uso-proprio` |
| F20 | Página "Declaração de Conformidade para uso próprio" | `https://www.gov.br/anatel/pt-br/regulado/certificacao-de-produtos/declaracao-de-conformidade` |
| F21 | Página "Formulários e Manuais" de certificação | `https://www.gov.br/anatel/pt-br/regulado/certificacao-de-produtos/formularios-certificacao` |
| F22 | Portaria Anatel 3102/2025 | `https://informacoes.anatel.gov.br/legislacao/instrucoes-de-fiscalizacao/2089-portaria-3102` |
| F23 | Ato 10.988/2024 (requisitos técnicos de BSR) | `https://informacoes.anatel.gov.br/legislacao/component/content/article/165-atos-de-requisitos-tecnicos-de-gestao-do-espectro/2024/1969-ato-10988` |
| F24 | Res. 506/2008 (revogada; histórico) | `https://informacoes.anatel.gov.br/legislacao/resolucoes/2008/104-resolucao-506` |
| F25 | Textos de consulta pública no Participa Anatel que citam dispositivos da Lei 9.472 e da Res. 715 (a busca não permitiu atribuir cada fragmento a um identificador) | `https://apps.anatel.gov.br/ParticipaAnatel/VisualizarTextoConsulta.aspx?TelaDeOrigem=2&ConsultaId=20181` e identificadores 20322 e 20406 |
| F26 | Manual de orientações de homologação (anexo no sistema da Anatel) | `https://sistemas.anatel.gov.br/anexar-api/publico/anexos/download/6b6b7fbe4c10d3369afbefe22a125180` |
| F27 | Res. 780/2025 (endereço devolvido à busca da revisão, R10) | `https://informacoes.anatel.gov.br/legislacao/component/content/article/170-resolucoes/2025/2054-resolucao-780` |

Fontes secundárias e fontes não normativas (apenas pistas, salvo S13 e S14, abertas na revisão):

| Id | Conteúdo | Endereço |
|---|---|---|
| S01 | Espressif, certificado Anatel do ESP32-S3-WROOM-1 (PDF) | `https://documentation.espressif.com/ESP32-S3-WROOM-1%20Anatel%20Certification.pdf` |
| S02 | Espressif, certificado Anatel do ESP32-S3-WROOM-1 (PDF, outro endereço). A busca da revisão (R6) devolveu esse endereço com a grafia `...ESP32-S3-WROOM-1%20Anatel%20Certification.pdf`; não se sabe qual das duas grafias é a correta | `https://www.espressif.com/sites/default/files/ESP32-S3-WROOM-1%20ANATEL%20Certification.pdf` |
| S03 | Espressif, página de certificados | `https://www.espressif.com/en/support/documents/certificates` |
| S04 | LegisWeb, Res. Anatel 792/2026 | `https://www.legisweb.com.br/legislacao/?id=502950` |
| S05 | Teletime, novo Regulamento de Uso do Espectro (06/11/2026). Endereço não repetido pela busca da revisão (seção 7, item 19); ver S22 | `https://teletime.com.br/06/11/2026/anatel-aprova-novo-regulamento-de-uso-do-espectro/` |
| S06 | Mobile Time, novo RUE (06/11/2026). Endereço não repetido pela busca da revisão (seção 7, item 19); ver S23 | `https://www.mobiletime.com.br/noticias/06/11/2026/anatel-aprova-novo-rue/` |
| S07 | Atlas Público, novo RUE | `https://atlaspublico.com.br/noticias/anatel-aprova-novo-regulamento-de-uso-do-espectro-e-revoga-99780` |
| S08 | GMA Labs, regra de 500 kHz | `https://www.gmalabs.com/post/brazil-anatel-restricted-radiation-equipment-500-khz-rule` |
| S09 | C-PRAV, fim da exceção de largura a 6 dB abaixo de 1 GHz | `https://c-prav.com/2026/11/04/brazil-anatel-ends-the-6-db-bandwidth-exemption-for-sub-1-ghz-digital-modulation-devices/` |
| S10 | NCC, Ato 10.400/2026 | `https://nccgroup.com.br/pt-br/news/3030` |
| S11 | BRICS-OCP, Ato 10.400/2026 | `https://www.brics-ocp.com.br/ato-10400-anatel-wifi-6-ghz/` |
| S12 | Receita Federal, manual de importação, tópico Anatel (página oficial, mas comentário e não norma) | `https://www.gov.br/receitafederal/pt-br/assuntos/aduana-e-comercio-exterior/manuais/despacho-de-importacao/topicos-1/procedimentos-preliminares/outros-controles-1/anatel` |
| S13 | Repositório do Bruce, ref `1.16.1`. **Aberto na revisão** (2026-11-12): `README.md` e oito arquivos-fonte (menus de RF, BLE, WiFi, IR e NRF24 e os três arquivos de bloqueador; lista na seção 11.2), lidos em `https://raw.githubusercontent.com/pr3y/Bruce/1.16.1/<caminho>`. É fonte primária sobre o que o Bruce contém, não sobre norma. SHA-256 dos arquivos lidos: seção 11.2 | `https://github.com/pr3y/Bruce` |
| S14 | Texas Instruments, "CC1101 Low-Power Sub-1 GHz RF Transceiver", SWRS061I. **Aberto na revisão** (2026-11-12) em cópia hospedada por terceiro no GitHub, a mesma registrada como F-CC em `docs/hardware/sources/PERIPHERALS.md`; SHA-256 `a9314dc36ea0ca424bb47f5e5ce12125a76b3405467092fdf483b3c8cb8e9d6f`. O endereço do fabricante (`https://www.ti.com/lit/ds/symlink/cc1101.pdf`, segundo aquele arquivo) não foi tentado na revisão | `https://raw.githubusercontent.com/CaelC/cc1101_only/master/cc1101.pdf` |
| S15 | IB-Lenhardt, fim da exceção de largura a 6 dB em 06/10/2026 (devolvido à busca da revisão, R12) | `https://ib-lenhardt.com/news/anatel-6-db-bandwidth-exemption-to-end-on-october-6-2026` |
| S16 | IB-Lenhardt, Ato 14.158 (devolvido à busca da revisão, R4 e R12) | `https://ib-lenhardt.com/news/anatel-act-14158-update-of-conformity-requirements` |
| S17 | Atlas Público, reprodução do DOU com a Res. Anatel 793/2026 (devolvido à busca da revisão, R1) | `https://atlaspublico.com.br/regulacao/dou/resolucao-anatel-n-793-de-5-de-novembro-de-2026-903394` |
| S18 | Normas Brasil, cópia da Res. Anatel 760/2023 (devolvido à busca da revisão, R8) | `https://www.normasbrasil.com.br/norma/resolucao-760-2023_442215.html` |
| S19 | Tecnoblog, matéria que parafraseia o § 1º do art. 4º do Regulamento de BSR (devolvido à busca da revisão, R8) | `https://tecnoblog.net/noticias/casamento-de-isis-valverde-tera-bloqueador-de-celular-o-que-e-ilegal` |
| S20 | Teletime, 08/10/2026, título sobre eliminação de regras ociosas e do regulamento de white spaces (só o título foi visto; devolvido à busca da revisão, R1) | `https://teletime.com.br/08/10/2026/anatel-elimina-regras-ociosas-e-acaba-com-regulamento-de-white-spaces/` |
| S21 | DPL News, matéria sobre 12 normas na mira de nova guilhotina regulatória (devolvido à busca da revisão, R11) | `https://dplnews.com/?p=295405` |
| S22 | Teletime, boletim de 09/11/2026 sobre espectro (devolvido à busca da revisão, R13) | `https://teletime.com.br/09/11/2026/boletim-teletime-espectro/` |
| S23 | Mobile Time, página de etiqueta com o título "Anatel aprova novo Regulamento de Uso do Espectro" (devolvido à busca da revisão, R1 e R13) | `https://www.mobiletime.com.br/tag/regulamento/` |

### 9.2 Cadeias de busca usadas (2026-11-12)

As consultas foram feitas com a ferramenta de busca do ambiente; os resultados mudam com o tempo.
Estão listadas as consultas que sustentam as pistas desta nota; as marcadas com `[oficial]` foram
restritas a domínios oficiais.

1. `Resolução nº 680, de 27 de junho de 2017 Regulamento sobre Equipamentos de Radiocomunicação de Radiação Restrita "alterada pela Resolução" texto compilado` `[oficial]`
2. `Ato nº 14448, de 4 de dezembro de 2017 requisitos técnicos avaliação da conformidade equipamentos de radiocomunicação de radiação restrita alterado revogado ato vigente`
3. `Anatel "Resolução nº 772, de 16 de janeiro de 2025" radiação restrita altera Resolução 680`
4. `Anatel "Resolução nº 779, de 28 de abril de 2025" revoga art. 2º Resolução 680 definições`
5. `Anatel "Ato nº 10400" 12 de agosto de 2026 radiação restrita Ato 14448 Wi-Fi faixas`
6. `Anatel 2026 nova resolução radiação restrita revoga Resolução 680 Agenda Regulatória 2025-2026 item 23 aprovada Conselho Diretor`
7. `Anatel "Resolução Anatel nº 772" "REVOGADA" revogada pela Resolução PDFF 2025 2026 Plano de Atribuição, Destinação e Distribuição de Faixas de Frequências nova resolução`
8. `"Resolução Anatel nº 793" 2026 Regulamento de Uso do Espectro altera Regulamento sobre Equipamentos de Radiocomunicação de Radiação Restrita Resolução 680 artigos alterados`, e variante restrita a domínios oficiais
9. `"Resolução Anatel nº 792" 15 de outubro de 2026 guilhotina regulatória revoga altera Resolução 680 radiação restrita artigo`, e variante com `"Resolução nº 680, de 27 de junho de 2017" revogar artigos`
10. `"Resolução Anatel nº 789, de 06 de julho de 2026" PDFF Anexo I Resolução 680 tabela faixas de radiofrequências com restrições de uso radiação restrita`
11. `Ato 14448 Anatel radiação restrita "433" MHz "435 MHz" limite potência intensidade de campo item requisitos técnicos` `[oficial]`
12. `Ato 14448 Anatel item 4.1.5 "433" "435 MHz" "10 mW" "e.i.r.p." texto completo item 4.1 "Equipamentos de radiação restrita" condições "250 nW"` `[oficial]`
13. `Ato 14448 Anatel "13,553" "13,567" MHz intensidade de campo microvolts por metro 30 m radiação restrita` `[oficial]`
14. `Ato 14448 Anatel item "13,553-13,567" MHz qual item seção "Equipamentos operando na faixa" "13,110-14,010" tolerância de frequência "0,01%" RFID item 13.1 tabela` `[oficial]`
15. `Ato 14448 Anatel "902-907,5" "915-928" "2400-2483,5" espalhamento espectral salto em frequência modulação digital potência de pico máxima 1 W densidade espectral` `[oficial]`
16. `Ato 14448 Anatel item 10.2 10.3 "2.400-2.483,5" salto em frequência "75" frequências de salto "125 mW" modulação digital "6 dB" "500 kHz" "8 dBm" "3 kHz" e.i.r.p. ganho antena "6 dBi"` `[oficial]`
17. `Ato 14448 Anatel item 10.2.5 salto em frequência "902-907,5" "915-928" "35" canais de salto "0,25 Watt" "tempo médio de ocupação" "0,4 segundos" "20 dB" "250 kHz" "500 kHz"` `[oficial]`
18. `Ato 14448 Anatel "intensidade de campo média" "3 metros" "902-907,5 MHz, 915-928 MHz, 2.400-2.483,5 MHz, 5.725-5.875 MHz e 24,00-24,25 GHz" "50" milivolts por metro fundamental harmônicos "500" microvolts item` `[oficial]`
19. `Ato 14448 Anatel "operação periódica" acima de 70 MHz "260-470" intensidade de campo "3.750" "12.500" cessar transmissão 5 segundos telecomando` `[oficial]`
20. `Ato 14448 Anatel item 6 "dispositivos de operação periódica" "transmissor acionado manualmente" "cessar a transmissão" segundos "após" liberação condições "não são permitidas transmissões periódicas"` `[oficial]`
21. `Ato 14448 Anatel "864-868" MHz "944-948" radiação restrita aplicação item tabela "868" MHz Brasil faixa permitida equipamento radiação restrita` `[oficial]`
22. `Ato 14448 Anatel antena "conector" "antena integrada" equipamento de radiação restrita "somente a antena" requisito item 3 "ganho da antena" troca de antena usuário` `[oficial]`
23. `Resolução 680 Anatel Anexo I "faixas de radiofrequências com restrições de uso" tabela limites gerais de emissão "216-960" "200" microvolts por metro radiação restrita` `[oficial]`
24. `Resolução 680 Anatel Anexo I "Redação dada pela Resolução" 772 789 tabela faixas radiação restrita "433-435" "902-907,5" "915-928" "13,553-13,567" "2.400-2.483,5" aplicações` `[oficial]`
25. `Resolução 680 Anatel Regulamento radiação restrita "caráter secundário" "não têm direito à proteção contra interferências prejudiciais" "não podem causar interferência" art. 4º art. 5º art. 6º homologação "licenciamento" texto` `[oficial]`
26. `"Ato nº 14158" 1º de outubro de 2025 Anatel altera Ato 14448 item 10.3.4.4 revoga largura de faixa 6 dB 500 kHz radiação restrita`, e variante `[oficial]` sobre ementa, itens e vigência
27. `"Ato nº 4746" 2 de abril de 2026 Anatel altera Ato 14158 período de transição 6 de outubro de 2026 radiação restrita`
28. `gmalabs "Brazil: ANATEL Restricted Radiation Equipment 500 kHz Rule" Ato Anatel 2026 largura de faixa 500 kHz modulação digital radiação restrita`
29. `Resolução 715 de 23 de outubro de 2019 Anatel Regulamento de Avaliação da Conformidade e de Homologação de Produtos para Telecomunicações "uso próprio" homologação dispensa "sem direito à comercialização" artigo alterada pela Resolução` `[oficial]`
30. `Resolução 715 Anatel art. "homologação é pré-requisito obrigatório" comercialização e utilização produtos para telecomunicações "É vedada" utilização produto não homologado artigo sanção Lei 9472 art. 173` `[oficial]`
31. `Anatel "fabricação artesanal para uso próprio" homologação Resolução 715 artigo "Declaração de Conformidade" produtos artesanais procedimento operacional Ato` `[oficial]`
32. `Resolução 715 Anatel "construção artesanal" "uso do próprio Requerente" artigo "Declaração de Conformidade" "qualquer pessoa física ou jurídica" art. 13 art. 22 Ato procedimento operacional Declaração de Conformidade` `[oficial]`
33. `Anatel homologação "uso próprio" produto montado pelo próprio usuário protótipo sem comercialização Declaração de Conformidade Resolução 715 radiação restrita pessoa física`
34. `gov.br Anatel "Declaração de Conformidade para uso próprio" página quais produtos "fabricação artesanal" radiação restrita requisitos documentos pessoa física sistema Mosaico SCH` `[oficial]`
35. `ESP32-S3-WROOM-1 Anatel homologação número certificado Espressif "ESP32-S3-WROOM-1" Anatel`
36. `Lei 9.472 de 1997 art. 162 "radiação restrita" art. 163 § 2º "independerá de outorga" art. 183 "Desenvolver clandestinamente atividades de telecomunicação" detenção texto planalto` `[oficial]`
37. `Lei 9472 "Art. 163" "radiação restrita" "Independerá de outorga" "equipamentos de radiação restrita definidos pela Agência" "Art. 162" certificação equipamentos "Art. 156"`
38. `Lei 9472 "Art. 162" "A operação de estação transmissora de radiocomunicação está sujeita à licença de funcionamento prévia" "§ 2º" "É vedada a utilização de equipamentos emissores de radiofreqüência sem certificação expedida ou aceita pela Agência"`
39. `Resolução Anatel nº 760 de 6 de fevereiro de 2023 Regulamento sobre Bloqueador de Sinais de Radiocomunicações BSR uso restrito estabelecimentos penitenciários vedado artigo texto vigente` `[oficial]`
40. `Portaria Anatel nº 3102 de 22 de dezembro de 2025 instrução de fiscalização art. 183 Lei 9.472 uso não autorizado de radiofrequência interferência prejudicial radiação restrita` `[oficial]`
41. `Anatel Glossário Resolução 779 "Ondas de Rádio" "3.000 GHz" definição "Radiocomunicação" "ondas eletromagnéticas" frequências inferiores infravermelho` `[oficial]`
42. `Bruce firmware pr3y RF menu "Jammer" "Jammer Full" "Jammer Intermittent" WiFi deauth "BLE spam" features list wiki`

Consultas da revisão adversarial (2026-11-12, mesma ferramenta; `[oficial]` = restrita a
`informacoes.anatel.gov.br`, `anatel.gov.br`, `gov.br` e `in.gov.br`):

- R1. `"Resolução Anatel nº 793" 2026 Regulamento de Uso do Espectro radiação restrita`
- R2. `"Resolução Anatel nº 792" outubro 2026 revoga altera "Resolução nº 680" radiação restrita`
- R3. `Resolução Anatel 680/2017 Regulamento radiação restrita "Art. 4º" "Art. 5º" "Art. 6º" texto compilado certificação antena aviso cadastramento licenciamento` `[oficial]`
- R4. `Ato 14448 Anatel "433-435 MHz" "potência irradiada limitada ao valor máximo de 10 mW" emissões fora da faixa item`
- R5. `Ato 14448 Anatel salto em frequência "902-907,5 MHz e 915-928 MHz" "potência de pico máxima de saída" transmissor canais de salto Watt item 10.2.5` `[oficial]`
- R6. `ESP32-S3-WROOM-1 Anatel homologação número "ICC" certificado conformidade técnica Espressif`
- R7. `"Resolução Anatel nº 789" julho 2026 Plano de Atribuição, Destinação e Distribuição de Faixas de Frequências revoga Resolução 772`
- R8. `Resolução Anatel 760/2023 Regulamento Bloqueador de Sinais de Radiocomunicações "São vedados em todo o país a comercialização, a posse e o emprego de BSR" artigo "Em hipótese alguma a Anatel anuirá"`
- R9. `Ato 14448 Anatel item "Equipamentos de Radiação Restrita operando nas faixas 902-907,5 MHz, 915-928 MHz, 2.400-2.483,5 MHz, 5.725-5.875 MHz e 24,00-24,25 GHz" intensidade de campo média número do item` `[oficial]`
- R10. `Resolução Anatel 715/2019 Regulamento de Avaliação da Conformidade "Art. 55" homologação pré-requisito obrigatório "Art. 83" produto não homologado "Declaração de Conformidade" construção artesanal uso próprio alterado Resolução 780` `[oficial]`
- R11. `legisweb "Resolução ANATEL" "Nº 792" 2026 guilhotina regulatória revoga dispositivos`
- R12. `Anatel 500 kHz 6 dB bandwidth rule restricted radiation Act 14158 item 10.3.4.4 revoked LoRa 125 kHz October 2026 transition Act 4746`
- R13. `teletime mobiletime "novo Regulamento de Uso do Espectro" Anatel aprova novembro 2026 radiação restrita Glossário vigência`

As consultas R3, R5, R8, R9 e R10 também continham expressões candidatas (por exemplo os números
de artigo, a frase de vedação do BSR e a frase das faixas), de modo que o risco de eco descrito
na seção 3.2 vale igualmente para elas.

## 10. Política padrão do firmware

Esta seção é a tradução da nota em regra de produto. Ela é decisão de engenharia do projeto,
mais restritiva do que as pistas normativas exigiriam, justamente porque essas pistas não foram
verificadas.

### 10.1 Regras

1. **Recepção permitida** em todos os rádios, sem habilitação prévia.
2. **Transmissão desabilitada por padrão** em todos os rádios e em todas as faixas. Isso inclui o
   campo do leitor NFC, o WiFi, o Bluetooth LE e o rádio de qualquer cartucho. O infravermelho,
   que não é rádio, segue a mesma regra por decisão do projeto.
3. A transmissão só pode ser habilitada **por faixa**, por **configuração local explícita do
   usuário**, feita no próprio aparelho. Não há habilitação remota, por rede, por script carregado
   de fora nem pelo modo agente (o `README.md` já exclui RF, NFC e IR do alcance do agente).
4. Só existe habilitação para faixas com linha na tabela da seção 10.2 com estado "habilitável".
   Fora delas, toda requisição de transmissão é recusada com `Status::NotPermitted`.
5. A verificação é feita no ponto mais baixo possível do firmware (antes de comandar o estado de
   transmissão do transceptor) e considera a emissão inteira: frequência central menos e mais
   metade da largura ocupada deve ficar dentro de f_min e f_max. Duas linhas não comportam essa
   verificação e ficam fora dela (correção de coerência feita na revisão): em `NFC_13M56`, a
   portadora é fixa e a emissão modulada é mais larga que os 14 kHz da subfaixa principal, de
   modo que a regra aplicada ao pé da letra recusaria toda operação NFC (bloco 6.6); em
   `WIFI_BLE_2G4`, frequência e largura são controladas pela pilha do fabricante e pelo código de
   país (bloco 6.7). Nessas duas linhas, o que o firmware decide é ligar ou não o rádio; o
   atendimento da máscara ou dos limites só se demonstra por medição (seção 10.4).
6. Cada linha carrega o indicador "verificada em fonte primária". Enquanto o indicador for
   "não", a interface mostra o aviso de limite não verificado e pede uma segunda confirmação antes
   de habilitar. A habilitação não persiste entre versões de firmware que alterem a tabela.
7. Cada linha tem teto de potência e limites de tempo próprios (duração máxima por emissão,
   intervalo mínimo, tempo máximo por canal quando houver salto). Os valores iniciais são
   conservadores e só mudam com medição registrada.
8. **Nunca há função de bloqueio de sinais** (seção 6.10): nenhuma emissão destinada a impedir ou
   degradar a recepção de terceiros, em nenhuma faixa.
9. Toda habilitação, e toda recusa por política, fica registrada no log local.
10. A política vale para o firmware próprio do projeto. O build do Bruce em `firmware/bruce/` não
    a implementa; a exclusão dos três bloqueadores de sinal, feita na árvore de trabalho durante
    a revisão, cobre só parte da regra 8 (seção 6.10).

### 10.2 Tabela de faixas

Coluna "Verificada": verificação do texto de limite e das bordas em fonte primária aberta.
**Todas as linhas estão "não"** em 2026-11-12. As bordas e os limites abaixo são pistas de nível B
(seção 3.2) e devem ser conferidos antes de qualquer uso como regra. A revisão adversarial do
mesmo dia não conseguiu abrir nenhuma fonte normativa e **manteve "Não" em todas as linhas**; a
linha `SUBGHZ_433` tem, além disso, risco de eco registrado (seção 7, item 12).

Linhas habilitáveis por configuração local explícita:

| Nome | Rádio | f_min (Hz) | f_max (Hz) | Texto do limite (pista, não verificado) | Fonte da pista | Verificada |
|---|---|---:|---:|---|---|---|
| `SUBGHZ_433` | CC1101 | 433 000 000 | 435 000 000 | Potência irradiada de até 10 mW e.i.r.p.; emissões fora da faixa abaixo de 250 nW e.i.r.p. (até 1000 MHz; valor acima disso não recuperado) | Ato 14.448/2017, Anexo I, item 4.1.5 (bloco 6.2) | Não |
| `SUBGHZ_902` | CC1101; cartucho LoRa | 902 000 000 | 907 500 000 | Regime de intensidade de campo: 50 mV/m (média) a 3 m na fundamental e 500 µV/m nos harmônicos, pico até 20 dB acima da média (cerca de 0,75 mW e.i.r.p., cálculo). Os regimes de salto em frequência e de modulação digital do item 10 têm condições próprias e limite de potência não apurado | Ato 14.448/2017, Anexo I, item de intensidade de campo não identificado (pistas conflitantes: 4.1.4 ou 9.1.1; seção 7, item 4) e item 10 (bloco 6.5) | Não |
| `SUBGHZ_915` | CC1101; cartucho LoRa | 915 000 000 | 928 000 000 | Igual a `SUBGHZ_902` | Igual a `SUBGHZ_902` | Não |
| `NFC_13M56` | PN532 | 13 553 000 | 13 567 000 | Intensidade de campo de até 15.848 µV/m a 30 m; subfaixas adjacentes (13,410–13,553 e 13,567–13,710 MHz) com 334 µV/m a 30 m | Ato 14.448/2017, Anexo I, item 13.1, redação do Ato 1379/2019 (bloco 6.6) | Não |
| `WIFI_BLE_2G4` | ESP32-S3 | 2 400 000 000 | 2 483 500 000 | Item 10: salto em frequência com no mínimo 15 frequências e pico de 125 mW (menos de 75 frequências) ou 1 W (75 ou mais); modulação digital com pico de até 1 W e densidade de até 8 dBm em 3 kHz. Na prática, o limite é a configuração certificada do módulo | Ato 14.448/2017, Anexo I, itens 10.2.6, 10.3.2 e 10.3.3 (bloco 6.7) | Não |

Linhas bloqueadas (não habilitáveis nem por configuração local, até verificação):

| Nome | Rádio | f_min (Hz) | f_max (Hz) | Motivo | Fonte da pista | Verificada |
|---|---|---:|---:|---|---|---|
| `SUBGHZ_315_CONTROLE` | CC1101 | 260 000 000 | 470 000 000 | Os valores são os da linha de tabela do regime de operação periódica (3.750 a 12.500 µV/m a 3 m, interpolação linear; cerca de 6.042 µV/m em 315 MHz, cálculo), não uma faixa de permissão. Faixas proibidas da Tabela I não obtidas; condições de tempo e de acionamento a conferir | Ato 14.448/2017, Anexo I, itens 6.2 e subitens (bloco 6.3) | Não |
| `SUBGHZ_868` | CC1101; cartuchos | sem valor | sem valor | Nenhuma base normativa localizada para uso genérico de 868 MHz | Bloco 6.4 | Não |
| `SUBGHZ_INTERVALO_907_915` | CC1101; cartucho LoRa | 907 500 000 | 915 000 000 | Intervalo entre as duas faixas candidatas; transmissão sempre recusada | Bloco 6.5 | Não |

Qualquer outra frequência que o hardware alcance (por exemplo 300–348 MHz fora do caso acima,
387–433 MHz, 435–464 MHz, 779–902 MHz) não tem linha: recepção permitida, transmissão recusada.

Precedência (esclarecimento feito na revisão): os valores de 260 a 470 MHz da linha
`SUBGHZ_315_CONTROLE` são os da linha de tabela do regime relatado, não uma faixa de bloqueio do
firmware, e contêm a linha habilitável `SUBGHZ_433`. Em 433–435 MHz vale `SUBGHZ_433`. No
restante, "linha bloqueada" e "sem linha" produzem o mesmo resultado (transmissão recusada com
`Status::NotPermitted`), de modo que a sobreposição não muda o comportamento; ao codificar a
tabela, a linha `SUBGHZ_315_CONTROLE` não deve ser tratada como intervalo que anule
`SUBGHZ_433`.

### 10.3 Por que o firmware não garante conformidade de potência irradiada

O firmware controla registradores; os limites são de campo ou de potência **irradiados**. Entre
uma coisa e outra há elementos que o firmware não mede nem conhece:

- **Módulo:** a potência conduzida para um mesmo valor de registrador varia com o módulo, com a
  unidade, com a tensão de alimentação e com a temperatura. Módulos genéricos podem usar chips
  compatíveis com comportamento diferente do datasheet.
- **Casamento e filtro:** o módulo CC1101 do projeto é vendido como de 433 MHz e presume-se
  casado para essa faixa (circuito não inspecionado). Em outras faixas, a potência na saída e o
  nível de harmônicos não seguem a tabela de PA nominal.
- **Antena:** o conector SMA aceita qualquer antena. O ganho, o descasamento e o cabo alteram a
  potência irradiada, e o firmware não sabe qual antena está instalada.
- **Tabela de PA:** a relação entre o índice de potência e os dBm de saída é nominal, definida
  pelo fabricante do chip para um circuito de referência que não é o do módulo. O datasheet diz
  que os valores de potência de saída são entregues a uma carga de 50 Ω pela rede de casamento
  dos projetos de referência CC1101EM, e que a potência de saída pode ser restringida por limites
  regulatórios (SWRS061I, Table 10, p. 16 e 17, S14).
- **Carcaça e montagem:** bateria, alto-falante, mão do usuário e a carcaça mudam o diagrama de
  irradiação; no NFC, mudam o campo a 30 m.
- **Rádio do ESP32-S3:** a conformidade depende de manter antena, leiaute e dados de calibração
  certificados; o firmware pode piorar esse quadro, não melhorá-lo.

Logo, os limites de firmware são necessários e não suficientes. A coluna "Verificada" trata do
texto da regra; a conformidade do aparelho é outra demonstração, feita por medição.

### 10.4 O que precisaria ser medido

Para cada combinação de módulo, antena, carcaça e faixa habilitável:

1. Potência conduzida na porta de antena para cada índice da tabela de PA e cada frequência de
   uso, com analisador de espectro ou medidor de potência e atenuador calibrado.
2. Potência irradiada (e.i.r.p.) ou intensidade de campo na distância que a norma indicar (3 m
   nas faixas de UHF e 2,4 GHz e 30 m em 13,56 MHz, conforme as pistas), em ambiente de ensaio
   adequado, com a antena e a carcaça finais.
3. Largura de faixa ocupada (a 20 dB e a 6 dB, conforme o regime) e posição da emissão em relação
   às bordas da faixa.
4. Emissões fora da faixa e espúrias, com atenção aos harmônicos (segundo e terceiro) e às faixas
   proibidas da Tabela I da Res. 680.
5. Erro e deriva de frequência com temperatura e com a tensão da bateria ao longo da descarga.
6. Comportamento no tempo: duração de cada emissão, intervalo entre emissões, tempo de ocupação
   por canal e número de canais em salto de frequência; verificação de que os limitadores de
   firmware atuam.
7. No NFC: campo com o leitor ativo, bandas laterais nas subfaixas adjacentes e efeito da
   carcaça.
8. No 2,4 GHz: potência e densidade espectral por modo (802.11b/g/n e Bluetooth LE), emissões nas
   bordas da faixa, com o leiaute final da PCB.
9. Emissões com dois rádios ativos ao mesmo tempo (por exemplo CC1101 e WiFi).
10. Incerteza de medição e rastreabilidade dos instrumentos. Para fins de avaliação da
    conformidade, os ensaios seguem os procedimentos e laboratórios aceitos pela Anatel (requisito
    não verificado nesta sessão); medições de bancada servem para engenharia, não para
    homologação.

Até que essas medições existam, o teto de potência de cada linha permanece no menor ajuste útil
e a documentação do projeto não deve afirmar conformidade.

## 11. Registro da revisão adversarial (2026-11-12)

Revisão feita no mesmo dia da elaboração, por revisor distinto do autor, com a tarefa de reabrir
as fontes citadas, comparar fragmentos, números de dispositivo, bordas e limites, e conferir a
situação de cada norma.

### 11.1 O que foi possível conferir

| Objeto | Resultado |
|---|---|
| Fragmentos, números de artigo e de item, bordas de faixa e limites frente à fonte primária | **Nenhum pôde ser comparado.** Os hosts oficiais continuaram inacessíveis (seção 3.1). Nenhuma pista da seção 6 mudou de nível |
| Situação de cada norma em 2026-11-12 | Só por novas buscas (nível B). Res. 793/2026 repetida por imprensa e agregador; Res. 789/2026 e Res. 792/2026 **não repetidas**; demais pistas sem contradição encontrada, salvo as divergências dos itens 12 a 19 da seção 7 |
| Cálculos (nível C) | Refeitos e conferidos: 10 mW = +10 dBm; 250 nW ≈ −36,0 dBm; harmônicos de 433,92 MHz em 867,84 e 1301,76 MHz; interpolação em 315 MHz = 6.041,7 µV/m; (E·d)²/30 = 10,95 µW ≈ −19,6 dBm; 0,25% de 315 MHz = 787,5 kHz; 50 mV/m a 3 m = 0,75 mW ≈ −1,25 dBm; 500 µV/m a 3 m = 75 nW; 15.848 µV/m = 84,0 dBµV/m; c/940 nm ≈ 319.000 GHz. Sem correção |
| Afirmações de nível R (repositório) | Conferidas no `README.md` (linhas 35, 106 a 108, 124 a 130, 381, 507, 529 a 530, 541, 616 e 685 a 692 da árvore de trabalho de 2026-11-12), em `firmware/core/include/dbmo/core/status.h` (linha 24), em `firmware/teste_cc1101/teste_cc1101.ino` (linha 3), em `firmware/bruce/bmo.ini` e em `firmware/bruce/build.sh`. Sem divergência na versão em commit; um ajuste de redação (SX1276/1278 é exemplo do `README.md`, não decisão). Os arquivos de `firmware/bruce/` foram alterados por outra frente de trabalho durante a revisão, e as afirmações sobre eles foram reescritas para os dois estados (seção 4 e bloco 6.10) |
| Bruce 1.16.1 (S13) | **Aberto.** Achados no bloco 6.10: bloqueadores de RF, IR e NRF24, "BLE Spam" e "Wifi Atks" presentes no código da ref que o projeto compila |
| Datasheet do CC1101 (S14) | **Aberto.** Table 3 (p. 8): 300–348, 387–464 ("If using a 27 MHz crystal, the lower frequency limit for this band is 392 MHz") e 779–928 MHz. Table 10 (p. 16 e 17): ajuste mais alto de +10 dBm em 315 e 433 MHz, +12 dBm em 868 MHz e +11 dBm em 915 MHz; ajuste mais baixo de −30 dBm; "Output power may be restricted by regulatory limits." |

### 11.2 Identificação dos arquivos abertos

Arquivos lidos em `https://raw.githubusercontent.com/pr3y/Bruce/1.16.1/<caminho>` (SHA-256 do
conteúdo baixado em 2026-11-12):

| Caminho | SHA-256 |
|---|---|
| `README.md` | `1628dc0783d9b3ab5608bd1de478d5bcd04ff73d70f082ac17282c928d1af7f4` |
| `src/core/menu_items/RFMenu.cpp` | `5f1e9bf7bd89bacdf9e91141170d4eae2850539de7e82913a8ea0449e002b456` |
| `src/core/menu_items/BleMenu.cpp` | `c25ff85b34a06fb491b30da7e0e240672a8c4e27ac532ba130ddaa513a8bff2a` |
| `src/core/menu_items/WifiMenu.cpp` | `f5b39c3c2460d03e63997b0eb14a39f65d8e043b8ec428c5e13bbffb1a34a95a` |
| `src/modules/rf/rf_jammer.cpp` | `96205933caceef6a864fb5170b4a6b093483d846cc9534b038e8233fdf1af726` |
| `src/modules/ir/ir_jammer.cpp` | `6430d957567cc34986d5ff15e58f92d800b82ff8bdb59b67340e15f85cced439` |
| `src/modules/NRF24/nrf_jammer.cpp` | `cc2d2e70a6f680677cd516a746b447cf219d155b03f55b9f6835f51781ec62d7` |
| `src/core/menu_items/IRMenu.cpp` | `88e6688901468c5e53e53eda8ac9bd374eaf2a36dda2ade27ff5d5efa82a17ef` |
| `src/core/menu_items/NRF24.cpp` | `e7b28860968fe43feb0ae3519170baa4b01fb5fb2c843543270517fbb94350bd` |

Arquivos do próprio repositório alterados por outra frente de trabalho durante a revisão, no
estado lido às 03:21 UTC de 2026-11-12 (sem commit): `firmware/bruce/bmo.ini`, SHA-256
`2d7d8813ee65e8bd0f004112daaafcd6add9324061e229b37f83a6949472dacb`;
`firmware/bruce/disabled_features.cpp`, SHA-256
`4c29a6447a6ad0e77d75939f3d0c9a8eb40887ecfcb25a5a14ed2d30559b2d97`. Se esses arquivos mudarem
de novo, o bloco 6.10 precisa ser relido contra a versão que for registrada em commit.

Datasheet do CC1101: endereço e SHA-256 na linha S14 da seção 9.1. Os metadados do PDF trazem o
título "CC1101 Low-Power Sub-1 GHz RF Transceiver datasheet (Rev. I)" e a palavra-chave
"SWRS061I"; a cópia está em repositório de terceiro, não no sítio do fabricante.

### 11.3 Correções feitas no texto

1. Registro exato do que a mensagem de falha do ambiente diz e das tentativas da revisão,
   distinguindo falha registrada de ausência de tentativa (seção 3.1).
2. Tabela de risco de eco por pista, a partir das próprias cadeias de busca (seção 3.2).
3. Rebaixamento explícito para "não verificado" dos dois parágrafos de histórico que estavam
   redigidos como fato (blocos 6.2 e 6.4) e da linha de normas históricas (seção 5).
4. Marcação de divergências novas: Res. 789 e 792 não repetidas, Res. 726/2020, arts. 55 e 83 da
   Res. 715, numeração da Res. 760, validade do certificado da Espressif, item da intensidade de
   campo, potência no regime de salto, fim da exceção do item 10.3.4.4 e endereços S05 e S06
   (seção 7, itens 2, 4, 6 e 11 a 19).
5. Separação entre interpretação e texto normativo no bloco 6.10, parte (ii): as duas inferências
   sobre interferência passaram a estar rotuladas como leitura do autor, e a aplicação à faixa de
   2,4 GHz foi qualificada.
6. Substituição da pista secundária sobre o Bruce por evidência do código (bloco 6.10) e da
   referência ao "enunciado da tarefa" por citação do datasheet do CC1101 (seção 4).
7. Correção técnica no bloco 6.6: as bandas laterais do NFC não ficam só nas subfaixas
   adjacentes.
8. Duas correções de coerência na política: exceção à regra 5 para `NFC_13M56` e `WIFI_BLE_2G4`
   (seção 10.1) e precedência entre `SUBGHZ_315_CONTROLE` e `SUBGHZ_433` (seção 10.2).
9. Afirmações sobre o casamento do módulo CC1101 passaram a "presume-se", por não haver inspeção
   do circuito (blocos 6.3 e 6.5 e seção 10.3).

A coluna "Verificada" da seção 10.2 não foi alterada: continua "Não" em todas as linhas.

### 11.4 O que a revisão não pôde resolver

1. Tudo o que depende do texto das normas: seção 8.1, itens 1 a 14. Exige acesso aos hosts
   oficiais ou cópia dos textos fornecida por quem os tenha.
2. Existência, data, vigência e conteúdo das Res. 789, 792 e 793 de 2026 e seu efeito sobre os
   arts. 3º a 6º do Regulamento da Res. 680 (seção 7, itens 6, 10, 11 e 13).
3. Decisão do projeto: manter como habilitáveis, com aviso e segunda confirmação, linhas cujo
   limite não foi verificado (`SUBGHZ_433`, `SUBGHZ_902`, `SUBGHZ_915`, `NFC_13M56`,
   `WIFI_BLE_2G4`), ou bloqueá-las até a verificação, como já se faz com 315 MHz e 868 MHz. Na
   prática, bloquear `NFC_13M56` e `WIFI_BLE_2G4` tornaria o leitor NFC e o modo agente
   inoperantes.
4. Decisão do projeto sobre o build do Bruce: o que mais sai do build além dos três
   bloqueadores já excluídos na árvore de trabalho ("BLE Spam", ataques de WiFi e funções de
   transmissão sem tabela de faixas), ou documentar que ele fica fora da política. A exclusão
   dos bloqueadores ainda precisa ser compilada e testada.
5. Enquadramento do aparelho montado para uso próprio no regime de homologação (bloco 6.9), que
   pede consulta à Anatel.
6. Potência irradiada, harmônicos e largura ocupada reais de cada combinação de módulo, antena e
   carcaça (seção 10.4), que só se obtêm por medição.
