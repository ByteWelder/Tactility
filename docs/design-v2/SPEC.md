# tactility-mobile · redesign v2 — especificação visual

Status: **proposta para aprovação** (só mockups; nenhum código de firmware).
Alvo: ES3C28P (ESP32-S3), TFT 2.8" 240×320 retrato, ~143 ppi, touch capacitivo, LVGL.
Design novo, feito do zero; não deriva de `docs/design/`.

Arquivos:

```
docs/design-v2/
├── SPEC.md                    # este documento
├── render.mjs                 # gera PNGs + checks.json (Playwright + Chrome local)
├── src/index.html, styles.css # fonte dos mockups (9 telas, 240×320 cada)
├── src/fonts/                 # Inter 400/500 woff2 (só para o mockup)
└── png/1x/*.png  png/2x/*.png  contact-sheet.png  contact-sheet@2x.png  checks.json
```

Reproduzir: `PW=<node_modules com playwright> CHROME="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" node render.mjs`

## 1. Ideia

- Escuro, quieto, um único azul. O azul só aparece em **estado** (ativo, ligado, rodando, foco) e na **ação primária**, nunca como decoração.
- Elemento-assinatura: a **barra de gesto 36×4**. Aparece só onde existe um gesto de borda (lock: subir; painel rápido: fechar). Ensina o gesto sem precisar de texto.
- Na home, o relógio de 48 px fica à esquerda, com a data empilhada à direita na mesma linha de base. Isso ocupa uma linha só e sobra altura para a grade e para o espaço até a dock.
- Leveza como regra de design: **1 tamanho de ícone (24)** + 4 ícones de 16 px; nenhum ícone de 32 px foi necessário. Nenhuma imagem, sombra ou blur. A única transparência é a camada de escurecimento.

## 2. Paleta

| Token | Hex | Uso |
|---|---|---|
| `bg` | `#0E1116` | fundo de tela |
| `surface` | `#1A1F27` | cartões, tiles do grid, dock, painel, diálogo, teclas modificadoras |
| `surface-2` | `#242A33` | tiles do painel, teclas, botão secundário, trilho do slider |
| `line` / `line-soft` | `#242A33` / `#1F2530` | divisórias dentro de cartão / em listas sem cartão |
| `text` | `#E8ECF1` | texto e ícones principais |
| `text-2` | `#9AA4B2` | texto secundário, ícones de linha, legendas |
| `text-3` | `#8A94A3` | valores, chevrons, cadeado (mínimo absoluto) |
| `accent` | `#4C8DFF` | ligado/ativo, botão primário, preenchimento do slider, ponto de app aberto |
| `accent-text` | `#9CC0FF` | texto/ícone de destaque sobre `accent-soft` |
| `accent-soft` | `#1A2B45` | rede conectada; fim do gradiente do lock |
| `accent-line` | `#2B4C7E` | borda da rede conectada |
| `on-accent` | `#0B1118` | texto/ícone sobre `accent` |
| escurecimento | `#000000` a 60% | atrás do painel rápido e do diálogo |

Contraste medido (WCAG; todo texto ≥ 4,5:1):

| Par | Razão |
|---|---|
| `text` sobre `bg` / `surface` / `surface-2` / `accent-soft` | 15.94 / 13.95 / 12.17 / 12.00 |
| `text-2` sobre `bg` / `surface` / `surface-2` / fim do gradiente | 7.50 / 6.56 / 5.73 / 5.64 |
| `text-3` sobre `bg` / `surface` / `surface-2` | 6.16 / 5.39 / **4.71** (pior caso) |
| `accent-text` sobre `accent-soft` | 7.72 |
| `on-accent` sobre `accent` | 5.92 |

O texto do app escurecido atrás do painel e do diálogo fica abaixo de 4,5:1 de propósito: ele está inerte.

## 3. Tipografia

Família única: **Inter**, pesos 400 e 500. Todo texto é Medium, exceto as legendas de 12 px (Regular).

| Papel | Tamanho / altura de linha / peso | Onde |
|---|---|---|
| Relógio | 48 / 48 / 500, só `0123456789:`, tracking −1 | home, lock |
| Título | 22 / 26 / 500 | barras de ferramentas, Recents, título do diálogo, hora no painel |
| Corpo | 15 / 20 / 500 | linhas de lista, botões, corpo do diálogo, dia da semana na home |
| Controle denso | 14 / 16 / 500 | teclas, rótulos dos tiles do painel |
| Secundário | 13 / 16 / 500 | hora da barra de status, datas, valores, caminho, percentuais |
| Legenda | 12 / 14 / 400 | rótulos do grid, dicas, rodapé, "Available networks" |

