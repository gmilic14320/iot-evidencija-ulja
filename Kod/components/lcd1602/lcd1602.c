#include "lcd1602.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_SDA GPIO_NUM_6
#define LCD_SCL GPIO_NUM_7
#define LCD_RS 0x01
#define LCD_E  0x04
#define LCD_OSVETLJENJE 0x08

// PCF8574: P0=RS, P1=RW, P2=E, P3=svetlo, P4-P7=D4-D7.
static const char *OZNAKA = "LCD";
static i2c_master_bus_handle_t magistrala;
static i2c_master_dev_handle_t uredjaj;
static bool pokrenut;

// Oslobađa I2C vezu ako pokretanje displeja ne uspe.
static void oslobodi_magistralu(void)
{
    pokrenut = false;
    if (uredjaj != NULL) {
        i2c_master_bus_rm_device(uredjaj);
        uredjaj = NULL;
    }
    if (magistrala != NULL) {
        i2c_del_master_bus(magistrala);
        magistrala = NULL;
    }
}

// Šalje četiri bita LCD-u preko PCF8574 dodatka.
static esp_err_t posalji_cetiri_bita(uint8_t vrednost, bool podatak_za_ispis)
{
    uint8_t izlaz = ((vrednost & 0x0f) << 4) | LCD_OSVETLJENJE;
    if (podatak_za_ispis) izlaz |= LCD_RS;
    // RW ostaje na nuli pri upisu.
    const uint8_t niz_signala[] = {izlaz, izlaz | LCD_E, izlaz};
    return i2c_master_transmit(uredjaj, niz_signala, sizeof(niz_signala), 100);
}

// Šalje prvo gornja, pa donja četiri bita podatka ili komande.
static esp_err_t posalji_bajt(uint8_t vrednost, bool podatak_za_ispis)
{
    esp_err_t rezultat = posalji_cetiri_bita(vrednost >> 4, podatak_za_ispis);
    if (rezultat != ESP_OK) return rezultat;
    rezultat = posalji_cetiri_bita(vrednost & 0x0f, podatak_za_ispis);
    esp_rom_delay_us(!podatak_za_ispis && (vrednost == 0x01 || vrednost == 0x02) ? 3000 : 60);
    return rezultat;
}

// Pronalazi displej i podešava ga za rad sa dva reda.
esp_err_t lcd1602_pokreni(void)
{
    if (magistrala != NULL) return ESP_ERR_INVALID_STATE;
    const i2c_master_bus_config_t podesavanja = {
        .i2c_port = -1,
        .sda_io_num = LCD_SDA,
        .scl_io_num = LCD_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };
    esp_err_t rezultat = i2c_new_master_bus(&podesavanja, &magistrala);
    if (rezultat != ESP_OK) return rezultat;
    vTaskDelay(pdMS_TO_TICKS(50));

    // Adresa zavisi od verzije PCF8574 čipa.
    const uint8_t adrese[] = {0x27, 0x3f};
    uint8_t adresa = 0;
    for (size_t i = 0; i < sizeof(adrese); i++) {
        rezultat = i2c_master_probe(magistrala, adrese[i], 100);
        if (rezultat == ESP_OK) {
            adresa = adrese[i];
            break;
        }
        if (rezultat != ESP_ERR_NOT_FOUND) goto neuspeh;
    }
    if (adresa == 0) {
        ESP_LOGE(OZNAKA, "I2C dodatak nije pronadjen na 0x27 ili 0x3f.");
        rezultat = ESP_ERR_NOT_FOUND;
        goto neuspeh;
    }
    const i2c_device_config_t podesavanja_uredjaja = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = adresa,
        .scl_speed_hz = 100000
    };
    rezultat = i2c_master_bus_add_device(magistrala, &podesavanja_uredjaja, &uredjaj);
    if (rezultat != ESP_OK) goto neuspeh;
    ESP_LOGI(OZNAKA, "I2C dodatak: 0x%02x, SDA=6, SCL=7", adresa);

    const uint8_t izlaz = LCD_OSVETLJENJE;
    rezultat = i2c_master_transmit(uredjaj, &izlaz, 1, 100);
    if (rezultat != ESP_OK) goto neuspeh;
    // Prebacuje LCD u 4-bitni režim.
    const uint8_t cetvorobitne_komande[] = {0x03, 0x03, 0x03, 0x02};
    for (size_t i = 0; i < sizeof(cetvorobitne_komande); i++) {
        rezultat = posalji_cetiri_bita(cetvorobitne_komande[i], false);
        if (rezultat != ESP_OK) goto neuspeh;
        esp_rom_delay_us(i == 0 ? 4500 : 150);
    }
    // Podešava dva reda, briše ekran i uključuje prikaz bez kursora.
    const uint8_t komande[] = {0x28, 0x08, 0x01, 0x06, 0x0c};
    for (size_t i = 0; i < sizeof(komande); i++) {
        rezultat = posalji_bajt(komande[i], false);
        if (rezultat != ESP_OK) goto neuspeh;
    }
    pokrenut = true;
    return ESP_OK;

neuspeh:
    oslobodi_magistralu();
    return rezultat;
}

// Upisuje tekst u oba reda displeja.
esp_err_t lcd1602_upisi_redove(const char *prvi, const char *drugi)
{
    if (prvi == NULL || drugi == NULL) return ESP_ERR_INVALID_ARG;
    if (!pokrenut) return ESP_ERR_INVALID_STATE;
    const char *redovi[2] = {prvi, drugi};
    for (int red = 0; red < 2; red++) {
        esp_err_t rezultat = posalji_bajt(red == 0 ? 0x80 : 0xc0, false);
        if (rezultat != ESP_OK) return rezultat;
        size_t duzina = strnlen(redovi[red], 16);
        // Dopunjava red razmacima da stari znakovi ne ostanu na ekranu.
        for (int kolona = 0; kolona < 16; kolona++) {
            uint8_t znak = (size_t)kolona < duzina ? redovi[red][kolona] : ' ';
            rezultat = posalji_bajt(znak, true);
            if (rezultat != ESP_OK) return rezultat;
        }
    }
    return ESP_OK;
}
