## Problema

Como reproduzir em software, com C++ moderno, o hardware de um console de 16
bits, com fidelidade suficiente para que os jogos originais rodem sem nenhuma
alteração. E quanto custa, em desempenho e em complexidade, cada nível de
fidelidade adotado.

O emulador serve de instrumento de medida. Duas questões guiam a investigação:

- **Q1.** Quais recursos de C++ moderno descrevem as instruções de um
  processador antigo sem cobrar tempo enquanto o programa roda? Para responder,
  comparo três formas de organizar a execução das instruções.
- **Q2.** Quão perto de emuladores maduros chega uma implementação escrita só a
  partir de documentação pública? Para responder, uso suítes de teste e
  comparação de imagem contra um emulador de referência.

O tema se presta a isso porque dá para conferir o acerto por fora. Existem
suítes públicas de teste e emuladores consolidados, então a avaliação sai em
números.

## Escopo

| Implementado | Excluído |
|---|---|
| Processador 65C816: as 256 instruções, o cálculo de endereços e o tempo de cada operação | Áudio por inteiro. Exige um segundo processador (SPC700) e um sintetizador dedicado |
| Memória do console e leitura do cartucho, nos dois formatos usados pelos jogos | Chips extras que alguns cartuchos traziam |
| Transferência direta de dados para o vídeo, usada por quase todo jogo | Modos de vídeo de alta resolução |
| Vídeo desenhado linha a linha: fundos, objetos móveis, prioridade e memórias de vídeo | Salvar e restaurar estado, rebobinar, jogo em rede |
| Leitura do controle | Precisão abaixo do ciclo de relógio |

## Camada de apresentação

A imagem chega à tela por uma camada gráfica em Vulkan construída sobre um motor de renderização. A cada quadro, a imagem de
256 por 224 pixels produzida pelo emulador é enviada à placa de vídeo e
desenhada ocupando a tela, sem filtros que mudem a aparência. Sem filtro, dá
para compará-la pixel a pixel com a do emulador de referência. O motor e o
núcleo do emulador são os dois componentes de minha autoria no sistema
entregue.

## Entrega em três níveis

| Nível | Critério objetivo | Prazo |
|---|---|---|
| 1. Piso, obrigatório | 95% ou mais das instruções aprovadas na suíte pública de testes; jogos de teste de fundo e de objetos móveis corretos | 25/10 |
| 2. Alvo | *Super Mario World* com abertura, tela de título e primeira fase jogável, sem áudio | 01/11 |
| 3. Estendido | Efeitos de vídeo não exigidos pelo piso, como composição de cores e o modo de rotação e escala (Mode 7) | Se houver folga |

O nível 1 sustenta o artigo sozinho. Se o nível 2 não ficar pronto até o
congelamento, entrego no nível 1 e o artigo analisa por que não deu tempo, o que
também é resultado.

## Cronograma

| Período | Trabalho |
|---|---|
| 24/08-20/09 | Estudo da arquitetura do console; processador validado contra a suíte de testes |
| 21/09-11/10 | Memória, cartucho, tempo e interrupções, transferência de dados para o vídeo, ferramentas de inspeção |
| 12/10-25/10 | Vídeo: fundos, objetos móveis, leitura do controle. Nível 1 fechado |
| 26/10-01/11 | Integração e depuração do jogo-alvo. Congelamento em 01/11 |
| 02/11-08/11 | Medição, capítulo de resultados, revisão final |
| 10/11 | Entrega |

O artigo é escrito por etapa ao longo do semestre, com texto para revisão desde
as primeiras semanas. O objetivo é ter o projeto 99% pronto até
01/11 e usar a última semana para garantir que o artigo está bem escrito.

## Avaliação

Todas as métricas são objetivas e podem ser refeitas por outra pessoa a partir
do repositório:

- **Suíte pública de testes do processador** (ProcessorTests 65816), 10.000
  casos por instrução: percentual aprovado e percentual com contagem de tempo
  exata. Meta de 95%.
- **Jogos de teste públicos**: tabela de aprovado e reprovado, documentada
  integralmente.
- **Comparação de imagem contra o bsnes**, emulador usado como referência: percentual de pixels idênticos por quadro. Meta de 99% em cenas
  paradas.
- **Desempenho**: quadros por segundo e tempo gasto por quadro em cada parte do
  sistema, incluindo a camada gráfica e duas formas de enviar a imagem à placa
  de vídeo.

## Base já construída e riscos

A camada gráfica acima já foi exercitada em um emulador de CHIP-8 completo, o
que reduz o risco de integração.

O risco principal é a depuração do vídeo. Para reduzi-lo, construo as
ferramentas de inspeção das memórias de vídeo antes da lógica de desenho, na
semana de 05/10. O risco de prazo é coberto pela entrega em níveis e pelo
congelamento antecipado.

## Legalidade

A implementação parte apenas de documentação pública. Nenhum jogo comercial é
distribuído; o repositório contém apenas jogos de teste e
produções amadoras de licença livre. As patentes do console expiraram.