- A escala pedida é 13/15/22/48. O 14 entra só nos controles densos (teclas e tiles); o 12 só nas legendas.
- **Rótulos do grid são legenda (12 Regular), largura máxima de 52 px, 1 linha, com reticências.**
  - Medido: o mais largo é "Grok Bot", com 49,4 px. Nenhum rótulo é truncado.
  - O menor vão entre vizinhos é 12,2 px (Settings↔Grok Bot).
  - Em 13 Medium, "Grok Bot" e "Terminal" teriam 54 px e seriam truncados, por isso caiu para 12.
- O texto interativo nunca fica abaixo de 14. Onde há um rótulo de 12 ou 13, quem recebe o toque é o tile ou a linha inteira, não o texto.

## 4. Grade, raios, alvos de toque

- **Raios:** 12 px em cartões (tiles do grid, dock, cartões de lista, cartões de Recents, painel, diálogo, rede conectada). 8 px em controles (botões, campo, teclas, tiles do painel). Exceções circulares: interruptor 10, polegar 10, barra de gesto 2.
- **Faixas fixas:** barra de status 0–24, barra de ferramentas 24–68, conteúdo a partir de y=76. Margem lateral de 12 (cartões) ou 16 (texto solto).
- **Alvos de toque:**

| Alvo | Tamanho |
|---|---|
| Voltar, menu, interruptor | 44×44 |
| Linhas de lista e cartões de Recents | 44 de altura |
| Itens do grid e da dock | 60×64 e 60×56 |
| X das Recents | 40×44, ícone de 16 |
| Olho do campo de senha | 40×40 |
| Botões do diálogo e Join | 40 de altura |
| Linha do slider | 40 de altura |
| Tiles do painel | 68×52 + rótulo |
| Teclas | 23×40 |

  - As teclas têm 23 px de largura: é o único alvo abaixo de 32 e é inevitável com 10 teclas em 240 px. A altura de 40 px compensa.
- **Gestos só de borda:** faixa de 16 px em cada borda.

| Gesto | Ação |
|---|---|
| Topo, arrastar para baixo | Painel rápido |
| Base, arrastar para cima | Home |
| Base, arrastar para cima e segurar | Recents |
| Esquerda, arrastar para a direita | Voltar |

  - Nenhum controle fica nos 20 px de baixo da home (a dock termina em y=300).
  - Exceção conhecida: a última fileira do teclado encosta na borda de baixo. Toque funciona; só um arrasto maior que o limiar vira gesto.

## 5. Ícones

- Grade de 24, coordenadas inteiras, 1 cor (recolorida em tempo de desenho), traço de 2 px, pontas e junções arredondadas.
- Os 4 ícones de 16 px (Wi-Fi da barra de status, chevron, X, cadeado) também têm traço real de 2 px.
- A regra de 2,25 px em 32 continua valendo se um ícone de 32 for adicionado depois. Hoje nenhuma tela precisa de um.
- Como o traço de 2 px é centrado numa coordenada inteira, cada linha reta cobre exatamente 2 pixels inteiros e fica nítida em bitmap.
- O ícone `wifi` (arcos) significa "Wi-Fi". A força do sinal tem ícone próprio: 3 barras (`sig3`, `sig2`, `sig1`).
  - Barra preenchida = sinal; barra só contornada = sinal que falta.
  - Continua 1 cor, e as três variantes ficam alinhadas na mesma base.

Inventário: 23 ícones de 24 px (settings, bot, wifi, sig×3, folder, hub, notes, terminal, camera, back, more, sun, vol, bt, recents, eye, shift, del, display, power, file) + 4 de 16 px (wifis, chev, close, lock).

## 6. Telas — layout e restrições LVGL

Coordenadas em px, na tela de 240×320.

**01 Home**
- Barra de status: hora 13, Wi-Fi 16.
- Relógio 48 em (12, 28). À direita: "Friday" 15 e "24 October" 13 `text-2`, alinhados à direita em x=228.
- Grade em y=86: 4 colunas de 60 px, tile de 44 (12 de raio, `surface`), ícone de 24, rótulo 12 com 4 px de margem. Linhas de 64 + 8 de vão: 86–222.
- Dock em y=244–300 (x 8–232, `surface`, 12 de raio): 4 ícones de 24 nas **mesmas colunas da grade**. App aberto = ponto de 4 px `accent` a 6 px da base da dock.
- Folga entre a grade e a dock: **22 px**.
- LVGL:
  - Grid em flex/grid com tamanhos fixos.
  - A dock é um `lv_obj` filho da tela da home, não é um app.
  - Os pontos leem a pilha de janelas.
  - Sem scroll.

