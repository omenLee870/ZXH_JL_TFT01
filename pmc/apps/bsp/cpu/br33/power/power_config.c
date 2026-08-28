#include "power_interface.h"

void lowpower_init();

void board_power_init()
{
    //message_init();

    power_init(NULL);

    lowpower_init();
}
