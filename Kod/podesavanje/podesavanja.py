import math

NAJVISE_TACAKA = 32


# Prihvata broj sa zarezom ili tačkom kao decimalnim separatorom.
def procitaj_broj(vrednost, naziv_polja):
    try:
        rezultat = float(str(vrednost).strip().replace(",", "."))
    except ValueError:
        raise ValueError(f"{naziv_polja}: unesite broj.") from None
    if not math.isfinite(rezultat):
        raise ValueError(f"{naziv_polja}: broj mora biti konačan.")
    return rezultat


# Unos je u ml, a ostatak programa koristi litre.
def mililitri_u_litre(vrednost):
    mililitri = procitaj_broj(vrednost, "Količina u ml")
    if mililitri < 0:
        raise ValueError("Količina mora biti najmanje 0 ml.")
    return mililitri / 1000


# Proverava naziv i ograničenje od 11 znakova za prikaz na LCD-u.
def proveri_naziv(naziv):
    naziv = naziv.strip()
    if not naziv or len(naziv) > 11 or any(not (32 <= ord(znak) <= 126 or znak in "čćšđžČĆŠĐŽ") for znak in naziv):
        raise ValueError("Naziv: 1-11 znakova, latinica.")
    return naziv


# Količina mora da raste, a udaljenost do tečnosti da opada.
def proveri_tacke(tacke):
    if not 2 <= len(tacke) <= NAJVISE_TACAKA:
        raise ValueError("Unesite 2-32 tačke kalibracije.")
    prethodna = None
    for tacka in tacke:
        udaljenost_cm, kolicina_l = tacka["distance_cm"], tacka["volume_l"]
        if not math.isfinite(udaljenost_cm) or not 2 <= udaljenost_cm <= 400:
            raise ValueError("Udaljenost mora biti između 2 i 400 cm.")
        if not math.isfinite(kolicina_l) or kolicina_l < 0:
            raise ValueError("Količina mora biti nenegativan broj.")
        if prethodna and (udaljenost_cm >= prethodna["distance_cm"] or kolicina_l <= prethodna["volume_l"]):
            raise ValueError("Dodajte tačke redom: količina raste, udaljenost opada.")
        prethodna = tacka
    if tacke[0]["volume_l"] != 0:
        raise ValueError("Prva tačka mora biti prazna posuda, 0 ml.")
    return tacke


# Dimenzije su u cm, a izračunata zapremina u litrima.
def tacke_iz_dimenzija(oblik, visina=None, razmak_senzora=None, duzina=None, sirina=None, precnik=None, stranica=None):
    if oblik == "Kocka":
        if stranica is None or not math.isfinite(stranica) or stranica <= 0:
            raise ValueError("Stranica mora biti veća od nule.")
        visina = stranica
    if visina is None or not math.isfinite(visina) or visina <= 0:
        raise ValueError("Visina mora biti veća od nule.")
    if razmak_senzora is None or not math.isfinite(razmak_senzora) or razmak_senzora < 2 or visina + razmak_senzora > 400:
        raise ValueError("Proverite položaj senzora: pun tank najmanje 2 cm, prazan najviše 400 cm.")
    if oblik == "Kocka":
        kapacitet = stranica ** 3 / 1000
    elif oblik == "Pravougaoni":
        if duzina is None or sirina is None or not all(math.isfinite(dimenzija) and dimenzija > 0 for dimenzija in (duzina, sirina)):
            raise ValueError("Dužina i širina moraju biti veće od nule.")
        kapacitet = duzina * sirina * visina / 1000
    elif oblik == "Uspravni cilindar":
        if precnik is None or not math.isfinite(precnik) or precnik <= 0:
            raise ValueError("Prečnik mora biti veći od nule.")
        kapacitet = math.pi * (precnik / 2) ** 2 * visina / 1000
    else:
        raise ValueError("Nepoznat oblik tanka.")
    # Prazna i puna posuda su dovoljne jer je presek ovih oblika stalan.
    return proveri_tacke([
        {"distance_cm": visina + razmak_senzora, "volume_l": 0.0},
        {"distance_cm": razmak_senzora, "volume_l": kapacitet},
    ])


# Spaja provereni naziv i tačke u podatke za slanje uređaju.
# Ključevi ostaju isti kao u USB porukama koje ESP32 očekuje.
def napravi_profil(naziv, tacke):
    return {"name": proveri_naziv(naziv), "points": proveri_tacke(tacke)}
