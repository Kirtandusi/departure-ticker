#include "../include/parser.h"
#include "../include/proto/gtfs-realtime.pb-c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

DepartureList *parse_pb_to_list(char *pb, size_t pb_size,
                                char *stop_ids[], char *route_letters[], size_t n_stops) {
    if (!pb || pb_size == 0 || !stop_ids || !route_letters || n_stops == 0) {
        fprintf(stderr, "parse_pb_to_list: invalid arguments\n");
        return NULL;
    }

    TransitRealtime__FeedMessage *feed =
        transit_realtime__feed_message__unpack(NULL, pb_size, (uint8_t *)pb);
    if (!feed) {
        fprintf(stderr, "Failed to unpack protobuf\n");
        return NULL;
    }

    DepartureList *list = malloc(sizeof(DepartureList));
    if (!list) {
        transit_realtime__feed_message__free_unpacked(feed, NULL);
        return NULL;
    }

    list->items = malloc(sizeof(BusDeparture) * n_stops);
    if (!list->items) {
        free(list);
        transit_realtime__feed_message__free_unpacked(feed, NULL);
        return NULL;
    }

    list->count = n_stops;

    // initialize
    for (size_t i = 0; i < n_stops; i++) {
        strncpy(list->items[i].route_name, route_letters[i], sizeof(list->items[i].route_name)-1);
        list->items[i].route_name[sizeof(list->items[i].route_name)-1] = '\0';
        list->items[i].arrival_unix_time = 0;
    }

    time_t now = time(NULL);

    for (size_t i = 0; i < feed->n_entity; i++) {
        TransitRealtime__FeedEntity *entity = feed->entity[i];
        if (!entity->trip_update) continue;

        TransitRealtime__TripUpdate *tu = entity->trip_update;
        if (!tu->trip || !tu->trip->route_id) continue;

        for (size_t j = 0; j < tu->n_stop_time_update; j++) {
            TransitRealtime__TripUpdate__StopTimeUpdate *stu = tu->stop_time_update[j];
            if (!stu->stop_id) continue;

            for (size_t k = 0; k < n_stops; k++) {
                if (strcmp(stu->stop_id, stop_ids[k]) != 0) continue;

                int64_t ts = 0;
                if (stu->departure && stu->departure->time)
                    ts = stu->departure->time;   // use departure from stop
                else if (stu->arrival && stu->arrival->time)
                    ts = stu->arrival->time;      // fallback
                else
                    continue;

                if (ts < now - 60) continue; // ignore past departures

                if (ts > list->items[k].arrival_unix_time)
                    list->items[k].arrival_unix_time = ts;  // latest upcoming departure
            }
        }
    }

    transit_realtime__feed_message__free_unpacked(feed, NULL);
    return list;
}

void free_departure_list(DepartureList *list) {
    if (!list) return;
    free(list->items);
    free(list);
}
