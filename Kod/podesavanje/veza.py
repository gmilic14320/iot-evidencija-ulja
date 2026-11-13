import json
import threading
import time

import serial
from serial.tools import list_ports

# Ova oznaka odvaja poruke aplikacije od ostalog ispisa sa ploče.
OZNAKA_PORUKE = "@IOT "


# Pronalazi USB i COM portove koji se mogu izabrati u prozoru.
def pronadji_portove():
    portovi = [podaci_porta for podaci_porta in list_ports.comports() if podaci_porta.vid is not None or podaci_porta.device.startswith("COM")]
    # UART priključak ide prvi na listi.
    portovi.sort(key=lambda podaci_porta: (podaci_porta.vid == 0x303A, podaci_porta.device))
    return portovi


class Uredjaj:
    # Otvara port bez aktiviranja DTR i RTS signala pri povezivanju.
    def __init__(self, port):
        self.zakljucavanje = threading.Lock()
        self.brojac_poruka = 0
        self.serijska_veza = serial.Serial()
        self.serijska_veza.port = port
        self.serijska_veza.baudrate = 115200
        self.serijska_veza.timeout = 0.1
        self.serijska_veza.write_timeout = 2
        self.serijska_veza.dtr = False
        self.serijska_veza.rts = False
        self.serijska_veza.open()

    # Oslobađa port da ga može koristiti drugi program.
    def zatvori(self):
        self.serijska_veza.close()

    # Šalje jednu komandu i čeka odgovor sa istim brojem poruke.
    def posalji_komandu(self, komanda, vreme_cekanja=8, **podaci):
        # Komande idu redom da se odgovori ne bi pomešali.
        with self.zakljucavanje:
            self.brojac_poruka += 1
            broj_poruke = self.brojac_poruka
            poruka = {"id": broj_poruke, "cmd": komanda, **podaci}
            self.serijska_veza.reset_input_buffer()
            paket = OZNAKA_PORUKE + json.dumps(poruka, separators=(",", ":"), allow_nan=False) + "\n"
            self.serijska_veza.write(paket.encode("ascii"))
            krajnje_vreme = time.monotonic() + vreme_cekanja
            # Čuva pristigle delove dok se ne sastavi ceo red_poruke.
            primljeni_bajtovi = bytearray()
            while time.monotonic() < krajnje_vreme:
                primljeni_bajtovi.extend(self.serijska_veza.read(256))
                if len(primljeni_bajtovi) > 8192:
                    primljeni_bajtovi.clear()
                while b"\n" in primljeni_bajtovi:
                    primljeni_red, _, ostatak = primljeni_bajtovi.partition(b"\n")
                    primljeni_bajtovi = bytearray(ostatak)
                    red_poruke = primljeni_red.decode("utf-8", errors="replace").strip()
                    # Preskače običan ispis sa ploče i poruke koje nisu naš odgovor.
                    if not red_poruke.startswith(OZNAKA_PORUKE):
                        continue
                    try:
                        odgovor = json.loads(red_poruke[len(OZNAKA_PORUKE):])
                    except json.JSONDecodeError:
                        continue
                    if not isinstance(odgovor, dict) or odgovor.get("id") != broj_poruke:
                        continue
                    if not odgovor.get("ok"):
                        raise ValueError(odgovor.get("error", "Uređaj je odbio podešavanja."))
                    return odgovor
            raise TimeoutError("Uređaj ne odgovara. Proverite UART prikljucak, novu verziju programa i zatvorite serijski monitor.")
