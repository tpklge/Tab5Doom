# ZDoom no Tab5 — primeira tentativa de Chex Quest 3

O projeto experimental agora **compila e liga o engine com os backends ESP-IDF**.
**Chex Quest 3 original v1.4 em E1M1 abriu no aparelho**, a 320×200,
com display ST7123/PPA e teclado Tab5/USB. A serial registrou 16–22 FPS.
O firmware atual integra efeitos sonoros ES8388; música permanece desativada.

O áudio passou na compilação e nos testes nativos; o usuário confirmou som
funcionando no aparelho antes da integração do menu.
Não houve flash. O menu dos três jogos foi implementado; seleção e lançamento de Chex 1/2
ainda precisam de teste físico.
A aplicação principal e os outros projetos permanecem preservados.

## Arquivos para o primeiro teste

Pacote local: `../../build/chex3-bringup/`.

- `Tab5CHEX_Chex3_Experimental.bin`: aplicativo para instalar pelo M5Launcher.
- `microSD/doom/zdoom.pk3`: recursos obrigatórios do engine.
- `microSD/doom/chex3.wad`: WAD original v1.4 baixado do site do autor.
- Leia-me/créditos originais incluídos junto ao WAD.
- `MANIFEST.json`: tamanhos, hashes e indicação de que falta validar no aparelho.

Copie o conteúdo de `microSD/doom/` para a pasta `/doom` do cartão.
Mantenha ambos os arquivos `zdoom.pk3` e `chex3.wad` nessa pasta.
O WAD Chex 3 oficial tem cabeçalho **PWAD**, aceito pelo engine.
O PK3 gerado para esta revisão tem 584 arquivos e 634800 bytes; SHA256
`d6486b561a11249d34fb0d00208312cc9255a5a954a44a4e726f6ca5ed6e8b78`.
Não é necessário `chex.wad` para esta tentativa do terceiro jogo.

Ao iniciar, a serial 115200 deve registrar montagem do SD, memória disponível,
`Starting original Chex Quest 3, E1M1`, etapas de carga do engine e
`First engine frame`. Depois registra taxa de apresentação/PPA a cada 5 segundos.
Os primeiros números de RAM são antes da inicialização; o primeiro frame fornece
outra medição, ainda sem representar o pior caso durante gameplay.
Se faltar arquivo, o log informa o caminho. Erros lançados pelo engine são
capturados e registrados; reinicie o aplicativo antes de tentar novamente.
O backlight acende somente após apresentar o primeiro frame completo.

Controles da base: setas/WASD, Ctrl para disparar, E/espaço para usar,
Aa para correr, Enter para confirmar e Esc para abrir o menu.
A tradução de eventos atende ao menu GUI e aos scan codes do ZDoom.
A saída pelo menu executa callbacks e reinicia com o backlight apagado.
O reinício volta a esta aplicação experimental, que inicia E1M1 novamente.

## Integração concluída para a tentativa

- 269 unidades do core, incluindo a interface de áudio e fallback nulo do upstream.
- 59 unidades C: zlib, bzip2, LZMA/7z, JPEG e conversão decimal gdtoa.
- Parsers Lemon/re2c, revisão e recursos PK3 gerados sem dependência SDL.
- Contratos ESP de tempo, diretórios, caminhos de config/saves/cache,
  exceções, log, inicialização, modo single player e dispositivos opcionais.
- Relógio de 35 tics/s com pausa/retomada; `I_MSTime` continua durante pausa.
- Framebuffer paletizado 320×200, conversão RGB565, gamma e flashes,
  apresentação com a camada MIPI/PPA validada da base.
- BSP copiado para a experiência, com 16 arquivos simultâneos no FAT VFS.
  Nomes longos habilitados; cartão nunca formatado automaticamente.
- BSS do core em PSRAM: o mapa atual reserva aproximadamente 937 KiB.
  Buffer RGB565 do PPA também usa PSRAM para preservar a RAM interna.
- `WHOLE_ARCHIVE` e `KEEP/SORT` preservam e ordenam registros de classes,
  ações e propriedades. Tabelas ficam em SRAM gravável: o engine ordena classes.
- 623 slots de ações, 292 de classes, 196 de propriedades, 36 de variáveis
  e 45 de opções MAPINFO. Auditoria do ELF verifica limites e classes do backend.
