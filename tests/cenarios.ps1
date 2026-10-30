# ---------------------------------------------------------------------------
# Testes de cenário (integração) do DogWalker - Windows / PowerShell 5.1+
#
# Executa o programa compilado com entradas de teclado pré-definidas
# (redirecionadas de arquivo) e confere as mensagens e o conteúdo do CSV.
# Como a entrada é redirecionada, a interface roda no modo simples.
#
# Uso (na pasta do projeto, depois de compilar):
#   powershell -NoProfile -ExecutionPolicy Bypass -File tests\cenarios.ps1
#   mingw32-make cenarios
#
# Retorna código 0 se todos os cenários passarem e 1 caso contrário.
# Os dados de teste ficam em %TEMP% e são apagados ao final.
# ---------------------------------------------------------------------------
param(
    [string]$Projeto = (Split-Path -Parent $PSScriptRoot),
    [string]$Exe = ''
)

[Console]::OutputEncoding = New-Object Text.UTF8Encoding $false
$OutputEncoding = New-Object Text.UTF8Encoding $false
Set-Location -LiteralPath $Projeto

$exe = if ($Exe) { $Exe } else { Join-Path $Projeto 'dogwalker.exe' }
if (-not (Test-Path -LiteralPath $exe)) {
    Write-Output "Executável não encontrado: $exe (compile com mingw32-make)"
    exit 1
}
$dados = Join-Path $env:TEMP 'dw_cenarios.csv'
$entrada = Join-Path $env:TEMP 'dw_cenarios_entrada.txt'
$script:falhas = 0
$script:total = 0

# Executa um cenário: grava as linhas de entrada em arquivo (UTF-8 sem BOM,
# CRLF) e redireciona via cmd, que simula um usuário digitando. O pipe do
# PowerShell 5.1 não é usado porque ele insere um BOM na primeira linha.
function Rodar([string]$nome, [string[]]$linhas, [string[]]$esperado,
               [string[]]$proibido = @(), [string]$arquivo = $dados) {
    $script:total++
    [IO.File]::WriteAllText($entrada, (($linhas -join "`r`n") + "`r`n"), (New-Object Text.UTF8Encoding $false))
    $saida = cmd /c "`"$exe`" `"$arquivo`" < `"$entrada`"" | Out-String
    $script:ultimaSaida = $saida
    $ok = $true
    foreach ($e in $esperado) { if ($saida -notlike "*$e*") { $ok = $false; Write-Output "    faltou: '$e'" } }
    foreach ($p in $proibido) { if ($saida -like "*$p*") { $ok = $false; Write-Output "    não deveria ter: '$p'" } }
    if ($ok) { Write-Output "[ OK ] $nome" } else { $script:falhas++; Write-Output "[FALHA] $nome"; Write-Output $saida }
}

# Confere se, na saída do último cenário e depois do texto $marcador, os
# trechos aparecem exatamente na ordem dada (usado na ordenação da listagem).
function EmOrdem([string]$marcador, [string[]]$trechos) {
    $posicao = $script:ultimaSaida.LastIndexOf($marcador)
    if ($posicao -lt 0) { return $false }
    foreach ($t in $trechos) {
        $proxima = $script:ultimaSaida.IndexOf($t, $posicao)
        if ($proxima -lt 0) { return $false }
        $posicao = $proxima + $t.Length
    }
    return $true
}

function Verificar([string]$nome, [bool]$condicao) {
    $script:total++
    if ($condicao) { Write-Output "[ OK ] $nome" } else { $script:falhas++; Write-Output "[FALHA] $nome" }
}

Remove-Item $dados, "$dados.tmp" -ErrorAction SilentlyContinue

# --- Login e menu ----------------------------------------------------------
Rodar 'Login correto' @('admin','admin','5') @('Login efetuado com sucesso','Fim da execução')
Rodar 'Login incorreto 3x' @('admin','x','root','admin','a','b') @('Tentativas restantes: 2','Tentativas restantes: 1','máximo de tentativas','Acesso negado') @('Login efetuado')
Rodar 'Login com usuário gigante' @(('A'*500),'admin','admin','admin','5') @('Tentativas restantes: 2','Login efetuado com sucesso')
Rodar 'Arquivo inexistente: listagem vazia' @('admin','admin','3','','5') @('Nenhum cliente cadastrado até o momento')
Rodar 'Menu: entradas inesperadas' @('admin','admin','abc','9','1abc','','-1','5') @('Opção inválida','Fim da execução')

