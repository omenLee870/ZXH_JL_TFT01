#ifndef __PORT_H__
#define __PORT_H__

#define PORT_TABLE(arg)		u32 gpio_confi##arg[16]; \
							memset(gpio_confi##arg, 0, 16*4)

void port_protect(u32 *gpio_config, u32 gpio);
#define PORT_PROTECT(gpio)	port_protect(gpio_config, gpio)

void init_boot_rom();

void set_boot_rom(u8 val);
u8 get_boot_rom();

void gpio_close(enum gpio_port port, u32 val);

void *port_init(u32 arg);

#endif
