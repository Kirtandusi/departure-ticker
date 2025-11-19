#include "../include/parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/proto/gtfs-realtime.pb-c.h"
#include <time.h>

DepartureList *parse_json_to_list(char *json, char *stop) {
    
    DepartureList *list = malloc(sizeof(DepartureList));
    if (!list) return NULL;

    list->count = 0;
    list->items = malloc(sizeof(BusDeparture) * 1); // placeholder
    if (!list->items) {
        free(list);
        return NULL;
    }

    return list;
}

DepartureList *parse_pb_to_list(char *pb, char *stop) {
    if (!pb) {
        fprintf(stderr, "parse_pb_to_list: NULL pb buffer\n");
        return NULL;
    }
    if (!stop) {
        fprintf(stderr, "parse_pb_to_list: NULL stop id\n");
        return NULL;
    }

    extern size_t global_pb_size;
    size_t pb_size = global_pb_size;
    if (pb_size == 0) {
        fprintf(stderr, "parse_pb_to_list: pb_size is 0\n");
        return NULL;
    }
    char *json = NULL;

    /* unpack protobuf structure into C structs    */
    TransitRealtime__FeedMessage *feed =
        transit_realtime__feed_message__unpack(NULL, pb_size, (uint8_t *)pb);
    if (!feed) {
        fprintf(stderr, "parse_pb_to_list: failed to unpack protobuf\n");
        return NULL;
    }
    
    /* allocate DepartureList and initial capacity */
    DepartureList *list = malloc(sizeof(DepartureList));
    if (!list) {
        transit_realtime__feed_message__free_unpacked(feed, NULL);
        return NULL;
    }
    size_t capacity = 8; /* start small and grow later */
    list->items = malloc(sizeof(BusDeparture) * capacity);
    if (!list->items) {
        free(list);
        transit_realtime__feed_message__free_unpacked(feed, NULL);
        return NULL;
    }
    list->count = 0;

    return list;
}
    

void free_departure_list(DepartureList *list) {
    if (list) {
        free(list->items);
        free(list);
    }
}