#include <math.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hcsr04.h"
#include "lcd1602.h"
#include "tank.h"
#include "podesavanje_tanka.h"
#include "podesavanja_projekta.h"
#include "prikaz_kolicine.h"

static const char *OZNAKA = "MAIN";

// Pokreće komponente i zatim ponavlja merenje i prikaz.
void app_main(void)
{
    // Učitava sačuvani tank i priprema senzor i displej.
    ESP_ERROR_CHECK(podesavanje_tanka_pokreni());
    ESP_ERROR_CHECK(hcsr04_pokreni());
    bool lcd_spreman = false;
    if (LCD_UKLJUCEN) {
        esp_err_t rezultat = lcd1602_pokreni();
        lcd_spreman = rezultat == ESP_OK;
        if (!lcd_spreman) ESP_LOGE(OZNAKA, "LCD: %s", esp_err_to_name(rezultat));
    }

    while (1) {
        // Obrađuje komande koje stižu iz prozora za kalibraciju.
        podesavanje_tanka_obradi_komande();
        bool kalibracija_u_toku = podesavanje_tanka_kalibracija_u_toku() || !podesavanje_tanka_ima_tabelu();
        char prvi_red[17];
        char drugi_red[17];
        if (kalibracija_u_toku) snprintf(prvi_red, sizeof(prvi_red), "Kalibrisi tank");
        else snprintf(prvi_red, sizeof(prvi_red), "Tank %.11s", podesavanje_tanka_naziv());

        float udaljenost_cm;
        esp_err_t rezultat = hcsr04_ocitaj_stabilnu_udaljenost(&udaljenost_cm);
        if (rezultat != ESP_OK) {
            snprintf(drugi_red, sizeof(drugi_red), "Greska senzora");
        } else {
            // Isto zaokruživanje kao pri unosu sa LCD-a.
            udaljenost_cm = roundf(udaljenost_cm * 10.0f) / 10.0f;
            // Tokom kalibracije prikazuje cm, a posle nje izračunate litre.
            if (kalibracija_u_toku) {
                snprintf(drugi_red, sizeof(drugi_red), "Dist:%7.1f cm", (double)udaljenost_cm);
            } else {
                float kolicina_l;
                rezultat = tank_izracunaj_kolicinu(podesavanje_tanka_tabela(), udaljenost_cm, &kolicina_l);
                if (rezultat != ESP_OK) {
                    snprintf(drugi_red, sizeof(drugi_red), "Van opsega");
                } else {
                    prikazi_kolicinu(kolicina_l, drugi_red);
                }
            }
        }
        // Upisuje oba reda i pravi pauzu do sledećeg merenja.
        if (lcd_spreman) {
            rezultat = lcd1602_upisi_redove(prvi_red, drugi_red);
            if (rezultat != ESP_OK) ESP_LOGE(OZNAKA, "Upis na LCD: %s", esp_err_to_name(rezultat));
        }
        vTaskDelay(pdMS_TO_TICKS(PAUZA_MERENJA_MS));
    }
}
