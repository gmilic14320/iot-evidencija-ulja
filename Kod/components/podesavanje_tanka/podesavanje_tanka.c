#include "podesavanje_tanka.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "driver/uart.h"
#include "nvs.h"
#include "nvs_flash.h"

#define VERZIJA_PROFILA 2
#define VELICINA_REDA 4096
#define OZNAKA_PORUKE "@IOT "

// Naziv i tabela koji se zajedno čuvaju u memoriji uređaja.
typedef struct {
    uint32_t verzija;
    char naziv[DUZINA_NAZIVA_TANKA];
    uint32_t broj;
    tacka_kalibracije_t tacke[NAJVISE_TACAKA];
} profil_tanka_t;

static profil_tanka_t profil;
static podesavanja_tanka_t podesavanja;
static char naziv_za_lcd[12];
static bool podesen;
static bool kalibracija_u_toku;
static nvs_handle_t memorija;
static char red_teksta[VELICINA_REDA];
static size_t duzina_reda;
static bool red_predugacak;

// Prilagođava slova naziva za LCD.
static size_t procitaj_slovo_naziva(const unsigned char *tekst, char *znak_za_lcd)
{
    if (*tekst >= 32 && *tekst <= 126) {
        *znak_za_lcd = (char)*tekst;
        return 1;
    }
    static const struct { unsigned char prvi, drugi; char zamena; } slova[] = {
        {0xc4, 0x8c, 'C'}, {0xc4, 0x8d, 'c'}, {0xc4, 0x86, 'C'}, {0xc4, 0x87, 'c'},
        {0xc5, 0xa0, 'S'}, {0xc5, 0xa1, 's'}, {0xc4, 0x90, 'D'}, {0xc4, 0x91, 'd'},
        {0xc5, 0xbd, 'Z'}, {0xc5, 0xbe, 'z'}
    };
    for (size_t i = 0; i < sizeof(slova) / sizeof(slova[0]); i++) {
        if (tekst[0] == slova[i].prvi && tekst[1] == slova[i].drugi) {
            *znak_za_lcd = slova[i].zamena;
            return 2;
        }
    }
    return 0;
}

// Proverava dozvoljena slova i dužinu naziva tanka.
static bool ispravan_naziv(const char *naziv)
{
    if (naziv == NULL || naziv[0] == '\0' || strlen(naziv) >= DUZINA_NAZIVA_TANKA) return false;
    bool ima_znak = false;
    size_t broj = 0;
    const unsigned char *tekst = (const unsigned char *)naziv;
    while (*tekst) {
        char znak;
        size_t broj_bajtova = procitaj_slovo_naziva(tekst, &znak);
        if (broj_bajtova == 0 || ++broj > 11) return false;
        if (znak != ' ') ima_znak = true;
        tekst += broj_bajtova;
    }
    return ima_znak;
}

// Proverava verziju, naziv i tačke učitanog profila.
static esp_err_t proveri_profil(const profil_tanka_t *vrednost)
{
    if (vrednost->verzija != VERZIJA_PROFILA || vrednost->broj > NAJVISE_TACAKA ||
        memchr(vrednost->naziv, '\0', DUZINA_NAZIVA_TANKA) == NULL || !ispravan_naziv(vrednost->naziv)) {
        return ESP_ERR_INVALID_ARG;
    }
    const podesavanja_tanka_t nova_podesavanja = {.tacke = vrednost->tacke, .broj_tacaka = vrednost->broj};
    return tank_proveri_tabelu(&nova_podesavanja);
}

// Postavlja sačuvanu tabelu za merenje i priprema naziv za LCD.
static void primeni_profil(void)
{
    const unsigned char *tekst = (const unsigned char *)profil.naziv;
    size_t broj = 0;
    while (*tekst && broj < sizeof(naziv_za_lcd) - 1) {
        char znak;
        size_t broj_bajtova = procitaj_slovo_naziva(tekst, &znak);
        if (broj_bajtova == 0) break;
        naziv_za_lcd[broj++] = znak;
        tekst += broj_bajtova;
    }
    naziv_za_lcd[broj] = '\0';
    podesavanja.tacke = profil.tacke;
    podesavanja.broj_tacaka = profil.broj;
    podesen = true;
}

