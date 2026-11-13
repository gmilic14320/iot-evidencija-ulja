#ifndef PODESAVANJE_TANKA_H
#define PODESAVANJE_TANKA_H

#include <stdbool.h>
#include "esp_err.h"
#include "tank.h"

// Ograničenja za tabelu i prostor za naziv sa našim slovima.
#define NAJVISE_TACAKA 32
#define DUZINA_NAZIVA_TANKA 24

// Pokretanje memorije i serijske veze.
esp_err_t podesavanje_tanka_pokreni(void);
// Provera novih komandi bez čekanja na unos sa računara.
void podesavanje_tanka_obradi_komande(void);
// Stanje kalibracije i podaci trenutno izabranog tanka.
bool podesavanje_tanka_kalibracija_u_toku(void);
bool podesavanje_tanka_ima_tabelu(void);
const char *podesavanje_tanka_naziv(void);
const podesavanja_tanka_t *podesavanje_tanka_tabela(void);

#endif