**02 Lock**
- Gradiente vertical de 2 paradas: `#0E1116` até 45%, depois até `#1A2B45`.
- Wi-Fi de 16 em `text-2` no canto superior direito.
- Relógio 48 centralizado em y=100. Data 15 `text-2` em y=154.
- Legenda "Swipe up to unlock" 12 em y=276. Barra de gesto 36×4 em y=302.
- LVGL:
  - Gradiente via `bg_grad`, sem imagem.
  - Sem barra de status (a hora já é o relógio).
  - Destrava com arrasto para cima a partir da borda de baixo.

**03 Painel rápido**
- Abre por cima do app atual (no mockup, Files) com escurecimento de 60%.
- Painel com 240×230, `surface`, raio 12 só nos cantos de baixo, linha de 1 px na base.
- Hora 22 e data 13 em y=12.
- Sliders em y=48 e y=88: ícone 24, trilho de 6 px (`surface-2`, preenchimento `accent`), polegar de 20 px `text`, valor 13 `text-2`, linha tocável de 40.
- Tiles em y=136: 3×68 com vão de 6 (x = 12, 86, 160). Corpo de 68×52 com raio 8, ícone de 24 central. Rótulo 14 abaixo do tile, 6 px de margem.
- Estados dos tiles:
  - Ligado: `accent` com ícone `on-accent`.
  - Desligado: `surface-2` com ícone `text-2`.
- Barra de gesto em y=218: arrastar para cima fecha.
- Sem notificações.
- LVGL:
  - O escurecimento é **um único** `lv_obj` de tela cheia, `bg_color #000`, `bg_opa 153`. Não há outra transparência.
  - Os sliders são `lv_slider` com `ext_click_area` até 40 px.
  - O app de trás não é redesenhado durante a animação.

**04 Recents**
- **Fundo opaco `bg`, cobrindo também a barra de status.** Nada do app de trás aparece.
- Cabeçalho próprio de 40 px (y=28–68): "Recents" 22 em x=16. "Swipe up to close" 12 `text-2` alinhado à direita, na mesma linha de base.
- Lista começa em y=76 (8 px de folga): até 5 cartões de 208×44, vão de 4, terminando em y=312.
- Cartão: ícone 24, nome 15, X de 16 dentro de um alvo de 40×44.
- App ativo: barrinha `accent` de 3×20, raio 2, recuada 4 px da borda esquerda do cartão. Recuada, ela lê como indicador; colada na borda, leria como borda colorida.
- O cartão inteiro abre o app; só o X fecha.
- LVGL:
  - É uma tela, não uma sobreposição com transparência.
  - Os cartões são `lv_obj` clicáveis; o X é um filho com `LV_OBJ_FLAG_EVENT_BUBBLE` desligado.
  - Sem miniaturas.

**05 Settings**
- Barra de status + barra de ferramentas (voltar 44, título 22).
- Cartão em y=76–296 com 5 linhas de 44: Display 70%, Sound 40%, Wi-Fi HomeNet, Bluetooth Off, Power 5 min.
- Cada linha: ícone 24 `text-2`, rótulo 15, valor 13 `text-3`, chevron 16 `text-3`.
- Divisória `line` de 1 px recuada até x=48.
- LVGL:
  - `lv_list` não é necessário: cartão com 5 filhos em flex de coluna.
  - Valores atualizados por evento, sem polling na UI.

**06 Wi-Fi**
- Barra de ferramentas: voltar, título, interruptor 36×20 (ligado = trilho `accent` com botão `text` à direita; desligado = trilho `surface-2` com botão `text-2` à esquerda) dentro de um alvo de 44×44.
- Rede atual em y=76: cartão de 216×56 `accent-soft` com borda `accent-line`. Ícone e "Connected" em `accent-text`; nome 15.
- "Available networks" 12 em y=144.
- Cartão de redes em y=162: 3 linhas de 44 com ícone de força, nome e cadeado de 16 `text-3` quando a rede é protegida.
- LVGL:
  - A lista rola se houver mais de 3 redes. A rede atual fica fixa.
  - Scan sem spinner animado: o estado vira texto ("Scanning…" 13 no lugar da legenda).

