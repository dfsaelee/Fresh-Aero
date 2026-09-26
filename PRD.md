# G3X Fresh Air — Product Requirements Document

| Campo | Definição |
| --- | --- |
| Versão | 0.1.0 |
| Status | M3 implementado; M4 planejado |
| Target inicial | C++20, JUCE fixado, CMake |
| Entrega inicial | VST3 64-bit para Windows; Standalone para desenvolvimento |

## 1. Visão do produto

G3X Fresh Air é um processador musical de presença e ar com duas regiões
complementares. `Presence` recupera definição e proximidade nos médios-altos;
`Air` adiciona abertura e detalhe no extremo superior. O objetivo é chegar
rapidamente a um resultado claro, preservando controle de nível e evitando que
o tratamento se torne áspero.

O conceito de interação é informado pelo Slate Digital Fresh Air. A
documentação oficial da referência descreve dois controles principais, um
processamento dinâmico de altas frequências, vínculo entre controles, trim,
bypass e medição de saída. Curvas, detectores, constantes, algoritmo e
implementação G3X serão projetados e validados de forma independente.

## 2. Objetivos

- Dar presença e inteligibilidade a vocais, instrumentos e locução.
- Adicionar abertura a fontes fechadas sem depender apenas de ganho estático.
- Oferecer duas decisões musicais claras em vez de parâmetros técnicos.
- Preservar transientes, corpo e imagem estéreo.
- Permitir comparação honesta por meio de trim e bypass.
- Evitar sibilância excessiva, aspereza, pumping e clipping.
- Preservar automações e sessões por meio de IDs estáveis.

## 3. Usuários e casos de uso

- Presença e inteligibilidade em vocais, diálogos e locução.
- Ataque e definição em caixa, percussão, violão e guitarra.
- Abertura em overheads, pratos, pads, samples e sintetizadores.
- Recuperação de mixes percebidas como opacas ou abafadas.
- Realce sutil no mix bus com comparação de nível.

O plugin não é um de-esser, ferramenta de restauração, equalizador cirúrgico
nem uma reprodução do produto da Slate Digital.

## 4. Controles públicos

### 4.1 Presence (`presenceAmount`)

- Faixa exibida: 0.0 a 100.0%.
- Faixa normalizada interna: 0.0 a 1.0.
- Padrão: 0.0%, estado neutro.
- Atua prioritariamente na percepção de definição dos médios-altos.
- Automação suavizada, sem zipper noise.

### 4.2 Air (`airAmount`)

- Faixa exibida: 0.0 a 100.0%.
- Faixa normalizada interna: 0.0 a 1.0.
- Padrão: 0.0%, estado neutro.
- Atua prioritariamente na abertura e no detalhe do extremo superior.
- Automação suavizada, sem zipper noise.

### 4.3 Link (`linkBands`)

- Padrão: desligado.
- Quando ativado, os dois detectores aplicam a maior redução dinâmica às duas
  regiões, preservando a relação estática entre `Presence` e `Air`.
- Não movimenta silenciosamente os valores dos macrocontroles.

### 4.4 Output (`outputTrimDb`)

- Faixa proposta: -12.0 a +3.0 dB.
- Padrão: 0.0 dB.
- Aplicado após o processamento e antes da medição final.

### 4.5 Bypass (`bypass`)

- Padrão: desligado.
- Transição curta e sem clique entre processado e não processado.
- Estado automatizável e serializado.

Todos os controles aceitam edição por teclado, ajuste fino, reset por duplo
clique e nomes acessíveis.

## 5. Processamento proposto

```text
Input
  -> segurança contra denormals/NaN/Inf
  -> divisão em caminhos Presence e Air
  -> detectores independentes e suavizados
  -> realce espectral dinâmico próprio em cada caminho
  -> recombinação paralela com controle de energia
  -> output trim
  -> medição Peak/RMS e proteção numérica
  -> Output
```

### 5.1 Caminho Presence

- Região inicial de pesquisa: aproximadamente 2 a 8 kHz.
- Combinar curva larga com modulação dinâmica lenta e limitada.
- Priorizar inteligibilidade sem enfatizar nasalidade ou sibilância.
- Manter ganho e Q limitados em todos os sample rates.

### 5.2 Caminho Air

- Região inicial de pesquisa: aproximadamente 8 a 20 kHz, adaptada ao sample
  rate e à frequência de Nyquist.
- Usar shelf amplo e, se aprovado em protótipo, geração harmônica muito suave.
- Evitar energia instável próxima a Nyquist e aliasing audível.
- Reduzir automaticamente a contribuição quando o detector indicar excesso de
  energia aguda, sem comportamento de de-esser evidente.

Essas faixas são hipóteses próprias para prototipagem. Não representam dados
medidos, extraídos ou alegados sobre o produto de referência.

### 5.3 Dinâmica e ganho

- O primeiro protótipo comparará uma versão linear com uma versão dinâmica.
- Detectores não podem produzir pumping em material sustentado ou percussivo.
- O estado 0/0 deve produzir identidade numérica dentro da tolerância.
- A soma paralela deve manter headroom interno em ponto flutuante.
- `Output` é explícito; não haverá normalização escondida no protótipo.

### 5.4 Estéreo

- Suporte a mono e estéreo na primeira versão.
- Detectores vinculados entre canais por padrão para preservar a imagem.
- Mesma latência e topologia nos dois canais.
- Sem processamento mid/side inicialmente.

## 6. Interface proposta

