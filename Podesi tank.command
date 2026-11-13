#!/bin/zsh
koren_projekta="${0:A:h}"
if [[ -d "$koren_projekta/Podesi tank.app" ]]; then
    /usr/bin/open "$koren_projekta/Podesi tank.app"
    exit $?
fi

cd "$koren_projekta/Kod/podesavanje" || exit 1
greska() {
    print "$1"
    print "Instalirajte Python 3 sa Tkinter-om sa python.org i pokušajte ponovo."
    read "?Pritisnite Enter za zatvaranje."
    exit 1
}

# Koristi postojeće okruženje ili priprema novo.
if [[ ! -x .venv/bin/python ]]; then
    python_program="/Library/Frameworks/Python.framework/Versions/Current/bin/python3"
    [[ -x "$python_program" ]] || python_program="$(command -v python3)"
    [[ -n "$python_program" ]] || greska "Python 3 nije pronađen."
    "$python_program" -c 'import tkinter; assert tkinter.TkVersion >= 8.6' 2>/dev/null ||
        greska "Ova Python instalacija nema odgovarajući Tkinter."
    "$python_program" -m venv .venv || greska "Priprema okruženja nije uspela."
fi

.venv/bin/python -c 'import tkinter; assert tkinter.TkVersion >= 8.6' 2>/dev/null ||
    greska "Tkinter nije dostupan u okruženju."
if ! .venv/bin/python -c 'import serial' 2>/dev/null; then
    print "Prvo pokretanje: preuzimanje potrebne biblioteke."
    .venv/bin/python -m pip install -r requirements.txt ||
        greska "Instalacija biblioteke nije uspela. Proverite internet vezu."
fi
.venv/bin/python podesi_tank.pyw
[[ $? == 0 ]] || greska "Aplikacija nije pokrenuta."
