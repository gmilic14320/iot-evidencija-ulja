#ifndef LCD1602_H
#define LCD1602_H

#include "esp_err.h"

// Pokreće I2C vezu i priprema LCD za ispis.
esp_err_t lcd1602_pokreni(void);
// Upisuje dva reda od po 16 znakova.
esp_err_t lcd1602_upisi_redove(const char *prvi, const char *drugi);

#endif
