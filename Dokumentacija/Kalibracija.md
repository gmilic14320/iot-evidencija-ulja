# Podešavanje tanka

Izabrati UART port, povezati uređaj i uneti naziv tanka (do 11 znakova, latinicom). Za izmenu sačuvanih podataka izabrati Nova kalibracija. Senzor i posuda ostaju na istom mestu.

## Ručni unos

1. Za praznu posudu uneti 0 ml i udaljenost pročitanu sa LCD-a. Dodati tačku.
2. Dodavati poznate količine. Za svaku tačku uneti ukupne mililitre i novu udaljenost sa LCD-a.
3. Poslednja tačka predstavlja najviši izmereni nivo. Izabrati Sačuvaj u uređaj.

Dve tačke dovoljne su za jednak presek po visini. Za šolju ili činiju uneti više tačaka. Količina mora rasti, a udaljenost opadati. Podržane su 2-32 tačke i udaljenosti 2-400 cm. Van tabele se ne procenjuju litre.

Količina se unosi u ml, a LCD prikazuje litre sa tri decimale: 700 ml = 0,700 L; 25450 ml = 25,450 L.

## Poznate dimenzije

Izabrati pravougaoni tank, kocku ili uspravni cilindar. Dimenzije su unutrašnje, u cm. Uneti i rastojanje od lica senzora do unutrašnjeg vrha tanka. Pražnjenje i probno punjenje nisu potrebni.

Primer: dužina, širina i visina po 100 cm, senzor 10 cm iznad vrha. Prazno je na 110 cm, puno na 10 cm, kapacitet je 1000 L. Za kocku se unosi jedna stranica, a za cilindar prečnik i visina. Za druge oblike koristiti odgovarajuću tabelu.

Podešavanja se pamte u ESP32. Odustani vraća prethodno sačuvana. Ako se senzor pomeri, proveriti i obnoviti podešavanja. Broj decimala na LCD-u ne predstavlja garantovanu tačnost.

Bez povezanog uređaja moguće je pregledati polja i isprobati unos. Slanje zahteva vezu sa ESP32.
