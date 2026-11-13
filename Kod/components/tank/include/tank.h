#ifndef TANK_H
#define TANK_H

#include "esp_err.h"
#include <stddef.h>

// Jedna tačka povezuje udaljenost senzora i poznatu količinu.
typedef struct {
    float udaljenost_cm;
    float kolicina_l;
} tacka_kalibracije_t;

// Tabela je poređana od prazne ka punoj posudi.
typedef struct {
    const tacka_kalibracije_t *tacke;
    size_t broj_tacaka;
} podesavanja_tanka_t;

// Provera ispravnosti tabele pre korišćenja.
esp_err_t tank_proveri_tabelu(const podesavanja_tanka_t *podesavanja);
// Interpolacija između dve susedne tačke kalibracije.
esp_err_t tank_izracunaj_kolicinu(const podesavanja_tanka_t *podesavanja,
                                float udaljenost_cm, float *kolicina_l);

#endif
