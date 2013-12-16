#include <time.h>
#include <stdio.h>
#include <assert.h>
#include "wait/platform.h"

#include "wait/module.h"
#include "wait/callout.h"
#include "wait/slotwait.h"

static size_t _still_tick;
static slotcb _still_timers;

static size_t _micro_tick;
static size_t _micro_wheel;
static slotcb _micro_timers[50];

static size_t _macro_tick;
static size_t _macro_wheel;
static slotcb _macro_timers[60];

void callout_reset(struct waitcb *evt, size_t millisec)
{
	size_t wheel;
	size_t micro_wheel, macro_wheel;

	waitcb_cancel(evt);
	evt->wt_value = (millisec + tx_getticks());
   	micro_wheel = (evt->wt_value - _micro_tick) / 20;
   	macro_wheel = (evt->wt_value - _macro_tick) / 1000;

	if (micro_wheel == 0) {
		fprintf(stderr, "warn: too small timer not supported!\n");
		micro_wheel = 1;
	}

	if (micro_wheel < 50) {
		wheel = (_micro_wheel + micro_wheel) % 50;
		slot_record(&_micro_timers[wheel], evt);
		return;
	}

	if (macro_wheel < 60) {
		wheel = (_macro_wheel + macro_wheel) % 60;
		slot_record(&_macro_timers[wheel], evt);
		return;
	}

	slot_record(&_still_timers, evt);
	return;
}

void callout_invoke(void *upp)
{
	size_t tick;
	size_t wheel;
	struct waitcb *event, *evt_next;

	tick = tx_getticks();

	for ( ; ; ) {
		if ((int)(tick - _micro_tick - 20) < 0)
			break;
		_micro_tick += 20;
		_micro_wheel++;
		wheel = (_micro_wheel % 50);
		slot_wakeup(&_micro_timers[wheel]);
	}

	for ( ; ; ) {
		if ((int)(tick - _macro_tick - 1000) < 0)
			break;
		_macro_tick += 1000;
		_macro_wheel++;
		wheel = (_macro_wheel % 60);
		while (_macro_timers[wheel] != NULL) {
			event = _macro_timers[wheel];
			if ((int)(event->wt_value - tick) < 20) {
				waitcb_cancel(event);
				waitcb_switch(event);
			} else {
				waitcb_cancel(event);
				callout_reset(event, event->wt_value - tick);
			}
		}
	}

	if ((int)(tick - _still_tick - 60000) >= 0) {
	   	_still_tick = tick;
	   	event = _still_timers;
	   	while (event != NULL) {
		   	evt_next = event->wt_next;
		   	if ((int)(event->wt_value - tick) < 60000) {
			   	if ((int)(event->wt_value - tick) < 20) {
					waitcb_cancel(event);
					waitcb_switch(event);
			   	} else {
					waitcb_cancel(event);
				   	callout_reset(event, event->wt_value - tick);
			   	}
		   	}
		   	event = evt_next;
	   	}
	}
	
	return;
}

extern u_int tx_getticks(void)
{
#ifndef WIN32
    struct timespec ts; 
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#else
	LARGE_INTEGER now = {0};
	static LARGE_INTEGER bootup = {0};
	static LARGE_INTEGER frequency = {0};

	if (frequency.QuadPart == 0) {
		QueryPerformanceCounter(&bootup);
		QueryPerformanceFrequency(&frequency);
	}

	QueryPerformanceCounter(&now);
	return (now.QuadPart - bootup.QuadPart) * 1000LL / frequency.QuadPart;
#endif
}

static struct waitcb _time_waitcb;
static void module_init(void)
{
	size_t tick;
	tick = tx_getticks();
	_still_tick = tick;

	_micro_tick = tick;
	_micro_wheel = 0;

	_macro_tick = tick;
	_macro_wheel = 0;

	waitcb_init(&_time_waitcb, callout_invoke, 0);
	_time_waitcb.wt_flags &= ~WT_EXTERNAL;
	_time_waitcb.wt_flags |= WT_WAITSCAN;
	waitcb_switch(&_time_waitcb);
}

static void module_clean(void)
{
	waitcb_clean(&_time_waitcb);
}

struct module_stub timer_mod = {
	module_init, module_clean
};

