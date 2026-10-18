@echo off
setlocal
cd /d "%~dp0"
set "REPO=https://github.com/Nexvyra-studio/Backrooms.git"

echo ============================================
echo   Backup do projeto Backrooms no GitHub
echo ============================================
echo.

rem --- 1) Verifica Git e Git LFS ---
where git >nul 2>nul
if errorlevel 1 (
    echo [ERRO] Git nao encontrado.
    echo Instale o Git for Windows: https://git-scm.com/download/win
    pause
    exit /b 1
)
git lfs version >nul 2>nul
if errorlevel 1 (
    echo [ERRO] Git LFS nao encontrado. Reinstale o Git for Windows marcando "Git LFS".
    pause
    exit /b 1
)

rem --- 2) Avisa se a Unreal estiver aberta ---
tasklist /FI "IMAGENAME eq UnrealEditor.exe" 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo [AVISO] A Unreal esta aberta. Salve tudo ^(Ctrl+Shift+S^) antes de continuar.
    choice /M "Continuar mesmo assim"
    if errorlevel 2 exit /b 0
    echo.
)

rem --- 3) Nome e e-mail do Git (so na primeira vez) ---
git config user.email >nul 2>nul
if errorlevel 1 goto :configurar
goto :repositorio

:configurar
echo Primeira vez: configure seu nome e e-mail do GitHub.
set /p "GITNOME=Seu nome: "
set /p "GITEMAIL=Seu e-mail do GitHub: "
git config --global user.name "%GITNOME%"
git config --global user.email "%GITEMAIL%"
echo.

:repositorio
rem --- 4) Cria o repositorio local (so na primeira vez) ---
if not exist ".git" (
    echo Criando repositorio local...
    git init -b main
    git lfs install
    echo.
)
git remote get-url origin >nul 2>nul
if errorlevel 1 git remote add origin "%REPO%"

rem --- 5) Mensagem do commit ---
set "MSG="
set /p "MSG=Mensagem do commit (Enter = data e hora): "
if not defined MSG set "MSG=Atualizacao %date% %time:~0,5%"

rem --- 6) Commit ---
git add -A
git diff --cached --quiet
if not errorlevel 1 (
    echo.
    echo Nada mudou desde o ultimo envio.
    pause
    exit /b 0
)
git commit -m "%MSG%"

rem --- 7) Envia para o GitHub ---
echo.
echo Enviando para o GitHub...
git push -u origin main
if errorlevel 1 (
    echo Tentando sincronizar com o GitHub e enviar de novo...
    git pull --rebase --allow-unrelated-histories origin main
    git push -u origin main
)

echo.
if errorlevel 1 (
    echo [ERRO] Nao foi possivel enviar. Veja as mensagens acima.
) else (
    echo Enviado com sucesso!
)
pause
