#include "gc_config.h"

gc_controller* gc_init(){
		gc_controller* gc_temp = malloc(sizeof(gc_controller));
		
		struct timeval timeout = { 1, 500000 };
		
		gc_temp->redis = redisConnectWithTimeout(GH_BROKERHOST, GH_BROKERPORT, timeout);
		gc_temp->redis_async = redisAsyncConnect(GH_BROKERHOST, GH_BROKERPORT);
				
		mongoc_init();

    gc_temp->client = mongoc_client_new(GH_MONGO);
    gc_temp->collection = NULL;
    gc_temp->bson = NULL;
    
		return gc_temp;
}

void gc_clean(gc_controller* gc){
	free(gc);
}
