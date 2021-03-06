#include <stdio.h>
#include "wait/platform.h"

#include "wait/module.h"
#include "wait/slotwait.h"
#include "wait/callout.h"

static struct waitcb _timer;
static void flush_delack(void *up)
{
	callout_reset(&_timer, 200);
	printf("Hello World\n");
	return;
}

static void module_init(void)
{
	waitcb_init(&_timer, flush_delack, NULL);
	callout_reset(&_timer, 200);
	return;
}

static void module_clean(void)
{
	waitcb_clean(&_timer);
	return;
}

struct module_stub timer_test_mod = { 
	module_init, module_clean
};

extern struct module_stub timer_mod;
extern struct module_stub slotsock_mod;
struct module_stub *modules_list[] = { 
	&slotsock_mod, &timer_mod, &timer_test_mod, NULL
};

int main(int argc, char *argv[])
{
	slotwait_held(0);
	initialize_modules(modules_list);

	slotwait_start();
	for ( ; slotwait_step(); ) {
	}

	cleanup_modules(modules_list);
	return 0;
}

