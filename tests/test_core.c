#define _POSIX_C_SOURCE 200809L
#include "config.h"
#include "networking.h"
#include "parser.h"
#include "render.h"
#include <assert.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char *stops[] = {"0454", "0133"};
static const time_t summer = 1784048400; /* 2026-07-14 12:00 CDT */

static DepartureList *parse(const char *predictions, time_t now) {
    char json[8192];
    snprintf(json, sizeof(json), "{\"bustime-response\":{\"prd\":[%s]}}", predictions);
    return parse_json_predictions(json, stops, 2, now);
}

static void check_time(const char *timestamp, time_t expected) {
    char item[512];
    snprintf(item, sizeof(item), "{\"stpid\":\"0454\",\"rt\":\"A\",\"prdtm\":%s}", timestamp);
    DepartureList *list = parse(item, expected - 300);
    assert(list && list->count == 2);
    assert(list->items[0].arrival_unix_time == (int64_t)expected);
    assert(!list->items[1].arrival_unix_time);
    free_departure_list(list);
}

static void test_times(void) {
    setenv("TZ", "UTC", 1);
    tzset();
    assert(init_transit_timezone() == 0);
    check_time("\"20260714 12:00\"", summer);
    check_time("\"20260114 12:00\"", 1768413600);
    check_time("\"20260714 12:00:45\"", summer + 45);
    check_time("1784048400000", summer);
    check_time("\"1784048400000\"", summer);
    check_time("\"20260308 01:55\"", 1772956500);
    check_time("\"20260308 03:05\"", 1772957100);
    /* Absolute times distinguish the two 01:xx hours in fall. */
    check_time("1793516100000", 1793516100);
    check_time("1793516700000", 1793516700);
    const char *invalid[] = {"\"20260230 12:00\"", "\"20260308 02:30\"",
        "\"20260714 12:00junk\"", "\"1784048400000x\"", "null", "1e300", "-1"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        char item[512];
        snprintf(item, sizeof(item), "{\"stpid\":\"0454\",\"rt\":\"A\",\"prdtm\":%s}", invalid[i]);
        assert(parse(item, summer - 60) == NULL);
    }
}

static void test_selection(void) {
    const char *items =
        "{\"stpid\":\"0454\",\"rt\":\"OLD\",\"prdtm\":\"20260714 11:00\"},"
        "{\"stpid\":\"0454\",\"rt\":\"A\",\"prdtm\":\"20260714 12:05\"},"
        "{\"stpid\":\"0454\",\"rt\":\"B\",\"prdtm\":\"20260714 12:10\"},"
        "{\"stpid\":\"0133\",\"rt\":\"C\",\"prdtm\":\"20260714 12:07\"}";
    DepartureList *list = parse(items, summer);
    assert(list);
    assert(!strcmp(list->items[0].route_name, "A"));
    assert(list->items[0].arrival_unix_time == summer + 300);
    assert(!strcmp(list->items[1].route_name, "C"));
    free_departure_list(list);
    list = parse("{\"stpid\":\"0454\",\"rt\":\"B\",\"prdtm\":\"20260714 12:10\"},"
                 "{\"stpid\":\"0454\",\"rt\":\"A\",\"prdtm\":\"20260714 12:05\"}", summer);
    assert(list && !strcmp(list->items[0].route_name, "A"));
    free_departure_list(list);
    const int excluded[] = {1, 4, 16, 17, 18};
    for (size_t i = 0; i < sizeof(excluded) / sizeof(*excluded); ++i) {
        char item[512];
        snprintf(item, sizeof(item), "{\"stpid\":\"0454\",\"rt\":\"A\","
                 "\"prdtm\":\"20260714 12:05\",\"dyn\":%d}", excluded[i]);
        list = parse(item, summer);
        assert(list && !list->items[0].arrival_unix_time);
        free_departure_list(list);
    }
}