**07 App genérico (Files)**
- Barra de ferramentas: voltar, título, menu ⋮ (44×44).
- Caminho "/sdcard/projects" 13 `text-2` em y=76.
- Lista **sem cartão**, ocupando a largura toda, a partir de y=96: linhas de 44 com ícone (pasta/arquivo), nome 15 e meta 13 `text-3` à direita; divisória `line-soft` a partir de x=52.
- Rodapé fixo em y=288–320 com linha no topo: "6 items · 2 folders" 12 centralizado.
- LVGL: só a lista rola; a barra de ferramentas e o rodapé ficam fixos.
- Este é o padrão de chrome para qualquer app.

**08 Diálogo**
- Sobre a tela de Wi-Fi escurecida a 60%.
- Cartão de 216×164 **ancorado embaixo** em (12, 144), com 12 px de margem igual às laterais. Padding 16, raio 12, borda `line`.
  - Os botões ficam ao alcance do polegar.
  - O cartão da rede que vai ser esquecida continua visível acima, escurecido. Isso dá contexto à pergunta.
  - O diálogo nunca corta texto do fundo pela metade.
- Título "Forget network?" 22 (cabe em 1 linha: 26 px medidos).
- Corpo 15 `text-2`, 2 linhas.
- Botões de 40 com vão de 8: Cancel (`surface-2`) e Forget (`accent`/`on-accent`).
- LVGL:
  - `lv_msgbox` reestilizado, ou um `lv_obj` simples.
  - O mesmo objeto de escurecimento do painel rápido.

**09 Teclado (senha do Wi-Fi)**
- Barra de ferramentas: voltar + nome da rede 22.
- Campo de 160×40 (borda `accent` = foco) com pontos 15, cursor `accent` 2×18 estático e olho de 24 num alvo de 40×40.
- Botão "Join" 48×40 `accent` à direita do campo.
- Legenda "WPA2 · case-sensitive" 12 em y=124.
- Teclado em y=152–320: linha `line-soft` no topo + 4 px de respiro, teclas de y=157 a 320. São 4 fileiras de **40 px**, vão de **1 px**, letras **14**, raio 8.
- Fileiras:
  - QWERTYUIOP: 10 teclas de 23 px.
  - ASDFGHJKL: 9 teclas, recuo de meia tecla.
  - Shift (1,5) + ZXCVBNM + apagar (1,5).
  - 123 (2) + space (5,5) + Go (2,5).
- Cores das teclas: letras `surface-2`; modificadoras `surface` com `text-2`; Go `accent`.
- LVGL:
  - `lv_keyboard` (buttonmatrix: **1 objeto** para todas as teclas) com mapa e `ctrl_map` próprios para larguras relativas.
  - `pad_gap 1`.
  - Sem balão de tecla pressionada (economiza redesenho). O estado pressionado segue a regra da seção 6.1.

### 6.1 Estados (para todas as telas)

Cada estado é só uma troca de cor de fundo, sem nova imagem nem transparência:

| Estado | Regra |
|---|---|
| Pressionado (neutro) | o fundo sobe um degrau: `bg` → `surface` → `surface-2` → `accent-soft` |
| Pressionado (primário) | `accent` → `accent-text` (`#9CC0FF`); o texto continua `on-accent` |
| Desligado / indisponível | texto e ícone `text-3`, sem destaque; interruptor com trilho `surface-2` |
| Foco (só para encoder/teclado físico) | contorno de 2 px `accent`, afastado 2 px |
| Vazio | 1 linha 15 `text-2` centrada: Recents "No open apps", Files "This folder is empty", Wi-Fi "No networks found" |
| Carregando | texto estático 13 `text-2` ("Scanning…", "Joining…"), nunca spinner |
| Erro | linha 13 sob o campo, em `accent-text`, dizendo o que fazer: "Wrong password. Check it and try again." Sem vermelho: a paleta não tem cor de erro |

Contraste extra: `on-accent` sobre `accent-text` = 10,3:1.

## 7. Transições (só entre estados, nada em loop)

| Transição | Duração | Tipo |
|---|---|---|
| Lock → app/home | 180 ms | lock desliza para cima (translate Y), `ease_out` |
| Abrir painel rápido | 160 ms | painel desliza do topo (translate Y −230 → 0) e o escurecimento aparece no mesmo tempo (opa 0 → 153) |
| Fechar painel rápido | 120 ms | reverso, `ease_in` |
| Abrir Recents | 150 ms | tela entra de baixo (translate Y +24 → 0), sem fade |
| Fechar app em Recents (X ou arrasto) | 120 ms | cartão sai para cima e os de baixo sobem 48 px |
| Abrir app | 140 ms | app entra deslizando 24 px + troca de tela (`lv_screen_load_anim` MOVE_LEFT curto) |
| Voltar | 140 ms | reverso de abrir app |
| Diálogo | 120 ms | escurecimento opa 0 → 153 e cartão translate Y +8 → 0 |
| Teclado | 140 ms | sobe de y=320 até y=156 |
| Interruptor / tile ligado | 100 ms | troca de cor + deslocamento do botão |
| Toque pressionado | 0 ms | troca de cor instantânea |