// Šalje prozoru ishod komande i trenutno stanje kalibracije.
static void posalji_odgovor(int broj_poruke, const char *greska)
{
    cJSON *odgovor = cJSON_CreateObject();
    if (odgovor == NULL) return;
    cJSON_AddNumberToObject(odgovor, "id", broj_poruke);
    cJSON_AddBoolToObject(odgovor, "ok", greska == NULL);
    cJSON_AddBoolToObject(odgovor, "configured", podesen);
    cJSON_AddBoolToObject(odgovor, "calibrating", kalibracija_u_toku);
    cJSON_AddStringToObject(odgovor, "name", podesen ? profil.naziv : "");
    if (greska != NULL) cJSON_AddStringToObject(odgovor, "error", greska);
    char *tekst = cJSON_PrintUnformatted(odgovor);
    if (tekst != NULL) {
        printf(OZNAKA_PORUKE "%s\n", tekst);
        fflush(stdout);
        cJSON_free(tekst);
    }
    cJSON_Delete(odgovor);
}

// Čita predložena podešavanja iz JSON poruke i proverava ih.
static bool procitaj_nova_podesavanja(cJSON *zahtev, profil_tanka_t *nova_podesavanja)
{
    cJSON *naziv = cJSON_GetObjectItemCaseSensitive(zahtev, "name");
    cJSON *tacke = cJSON_GetObjectItemCaseSensitive(zahtev, "points");
    if (!cJSON_IsString(naziv) || !ispravan_naziv(naziv->valuestring) || !cJSON_IsArray(tacke)) return false;
    int broj = cJSON_GetArraySize(tacke);
    if (broj < 2 || broj > NAJVISE_TACAKA) return false;
    memset(nova_podesavanja, 0, sizeof(*nova_podesavanja));
    nova_podesavanja->verzija = VERZIJA_PROFILA;
    strcpy(nova_podesavanja->naziv, naziv->valuestring);
    nova_podesavanja->broj = broj;
    for (int i = 0; i < broj; i++) {
        cJSON *tacka = cJSON_GetArrayItem(tacke, i);
        cJSON *udaljenost = cJSON_GetObjectItemCaseSensitive(tacka, "distance_cm");
        cJSON *kolicina = cJSON_GetObjectItemCaseSensitive(tacka, "volume_l");
        if (!cJSON_IsNumber(udaljenost) || !cJSON_IsNumber(kolicina)) return false;
        nova_podesavanja->tacke[i].udaljenost_cm = (float)udaljenost->valuedouble;
        nova_podesavanja->tacke[i].kolicina_l = (float)kolicina->valuedouble;
    }
    return proveri_profil(nova_podesavanja) == ESP_OK;
}

