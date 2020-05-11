#ifndef GH_BROKER
#define GH_BROKER

#include <inttypes.h>
#include <stdarg.h>

#include "gc_config.h"

#if defined(GH_USEBROOKER)
#include "hiredis.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#endif

#define PUSH 1
#define PULL 2
#define CHILD 3

typedef struct BROKER {
  g_str_t *channel;
  g_str_t *message;
  
  #if defined (GH_USEBROOKER)
    redisContext *redis;
    redisReply *reply;
  #endif
} gc_broker;

gc_broker* broker_init(gc_controller*);
void broker_clean(gc_broker* broker);

void broker_channel(gc_broker* broker, char* channel, ...);
void broker_message(gc_broker* broker, char* message, ...);
void broker_reply_clean(gc_broker* broker);
void broker_listen(redisAsyncContext *c, void *reply, void *privdata);
uint8_t broker_router(gc_controller* gc, redisReply* message);


#endif
