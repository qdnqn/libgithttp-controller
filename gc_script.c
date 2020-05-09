#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include <bson/bson.h>
#include <mongoc/mongoc.h>

static mongoc_client_t *client;
static mongoc_collection_t *collection;
static bson_error_t error;
static bson_oid_t oid;
static bson_t *bson;

redisContext *redis;
redisReply *reply;	

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = reply;
    if (reply == NULL) return;

    if (r->type == REDIS_REPLY_ARRAY) {
        for (int j = 0; j < r->elements; j++) {
            //printf("%u) %s\n", j, r->element[j]->str);
            
            if(j == 2 && r->element[j]->len > 0){
							printf("(%u) Message(%d): %s\n", j, r->element[j]->len, r->element[j]->str);
							bson = bson_new_from_json((const uint8_t *)r->element[j]->str, r->element[j]->len, &error);
														
							if(!bson) {
								printf("BSON error: %s\n", error.message);
								break;
							}
							
							bson_oid_init (&oid, NULL);
							BSON_APPEND_OID (bson, "_id", &oid);
							
							bson_append_timestamp (bson,
                       "timestamp",
                       -1,
                       time(NULL),
                       0);
							
							if (!mongoc_collection_insert_one (collection, bson, NULL, NULL, &error)) {
								printf("MONGOC error: %s\n", error.message);
							} else {
								char *str;
								str = bson_as_canonical_extended_json (bson, NULL);
								reply = redisCommand(redis,"PUBLISH nodejs %s", str);
								printf("Saved this message to git-server/log!\n");
								
								bson_free(str);
							}

							bson_destroy(bson);
						}
        }
    }
}

int main (int argc, char **argv) {
		struct timeval timeout = { 1, 500000 }; // 1.5 seconds
		redis = redisConnectWithTimeout("localhost", 6379, timeout);
		
		printf("Listening for new messages on test.repo chanel!\n");
		mongoc_init();

    client = mongoc_client_new ("mongodb://localhost:27017/?appname=insert-example");
    collection = mongoc_client_get_collection (client, "git-server", "log");
    
    signal(SIGPIPE, SIG_IGN);
    struct event_base *base = event_base_new();

    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        printf("error: %s\n", c->errstr);
        return 1;
    }

    redisLibeventAttach(c, base);
    redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE test.repo");
    event_base_dispatch(base);
    
    mongoc_collection_destroy(collection);
    mongoc_client_destroy(client);
    mongoc_cleanup();
    
    return 0;
}