- Software renderer criado explicitamente. Rede multiplayer, mouse, joystick,
  CD, serviços de distribuição e música não estão disponíveis neste protótipo.

O binário tem aproximadamente 3,02 MB; ver tamanho/hash exatos no manifest.
A partição standalone é de 14 MiB. Esse tamanho agora inclui registros e core
usados na inicialização; os binários mínimos históricos não mediam o engine.
A serial da versão sem áudio registrou 16–22 FPS; falta medir a versão com áudio
e o pico de memória durante exploração dos mapas.

## Reprodução

Na raiz de Tab5CHEX, com ZDoom 2.8.1 no commit registrado e o patch aplicado:

```sh
# No Mac usado neste projeto, ferramentas nativas precisam do SDK 15.4.
export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk
python3 tools/prepare_zdoom.py
# Ative ESP-IDF 5.4.4 e use um sdkconfig criado com sdkconfig.defaults atual.
idf.py -C research/idf build
python3 tests/test_zdoom_registry.py
python3 tools/package_chex_bringup.py
```

O build local atual usa `research/idf/sdkconfig_bringup`, selecionado no cache
CMake, para preservar a configuração do programa mínimo anterior.
Em uma configuração nova, `sdkconfig.defaults` contém os parâmetros necessários.
Sem reutilizar o cache local:
`idf.py -C research/idf -B build-fresh build`.

Parsers/`arith.h`/`gd_qnan.h` ficam em `research/generated/`. A configuração
numérica é RV32 little-endian/IEEE binary64, sem reaproveitar arithchk LP64 do Mac.
Patch reproduzível: `../probe/zdoom-rv32-types.patch`.
Log do link completo: `../full-link.log`; registros: `../REGISTRY_AUDIT.json`.
O inventário anterior `ARCHIVE_AUDIT.json` é histórico da biblioteca isolada.

## Verificação realizada

```sh
python3 tests/test_wad_inspector.py
python3 tests/test_zdoom_paths.py
python3 tests/test_zdoom_specials.py
python3 tests/test_zdoom_compression.py
python3 tests/test_zdoom_clock.py
python3 tests/test_zdoom_file_backend.py
python3 tests/test_zdoom_registry.py
```

Testes nativos passaram no Mac com SDK 15.4. O backend de diretórios foi
exercitado com ASan/UBSan, wildcard sem distinguir maiúsculas, atributos,
caminhos ausentes e sem mudar o diretório de trabalho. A descompressão compara
bytes/CRC e rejeita um cabeçalho zlib corrompido. A auditoria de registros lê o
ELF RV32 realmente ligado. Display, teclado e gameplay ainda exigem o teste físico.

## Próximo marco

Instalar a experiência, iniciar Chex 3 e capturar serial. Corrigir erros de
inicialização ou memória que surgirem, medir E1M1 e testar entrada/saída.
Depois validar o áudio e integrar o menu dos três jogos, com saves separados e lançamento
correto das distribuições originais Chex 1/2. O port dos três jogos ainda está em curso.

## Primeiro log e correção

O primeiro teste confirmou 584 lumps do PK3 e 2240 do WAD, com aproximadamente
30,5 MB de PSRAM livre antes do engine. O crash em `0x4002c212` foi localizado
em `CheckCmdLine`, `d_main.cpp:2206`: autostart escreve em `StartScreen`, mas
`CreateInstance` retornava nulo. O backend agora cria `FStartupScreen` do upstream.
O pacote foi atualizado; essa correção ainda não foi validada no aparelho.

Segunda tentativa: a correção StartScreen foi confirmada; a carga avançou até
`Resolution: 320 x 200`. Depois parou porque o backend informava caminho para
`bots.cfg` mesmo ausente. O contrato foi corrigido para retornar string vazia
quando o arquivo opcional não existe, como faz o upstream. Não é necessário
criar esse arquivo para single player. Pacote atualizado, novo teste pendente.