// Prepoznaje komandu za povezivanje, kalibraciju, odustajanje ili čuvanje.
static void obradi_poruku(void)
{
    if (strncmp(red_teksta, OZNAKA_PORUKE, strlen(OZNAKA_PORUKE)) != 0) return;
    cJSON *zahtev = cJSON_Parse(red_teksta + strlen(OZNAKA_PORUKE));
    if (!cJSON_IsObject(zahtev)) {
        posalji_odgovor(0, "Neispravna poruka.");
        cJSON_Delete(zahtev);
        return;
    }
    cJSON *polje_id = cJSON_GetObjectItemCaseSensitive(zahtev, "id");
    int broj_poruke = cJSON_IsNumber(polje_id) ? polje_id->valueint : 0;
    cJSON *komanda = cJSON_GetObjectItemCaseSensitive(zahtev, "cmd");
    if (!cJSON_IsString(komanda)) {
        posalji_odgovor(broj_poruke, "Nedostaje komanda.");
    } else if (strcmp(komanda->valuestring, "hello") == 0) {
        posalji_odgovor(broj_poruke, NULL);
    } else if (strcmp(komanda->valuestring, "begin") == 0) {
        // Stara tabela ostaje do potvrde nove.
        kalibracija_u_toku = true;
        posalji_odgovor(broj_poruke, NULL);
    } else if (strcmp(komanda->valuestring, "cancel") == 0) {
        kalibracija_u_toku = !podesen;
        posalji_odgovor(broj_poruke, NULL);
    } else if (strcmp(komanda->valuestring, "save") == 0) {
        profil_tanka_t nova_podesavanja;
        if (!kalibracija_u_toku) {
            posalji_odgovor(broj_poruke, "Prvo pokrenite novu kalibraciju.");
        } else if (!procitaj_nova_podesavanja(zahtev, &nova_podesavanja)) {
            posalji_odgovor(broj_poruke, "Proverite naziv, količine i udaljenosti (2-400 cm).");
        } else {
            // Nova tabela postaje aktivna tek kada se uspešno sačuva u NVS.
            esp_err_t rezultat = nvs_set_blob(memorija, "profile", &nova_podesavanja, sizeof(nova_podesavanja));
            if (rezultat == ESP_OK) rezultat = nvs_commit(memorija);
            if (rezultat != ESP_OK) {
                posalji_odgovor(broj_poruke, "Podešavanja nisu sačuvana. Pokušajte ponovo.");
            } else {
                profil = nova_podesavanja;
                primeni_profil();
                kalibracija_u_toku = false;
                posalji_odgovor(broj_poruke, NULL);
            }
        }
    } else {
        posalji_odgovor(broj_poruke, "Nepoznata komanda.");
    }
    cJSON_Delete(zahtev);
}

// Učitava prethodnu kalibraciju i pokreće UART vezu sa računarom.
esp_err_t podesavanje_tanka_pokreni(void)
{
    esp_err_t rezultat = nvs_flash_init();
    if (rezultat != ESP_OK) return rezultat;
    rezultat = nvs_open("oil_tank", NVS_READWRITE, &memorija);
    if (rezultat != ESP_OK) return rezultat;
    size_t velicina = sizeof(profil);
    rezultat = nvs_get_blob(memorija, "profile", &profil, &velicina);
    if (rezultat == ESP_OK && velicina == sizeof(profil) && proveri_profil(&profil) == ESP_OK) {
        primeni_profil();
    }
    kalibracija_u_toku = !podesen;

    const uart_config_t uart_podesavanja = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };
    rezultat = uart_param_config(UART_NUM_0, &uart_podesavanja);
    if (rezultat != ESP_OK) return rezultat;
    if (!uart_is_driver_installed(UART_NUM_0)) {
        rezultat = uart_driver_install(UART_NUM_0, VELICINA_REDA * 2, 0, 0, NULL, 0);
        if (rezultat != ESP_OK) return rezultat;
    }
    return ESP_OK;
}

// Sastavlja pristigle znakove u redove i obrađuje završene poruke.
void podesavanje_tanka_obradi_komande(void)
{
    uint8_t bajtovi[256];
    int broj;
    // Čita komande poslate preko USB-a.
    while ((broj = uart_read_bytes(UART_NUM_0, bajtovi, sizeof(bajtovi), 0)) > 0) {
        for (int i = 0; i < broj; i++) {
            if (bajtovi[i] == '\n') {
                // Predugačku poruku preskače do sledećeg novog reda.
                if (!red_predugacak) {
                    red_teksta[duzina_reda] = '\0';
                    obradi_poruku();
                }
                duzina_reda = 0;
                red_predugacak = false;
            } else if (bajtovi[i] != '\r') {
                if (duzina_reda + 1 < VELICINA_REDA && !red_predugacak) red_teksta[duzina_reda++] = bajtovi[i];
                else red_predugacak = true;
            }
        }
    }
}

// Podaci koje glavna petlja koristi za izbor prikaza i računanje količine.
bool podesavanje_tanka_kalibracija_u_toku(void) { return kalibracija_u_toku; }
bool podesavanje_tanka_ima_tabelu(void) { return podesen; }
const char *podesavanje_tanka_naziv(void) { return podesen ? naziv_za_lcd : ""; }
const podesavanja_tanka_t *podesavanje_tanka_tabela(void) { return podesen ? &podesavanja : NULL; }
