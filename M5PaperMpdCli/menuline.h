#include <vector>

using std::vector;

typedef struct menuline {
    uint16_t x;
    uint16_t y;
    const char* text;
} MENULINE;

typedef vector<MENULINE*> MenuLines;