static void test_responses(void) {
    const char *bad[] = {"", "garbage", "{}", "[]", "{\"bustime-response\":{}}",
        "{\"bustime-response\":{\"prd\":{}}}",
        "{\"bustime-response\":{\"prd\":[]}} trailing",
        "{\"bustime-response\":{\"error\":[{\"msg\":\"Invalid API access key supplied\"}]}}",
        "{\"bustime-response\":{\"prd\":[],\"error\":[{\"msg\":\"Invalid parameter provided\"}]}}"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); ++i)
        assert(parse_json_predictions(bad[i], stops, 2, summer) == NULL);
    assert(parse("{\"stpid\":\"0454\",\"rt\":12}", summer) == NULL);
    const char *empty[] = {
        "{\"bustime-response\":{\"prd\":[]}}",
        "{\"bustime-response\":{\"error\":[{\"stpid\":\"0454\",\"msg\":\"No service scheduled\"}]}}",
        "{\"bustime-response\":{\"error\":[{\"msg\":\"No arrival times\"}]}}"};
    for (size_t i = 0; i < sizeof(empty) / sizeof(*empty); ++i) {
        DepartureList *list = parse_json_predictions(empty[i], stops, 2, summer);
        assert(list && list->count == 2 && !list->items[0].arrival_unix_time);
        assert(!strcmp(list->items[0].stop_id, "0454"));
        free_departure_list(list);
    }
    assert(parse_json_predictions("{}", stops, 11, summer) == NULL);
    free_departure_list(NULL);
}

static void test_network_url(void) {
    char *url = build_url("test&key=with space", stops, 2, "feed & one");
    assert(url);
    assert(strstr(url, "key=test%26key%3Dwith%20space"));
    assert(strstr(url, "stpid=0454,0133"));
    assert(strstr(url, "unixTime=true"));
    assert(strstr(url, "rtpidatafeed=feed%20%26%20one"));
    free(url);
    char long_key[4097];
    memset(long_key, 'x', sizeof(long_key) - 1);
    long_key[sizeof(long_key) - 1] = '\0';
    url = build_url(long_key, stops, 2, NULL);
    assert(url && strlen(url) > 4096);
    free(url);
    char *invalid[] = {"1&key=injected"};
    assert(build_url("test", invalid, 1, NULL) == NULL);
    assert(build_url("", stops, 2, NULL) == NULL);
    assert(build_url("test", stops, 11, NULL) == NULL);
    size_t size = 99;
    assert(get_data(NULL, &size) == NULL && size == 0);
}

static void test_config(void) {
    char storage[MAX_STOPS][STOP_ID_SIZE];
    char *ids[MAX_STOPS];
    size_t count;
    assert(parse_stops("0454,0133,0857,10062", storage, ids, &count) == 0 && count == 4);
    assert(!strcmp(ids[0], "0454"));
    const char *bad[] = {"", "1,", ",1", "1,,2", "1,1", "1,a", "1, 2",
        "1,2,3,4,5,6,7,8,9,10,11", "12345678901234567890123456789012"};
    for (size_t i = 0; i < sizeof(bad) / sizeof(*bad); ++i)
        assert(parse_stops(bad[i], storage, ids, &count) == -1);
    unsigned seconds;
    assert(poll_interval(NULL, &seconds) == 0 && seconds == 15);
    assert(poll_interval("60", &seconds) == 0 && seconds == 60);
    assert(poll_interval("0", &seconds) == -1);
    assert(poll_interval("15junk", &seconds) == -1);
    assert(poll_interval("999999999999999999999999", &seconds) == -1);
    char path[] = "/tmp/ticker-dotenv-XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    FILE *file = fdopen(fd, "w");
    assert(file);
    fputs("# comment\r\n TICKER_TEST_VALUE = \"hello world\"\r\n"
          "export TICKER_TEST_EXISTING=from-file\n", file);
    fclose(file);
    unsetenv("TICKER_TEST_VALUE");
    setenv("TICKER_TEST_EXISTING", "from-shell", 1);
    assert(load_dotenv(path) == 0);
    assert(!strcmp(getenv("TICKER_TEST_VALUE"), "hello world"));
    assert(!strcmp(getenv("TICKER_TEST_EXISTING"), "from-shell"));
    unlink(path);
    assert(load_dotenv(path) == 0);
}

static void test_countdown(void) {
    BusDeparture departure = {.arrival_unix_time = summer};
    assert(departure_minutes(&departure, summer - 61) == 2);
    assert(departure_minutes(&departure, summer - 60) == 1);
    assert(departure_minutes(&departure, summer) == 0);
    assert(departure_minutes(&departure, summer + 59) == 0);
    assert(departure_minutes(&departure, summer + 60) == -1);
    departure.arrival_unix_time = 0;
    assert(departure_minutes(&departure, summer) == -1);
}

int main(void) {
    assert(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK);
    test_times();
    test_selection();
    test_responses();
    test_network_url();
    test_config();
    test_countdown();
    curl_global_cleanup();
    puts("Core regressions passed (time, routes, errors, URL, configuration, countdown).");
    return 0;
}
