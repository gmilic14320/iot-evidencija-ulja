#ifndef HCSR04_H
#define HCSR04_H

#include "esp_err.h"

// Priprema pinove senzora.
esp_err_t hcsr04_pokreni(void);
// Jedno očitavanje udaljenosti u cm.
esp_err_t hcsr04_ocitaj_udaljenost(float *udaljenost_cm);
// Vraća medijanu tri merenja.
esp_err_t hcsr04_ocitaj_stabilnu_udaljenost(float *udaljenost_cm);

#endif
