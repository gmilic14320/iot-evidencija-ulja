import queue
import threading
import time
import tkinter as tk
from tkinter import messagebox, ttk

from veza import Uredjaj, pronadji_portove
from podesavanja import NAJVISE_TACAKA, tacke_iz_dimenzija, napravi_profil, mililitri_u_litre, procitaj_broj


# Prozor za povezivanje uređaja i unos kalibracije.
class ProzorTanka:
    # Pravi polja za unos, tabelu tačaka i dugmad.
    def __init__(self, glavni_prozor):
        self.glavni_prozor = glavni_prozor
        self.glavni_prozor.title("Podešavanje tanka")
        self.glavni_prozor.geometry("650x590")
        self.glavni_prozor.minsize(620, 560)
        # Stanje veze i unosi iz prozora.
        self.uredjaj = None
        self.zauzet = False
        self.zatvoren = False
        self.menja_podesavanja = False
        self.odgovori = queue.Queue()
        self.tacke = []
        self.portovi = {}
        self.port = tk.StringVar()
        self.naziv = tk.StringVar()
        self.nacin_unosa = tk.StringVar(value="Ručna kalibracija")
        self.oblik = tk.StringVar(value="Pravougaoni")
        self.unos_kolicine_ml = tk.StringVar(value="0")
        self.unos_udaljenosti_cm = tk.StringVar()
        self.status = tk.StringVar(value="Uređaj nije povezan.")
        self.uputstvo = tk.StringVar()
        self.dimenzije = {kljuc: tk.StringVar() for kljuc in ("Visina", "Dužina", "Širina", "Prečnik", "Stranica", "Senzor iznad vrha")}

        # Izbor serijskog porta i povezivanje sa ESP32.
        glavni_okvir = ttk.Frame(glavni_prozor, padding=16)
        glavni_okvir.pack(fill="both", expand=True)
        ttk.Label(glavni_okvir, text="Kalibrišite tank", font=("TkDefaultFont", 18, "bold")).pack(anchor="w", pady=(0, 12))
        veza = ttk.Frame(glavni_okvir)
        veza.pack(fill="x")
        self.izbor_porta = ttk.Combobox(veza, textvariable=self.port, state="readonly")
        self.izbor_porta.pack(side="left", fill="x", expand=True)
        self.dugme_osvezi = ttk.Button(veza, text="Osveži", command=self.osvezi_portove)
        self.dugme_osvezi.pack(side="left", padx=6)
        self.dugme_povezi = ttk.Button(veza, text="Poveži", command=self.promeni_vezu)
        self.dugme_povezi.pack(side="left")
        ttk.Label(glavni_okvir, textvariable=self.status, wraplength=600).pack(anchor="w", pady=(10, 14))

        self.obrazac = ttk.Frame(glavni_okvir)
        self.obrazac.pack(fill="both", expand=True)
        red_naziva = ttk.Frame(self.obrazac)
        red_naziva.pack(fill="x")
        ttk.Label(red_naziva, text="Naziv tanka:").pack(side="left", padx=(0, 12))
        ttk.Entry(red_naziva, textvariable=self.naziv, width=24).pack(side="left")
        red_nacina = ttk.Frame(self.obrazac)
        red_nacina.pack(fill="x", pady=12)
        for nacin_unosa in ("Ručna kalibracija", "Poznate dimenzije"):
            ttk.Radiobutton(red_nacina, text=nacin_unosa, value=nacin_unosa, variable=self.nacin_unosa,
                            command=self.prikazi_nacin_unosa).pack(side="left", padx=(0, 20))

        # Ručni unos količine i udaljenosti pročitane sa LCD-a.
        self.rucni_unos = ttk.Frame(self.obrazac)
        ttk.Label(self.rucni_unos, textvariable=self.uputstvo, wraplength=600).pack(anchor="w", pady=(0, 10))
        polja_unosa = ttk.Frame(self.rucni_unos)
        polja_unosa.pack(fill="x")
        ttk.Label(polja_unosa, text="Ukupno (ml):").grid(row=0, column=0, sticky="w")
        ttk.Label(polja_unosa, text="Sa LCD-a (cm):").grid(row=0, column=1, sticky="w", padx=12)
        ttk.Entry(polja_unosa, textvariable=self.unos_kolicine_ml, width=15).grid(row=1, column=0, pady=4)
        self.polje_udaljenosti = ttk.Entry(polja_unosa, textvariable=self.unos_udaljenosti_cm, width=15)
        self.polje_udaljenosti.grid(row=1, column=1, padx=12, pady=4)
        ttk.Button(polja_unosa, text="Dodaj tačku", command=self.dodaj_tacku).grid(row=1, column=2)
        okvir_tabele = ttk.Frame(self.rucni_unos)
        okvir_tabele.pack(fill="both", expand=True, pady=8)
        self.tabela = ttk.Treeview(okvir_tabele, columns=("volume", "distance"), show="headings", height=5, selectmode="browse")
        self.tabela.heading("volume", text="Ukupno vode / ulja (ml)")
        self.tabela.heading("distance", text="Udaljenost sa LCD-a (cm)")
        for kolona in ("volume", "distance"):
            self.tabela.column(kolona, width=200, anchor="center")
        self.tabela.pack(side="left", fill="both", expand=True)
        klizac = ttk.Scrollbar(okvir_tabele, orient="vertical", command=self.tabela.yview)
        klizac.pack(side="right", fill="y")
        self.tabela.configure(yscrollcommand=klizac.set)
        ttk.Button(self.rucni_unos, text="Ukloni poslednju tačku", command=self.ukloni_poslednju_tacku).pack(anchor="w")

        # Unos dimenzija za posude pravilnog oblika.
        self.unos_dimenzija = ttk.Frame(self.obrazac)
        ttk.Label(self.unos_dimenzija, text="Oblik:").grid(row=0, column=0, sticky="w", pady=5)
        self.izbor_oblika = ttk.Combobox(self.unos_dimenzija, textvariable=self.oblik,
                                     values=("Pravougaoni", "Kocka", "Uspravni cilindar"), state="readonly", width=24)
        self.izbor_oblika.grid(row=0, column=1, sticky="w")
        self.izbor_oblika.bind("<<ComboboxSelected>>", lambda dogadjaj: self.prikazi_dimenzije())
        self.redovi_dimenzija = {}
        for red, (naziv_polja, promenljiva) in enumerate(self.dimenzije.items(), 1):
            naslov = ttk.Label(self.unos_dimenzija, text=naziv_polja + " (cm):")
            polje = ttk.Entry(self.unos_dimenzija, textvariable=promenljiva, width=18)
            naslov.grid(row=red, column=0, sticky="w", padx=(0, 12), pady=5)
            polje.grid(row=red, column=1, sticky="w")
            self.redovi_dimenzija[naziv_polja] = (naslov, polje)

        donji_okvir = ttk.Frame(glavni_okvir)
        donji_okvir.pack(fill="x", pady=(14, 0))
        self.dugme_nova_kalibracija = ttk.Button(donji_okvir, text="Nova kalibracija", command=self.zapocni_kalibraciju)
        self.dugme_nova_kalibracija.pack(side="left")
        self.dugme_sacuvaj = ttk.Button(donji_okvir, text="Sačuvaj u uređaj", command=self.sacuvaj)
        self.dugme_sacuvaj.pack(side="right")
        self.dugme_odustani = ttk.Button(donji_okvir, text="Odustani", command=self.odustani)
        self.dugme_odustani.pack(side="right", padx=8)
        self.glavni_prozor.protocol("WM_DELETE_WINDOW", self.zatvori)
        self.osvezi_portove()
        self.prikazi_nacin_unosa()
        self.osvezi_uputstvo()
        self.osvezi_dugmad()
        self.glavni_prozor.after(50, self.obradi_odgovore)

    # Osvežava listu priključenih uređaja.
    def osvezi_portove(self):
        self.portovi = {f"{podaci_porta.device} - {podaci_porta.description}": podaci_porta.device for podaci_porta in pronadji_portove()}
        self.izbor_porta.configure(values=list(self.portovi))
        if self.port.get() not in self.portovi:
            self.port.set(next(iter(self.portovi), ""))

    # Uključuje dugmad i unos prema stanju veze i kalibracije.
    def osvezi_dugmad(self):
        povezan = self.uredjaj is not None
        unos_dozvoljen = (not povezan or self.menja_podesavanja) and not self.zauzet
        def obidji_polja(roditelj):
            for element in roditelj.winfo_children():
                if isinstance(element, (ttk.Entry, ttk.Button, ttk.Radiobutton, ttk.Combobox)):
                    stanje = "readonly" if unos_dozvoljen and isinstance(element, ttk.Combobox) else "normal" if unos_dozvoljen else "disabled"
                    element.configure(state=stanje)
                obidji_polja(element)
        obidji_polja(self.obrazac)
        self.izbor_porta.configure(state="disabled" if povezan or self.zauzet else "readonly")
        self.dugme_osvezi.configure(state="disabled" if povezan or self.zauzet else "normal")
        self.dugme_povezi.configure(text="Prekini vezu" if povezan else "Poveži", state="disabled" if self.zauzet else "normal")
        self.dugme_nova_kalibracija.configure(state="disabled" if self.zauzet else "normal")
        self.dugme_sacuvaj.configure(state="normal" if povezan and self.menja_podesavanja and not self.zauzet else "disabled")
        self.dugme_odustani.configure(state="normal" if unos_dozvoljen else "disabled")

    # Serijska veza se obrađuje van glavne niti prozora.
    def pokreni_postupak(self, postupak, pri_uspehu, pri_gresci=None):
        if self.zauzet:
            return
        self.zauzet = True
        self.osvezi_dugmad()
        def obradi_u_pozadini():
            try:
                self.odgovori.put((pri_uspehu, pri_gresci, postupak(), None))
            except Exception as greska:
                self.odgovori.put((pri_uspehu, pri_gresci, None, greska))
        threading.Thread(target=obradi_u_pozadini, daemon=True).start()

    # Rezultati komandi se prikazuju u glavnoj niti.
    def obradi_odgovore(self):
        try:
            while True:
                obrada_odgovora, pri_gresci, rezultat, greska = self.odgovori.get_nowait()
                self.zauzet = False
                if greska:
                    if pri_gresci:
                        pri_gresci(greska)
                    else:
                        self.status.set("Postupak nije završen.")
                        messagebox.showerror("Podešavanje tanka", str(greska), parent=self.glavni_prozor)
                else:
                    obrada_odgovora(rezultat)
                if self.zatvoren:
                    return
                self.osvezi_dugmad()
        except queue.Empty:
            pass
        if not self.zatvoren:
            self.glavni_prozor.after(50, self.obradi_odgovore)

    # Otvara ili zatvara serijsku vezu sa izabranim uređajem.
    def promeni_vezu(self):
        if self.uredjaj:
            if self.menja_podesavanja:
                self.pokreni_postupak(lambda: self.uredjaj.posalji_komandu("cancel"), lambda rezultat: self.prekini_vezu())
            else:
                self.prekini_vezu()
            return
        port = self.portovi.get(self.port.get())
        if not port:
            messagebox.showinfo("Povezivanje", "Povežite uređaj i izaberite port.", parent=self.glavni_prozor)
            return
        self.status.set("Povezivanje...")
        def uspostavi_vezu():
            uredjaj = Uredjaj(port)
            try:
                # Sačeka pokretanje ploče pre prve komande.
                time.sleep(0.8)
                try:
                    rezultat = uredjaj.posalji_komandu("hello", vreme_cekanja=4)
                except TimeoutError:
                    rezultat = uredjaj.posalji_komandu("hello")
                if not rezultat["configured"]:
                    rezultat = uredjaj.posalji_komandu("begin")
                return uredjaj, rezultat
            except Exception:
                uredjaj.zatvori()
                raise
        def povezan(rezultat):
            self.uredjaj, stanje = rezultat
            self.menja_podesavanja = stanje["calibrating"]
            if not self.naziv.get().strip():
                self.naziv.set(stanje["name"])
            self.status.set("Kalibracija u toku." if self.menja_podesavanja else f"Tank: {stanje['name']}.")
        self.pokreni_postupak(uspostavi_vezu, povezan)

    # Zatvara port i vraća prozor u stanje bez uređaja.
    def prekini_vezu(self):
        if self.uredjaj:
            self.uredjaj.zatvori()
        self.uredjaj = None
        self.menja_podesavanja = False
        self.status.set("Uređaj nije povezan.")
        self.osvezi_dugmad()

    # Pokreće novu kalibraciju sa praznom tabelom.
    def zapocni_kalibraciju(self):
        def zapoceto(rezultat):
            self.menja_podesavanja = True
            self.tacke.clear()
            self.osvezi_tabelu()
            self.unos_kolicine_ml.set("0")
            self.unos_udaljenosti_cm.set("")
            self.status.set("Kalibracija u toku.")
        if self.uredjaj is None:
            zapoceto(None)
            self.status.set("Uređaj nije povezan.")
        else:
            self.pokreni_postupak(lambda: self.uredjaj.posalji_komandu("begin"), zapoceto)

    # Odustaje od trenutnog unosa i zadržava sačuvanu kalibraciju.
    def odustani(self):
        if self.uredjaj is None:
            self.tacke.clear()
            self.osvezi_tabelu()
            self.unos_kolicine_ml.set("0")
            self.unos_udaljenosti_cm.set("")
            self.menja_podesavanja = False
            self.status.set("Tačke su obrisane.")
            self.osvezi_dugmad()
            return
        def otkazano(stanje):
            self.menja_podesavanja = stanje["calibrating"]
            self.status.set("Vraćena su prethodna podešavanja." if stanje["configured"] else "Kalibrišite tank.")
        self.pokreni_postupak(lambda: self.uredjaj.posalji_komandu("cancel"), otkazano)

    # Prikazuje ručni unos ili polja za dimenzije.
    def prikazi_nacin_unosa(self):
        self.rucni_unos.pack_forget()
        self.unos_dimenzija.pack_forget()
        (self.rucni_unos if self.nacin_unosa.get() == "Ručna kalibracija" else self.unos_dimenzija).pack(fill="both", expand=True)
        self.prikazi_dimenzije()

    # Ostavlja samo dimenzije potrebne za izabrani oblik.
    def prikazi_dimenzije(self):
        vidljiva_polja = {
            "Pravougaoni": {"Visina", "Dužina", "Širina", "Senzor iznad vrha"},
            "Kocka": {"Stranica", "Senzor iznad vrha"},
            "Uspravni cilindar": {"Visina", "Prečnik", "Senzor iznad vrha"},
        }[self.oblik.get()]
        for naziv_polja, elementi in self.redovi_dimenzija.items():
            for element in elementi:
                if naziv_polja in vidljiva_polja:
                    element.grid()
                else:
                    element.grid_remove()

    # Označava da li se unosi prazna posuda ili sledeća tačka.
    def osvezi_uputstvo(self):
        self.uputstvo.set("Nulta tačka - prazna posuda" if not self.tacke else f"Referentna tačka {len(self.tacke) + 1}")

    # Osvežava tabelu i prikazuje unete količine u ml.
    def osvezi_tabelu(self):
        for stavka in self.tabela.get_children():
            self.tabela.delete(stavka)
        for tacka in self.tacke:
            self.tabela.insert("", "end", values=(f"{tacka['volume_l'] * 1000:.3f}".rstrip("0").rstrip(".").replace(".", ","), f"{tacka['distance_cm']:g}".replace(".", ",")))
        self.osvezi_uputstvo()

    # Proverava unos pre dodavanja nove tačke u tabelu.
    def dodaj_tacku(self):
        try:
            kolicina_l = mililitri_u_litre(self.unos_kolicine_ml.get())
            udaljenost_cm = procitaj_broj(self.unos_udaljenosti_cm.get(), "Udaljenost")
            if len(self.tacke) >= NAJVISE_TACAKA:
                raise ValueError("Najviše 32 tačke.")
            if not 2 <= udaljenost_cm <= 400 or kolicina_l < 0:
                raise ValueError("Udaljenost: 2-400 cm; količina: najmanje 0 ml.")
            if not self.tacke and kolicina_l != 0:
                raise ValueError("Prva tačka je prazna posuda: unesite 0 ml.")
            if self.tacke and (kolicina_l <= self.tacke[-1]["volume_l"] or udaljenost_cm >= self.tacke[-1]["distance_cm"]):
                raise ValueError("Količina mora rasti, a udaljenost opadati.")
            self.tacke.append({"volume_l": kolicina_l, "distance_cm": udaljenost_cm})
            self.osvezi_tabelu()
            self.unos_kolicine_ml.set("")
            self.unos_udaljenosti_cm.set("")
            self.polje_udaljenosti.focus_set()
        except ValueError as greska:
            messagebox.showerror("Proverite unos", str(greska), parent=self.glavni_prozor)

    # Uklanja poslednju tačku ako je pogrešno uneta.
    def ukloni_poslednju_tacku(self):
        if self.tacke:
            self.tacke.pop()
            self.osvezi_tabelu()
            if not self.tacke:
                self.unos_kolicine_ml.set("0")

    # Priprema naziv i tačke, pa ih šalje uređaju na čuvanje.
    def sacuvaj(self):
        if self.uredjaj is None:
            messagebox.showinfo("Pregled", "Za čuvanje prvo povezite ESP32.", parent=self.glavni_prozor)
            return
        try:
            if self.nacin_unosa.get() == "Ručna kalibracija":
                tacke = [dict(tacka) for tacka in self.tacke]
            else:
                podaci = self.dimenzije
                dimenzije_za_proracun = {"razmak_senzora": procitaj_broj(podaci["Senzor iznad vrha"].get(), "Senzor iznad vrha")}
                if self.oblik.get() == "Kocka":
                    dimenzije_za_proracun["stranica"] = procitaj_broj(podaci["Stranica"].get(), "Stranica")
                else:
                    dimenzije_za_proracun["visina"] = procitaj_broj(podaci["Visina"].get(), "Visina")
                    if self.oblik.get() == "Pravougaoni":
                        dimenzije_za_proracun.update(duzina=procitaj_broj(podaci["Dužina"].get(), "Dužina"), sirina=procitaj_broj(podaci["Širina"].get(), "Širina"))
                    else:
                        dimenzije_za_proracun["precnik"] = procitaj_broj(podaci["Prečnik"].get(), "Prečnik")
                tacke = tacke_iz_dimenzija(self.oblik.get(), **dimenzije_za_proracun)
            profil = napravi_profil(self.naziv.get(), tacke)
        except ValueError as greska:
            messagebox.showerror("Proverite unos", str(greska), parent=self.glavni_prozor)
            return
        def sacuvano(stanje):
            self.menja_podesavanja = False
            self.status.set(f"Podešavanja su sačuvana. Tank: {stanje['name']}.")
        self.status.set("Čuvanje u uređaj...")
        self.pokreni_postupak(lambda: self.uredjaj.posalji_komandu("save", **profil), sacuvano)

    # Pre zatvaranja odustaje od nezavršene kalibracije.
    def zatvori(self):
        if self.zauzet:
            return
        if self.uredjaj and self.menja_podesavanja:
            self.pokreni_postupak(lambda: self.uredjaj.posalji_komandu("cancel"), lambda rezultat: self.zavrsi_zatvaranje(), lambda greska: self.zavrsi_zatvaranje())
        else:
            self.zavrsi_zatvaranje()

    # Oslobađa port i zatvara prozor.
    def zavrsi_zatvaranje(self):
        self.zatvoren = True
        if self.uredjaj:
            self.uredjaj.zatvori()
            self.uredjaj = None
        self.glavni_prozor.destroy()


if __name__ == "__main__":
    glavni_prozor = tk.Tk()
    ProzorTanka(glavni_prozor)
    glavni_prozor.mainloop()