# --- Cadastro --------------------------------------------------------------
Rodar 'Cadastro com validações (CPF, vazio, longo, e-mail, UF, CEP, ;)' @(
  'admin','admin','1',
  'abc','123.456.789-00','111.111.111-11','529.982.247-25',
  '', ('N'*150), 'Ana;Souza', 'José D''Ávila & Cia.',
  'ana@gmail', 'Ana@Example.COM',
  'Rua das Flores','120','Centro','Campinas',
  'São Paulo','sp',
  '1234','13010-000',
  'talvez','s','','5') @(
  'CPF inválido','Campo obrigatório','Valor inválido. Use até 100','E-mail inválido','Estado inválido','CEP inválido',
  'Responda com','Cliente cadastrado com sucesso','529.982.247-25','ana@example.com','13010-000')

Rodar 'Cadastro: CPF duplicado e cancelar com ENTER' @('admin','admin','1','52998224725','','','5') @('Já existe um cliente cadastrado com este CPF','Cadastro cancelado')
Rodar 'Cadastro cancelado na confirmação' @('admin','admin','1','11144477735','Bruno','b@example.com','R','1','B','C','RJ','20000000','n','','3','','5') @('Cadastro cancelado. Nenhum dado foi gravado','Total: 1 cliente(s)')
Rodar 'Cadastro de 2 clientes adicionais' @('admin','admin',
  '1','11144477735','Bruno Lima','bruno@example.com','Avenida Brasil','1500','Jardim','Rio de Janeiro','RJ','20040002','s','',
  '1','12345678909','Cláudia Conceição','claudia@example.com','Rua São João','S/N','Savassi','Belo Horizonte','mg','30140071','s','',
  '3','','5') @('Total: 3 cliente(s)','Cláudia Conceição','Belo Horizonte/MG')

# --- Listagem --------------------------------------------------------------
Rodar 'Listagem com filtro (nome)' @('admin','admin','3','bruno','','5') @('Filtro: "bruno"','Exibindo 1 de 3','| 2   | Bruno Lima')
Rodar 'Listagem com filtro (CPF com máscara)' @('admin','admin','3','123.456','','5') @('Filtro: "123.456"','Exibindo 1 de 3','| 3   | Cláudia Conceição')
Rodar 'Listagem: filtro sem resultado' @('admin','admin','3','zzz','','5') @('Nenhum cliente corresponde ao filtro','Exibindo 0 de 3')
Rodar 'Listagem: filtro ignora acentos' @('admin','admin','3','conceicao','','5') @('Filtro: "conceicao"','Exibindo 1 de 3','| 3   | Cláudia Conceição')
Rodar 'Listagem ordenada por cidade' @('admin','admin','3','/c','','5') @('Ordenado por cidade','Exibindo 3 de 3')
Verificar 'Ordem por cidade: Belo Horizonte, Campinas, Rio de Janeiro' (EmOrdem 'Ordenado por cidade' @('| 3   | Cláudia','| 1   | José','| 2   | Bruno'))
Rodar 'Listagem ordenada por nome e de volta ao cadastro' @('admin','admin','3','/n','/o','','5') @('Ordenado por nome','Exibindo 3 de 3')
Verificar 'Ordem por nome: Bruno, Cláudia, José' (EmOrdem 'Ordenado por nome' @('| 2   | Bruno','| 3   | Cláudia','| 1   | José'))
# Três telas: cadastro, nome e (depois do /o) cadastro de novo, sem o aviso de ordem.
$telas = $script:ultimaSaida -split '/n /c /o ordem'
Verificar 'Ordem de cadastro restaurada com /o' ($telas.Count -eq 4 -and $telas[1] -like '*Ordenado por nome*' -and $telas[2] -notlike '*Ordenado por*' -and $telas[2] -like '*Exibindo 3 de 3*' -and $telas[2].IndexOf('| 1   | José') -ge 0 -and $telas[2].IndexOf('| 1   | José') -lt $telas[2].IndexOf('| 2   | Bruno'))
Rodar 'Listagem: ordem mantém o filtro' @('admin','admin','3','a','/c','','5') @('Filtro: "a"','Ordenado por cidade')
Rodar 'Listagem: comando desconhecido e texto longo' @('admin','admin','3','/z',('x'*300),'','5') @('Comando desconhecido. Digite ? para ver todos os comandos','Texto muito longo. O filtro anterior foi mantido','Exibindo 3 de 3') @('Filtro: "')
Rodar 'Atalhos por letra (l = listar, s = sair)' @('admin','admin','l','','s') @('Total: 3 cliente(s)','Fim da execução')