Terceira tentativa confirmou `e1m1 - Landing Zone` e o primeiro frame, com
27,3 MB de PSRAM livre e 44847 bytes de RAM interna livre. Em seguida houve
`Could not malloc 0 bytes`. Os wrappers ESP de malloc/realloc agora normalizam
tamanho zero para um byte, mantendo o contrato esperado pelo engine.
Regressão com ASan/UBSan passou; binário atualizado ainda exige teste físico.
O aviso ACS anterior corresponde ao BEHAVIOR vazio de 16 bytes de E1M1, que
o upstream ignora por ser menor que 32 bytes; não indica por si só WAD inválido.
Teste adicional: `python3 tests/test_zdoom_zero_alloc.py` (SDK 15.4 neste Mac).

Quarta tentativa: usuário confirmou E1M1 visível e azul em seguida. A serial
capturou Stack protection fault em `wallscan_np2` (`r_segs.cpp:1319`), com
a pilha de 32 KiB ultrapassada. Três vetores `short[MAXWIDTH]` reservavam
34560 bytes porque o limite desktop era 5760 pixels. No alvo ESP os limites
foram ajustados para 320×200, única resolução do backend. O maior frame
compilado de `r_segs.cpp` agora é 2016 bytes; a cadeia completa precisa de
validação em execução. Desktop mantém seus limites originais.

Auditoria: `python3 tools/audit_zdoom_stack.py`, relatório `STACK_AUDIT.json`.
Funções de screenshot e ZIP antigo ainda têm frames individuais acima de
32 KiB; não fazem parte da tentativa E1M1 e precisam de ajuste antes de uso.
O pacote foi atualizado; ainda é necessária a nova tentativa no aparelho.

## E1M1 validado e efeitos sonoros

Após limitar MAXWIDTH/MAXHEIGHT, o usuário confirmou o jogo aberto; a serial
registrou aproximadamente 16–22 FPS, sem o crash anterior da pilha.

Foi adicionado um SoundRenderer próprio para o ES8388/I2S: 16 vozes,
WAV PCM8/16 e raw PCM, reamostragem para 22050 Hz estéreo, volume, distância,
pan, pausa e loops. A tarefa de áudio executa no núcleo 1, sem chamar callbacks
do engine; término e liberação de canais ocorrem na tarefa do jogo.
O backend `tab5` é selecionado mesmo se a configuração anterior salvou `null`.
`-nosound` foi removido; `-nomusic` permanece. O leitor aceita a ausência de
padding no último chunk de sete WAVs originais, preservando os limites RIFF.

Teste: `SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.4.sdk python3 tests/test_zdoom_audio.py`.
Verifica os 84 sons do WAD, mistura, clipping, estéreo, reamostragem, pausa,
loops e rejeição de WAV truncado, com ASan/UBSan. Áudio e impacto no FPS ainda
precisam de validação física. Instalar o novo binário pelo M5Launcher e testar
tiros, portas e itens; WAD e PK3 permanecem os mesmos.

## Seletor implementado

O boot agora apresenta Chex 1/2/3. Chex 1 usa chex.wad; Chex 2 usa a mesma
base mais -file chex2.wad; Chex 3 usa chex3.wad. Todos exigem zdoom.pk3.
Configurações e saves têm nomes separados por jogo. Saída reinicia e retorna
ao seletor. Setas/W/S ou 1/2/3 selecionam, Enter confirma, R relê os arquivos.

O pacote inclui chex.wad oficial já obtido. Chex 2 original ainda precisa ser
fornecido e auditado; essa opção fica indisponível sem chex2.wad. Nenhum episódio
do WAD Chex 3 é usado como substituto do pacote original de Chex 1/2.
Build do menu passou; teste de seleção, retorno e Chex 1/2 pendente no Tab5.

## Correção da confirmação de saída

O usuário relatou efeitos sonoros ao confirmar saída, sem retornar ao seletor.
O menu upstream DQuitMenu::HandleResult chama ST_Endoom; a implementação ESP
estava vazia, enquanto o backend SDL termina o processo. ST_Endoom agora chama
I_Quit, executando os callbacks de fechamento, apagando o backlight e reiniciando.
A serial registra Quit requested e Shutdown complete para localizar eventual
problema posterior no fechamento. A correção ainda precisa de teste no aparelho.

Backlight na saída: I_Quit agora chama chex_prepare_restart antes de executar
os callbacks. Assim o painel já fica sem iluminação durante a desmontagem de
vídeo/áudio e permanece apagado até o primeiro frame após o reinício.
Validação visual pendente.
