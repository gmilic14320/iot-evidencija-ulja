#include "hcsr04.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TRIG_PIN GPIO_NUM_4
#define ECHO_PIN GPIO_NUM_5
#define ECHO_VREME_CEKANJA_US 30000
#define BRZINA_ZVUKA_CM_PO_US 0.0343f

static bool pokrenut = false;

// Čeka promenu na ECHO pinu i prekida ako senzor ne odgovori.
static esp_err_t sacekaj_nivo_signala(int level, int64_t *vreme_us)
{
    int64_t pocetak = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) != level) {
        if (esp_timer_get_time() - pocetak >= ECHO_VREME_CEKANJA_US) {
            return ESP_ERR_TIMEOUT;
        }
    }
    if (vreme_us != NULL) {
        *vreme_us = esp_timer_get_time();
    }
    return ESP_OK;
}

// Podešava TRIG kao izlaz, a ECHO kao ulaz.
esp_err_t hcsr04_pokreni(void)
{
    pokrenut = false;
    gpio_config_t trig_podesavanja = {
        .pin_bit_mask = 1ULL << TRIG_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config_t echo_podesavanja = trig_podesavanja;
    echo_podesavanja.pin_bit_mask = 1ULL << ECHO_PIN;
    echo_podesavanja.mode = GPIO_MODE_INPUT;

    esp_err_t rezultat = gpio_config(&trig_podesavanja);
    if (rezultat != ESP_OK) return rezultat;
    rezultat = gpio_config(&echo_podesavanja);
    if (rezultat != ESP_OK) return rezultat;
    rezultat = gpio_set_level(TRIG_PIN, 0);
    if (rezultat != ESP_OK) return rezultat;
    pokrenut = true;
    return ESP_OK;
}

// Iz trajanja ECHO impulsa računa udaljenost u centimetrima.
esp_err_t hcsr04_ocitaj_udaljenost(float *udaljenost_cm)
{
    if (udaljenost_cm == NULL) return ESP_ERR_INVALID_ARG;
    if (!pokrenut) return ESP_ERR_INVALID_STATE;

    // Čeka završetak prethodnog ECHO impulsa.
    esp_err_t rezultat = sacekaj_nivo_signala(0, NULL);
    if (rezultat != ESP_OK) return rezultat;
    rezultat = gpio_set_level(TRIG_PIN, 0);
    if (rezultat != ESP_OK) return rezultat;
    esp_rom_delay_us(2);
    // Kratak TRIG impuls pokreće novo merenje.
    rezultat = gpio_set_level(TRIG_PIN, 1);
    if (rezultat != ESP_OK) return rezultat;
    esp_rom_delay_us(10);
    rezultat = gpio_set_level(TRIG_PIN, 0);
    if (rezultat != ESP_OK) return rezultat;

    int64_t pocetak_impulsa_us;
    int64_t kraj_impulsa_us;
    rezultat = sacekaj_nivo_signala(1, &pocetak_impulsa_us);
    if (rezultat != ESP_OK) return rezultat;
    rezultat = sacekaj_nivo_signala(0, &kraj_impulsa_us);
    if (rezultat != ESP_OK) return rezultat;

    // Zvuk prelazi put do površine i nazad.
    float izmerena_udaljenost = (kraj_impulsa_us - pocetak_impulsa_us) * BRZINA_ZVUKA_CM_PO_US / 2.0f;
    if (izmerena_udaljenost < 2.0f || izmerena_udaljenost > 400.0f) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    *udaljenost_cm = izmerena_udaljenost;
    return ESP_OK;
}

// Uzima tri merenja i vraća srednju vrednost po veličini.
esp_err_t hcsr04_ocitaj_stabilnu_udaljenost(float *udaljenost_cm)
{
    if (udaljenost_cm == NULL) return ESP_ERR_INVALID_ARG;
    float merenja[3];
    for (int i = 0; i < 3; i++) {
        esp_err_t rezultat = hcsr04_ocitaj_udaljenost(&merenja[i]);
        if (rezultat != ESP_OK) return rezultat;
        // Pauza razdvaja uzastopna merenja senzora.
        if (i < 2) vTaskDelay(pdMS_TO_TICKS(70));
    }
    for (int i = 0; i < 2; i++) {
        for (int j = i + 1; j < 3; j++) {
            if (merenja[j] < merenja[i]) {
                float privremena = merenja[i];
                merenja[i] = merenja[j];
                merenja[j] = privremena;
            }
        }
    }
    // Medijana ublažava uticaj jednog odstupajućeg očitavanja.
    *udaljenost_cm = merenja[1];
    return ESP_OK;
}