# --- Memória da sessão: filtro e ordem lembrados -------------------------
Rodar 'Listagem lembra filtro e ordem ao voltar' @('admin','admin','3','bruno','/n','','3','','5') @('Filtro: "bruno"')
$ultimaTela = ($script:ultimaSaida -split 'LISTA DE CLIENTES')[-1]
Verificar 'Segunda abertura mostra o filtro e a ordem anteriores' ($ultimaTela -like '*Filtro: "bruno"*' -and $ultimaTela -like '*Ordenado por nome*' -and $ultimaTela -like '*Exibindo 1 de 3*')
Rodar 'Listagem: /t limpa o filtro' @('admin','admin','3','bruno','/t','','5') @('Filtro: "bruno"')
$ultimaTela = ($script:ultimaSaida -split 'LISTA DE CLIENTES')[-1]
Verificar 'Depois do /t: todos os clientes e nenhum filtro' ($ultimaTela -like '*Exibindo 3 de 3*' -and $ultimaTela -notlike '*Filtro:*')

# --- Ficha, ajuda e exportação -------------------------------------------
Rodar 'Ficha do cliente pelo número (#2)' @('admin','admin','3','#2','','','5') @('FICHA DO CLIENTE','Cliente nº 2 de 3','Bruno Lima','111.444.777-35','voltar à lista')
Rodar 'Ficha: número inválido' @('admin','admin','3','#99','#abc','#0','','5') @('Número de cliente inválido') @('FICHA DO CLIENTE')
Rodar 'Ajuda pelo menu (?)' @('admin','admin','?','','5') @('AJUDA','/+ /-','#3','/e','/x','Pressione ENTER para voltar ao menu','Fim da execução')
Rodar 'Ajuda pela listagem (?)' @('admin','admin','3','?','','','5') @('AJUDA','Pressione ENTER para voltar à lista','Exibindo 3 de 3')

$relatorio = Join-Path $env:TEMP 'relatorio.html'
$planilha = Join-Path $env:TEMP 'exportacao.csv'
Remove-Item $relatorio, $planilha -ErrorAction SilentlyContinue
Rodar 'Exportar relatório HTML com filtro (/e)' @('admin','admin','3','bruno','/e','','5') @('Relatório gerado com 1 cliente(s)','relatorio.html')
$html = if (Test-Path $relatorio) { [IO.File]::ReadAllText($relatorio, [Text.Encoding]::UTF8) } else { '' }
Verificar 'Relatório tem só o recorte filtrado, com CPF mascarado' ($html -like '*Bruno Lima*' -and $html -notlike '*Cláudia*' -and $html -like '*`*`*`*.444.777-`*`**' -and $html -notlike '*111.444.777-35*' -and $html -like '*Clientes com &quot;bruno&quot;*')
Rodar 'Exportar relatório HTML completo (/e)' @('admin','admin','3','/e','','5') @('Relatório gerado com 3 cliente(s)')
$html = if (Test-Path $relatorio) { [IO.File]::ReadAllText($relatorio, [Text.Encoding]::UTF8) } else { '' }
Verificar 'Relatório escapa caracteres especiais do nome' ($html -like '*José D&#39;Ávila &amp; Cia.*' -and $html -notlike "*José D'Ávila & Cia.*")
Rodar 'Exportar planilha ordenada por nome (/n e /x)' @('admin','admin','3','/n','/x','','5') @('Planilha gerada com 3 cliente(s)','exportacao.csv')
$csv = if (Test-Path $planilha) { [IO.File]::ReadAllText($planilha, [Text.Encoding]::UTF8) } else { '' }
$posBruno = $csv.IndexOf('Bruno Lima;111.444.777-35;'); $posClaudia = $csv.IndexOf('Cláudia Conceição;'); $posJose = $csv.IndexOf("José D'Ávila & Cia.;")
Verificar 'Planilha na ordem da tela (Bruno, Cláudia, José) e com CPF completo' ($posBruno -gt 0 -and $posClaudia -gt $posBruno -and $posJose -gt $posClaudia)
Remove-Item $relatorio, $planilha -ErrorAction SilentlyContinue

