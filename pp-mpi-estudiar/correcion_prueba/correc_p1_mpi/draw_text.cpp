#include "draw_text.h"
#include "arial_ttf.h"

bool cargar_fuente_arial(sf::Font& fuente)
{
    return fuente.openFromMemory(arial_ttf::data, arial_ttf::data_len);
}