- Dois macrocontroles de igual importância: `Presence` e `Air`.
- Controle de vínculo central e inequívoco, com estado visível.
- Output trim menor, separado da área criativa.
- Medidor horizontal ou vertical de saída com Peak, RMS e retenção de clip.
- Bypass acessível e visualmente discreto.
- Valores numéricos sempre legíveis e editáveis.
- Janela compacta, redimensionável e compatível com HiDPI.
- Identidade G3X original, sem copiar céu, nuvens, logotipo, paleta, molduras,
  proporções, tipografia ou ornamentação da referência.
- Navegação por teclado, foco visível e nomes acessíveis para leitor de tela.

## 7. Medição e segurança

- Peak e RMS na saída, em dBFS.
- Indicador de clipping com retenção e reset pelo usuário.
- Medição desacoplada da thread de áudio por estrutura lock-free.
- Saída finita para silêncio, impulsos, DC, ruído e níveis extremos.
- Sem limitação brickwall silenciosa; clipping potencial deve ser indicado.

## 8. Requisitos de tempo real

- Nenhuma alocação, mutex, I/O, logging ou chamada de UI em `processBlock`.
- Funcionamento de 44.1 a 192 kHz e buffers de 16 a 2048 samples.
- Latência zero no protótipo; qualquer lookahead futuro exige decisão de
  produto, compensação correta e atualização deste PRD.
- Estado serializado, versionado e compatível entre versões.
- Mudanças rápidas e automação sem cliques, divergência ou instabilidade.

## 9. Presets iniciais

- Neutral
- Vocal Presence
- Vocal Air
- Drum Detail
- Acoustic Clarity
- Mix Open

Os presets serão desenvolvidos do zero e usarão somente parâmetros G3X.

## 10. Estratégia de validação

### 10.1 Testes automatizados

- `Presence = 0` e `Air = 0`: identidade dentro da tolerância.
- Respostas por impulso em 0, 25, 50, 75 e 100% de cada controle.
- Continuidade do vínculo em valores centrais e limites.
- Estabilidade de filtros e detectores em todos os sample rates.
- Ganho máximo limitado e ausência de NaN/Inf.
- Paridade entre canais para entradas estéreo idênticas.
- Recuperação exata de estado e IDs de automação.
- Medição correta para sinais senoidais e silêncio conhecidos.

### 10.2 Testes auditivos

- Vocais masculinos e femininos, locução, bateria, violão e mix completa.
- Comparação com loudness aproximado usando o output trim.
- Avaliação de sibilância, fadiga, pumping, perda de corpo e alteração estéreo.
- Automação lenta e rápida em 44.1, 48, 96 e 192 kHz.

### 10.3 Referência externa

O Slate Digital Fresh Air poderá ser usado em testes perceptivos somente como
referência de categoria e fluxo. O objetivo não é correspondência
sample-a-sample, engenharia reversa nem clonagem de curvas ou comportamento.

## 11. Critérios de aceitação do protótipo

- Build Debug e Release em Linux; Release VST3 em Windows CI com MSVC.
- Resposta neutra comprovada em 0/0 e zero latência reportada.
- Curvas contínuas e estáveis em toda a faixa de parâmetros.
- Vínculo previsível sem saltos ao ativar, ajustar ou atingir limites.
- Sem clicks, NaN/Inf, pumping evidente ou divergência entre canais.
- Estado recuperado após salvar e reabrir o host.
- Plugin reconhecido como efeito e aprovado no pluginval/VST3 Validator.
- Validação manual no FL Studio em Windows.

## 12. Fora do escopo inicial

- Reprodução exata ou engenharia reversa do Slate Digital Fresh Air.
- Uso de marca, código, assets, presets ou trade dress da Slate Digital.
- De-esser, EQ manual, analisador espectral, mid/side e sidechain externo.
- AAX, formatos nativos de DAWs, iOS e versão final para macOS.

## 13. Marcos propostos

1. **M0 — Fundação (concluído):** PRD, naming e arquitetura.
2. **M1 — Curvas (concluído):** caminhos Presence/Air lineares e testes de resposta.
3. **M2 — Dinâmica (concluído):** detectores, smoothing, vínculo de redução,
   output trim, bypass por crossfade e segurança.
4. **M3 — Interface (concluído):** controles, medição Peak/RMS, presets,
   retenção de clip, redimensionamento e acessibilidade.
5. **M4 — Windows Alpha:** CI MSVC, artefato VST3 e FL Studio.
6. **M5 — Beta:** validadores, regressão, documentação e empacotamento.

## 14. Decisões que precisam de confirmação

- Confirmar `G3X Fresh Air` como nome público ou manter apenas como codinome,
  considerando a proximidade com a marca do produto de referência.
- Escolher DSP totalmente linear ou realce dinâmico como comportamento padrão.
- Definir se o link preserva diferença absoluta, razão ou movimento relativo.
- Manter output trim manual ou avaliar compensação assistida em versão futura.
- Modelo de licença e compatibilidade com a licença do JUCE.

## 15. Fontes de pesquisa

- [Guia oficial do Slate Digital Fresh Air](https://docs.slatedigital.com/FreshAir/Fresh%20Air.html)
- [Página oficial do produto](https://slatedigital.com/fresh-air-2/)
- [Imagem oficial da interface](https://docs.slatedigital.com/FreshAir/gui.png)
- [Catálogo oficial de plugins gratuitos](https://slatedigital.com/free-plugins/)

Consulta realizada em 4 de setembro de 2026. As fontes são documentação de
referência; não constituem especificação de clonagem.
