# Chex Quest no Tab5 — avaliação de engine

2026-10-05. Alvo: ESP32-P4, 32 MiB PSRAM, 16 MiB flash, ESP-IDF 5.4.4.

Atualização: o [projeto ESP-IDF experimental](idf/README.md) compilou 268
unidades selecionadas e gerou um programa mínimo. Ainda falta o backend e
o link/inicialização do engine completo. A avaliação inicial abaixo registra
a etapa anterior e seus critérios; a viabilidade de gameplay continua aberta.

## Resultado

**ZDoom 2.8.1 é o candidato escolhido para o protótipo do Chex Quest 3
original, ainda sem viabilidade completa demonstrada no aparelho.**
Não foi gerado firmware jogável. A aplicação principal mantém a base Freedoom;
os testes experimentais estão isolados em `research/probe/`.

A escolha usa o renderer por software, a identificação nativa de Chex/Chex 3
e os interpretadores necessários. A versão fixa reduz mudanças durante o port;
isso não transforma um engine de desktop em um engine embarcado pronto.

## Arquivos originais examinados

Pacotes obtidos pelos links da [página oficial](https://www.chexquest3.com/downloads/):
`chex.zip` e `ChexQuest3_v1.4.zip`. A versão 1.4 é a edição original histórica,
não uma conversão vanilla nem a atualização v2. Os dados e readmes extraídos
estão em `research/assets/`, excluídos do controle de versão.

| Arquivo | Bytes | Cabeçalho | Conteúdo observado |
| --- | ---: | --- | --- |
| chex.wad | 12361532 | PWAD | Marcadores E1–E4; não confundir os mapas residuais com os cinco níveis jogáveis de Chex 1 |
| chex3.wad | 19191031 | PWAD | 15 marcadores de mapas; BEHAVIOR em todos, DECORATE, MAPINFO, SNDINFO e ANIMDEFS |

Os arquivos são estruturalmente válidos, apesar de usarem **PWAD**. O menu
herdado aceita apenas IWAD e precisa de validação própria para Chex.
O nome do arquivo e a assinatura não bastam para determinar compatibilidade.

O readme incluído em CQ3 confirma a presença de versões dos episódios de
Chex 1 e 2 no terceiro jogo. Isso não equivale a executar separadamente os
pacotes DOS originais. Não substituir silenciosamente os jogos solicitados.
A distribuição standalone de Chex 2 ainda precisa ser obtida e inspecionada.

Hashes calculados localmente, URLs e commit estão em `SOURCES.json`.
O site não forneceu checksum independente desses ZIPs; não confundir o hash
calculado com autenticação por checksum publicado. Estrutura dos WADs e
recursos encontrados estão em `WAD_AUDIT.json`.

## Comparação

| Caminho | Adequação | Decisão |
| --- | --- | --- |
| doomgeneric da base | Hardware pronto; identifica Chex 1, mas DEHACKED está desativado e falta sua implementação; não interpreta os recursos CQ3 observados | Não resolve a trilogia original apenas trocando WADs |
| ZDoom 2.8.1 | Identificação Chex/Chex 3, renderer software, DECORATE/ACS e formatos estendidos | Candidato do protótipo, com código clonado e compilação parcial RV32 verificada |
| Chex 3 GZD / GZDoom atual | Caminho oferecido pelo site para CQ3 moderno; outro conjunto de dependências e integração | Não investigado como backend neste primeiro protótipo; não presumir port direto |
| CQ3 para Odamex/ZDaemon | Site fornece outro WAD próprio, `chex3odx.wad`, com diferenças em relação à edição original | Não usar como substituição à escolha do usuário |

Fontes primárias: [ZDoom](https://github.com/rheit/zdoom/tree/2.8.1),
[compatibilidade oficial CQ3](https://www.chexquest3.com/about/),
[Chex 1 no Chocolate Doom](https://www.chocolate-doom.org/wiki/index.php/Chex_Quest).

## Evidência no código

- `wadsrc/static/iwadinfo.txt`: identificação nativa Chex 1 e Chex 3 por lumps.
- `src/CMakeLists.txt`: backend desktop SDL2, opção NO_ASM, opções de áudio;
  precisaremos de um backend ESP-IDF, em vez de compilar a plataforma desktop.
- `src/posix/sdl/sdlvideo.cpp`: classe SDLFB derivada de DFrameBuffer.
  A implementação Tab5 deverá oferecer buffer/paleta e apresentar RGB565 via PPA.
- `src/sound/i_sound.cpp`: NullSoundRenderer permite protótipo sem áudio;
  depois será necessário integrar um mixer com ES8388. O mixer Doom atual não
  implementa diretamente a interface SoundRenderer do ZDoom.
- `src/posix/sdl/crashcatcher.c`: chamadas de processo desktop como fork devem
  ficar fora do backend ESP-IDF.
- `CMakeLists.txt` e `wadsrc/CMakeLists.txt`: ferramentas nativas de geração
  e pacote `zdoom.pk3`; o port precisará desses recursos além do WAD do jogo.

Clone: `research/zdoom-2.8.1/`, tag 2.8.1,
commit `1a9bc53d84b5cceb567fd8246c44984aac88388a`.
Os avisos e licenças dessa árvore precisam ser preservados na integração.

## Prova de compilação RISC-V

Compilados com o toolchain ESP-IDF para RV32IMAFC/ilp32f, sem ASM/SSE:

| Objeto | Código | Dados inicializados | BSS |
| --- | ---: | ---: | ---: |
| fixed_probe.o | 60 | 0 | 0 |
| sfmt.o | 1962 | 0 | 0 |
| file_wad.o | 5104 | 8 | 0 |
| r_draw.o | 12286 | 24 | 146528 |

O primeiro é um wrapper da matemática real; os outros são módulos reais do
engine. **Não foram ligados num executável nem executados no Tab5.** Tamanhos
de objetos não são o orçamento final do jogo nem uma previsão de FPS.

Ajustes isolados: nomes stricmp/strnicmp para as funções Newlib; tipos
SDWORD/uint32 compatíveis com templates C++ no RV32; evitar redefinir strlwr,
já oferecido pelo Newlib. Patch preservado em `probe/zdoom-rv32-types.patch`.

Reprodução, após ativar o ambiente ESP-IDF:

```sh
git clone --depth 1 --branch 2.8.1 https://github.com/rheit/zdoom.git research/zdoom-2.8.1
git -C research/zdoom-2.8.1 apply ../probe/zdoom-rv32-types.patch
sh research/probe/run.sh
python3 tools/inspect_chex_wad.py research/assets/chex.wad research/assets/chex3.wad
python3 tests/test_wad_inspector.py
```

Clone/apply acima são para uma árvore nova; a árvore local já contém o patch.
Os cinco testes do inspetor passaram: PWAD válido, mapas com BEHAVIOR/scripts,
cabeçalho truncado, diretório truncado e lump fora do arquivo.

## Próximo marco de implementação

1. Criar build experimental ESP-IDF do ZDoom e gerar recursos nativos/PK3.
2. Substituir sistema/tempo/arquivos/entrada; iniciar sem áudio e rede.
3. Implementar DFrameBuffer 320x200, conversão de paleta/RGB565 e saída PPA.
4. Medir RAM, maior bloco PSRAM e tamanho real do firmware; planejar BSS em
   PSRAM e só então ajustar a partição do aplicativo se necessário.
5. Inicializar chex3.wad e carregar E1M1, antes de implementar seleção definitiva.
6. Integrar áudio, saves, iluminação e retorno ao menu; inspecionar Chex 2,
   definir as três configurações e testar abertura/gameplay/saída no aparelho.

Nem o WAD de 19 MB nem a compilação parcial demonstram que o engine completo
caberá nos 32 MiB. Esse é o limite técnico a medir no próximo marco.
