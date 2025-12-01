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
    list->items = malloc(sizeof(BusDeparture) * 1);
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

    /* ---- unpack protobuf ---- */
    TransitRealtime__FeedMessage *feed =
        transit_realtime__feed_message__unpack(NULL, pb_size, (uint8_t *)pb);

    if (!feed) {
        fprintf(stderr, "parse_pb_to_list: failed to unpack protobuf\n");
        return NULL;
    }

    /* ---- allocate result list ---- */
    DepartureList *list = malloc(sizeof(DepartureList));
    if (!list) {
        transit_realtime__feed_message__free_unpacked(feed, NULL);
        return NULL;
    }

    size_t capacity = 8;
    list->items = malloc(sizeof(BusDeparture) * capacity);
    if (!list->items) {
        free(list);
        transit_realtime__feed_message__free_unpacked(feed, NULL);
        return NULL;
    }
    list->count = 0;

    /* ---- iterate entities ---- */
    for (size_t i = 0; i < feed->n_entity; i++) {
        TransitRealtime__FeedEntity *entity = feed->entity[i];
        if (!entity->trip_update)
            continue;

        TransitRealtime__TripUpdate *tu = entity->trip_update;

        if (!tu->trip || !tu->trip->route_id)
            continue;

        /* iterate stop-time updates */
        for (size_t j = 0; j < tu->n_stop_time_update; j++) {

            TransitRealtime__TripUpdate__StopTimeUpdate *stu =
                tu->stop_time_update[j];

            if (!stu->stop_id)
                continue;

            /* filter by the requested stop ID */
            if (strcmp(stu->stop_id, stop) != 0)
                continue;

            /* arrival may not always exist */
            if (!stu->arrival || !stu->arrival->time)
                continue;

            /* grow list if needed */
            if (list->count == capacity) {
                capacity *= 2;
                BusDeparture *tmp = realloc(list->items,
                                            capacity * sizeof(BusDeparture));
                if (!tmp) {
                    fprintf(stderr, "parse_pb_to_list: realloc failed\n");
                    free(list->items);
                    free(list);
                    transit_realtime__feed_message__free_unpacked(feed, NULL);
                    return NULL;
                }
                list->items = tmp;
            }

            /* fill out a BusDeparture */
            BusDeparture *d = &list->items[list->count++];
            printf("[DEBUG] Matched stop %s | Route %s | Arrival %ld\n",
            stu->stop_id, 
            tu->trip->route_id,
            stu->arrival->time);

            strncpy(d->route_name, tu->trip->route_id, sizeof(d->route_name));
            d->route_name[sizeof(d->route_name) - 1] = '\0';
            d->departure_minutes = stu->arrival->time;   // unix timestamp
        }
    }

    /* ---- cleanup ---- */
    transit_realtime__feed_message__free_unpacked(feed, NULL);

    return list;
}

void free_departure_list(DepartureList *list) {
    if (!list) return;

    for (size_t i = 0; i < list->count; i++) {
        free(list->items[i].route_name);
    }

    free(list->items);
    free(list);
}