# --- Desfazer remoção (em uma cópia do arquivo de exemplo) -----------------
$dadosDesfazer = Join-Path $env:TEMP 'dw_cenarios_desfazer.csv'
Copy-Item (Join-Path $Projeto 'data\clientes_exemplo.csv') $dadosDesfazer -Force
Rodar 'Desfazer a última remoção' @('admin','admin','4','529.982.247-25','s','','4','s','','3','','5') @('Mudou de ideia','Desfazer a remoção de Ana Souza','restaurado com sucesso','Total: 3 cliente(s)') @() $dadosDesfazer
Verificar 'Cliente restaurado está no arquivo' ((Get-Content $dadosDesfazer -Encoding UTF8) -match '^52998224725;Ana Souza;').Count -eq 1
Rodar 'Recusar o desfazer e remover outro cliente' @('admin','admin','4','529.982.247-25','s','','4','n','111.444.777-35','s','','3','','5') @('Desfazer a remoção de Ana Souza','Cliente removido com sucesso','Total: 1 cliente(s)') @('restaurado com sucesso') $dadosDesfazer
Remove-Item $dadosDesfazer, "$dadosDesfazer.tmp" -ErrorAction SilentlyContinue

# --- Paginação (20 clientes gerados) ---------------------------------------
# Gera CPFs válidos com a mesma regra dos testes unitários.
function Gerar-Cpf([int]$semente) {
    $base = '{0:D9}' -f (100000000 + $semente * 7919)
    $d = @($base.ToCharArray() | ForEach-Object { [int][string]$_ })
    $s = 0; for ($i = 0; $i -lt 9; $i++) { $s += $d[$i] * (10 - $i) }; $d += ($s * 10) % 11 % 10
    $s = 0; for ($i = 0; $i -lt 10; $i++) { $s += $d[$i] * (11 - $i) }; $d += ($s * 10) % 11 % 10
    return ($d -join '')
}
$dadosPaginas = Join-Path $env:TEMP 'dw_cenarios_paginas.csv'
$linhasCsv = @('cpf;nome;email;rua;numero;bairro;cidade;estado;cep')
for ($n = 1; $n -le 20; $n++) { $linhasCsv += "$(Gerar-Cpf $n);Cliente $n;c$n@example.com;Rua A;$n;Centro;Campinas;SP;13010000" }
[IO.File]::WriteAllText($dadosPaginas, (($linhasCsv -join "`n") + "`n"), (New-Object Text.UTF8Encoding $false))
Rodar 'Paginação: avançar, limites e voltar' @('admin','admin','3','/+','/+','/-','/-','','5') @('Página 1 de 2','Página 2 de 2','| 16  | Cliente 16','| 20  | Cliente 20','Esta já é a última página','Esta já é a primeira página','Exibindo 20 de 20') @() $dadosPaginas
Verificar 'Primeira página mostra só 15 clientes' (($script:ultimaSaida -split 'Página 1 de 2')[0] -like '*| 15  | Cliente 15*' -and ($script:ultimaSaida -split 'Página 1 de 2')[0] -notlike '*| 16  |*')
Rodar 'Paginação: filtro volta para a página 1' @('admin','admin','3','/+','cliente 1','','5') @('Página 2 de 2','Filtro: "cliente 1"','Exibindo 11 de 20') @() $dadosPaginas
Remove-Item $dadosPaginas -ErrorAction SilentlyContinue

