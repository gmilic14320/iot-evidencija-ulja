#include "prikaz_kolicine.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// Priprema količinu u litrima za jedan red LCD-a.
void prikazi_kolicinu(float kolicina_l, char red_teksta[17])
{
    if (!isfinite(kolicina_l) || kolicina_l < 0) {
        snprintf(red_teksta, 17, "Greska kolicine");
        return;
    }
    if (kolicina_l > 999999.9f) {
        snprintf(red_teksta, 17, "Kolicina prevel.");
        return;
    }
    int duzina = snprintf(red_teksta, 17, "Ulje: %.3f L", (double)kolicina_l);
    // Za veće količine izostavlja naziv da broj stane u 16 znakova.
    if (duzina >= 17) snprintf(red_teksta, 17, "%.3f L", (double)kolicina_l);
    // Na displeju koristi decimalni zarez.
    char *decimalni_znak = strchr(red_teksta, '.');
    if (decimalni_znak != NULL) *decimalni_znak = ',';
}
