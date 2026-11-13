# IoT sistem za merenje i evidenciju otpadnog jestivog ulja

Gavrilo Milic - 143/2020  
Vuk Neskovic - 67/2023

## Ideja projekta

Prvobitna namena projekta je merenje nivoa i količine otpadnog jestivog ulja u tankovima. Program računa zapreminu u litrima nezavisno od vrste tečnosti, pa se isti program i kalibraciona tabela mogu koristiti i za druge tečnosti.

Za demonstraciju koristimo vodu i manju posudu, dok je princip rada isti: merimo udaljenost do površine tečnosti i na osnovu podataka o posudi računamo njenu količinu.

Glavni deo projekta je upravo pretvaranje udaljenosti u centimetrima u količinu izraženu u litrima. Senzor sam po sebi ne meri litre, pa je za taj proračun potrebno povezati njegovo očitavanje sa zapreminom konkretne posude.

## Hardver

Sistem čine tri glavne komponente:

- **ESP32-S3 mikrokontroler** upravlja senzorom i ekranom, obrađuje merenja i računa količinu tečnosti. Naziv tanka i kalibracioni profil čuvaju se u neisparljivoj NVS memoriji, pa ostaju sačuvani i posle isključivanja napajanja.
- **Ultrazvučni senzor HC-SR04** postavljen je iznad posude i usmeren prema površini tečnosti. Meri udaljenost od senzora do površine. Kako se posuda puni, ta udaljenost se smanjuje.
- **LCD 1602A ekran sa I2C dodatkom** prikazuje podatke u dva reda. U prvom je naziv tanka ili status kalibracije, a u drugom udaljenost tokom kalibracije ili izračunata količina u litrima.

## Kalibracija i pretvaranje centimetara u litre

Pošto ista udaljenost ne predstavlja istu količinu u svakoj posudi, sistem prvo treba kalibrisati.

Preko male Python aplikacije na računaru unosimo naziv tanka i referentne tačke. Aplikacija komunicira sa ESP32 preko USB serijske veze, koristeći JSON poruke.

Za praznu posudu unosimo 0 ml i udaljenost pročitanu sa LCD-a. Zatim dodajemo poznate količine tečnosti i za svaku tačku unosimo ukupnu količinu u posudi i novu udaljenost. Tako dobijamo kalibracionu tabelu koja povezuje centimetre i litre.

Program između dve susedne tačke računa približnu količinu linearnom interpolacijom. Na primer, ako na 5,6 cm imamo 100 ml, a na 3,6 cm imamo 200 ml, očitavanje od 4,6 cm odgovara približno količini od 150 ml.

Za posude jednakog preseka dovoljne su dve tačke. Kod posuda koje menjaju širinu po visini unosimo više tačaka, kako bi proračun bolje pratio njihov oblik.

Ako znamo unutrašnje dimenzije posude, možemo uneti njih umesto probnog punjenja. Podržani su pravougaoni tank, kocka i uspravni cilindar, uz unos rastojanja senzora iznad vrha posude.

Količine unosimo u mililitrima, a na LCD-u ih prikazujemo u litrima sa tri decimale. Na primer, unos od 25450 ml prikazuje se kao **25,450 L**.

## Softverska logika

Program na ESP32 pisan je u C-u uz ESP-IDF. Prozor za podešavanje pisan je u Pythonu, pomoću Tkinter biblioteke.

Rad uređaja počinje u funkciji `app_main()`. Pre ulaska u glavnu petlju učitavaju se sačuvana podešavanja i pokreću senzor i ekran.

U petlji mikrokontroler proverava komande sa računara i očitava udaljenost. Senzor šalje ultrazvučni impuls, a iz vremena potrebnog da se signal vrati računa se udaljenost do površine. Funkcija `hcsr04_ocitaj_stabilnu_udaljenost()` uzima tri očitavanja i bira medijanu, da jedno odstupajuće merenje manje utiče na rezultat.

Tokom kalibracije na LCD-u se prikazuju centimetri koje korisnik prepisuje u aplikaciju. Kada se podešavanja sačuvaju, funkcija `tank_izracunaj_kolicinu()` koristi kalibracionu tabelu da iz očitane udaljenosti izračuna litre.

Dobijena količina se priprema za prikaz i ispisuje na LCD-u. Ako senzor ne odgovori ili je očitavanje van kalibrisanog opsega, uređaj prikazuje odgovarajuću poruku umesto količine.

Nakon čuvanja kalibracije mikrokontroler samostalno nastavlja merenje. Prozor na računaru može da se zatvori, a uređaj radi dok ima napajanje.

## Dalji razvoj

Sledeći deo projekta je povezivanje uređaja sa serverom, bazom podataka i veb-aplikacijom. Planirano je praćenje više tankova, istorije merenja i evidencije preuzetih i izdatih količina ulja.

[Podešavanje tanka](Dokumentacija/Kalibracija.md) · [Povezivanje komponenti](Sema/Povezivanje.md)