# --- Alteração -------------------------------------------------------------
$antes = Get-Content $dados -Encoding UTF8
Rodar 'Alteração: só o e-mail do Bruno' @('admin','admin','2','111.444.777-35','','bruno.novo@example.com','','','','','','','s','','5') @('Cliente encontrado','[Bruno Lima]','Dados alterados com sucesso')
$depois = Get-Content $dados -Encoding UTF8
$dif = Compare-Object $antes $depois
Verificar 'Alteração mudou SOMENTE a linha do Bruno no arquivo' ($dif.Count -eq 2 -and ($depois -match 'bruno.novo@example.com').Count -eq 1 -and $depois[1] -eq $antes[1] -and $depois[3] -eq $antes[3])

Rodar 'Alteração sem mudanças' @('admin','admin','2','11144477735','','','','','','','','','','5') @('Nenhum dado foi modificado')
Rodar 'Alteração: valor inválido não apaga o atual' @('admin','admin','2','11144477735','','email-ruim','','','','','','','','','5') @('E-mail inválido','Nenhum dado foi modificado')
Rodar 'Alteração: cliente inexistente' @('admin','admin','2','98765432100','','5') @('Nenhum cliente encontrado com o CPF 987.654.321-00')

# --- Remoção ---------------------------------------------------------------
Rodar 'Remoção: CPF inexistente' @('admin','admin','4','98765432100','','5') @('Nenhum cliente encontrado')
Rodar 'Remoção cancelada' @('admin','admin','4','52998224725','n','','3','','5') @('Remoção cancelada','Total: 3 cliente(s)')
$antes = Get-Content $dados -Encoding UTF8
Rodar 'Remoção: só a Ana' @('admin','admin','4','529.982.247-25','s','','3','','5') @('Cliente removido com sucesso','Total: 2 cliente(s)')
$depois = Get-Content $dados -Encoding UTF8
Verificar 'Remoção preservou os outros clientes intactos e na ordem' ($depois.Count -eq 3 -and -not ($depois -match '52998224725') -and $depois[1] -eq $antes[2] -and $depois[2] -eq $antes[3])

# --- Arquivo e ambiente ----------------------------------------------------
Set-Content -Path $dados -Value '' -NoNewline
Rodar 'Arquivo vazio' @('admin','admin','3','','5') @('Nenhum cliente cadastrado até o momento')

Copy-Item (Join-Path $Projeto 'data\clientes_exemplo.csv') $dados
Add-Content -Path $dados -Value 'lixo;lixo' -Encoding UTF8
Rodar 'Arquivo com linha corrompida' @('admin','admin','3','','5') @('1 linha(s) inválida(s)','Total: 3 cliente(s)','Cláudia Conceição')

Rodar 'Pasta inexistente' @('admin','admin','1','52998224725','Ana','a@example.com','R','1','B','C','SP','13010000','s','','5') @('não foi possível gravar','Nenhuma alteração foi gravada') @('cadastrado com sucesso') (Join-Path $env:TEMP 'dw_pasta_que_nao_existe\c.csv')
Rodar 'Fim da entrada no meio do cadastro' @('admin','admin','1','52998224725','Ana') @('Entrada encerrada')

$env:DOGWALKER_USUARIO = 'maria'; $env:DOGWALKER_SENHA = 's3nh@'
Rodar 'Credenciais via variável de ambiente' @('maria','s3nh@','5') @('Bem-vindo(a), maria')
Rodar 'Senha padrão recusada quando há variável' @('admin','admin') @('Tentativas restantes: 2') @('Login efetuado')
Remove-Item Env:DOGWALKER_USUARIO, Env:DOGWALKER_SENHA

Remove-Item $dados, "$dados.tmp", $entrada -ErrorAction SilentlyContinue

Write-Output ''
Write-Output "$($script:total) cenários, $($script:falhas) falha(s)."
if ($script:falhas -gt 0) { exit 1 }
exit 0
