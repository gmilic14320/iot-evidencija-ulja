@echo off
chcp 65001 >nul
cd /d "%~dp0Kod\podesavanje"
if not exist ".venv-win\Scripts\pythonw.exe" (
    echo Prvo pokretanje: priprema programa. Potrebna je internet veza.
    py -3 -m venv .venv-win
    if errorlevel 1 goto greska
)
".venv-win\Scripts\python.exe" -c "import tkinter"
if errorlevel 1 goto greska
".venv-win\Scripts\python.exe" -c "import serial" >nul 2>&1
if errorlevel 1 (
    ".venv-win\Scripts\python.exe" -m pip install -r requirements.txt
    if errorlevel 1 goto greska
)
start "" ".venv-win\Scripts\pythonw.exe" "podesi_tank.pyw"
exit /b
:greska
echo Potreban je Python 3 sa Tkinter-om i py pokretačem.
echo Proverite internet vezu i Python sa python.org, pa pokušajte ponovo.
pause
