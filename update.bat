@echo off

cd /d %~dp0

git.exe stash push -u -m "%date:~,10% %time%"

if not ERRORLEVEL 1 (
    git.exe fetch -v --progress "origin"
    git.exe reset --hard @{u}

    git.exe stash show >nul 2>nul
    if not ERRORLEVEL 1 (
        "C:\Program Files\TortoiseGit\bin\TortoiseGitProc.exe" /command:stashpop /path:"%~dp0"
    ) else (
        @ping -n 3 127.1 >nul 2>nul
    )

    exit /B 0
)

pause
