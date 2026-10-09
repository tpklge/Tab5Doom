# Tab5CHEX — M5Stack Tab5

Base de trabalho para portar **Chex Quest 1, 2 e 3 original**.
Criada em 2026-10-05 como cópia da versão local validada de Tab5Freedoom.
Projeto ESP-IDF: `Tab5CHEX`, alvo ESP32-P4, versão ESP-IDF 5.4.4.

## Estado atual

A aplicação principal permanece como referência funcional da cópia Freedoom.
O [projeto experimental ESP-IDF](research/idf/README.md) agora compila e liga
ZDoom 2.8.1 com SD, sistema, relógio, framebuffer ST7123/PPA e teclado Tab5/USB.
**Chex Quest 3 original v1.4 abriu E1M1 no Tab5**, confirmado pelo usuário,
com aproximadamente 16–22 FPS na serial após a correção da pilha do renderer.
O firmware atual acrescenta efeitos sonoros ES8388/I2S, com 16 vozes e saída
PCM estéreo a 22050 Hz. Música permanece desativada.

O binário e os arquivos do cartão estão em `build/chex3-bringup/`.
A integração de áudio compilou e passou nos testes dos 84 efeitos originais,
e o usuário confirmou som funcionando no aparelho. O menu de três jogos foi
adicionado e compilado; sua seleção e o lançamento de Chex 1/2 ainda precisam
de teste físico. Nenhum flash foi realizado.

WADs, fontes e avaliação do engine estão em
[research/ANALISE_CHEX.md](research/ANALISE_CHEX.md).

Mantidos da base: display ST7123/MIPI e PPA, teclado Tab5/USB, áudio ES8388,
leitura de WAD pelo microSD, nomes longos FAT, saves separados e saída por
reinicialização controlada. A iluminação apaga antes do reinício e acende
após desenhar o menu. Build, logs e downloads da base não foram copiados.
Os projetos Tab5Freedoom e Tab5FinalDoom permanecem intactos.

## Compatibilidade e próximos trabalhos

O objetivo é um menu com três opções e os dados na pasta `/doom` do microSD.
Os arquivos exatos e argumentos serão definidos conforme a distribuição de
cada jogo, incluindo dependências e patches; não basta trocar o nome do WAD.

- **Chex Quest 1:** a base identifica `chex.wad` e possui comportamento
  `exe_chex`. É necessário habilitar a aplicação de `chex.deh`: DEHACKED está
  desativado e os arquivos de implementação desse suporte não estão na base.
- **Chex Quest 2:** verificar a distribuição original e a relação entre o
  WAD de expansão e os recursos do primeiro jogo antes de definir o lançamento.
- **Chex Quest 3 original:** opção escolhida pelo usuário. O engine atual não
  oferece os recursos de ZDoom necessários. Avaliar e portar outro engine,
  preservando a camada de hardware validada. Não substituir por adaptação vanilla.

O [site oficial Chex Quest 3](https://www.chexquest3.com/about/) descreve
as versões e os engines compatíveis. O tamanho dos dados é apenas uma parte
da viabilidade: formatos de mapas, scripts, memória e dependências do engine
precisam ser considerados antes de prometer execução no ESP32-P4.

A sequência de implementação será: identificar os pacotes originais; escolher
um engine adequado e avaliar seu custo no P4; integrar display, entrada, áudio
e armazenamento; definir o menu de três jogos e saves isolados; compilar e
validar abertura, gameplay e saída de cada título no aparelho.

## Referências locais

- `README_FREEDOOM_BASE.md`: uso e validação da versão de origem.
- `TAB5FREEDOOM_BASE_HISTORY.txt`: histórico da base Freedoom.
- `TAB5FINALDOOM_BASE_HISTORY.txt`: histórico anterior Final Doom.
- `TAB5DOOM_DISPLAY_INIT_HISTORY.txt`: histórico herdado de inicialização do display.
- `TAB5DOOM_PROGRESS.txt`: progresso desta versão CHEX.

A cópia também preserva os metadados Git da base, incluindo seu remoto.
Nenhum commit ou publicação foi realizado.

## Licenças

O engine doomgeneric conserva seus avisos GPL-2.0-or-later. Os avisos de
hardware estão em `main/THIRD_PARTY_NOTICES.txt`. Os dados dos jogos Chex não
estão incluídos nesta pasta; sua distribuição e licença são independentes
oferecidas por seus autores.

## Menu Chex 1, 2 e 3

O firmware experimental inicia um seletor com setas/W/S ou teclas 1/2/3,
Enter para jogar e R para buscar novamente os WADs. Todos usam ZDoom 2.8.1.
Saída pelo menu do jogo reinicia a aplicação e retorna ao seletor, com o
backlight apagado durante o reinício.

| Opção | Dados em /doom | Lançamento |
| --- | --- | --- |
| Chex Quest 1 original | chex.wad + zdoom.pk3 | -iwad chex.wad |
| Chex Quest 2 original | chex.wad + chex2.wad + zdoom.pk3 | -iwad chex.wad -file chex2.wad |
| Chex Quest 3 original v1.4 | chex3.wad + zdoom.pk3 | -iwad chex3.wad |

Cada seleção inicia E1M1 e usa chex1/2/3-zdoom.ini e chex1/2/3-saves,
respectivamente. A configuração e os saves antigos de Chex 3 permanecem em
chex-zdoom.ini e chex-saves; não são migrados automaticamente.

A identificação nativa de Chex 1 está em wadsrc/static/iwadinfo.txt do ZDoom;
Chex 2 é carregado como expansão da base Chex 1. A distribuição original de
Chex 2 ainda não foi auditada localmente nem testada no aparelho. Ausência de
chex2.wad desabilita somente essa seleção. Não usamos os episódios de Chex 3
como substitutos dos arquivos originais de Chex 1/2.

O pacote inclui chex.wad e chex3.wad; fornecer chex2.wad original no cartão.
A validação do menu verifica assinatura IWAD/PWAD e limites do diretório;
a compatibilidade e os limites individuais dos lumps são tratados pelo engine.
