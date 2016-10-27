#ifndef RVstructWord2_h
#define RVstructWord2_h

#include <WString.h>

//Datentyp Word definieren für Nutzung mit GFX-Library. Enthalten sind Koordinaten x+y und die Strich/Wortlänge l
struct word_struct {
    uint8_t x;
    uint8_t y;
    uint8_t l;
};
typedef struct word_struct Word; //Das Kind bekommt einen Namen
// Definition Datentyp ENDE

#endif


