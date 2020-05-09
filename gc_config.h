#ifndef GH_CONFIG
#define GH_CONFIG

#define GH_USEBROOKER "redis"
#define GH_BROKERHOST	"localhost"
#define GH_BROKERPORT	6379
#define GH_MONGO 			"mongodb://localhost:27017/?appname=insert-example"

#include "hiredis.h"
#include "async.h"
#include <mongoc/mongoc.h>

#include "gh_string.h"

typedef struct GC_CONTROLLER {
	redisContext* redis;
	redisAsyncContext *redis_async;
	mongoc_client_t *client;
	mongoc_collection_t *collection;
	bson_error_t error;
  bson_oid_t oid;
  bson_t *bson;
} gc_controller;


gc_controller* gc_init();
void gc_clean(gc_controller* gc);


#endif