- Nenhum spinner, pulso ou cursor piscando.
- Se as animações estiverem desligadas no tema, tudo vira instantâneo.
- Animar só translate e opacidade da camada de escurecimento. Nunca escala, blur ou opacidade de árvore inteira.

## 8. Orçamento de flash e RAM

### 8.1 Fontes (bitmap LVGL 4 bpp, sem kerning, sem compressão)

Como foi estimado:
- Cada glifo foi medido no Chrome com Inter real: caixa de tinta → `ceil(w·h/2)` bytes de bitmap + 8 B de descritor (`lv_font_fmt_txt_glyph_dsc_t`), + ~1 KB de cmap/cabeçalho por fonte.
- Valores em `png/checks.json`.

| Fonte | Subconjunto | Glifos | Flash |
|---|---|---|---|
| Inter Regular 12 | ASCII 0x20–0x7E | 95 | 4,0 KB |
| Inter Medium 13 | ASCII | 95 | 4,5 KB |
| Inter Medium 14 | ASCII | 95 | 5,0 KB |
| Inter Medium 15 | ASCII | 95 | 5,3 KB |
| Inter Medium 22 | ASCII | 95 | 9,3 KB |
| Inter Medium 48 | `0123456789:` | 11 | 5,3 KB |
| **Total fontes** | | 486 | **≈ 33,4 KB** |

- Acentos do português (Latin-1, +96 glifos) só valem para 13 e 15 (texto de usuário): ≈ +9 KB.
- Kerning ligado: ≈ +1–2 KB por fonte.
- A compressão do `lv_font_conv` reduz ~20–30% e custa CPU por glifo. Não recomendo neste hardware.

### 8.2 Ícones (imagens de alfa A4, 1 cor, recoloridas)

| Tamanho | Qtde | Bytes cada (bitmap + `lv_image_dsc_t`) | Total |
|---|---|---|---|
| 24×24 A4 | 23 | 288 + 28 | 7,1 KB |
| 16×16 A4 | 4 | 128 + 28 | 0,6 KB |
| **Total ícones** | 27 | | **≈ 7,7 KB** |

Em A2, o bitmap cai pela metade (≈ 3,9 KB no total), mas o traço diagonal de 2 px fica serrilhado. Recomendo A4.

### 8.3 Demais recursos e RAM

| Recurso | Flash | RAM |
|---|---|---|
| Fundos (sólidos + 1 gradiente) | 0 | gradiente: cache de ~0,6 KB (320 px × RGB565) só no lock |
| Escurecimento 60% | 0 | 0 (é um retângulo; custo é de CPU no blend, não de RAM) |
| Buffers de desenho LVGL | — | 2 × 240×32×2 B = **30 KB** (parciais, DMA, RAM interna) |
| Estilos compartilhados (~20 `lv_style_t`) | — | ≈ 1,5 KB |
| Objetos da tela mais pesada (Home ≈ 40 objs) | — | ≈ 5 KB |
| Pior caso app + sobreposição (Files + painel ≈ 60 objs) | — | ≈ 8 KB |
| Teclado (`lv_keyboard` = 1 buttonmatrix) | — | ≈ 1,5 KB |
| Fontes e ícones | const, em flash | **0** (lidos direto da flash) |

**Gradiente do lock em RGB565:** o azul vai de 2 a 8 em 5 bits. São só ~6 degraus ao longo de ~176 px, e as faixas podem aparecer. Duas saídas:
- ativar o dithering de gradiente do LVGL, se a versão em uso oferecer;
- ou encurtar o gradiente para o terço de baixo, onde os degraus ficam mais estreitos.

Validar na placa antes de aprovar o lock.

**Total novo em flash ≈ 33,4 + 7,7 = ~41 KB** (≈ 50 KB com acentos e kerning). **RAM de UI ≈ 40 KB no pico**, dominada pelos buffers de desenho. Nenhum recurso exige PSRAM.

## 9. Fora do escopo desta proposta

- Código de firmware, temas LVGL e conversão real das fontes/ícones.
- Os números de flash são estimativas medidas; os definitivos saem do `lv_font_conv`.
- Telas de Display, Sound, Bluetooth e Power (só a raiz de Settings foi desenhada).
- Estados de erro e vazio estão descritos aqui, mas não desenhados.
