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
    $ok = $true
    foreach ($e in $esperado) { if ($saida -notlike "*$e*") { $ok = $false; Write-Output "    faltou: '$e'" } }
    foreach ($p in $proibido) { if ($saida -like "*$p*") { $ok = $false; Write-Output "    não deveria ter: '$p'" } }
    if ($ok) { Write-Output "[ OK ] $nome" } else { $script:falhas++; Write-Output "[FALHA] $nome"; Write-Output $saida }
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
Rodar 'Atalhos por letra (l = listar, s = sair)' @('admin','admin','l','','s') @('Total: 3 cliente(s)','Fim da execução')

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
