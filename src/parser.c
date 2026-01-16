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


        //     printf("DEBUG: route_id=%s, stop_id=%s\n", 
        //    tu->trip->route_id, stu->stop_id); 


            for (size_t k = 0; k < n_stops; k++) {
                if (strcmp(stu->stop_id, stop_ids[k]) != 0) continue;


                // printf("DEBUG: Matched stop %s\n", stop_ids[k]);


                int64_t ts = 0;
                if (stu->departure && stu->departure->time)
                    ts = stu->departure->time;   // use departure from stop
                else if (stu->arrival && stu->arrival->time)
                    ts = stu->arrival->time;      // fallback
                else
                    continue;


                // if (stu->departure && stu->departure->has_delay) {
                //     printf("DEBUG: Route %s has delay=%d seconds\n", 
                //         tu->trip->route_id, stu->departure->delay);
                //     ts += stu->departure->delay;  // Apply the delay!
                // } else if (stu->arrival && stu->arrival->has_delay) {
                //     printf("DEBUG: Route %s has delay=%d seconds\n",
                //         tu->trip->route_id, stu->arrival->delay);
                //     ts += stu->arrival->delay;  // Apply the delay!
                // }

                //                 if (stu->departure) {
                //     printf("DEBUG departure: has_time=%d, has_delay=%d", 
                //         stu->departure->has_time, stu->departure->has_delay);
                //     if (stu->departure->has_delay)
                //         printf(", delay=%d", stu->departure->delay);
                //     printf("\n");
                // }
                // if (stu->arrival) {
                //     printf("DEBUG arrival: has_time=%d, has_delay=%d", 
                //         stu->arrival->has_time, stu->arrival->has_delay);
                //     if (stu->arrival->has_delay)
                //         printf(", delay=%d", stu->arrival->delay);
                //     printf("\n");
                // }

                if (ts < now - 60) continue; // ignore past departures

                if (list->items[k].arrival_unix_time == 0 || ts < list->items[k].arrival_unix_time)
                    list->items[k].arrival_unix_time = ts;  // latest upcoming departure

            //     printf("DEBUG: Route %s, stop %s, ts=%lld, now=%ld, diff_min=%lld\n",
            //         route_letters[k], stop_ids[k], (long long)ts, now, (long long)((ts - now) / 60));
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
