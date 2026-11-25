# 🌵 Nordeste Jogos

Grupo: 
1.Kleber David da Silva filho
2.Pedro Emmanuell Neves dos Anjos
3.Luan Henrique de Melo Santos
4.Camila Araújo da Costa
5.Julio Cesar Brasileiro Santos

> Dois jogos em modo texto, escritos em **C**, para o terminal: um **quiz** sobre a cultura nordestina e uma **caça ao tesouro** pelo sertão, pela feira e pela Chapada Diamantina.

![Linguagem](https://img.shields.io/badge/linguagem-C11-blue)
![Plataforma](https://img.shields.io/badge/plataforma-terminal-lightgrey)

---

## 📖 Sobre o projeto

O **Nordeste Jogos** reúne dois jogos em um só programa, acessados por um menu principal. Os cenários, as perguntas e os personagens valorizam a cultura do Nordeste brasileiro: cordel, forró, casa de farinha, caatinga, Feira de Caruaru, Chapada Diamantina e muito mais.

É um projeto de estudo em C, feito em equipe, que pratica structs, enums, vetores, arquivos binários, ponteiros e organização de funções.

### Os jogos

| Jogo | Resumo |
|---|---|
| 🎯 **Quiz do Nordeste** | 5 etapas (Feira de Caruaru, Casa de Farinha, Sertão, Chapada Diamantina e Gruta do Tesouro), com 3 perguntas cada. Quanto mais rápido e certeiro, mais pontos. |
| 🗺️ **Tesouro do Nordeste** | Aventura em texto: explore o mapa, junte itens, administre a água, enfrente o ciclo de manhã, tarde e noite e abra o baú da gruta. |

Os dois jogos compartilham um **placar (scoreboard)** salvo em arquivo, com os 20 melhores resultados.

---

## ✨ Funcionalidades

- Menu principal com os dois jogos e o placar
- Quiz com **cronômetro**, **alternativas embaralhadas** a cada pergunta, curiosidade depois de cada resposta e títulos finais
- Aventura com **mapa**, **inventário**, **fases do dia**, **evento aleatório** e **tutorial guiado**
- **Arte ASCII colorida** para cada cenário
- **Placar persistente**, ordenado por pontos (e por tempo, em caso de empate)
- Cores ANSI que podem ser desligadas com o comando `cores`

---

## 🖼️ Prévia

**Menu principal**

```
========================================
      CENTRAL DE JOGOS - NORDESTE
========================================
[1] Quiz do Nordeste
[2] Tesouro do Nordeste
[3] Ver Scoreboard (0)
[4] Limpar Scoreboard
[0] Sair
Escolha:
```

**Uma cena do Tesouro do Nordeste**

```
+--- Casa de Farinha | Manha ---+
|           ) (                          |
|       ___(___)___                      |
|      /____________\       \|/   \|/    |
|      | [] |##| [] |        |     |     |
|========================================|
+------------------------------------------+
Agua: 8/8 | Fase: Manha | Casa antiga com cheiro de mandioca.
Item no chao: mapa
Saidas: norte leste
```

**Scoreboard**

```
=== SCOREBOARD (RAPIDEZ NO NORDESTE) ===
POS  JOGADOR            JOGO                   TEMPO  PONTOS
----------------------------------------------------------------
#1   Julio              Tesouro do Nordeste     0.0s    3200
```

---

## ⚙️ Como compilar e executar

### Requisitos

- Compilador **GCC** (ou Clang) com suporte ao padrão C11
- Um terminal com suporte a cores ANSI (a maioria dos terminais atuais)

### Compilando

```bash
gcc -Wall -Wextra -std=c11 -o nordeste_jogos nordeste_jogos.c
```

### Executando

```bash
./nordeste_jogos          # Linux e macOS
nordeste_jogos.exe        # Windows
```

> 💡 **Windows:** use o MinGW-w64, o WSL ou o Windows Terminal. Em terminais antigos, as cores podem aparecer como símbolos estranhos (`[93m`). Dentro do Tesouro, digite `cores` para desligá-las.

---

## 🎮 Como jogar

### 🎯 Quiz do Nordeste

1. Escolha `1` no menu e aperte **ENTER** para iniciar o cronômetro.
2. Responda cada pergunta digitando **1, 2, 3 ou 4**. A ordem das alternativas muda a cada pergunta, então não adianta decorar a posição da resposta certa.
3. Depois de cada resposta, aparece uma curiosidade sobre o tema.
4. No fim, você recebe um título e sua pontuação entra no placar.

| Acertos | Título |
|---|---|
| 13 a 15 | Mestre do Nordeste |
| 9 a 12 | Grande conhecedor do Nordeste |
| 0 a 8 | Bom viajante |

### 🗺️ Tesouro do Nordeste

**Objetivo:** chegar à **Gruta do Tesouro** e abrir o baú, sem deixar a água acabar.

#### Mapa

```
               [Gruta ]
                   |
  [Sertao ] -- [Chapada ]
      |
 [Casa ] ---- [Feira ]
```

#### Comandos

| Comando | O que faz |
|---|---|
| `ir <direção>` | Move para `norte`, `sul`, `leste` ou `oeste` |
| `olhar` | Mostra a cena e o status do local |
| `explorar` | Procura algo escondido no local (vale uma vez por local) |
| `pegar <item>` | Guarda um item do chão na mochila |
| `usar <item>` | Usa um item (o efeito depende do lugar) |
| `descansar` | Passa o tempo até a próxima fase do dia |
| `dica` | Mostra uma dica para a situação atual |
| `inventario` | Lista a água, a fase do dia e os itens |
| `cores` | Liga ou desliga as cores |
| `sair` | Volta ao menu |

#### Regras

- 💧 **Água:** você começa com 8. Cada passo gasta 1, exceto à **tarde**, quando gasta **2**. Se chegar a 0, você perde.
- 🚰 **Fonte:** na Feira de Caruaru, o cantil é reabastecido ao chegar.
- 🌗 **Fases do dia:** a cada 3 passos o dia muda (manhã → tarde → noite → manhã...). O jogo avisa a cada mudança.
- 🔦 **Noite:** sem o **lampião**, não é possível explorar no escuro (exceto na gruta).
- 🍈 **Evento aleatório:** a cada passo, 1 chance em 5 de achar um **umbu** (+2 de água).
- 🎒 **Mochila:** guarda até 6 itens.
- 🔒 **Locais trancados:** a Chapada exige a **corda** para entrar, e a Gruta exige a **chave**.

#### Itens

| Item | Onde está | Para que serve |
|---|---|---|
| 🗺️ Mapa | Casa de Farinha | `usar mapa` mostra onde você está |
| 🔦 Lampião | Escondido na Casa de Farinha | Permite explorar à noite |
| 🧵 Corda | Feira de Caruaru | Necessária para entrar na Chapada e abrir um atalho nela |
| 🔑 Chave | Chapada Diamantina | Abre o baú do tesouro |
| 🥤 Cantil | Atalho da Chapada | Aumenta a água máxima |

#### Tutorial

Ao iniciar o Tesouro, responda `s` para um tutorial guiado de 4 passos. Digite `pular` quando quiser sair dele.

<details>
<summary>🔓 Ver um passo a passo para vencer (contém spoilers)</summary>

1. `pegar mapa` e `explorar` (você acha o lampião)
2. `ir leste` até a Feira de Caruaru e `pegar corda`
3. `ir oeste` de volta à Casa e `ir norte` até o Sertão
4. `ir leste` até a Chapada Diamantina e `pegar chave`
5. `usar corda` para abrir o atalho (acha um cantil e ganha mais água máxima)
6. `ir norte` até a Gruta do Tesouro
7. `usar chave` para abrir o baú. Você venceu! 🏆

</details>

---

## 🏆 Pontuação e placar

O placar fica no arquivo **`scoreboard_nordeste.dat`**, criado na pasta onde o jogo é executado. Ele guarda os **20 melhores** resultados, ordenados por pontos. Em caso de empate, vence quem fez em menos tempo.

| Jogo | Fórmula |
|---|---|
| Quiz | `200 × acertos` + até `800 × bônus de velocidade` (bônus máximo 2, para quem termina em até 30 s) |
| Tesouro | `1000` + `1000 × bônus de velocidade` (bônus máximo 2,2, para quem vence em até cerca de 34 s) |

Quanto menor o tempo, maior o bônus. O Tesouro só registra pontuação se o jogador **vencer**.

---

## 🧱 Estrutura do código

O projeto é um único arquivo, **`nordeste_jogos.c`**, organizado em seções (o código tem comentários explicando cada parte):

| Seção | O que contém |
|---|---|
| **Utilitários** | Cores ANSI (`cor`, `fim_cor`), leitura segura do teclado (`ler_linha`), `minusculas` |
| **Scoreboard** | `carregar_scoreboard`, `salvar_scoreboard`, `ordenar_scoreboard`, `calcular_pontos`, `salvar_resultado`, `mostrar_scoreboard` |
| **Quiz** | Tabela `ETAPAS` com as perguntas, `embaralhar_ordem`, `rodar_quiz` |
| **Tesouro** | Mapa e locais (`criar_locais`), fases do dia, comandos (`cmd_ir`, `cmd_pegar`, `cmd_explorar`, `cmd_usar`, `cmd_descansar`), tutorial, `rodar_tesouro` |
| **Menu** | `main`, que mostra o menu e chama os jogos |

### Principais estruturas de dados

| Struct | Função |
|---|---|
| `RegistroScore` e `Scoreboard` | Um resultado e o placar completo (gravados em arquivo binário) |
| `PerguntaQuiz` e `EtapaQuiz` | Uma pergunta e uma etapa do quiz (lugar, arte e 3 perguntas) |
| `LocalJogo` | Um local do mapa: saídas, itens, fonte e se foi explorado |
| `Jogador` | Estado da partida: local, água, passos, mochila e flags |

### Conceitos de C praticados

- `struct` e `enum` para modelar dados e nomear constantes
- Vetores de strings indexados por `enum`
- Leitura segura de texto com `fgets` e `sscanf`
- Arquivos binários com `fopen`, `fread` e `fwrite`
- Passagem de structs por ponteiro (com `const` quando é só leitura)
- Números aleatórios (`rand` e `srand`) e medição de tempo (`time` e `difftime`)
- Códigos de escape ANSI para cores no terminal

---

## 🗂️ Dicas para o repositório

Para o Git não versionar o executável nem o placar de quem testou, crie um arquivo `.gitignore` com:

```
nordeste_jogos
nordeste_jogos.exe
scoreboard_nordeste.dat
```

---

## 🚀 Ideias para o futuro

- [ ] Mais perguntas e mais etapas no quiz
- [ ] Mais locais no Tesouro (Pelourinho, Lençóis Maranhenses, Olinda, Jericoacoara)
- [ ] Salvar e carregar o progresso da aventura
- [ ] Alinhar a moldura do título com a largura da arte
- [ ] Dividir o código em módulos (`.h` e `.c`) e criar um `Makefile`
- [ ] Níveis de dificuldade

---

## 🤝 Como contribuir

1. Faça um *fork* ou crie uma branch: `git checkout -b feature/nome-da-melhoria`
2. Faça commits pequenos e claros (`feat:`, `fix:`, `docs:`)
3. Compile sem warnings: `gcc -Wall -Wextra -std=c11 ...`
4. Abra um *Pull Request* descrevendo o que mudou

---

## 👥 Equipe

| Papel | Nome |
|---|---|
| Product Owner | _[seu nome]_ |
| Desenvolvedores | _[nomes dos amigos]_ |

## 📜 Licença

_[Defina a licença, por exemplo MIT ou uso acadêmico]_

---

*Feito com ☀️, cuscuz e muito código. Bora achar esse tesouro!*
