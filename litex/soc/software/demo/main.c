// This file is Copyright (c) 2020 Florent Kermarrec <florent@enjoy-digital.fr>
// License: BSD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <console.h>
#include <generated/csr.h>

/*-----------------------------------------------------------------------*/
/* Uart                                                                  */
/*-----------------------------------------------------------------------*/

static char *readstr(void)
{
	char c[2];
	static char s[64];
	static int ptr = 0;

	if(readchar_nonblock()) {
		c[0] = readchar();
		c[1] = 0;
		switch(c[0]) {
			case 0x7f:
			case 0x08:
				if(ptr > 0) {
					ptr--;
					putsnonl("\x08 \x08");
				}
				break;
			case 0x07:
				break;
			case '\r':
			case '\n':
				s[ptr] = 0x00;
				putsnonl("\n");
				ptr = 0;
				return s;
			default:
				if(ptr >= (sizeof(s) - 1))
					break;
				putsnonl(c);
				s[ptr] = c[0];
				ptr++;
				break;
		}
	}

	return NULL;
}

static char *get_token(char **str)
{
	char *c, *d;

	c = (char *)strchr(*str, ' ');
	if(c == NULL) {
		d = *str;
		*str = *str+strlen(*str);
		return d;
	}
	*c = 0;
	d = *str;
	*str = c+1;
	return d;
}

static void prompt(void)
{
	printf("\e[92;1mlitex-demo-app\e[0m> ");
}

/*-----------------------------------------------------------------------*/
/* Help                                                                  */
/*-----------------------------------------------------------------------*/

static void help(void)
{
	puts("\nLiteX minimal demo app built "__DATE__" "__TIME__"\n");
	puts("Available commands:");
	puts("help               - Show this command");
	puts("reboot             - Reboot CPU");
#ifdef CSR_LEDS_BASE
	puts("led                - Led demo");
#endif
	puts("donut              - Spinning Donut demo");
	puts("ident              - Identifier of the system");
	puts("dwa                - DAC write a (+1)");
	puts("dwb                - DAC write b (x2)");
	puts("dwcw               - DAC write cw (toggles)");
	puts("dr                 - DAC read all");
}

/*-----------------------------------------------------------------------*/
/* Commands                                                              */
/*-----------------------------------------------------------------------*/

static void reboot_cmd(void)
{
	ctrl_reset_write(1);
}

#ifdef CSR_LEDS_BASE
static void led_cmd(void)
{
	int i;
	printf("Led demo...\n");

	printf("Counter mode...\n");
	for(i=0; i<32; i++) {
		leds_out_write(i);
		busy_wait(100);
	}

	printf("Shift mode...\n");
	for(i=0; i<4; i++) {
		leds_out_write(1<<i);
		busy_wait(200);
	}
	for(i=0; i<4; i++) {
		leds_out_write(1<<(3-i));
		busy_wait(200);
	}

	printf("Dance mode...\n");
	for(i=0; i<4; i++) {
		leds_out_write(0x55);
		busy_wait(200);
		leds_out_write(0xaa);
		busy_wait(200);
	}
}
#endif

extern void donut(void);

static void donut_cmd(void)
{
	printf("Donut demo...\n");
	donut();
}

/**
 * Command "ident"
 *
 * Identifier of the system
 *
 */
#define IDENT_SIZE 256
static void ident_cmd()
{
	char buffer[IDENT_SIZE];

	get_ident(buffer);
	printf("Ident: %s\n", *buffer ? buffer : "-");
	printf("NICK ER SEJ\n");
}
static void dac_read_all()
{
    uint32_t dac_data_reg  = dac_data_read();
    uint32_t dac_data_a_reg  = dac_data_a_read();
    uint32_t dac_data_b_reg  = dac_data_b_read();
    uint32_t dac_cw_reg  = dac_cw_read();
    printf("Data:\t\t0x%08x\n", dac_data_reg);
    printf("Data A:\t\t0x%08x\n", dac_data_a_reg);
    printf("Data B:\t\t0x%08x\n", dac_data_b_reg);
    printf("Data CW:\t0x%08x\n", dac_cw_reg);

}

static void dac_write_a()
{
    uint32_t a = dac_data_a_read();
    dac_data_a_write(a+1);
    dac_read_all();
}

static void dac_write_b()
{
    uint32_t a = dac_data_a_read();
    dac_data_b_write(a<<1);
    dac_read_all();
}

static void dac_write_cw()
{
    uint32_t a = dac_cw_read();
    dac_cw_write(~a);
    dac_read_all();
}


/*-----------------------------------------------------------------------*/
/* Console service / Main                                                */
/*-----------------------------------------------------------------------*/

static void console_service(void)
{
	char *str;
	char *token;

	str = readstr();
	if(str == NULL) return;
	token = get_token(&str);
	if(strcmp(token, "help") == 0)
		help();
	else if(strcmp(token, "reboot") == 0)
		reboot_cmd();
#ifdef CSR_LEDS_BASE
	else if(strcmp(token, "led") == 0)
		led_cmd();
#endif
	else if(strcmp(token, "donut") == 0)
		donut_cmd();
	else if(strcmp(token, "ident") == 0)
		ident_cmd();
    else if(strcmp(token, "dwa") == 0)
		dac_write_a();
	else if(strcmp(token, "dwb") == 0)
		dac_write_b();
    else if(strcmp(token, "dwcw") == 0)
		dac_write_cw();
	else if(strcmp(token, "dr") == 0)
		dac_read_all();
	prompt();
}


int main(void)
{
#ifdef CONFIG_CPU_HAS_INTERRUPT
	irq_setmask(0);
	irq_setie(1);
#endif
	uart_init();

	help();
	prompt();

	while(1) {
		console_service();
	}

	return 0;
}
