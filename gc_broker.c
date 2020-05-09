#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "gc_config.h"

#if defined(GH_USEBROOKER)
#include "hiredis.h"
#include "async.h"
#include "adapters/libevent.h"

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include "gh_string.h"
#include "gc_broker.h"
#include "gc_database.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#endif

gc_broker* broker_init(gc_controller* gc){
	gc_broker* broker_temp = malloc(sizeof(gc_broker));
	
	broker_temp->channel = string_init();
	broker_temp->message = string_init();
		
	broker_temp->redis = gc->redis;
	
	return broker_temp;
}

void broker_clean(gc_broker* broker){
	string_clean(broker->channel);
	string_clean(broker->message);
	
	free(broker);
}

void broker_reply_clean(gc_broker* broker){
	freeReplyObject(broker->reply);
}

void broker_channel(gc_broker* broker, char* channel, ...){
	va_list args1, args2;
	va_start(args1, channel);
	va_copy(args2, args1);

	ssize_t len = vsnprintf(NULL, 0, channel, args1);								// Only number of chars without terminating char '\0'
	va_end(args1);
	
	string_free(broker->channel);
		
	if(broker->channel->size == 0){
		string_allocate(broker->channel, len+1);
		vsnprintf(broker->channel->str + broker->channel->size - len - 1, len+1, channel, args2);
	} else {
		string_reallocate(broker->channel, len);
		vsnprintf(broker->channel->str + broker->channel->size - len - 1, len+1, channel, args2);
	}
	
	va_end(args2);
}

void broker_message(gc_broker* broker, char* message, ...){
	va_list args1, args2;
	va_start(args1, message);
	va_copy(args2, args1);

	ssize_t len = vsnprintf(NULL, 0, message, args1);								// Only number of chars without terminating char '\0'
	va_end(args1);
	
	string_free(broker->message);
		
	if(broker->message->size == 0){
		string_allocate(broker->message, len+1);
		vsnprintf(broker->message->str + broker->message->size - len - 1, len+1, message, args2);
	} else {
		string_reallocate(broker->message, len);
		vsnprintf(broker->message->str + broker->message->size - len - 1, len+1, message, args2);
	}
	
	va_end(args2);
}

void broker_listen(redisAsyncContext *c, void *reply, void *privdata){
	gc_controller* gc = privdata;
	
	redisReply *r = reply;
	if (reply == NULL) return;

	if (r->type == REDIS_REPLY_ARRAY) {
		for (int j = 0; j < r->elements; j++) {
			if(j == 3 && r->element[j]->len > 0){
				broker_router(gc, r->element[j]);
			}
		}
	}
}

uint8_t broker_router(gc_controller* gc, redisReply* message){
	gc->bson = bson_new_from_json((const uint8_t *)message->str, message->len, &gc->error);
														
	if(!gc->bson) {
		printf("BSON error: %s\n", gc->error.message);
		return -1;
	}
	
	bson_oid_init (&gc->oid, NULL);
	BSON_APPEND_OID (gc->bson, "_id", &gc->oid);
	
	bson_append_timestamp (gc->bson,
												 "timestamp",
												 -1,
												 time(NULL),
												 0);
												 
	bson_iter_t iter;
	
	gc_broker* broker = broker_init(gc);
	
	g_str_t* repo = string_init();
	uint8_t found_repo = 0, action = 0;
	
	char *data;
	
	if (bson_iter_init (&iter, gc->bson)) {
		while (bson_iter_next (&iter)) {
			if(strcmp(bson_iter_key(&iter), "repo") == 0){
				string_append(repo, "%s", bson_iter_value(&iter)->value.v_utf8.str);
				string_append(repo, ".repo:trigger");
				
				found_repo = 1;
			}

			if(strcmp(bson_iter_key(&iter), "type") == 0){
				if(strcmp(bson_iter_value(&iter)->value.v_utf8.str, "child-commit") == 0){
					gc->collection = mongoc_client_get_collection (gc->client, "git-server", "log");
					
					if(mongoc_collection_insert_one (gc->collection, gc->bson, NULL, NULL, &gc->error)){
						data = bson_as_canonical_extended_json(gc->bson, NULL);
						broker->reply = redisCommand(broker->redis, "PUBLISH nodejs %s", data);
						broker_reply_clean(broker);						
						
						action = CHILD;
						break;
					}
				} else if(strcmp(bson_iter_value(&iter)->value.v_utf8.str, "push") == 0) {
					gc->collection = mongoc_client_get_collection (gc->client, "git-server", "log");
					
					if(mongoc_collection_insert_one (gc->collection, gc->bson, NULL, NULL, &gc->error)){
						data = bson_as_canonical_extended_json(gc->bson, NULL);
						broker->reply = redisCommand(broker->redis, "PUBLISH nodejs %s", data);
						broker_reply_clean(broker);
						
						action = PUSH;
						break;
					}
				} else if(strcmp(bson_iter_value(&iter)->value.v_utf8.str, "pull") == 0) {
					gc->collection = mongoc_client_get_collection (gc->client, "git-server", "log");
					
					if(mongoc_collection_insert_one (gc->collection, gc->bson, NULL, NULL, &gc->error)){
						data = bson_as_canonical_extended_json(gc->bson, NULL);
						broker->reply = redisCommand(broker->redis, "PUBLISH nodejs %s", data);
						broker_reply_clean(broker);
						
						action = PULL;
						break;
					}
				}
			}
		}
	}
	
	if(found_repo && action == PUSH){
		printf("Tryinng: %s\n", repo->str);
						
		broker->reply = redisCommand(broker->redis, "GET %s", repo->str);
		
		if (broker->reply->type != REDIS_REPLY_ERROR && broker->reply->type !=  REDIS_REPLY_NIL){
			lua_State* L;

			L = luaL_newstate();
			
			luaL_openlibs(L); 
			if (luaL_dostring(L, broker->reply->str))
        printf("cannot run configuration file: %s",lua_tostring(L, -1));
                 
			lua_getglobal(L, "trigger");
			lua_pushstring(L, data);			
			
			if(lua_pcall(L, 1, 1,0) != 0)
				printf("error running function `f': %s",lua_tostring(L, -1));
			
			lua_close(L);
			
			broker_reply_clean(broker);
		} 
	}
	
	string_clean(repo);
	broker_clean(broker);
	
	return 0;
}
