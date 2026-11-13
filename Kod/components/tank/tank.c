#include "tank.h"
#include <math.h>
#include <stddef.h>

// Proverava opseg merenja i redosled tačaka kalibracije.
esp_err_t tank_proveri_tabelu(const podesavanja_tanka_t *podesavanja)
{
    if (podesavanja == NULL || podesavanja->tacke == NULL || podesavanja->broj_tacaka < 2) {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t i = 0; i < podesavanja->broj_tacaka; i++) {
        const tacka_kalibracije_t *tacka = &podesavanja->tacke[i];
        if (!isfinite(tacka->udaljenost_cm) || !isfinite(tacka->kolicina_l) ||
            tacka->udaljenost_cm < 2.0f || tacka->udaljenost_cm > 400.0f ||
            tacka->kolicina_l < 0.0f) {
            return ESP_ERR_INVALID_ARG;
        }
        if (i > 0 && (tacka->udaljenost_cm >= podesavanja->tacke[i - 1].udaljenost_cm ||
                      tacka->kolicina_l <= podesavanja->tacke[i - 1].kolicina_l)) {
            return ESP_ERR_INVALID_ARG;
        }
    }
    return ESP_OK;
}

// Pronalazi dve okolne tačke i između njih računa količinu.
esp_err_t tank_izracunaj_kolicinu(const podesavanja_tanka_t *podesavanja,
                                float udaljenost_cm, float *kolicina_l)
{
    if (kolicina_l == NULL || !isfinite(udaljenost_cm) ||
        tank_proveri_tabelu(podesavanja) != ESP_OK) {
        return ESP_ERR_INVALID_ARG;
    }
    const tacka_kalibracije_t *tacke = podesavanja->tacke;
    // Van kalibrisanog opsega nema pouzdanog proračuna.
    if (udaljenost_cm > tacke[0].udaljenost_cm ||
        udaljenost_cm < tacke[podesavanja->broj_tacaka - 1].udaljenost_cm) {
        return ESP_ERR_INVALID_SIZE;
    }
    for (size_t i = 1; i < podesavanja->broj_tacaka; i++) {
        if (udaljenost_cm >= tacke[i].udaljenost_cm) {
            const tacka_kalibracije_t *prethodna = &tacke[i - 1];
            // Udeo pređenog rastojanja prenosi se na razliku u litrima.
            float udeo = (prethodna->udaljenost_cm - udaljenost_cm) /
                          (prethodna->udaljenost_cm - tacke[i].udaljenost_cm);
            *kolicina_l = prethodna->kolicina_l +
                        udeo * (tacke[i].kolicina_l - prethodna->kolicina_l);
            return ESP_OK;
        }
    }
    return ESP_ERR_INVALID_SIZE;
}
