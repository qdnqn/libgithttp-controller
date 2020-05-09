#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "gc_config.h"

#if defined(GH_USEBROOKER)
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"
#endif

#include "gc_broker.h"

int main (int argc, char **argv) {
	gc_controller* gc = gc_init();
		
	signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();
  
	redisLibeventAttach(gc->redis_async, base);
	redisAsyncCommand(gc->redis_async, broker_listen, gc, "PSUBSCRIBE *.repo");
	event_base_dispatch(base);
	
	return 0;	
}
