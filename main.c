
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mongoose.h"
#include "student.h"

// Allow your GitHub Pages website to access the API.
// Change this origin if your GitHub username is different.
#define CORS_HEADERS \
    "Access-Control-Allow-Origin: https://naremovsesyan.github.io\r\n" \
    "Access-Control-Allow-Methods: GET, OPTIONS\r\n" \
    "Access-Control-Allow-Headers: Content-Type\r\n"

#define JSON_HEADERS \
    "Content-Type: application/json\r\n" \
    CORS_HEADERS

// Safely append text to a JSON buffer
static int appendText(char *buffer, size_t capacity,
                      size_t *used, const char *text) {

    size_t length = strlen(text);

    if (*used + length >= capacity) {
        return 0;
    }

    memcpy(buffer + *used, text, length);
    *used += length;
    buffer[*used] = '\0';

    return 1;
}

// Add a JSON-escaped string
static int appendJsonString(char *buffer, size_t capacity,
                            size_t *used, const char *value) {

    if (!appendText(buffer, capacity, used, "\"")) {
        return 0;
    }

    for (const unsigned char *p =
         (const unsigned char *) value; *p; p++) {

        char escaped[8];

        if (*p == '"' || *p == '\\') {
            escaped[0] = '\\';
            escaped[1] = (char) *p;
            escaped[2] = '\0';
        }
        else if (*p < 0x20) {
            snprintf(escaped, sizeof(escaped),
                     "\\u%04x", (unsigned int) *p);
        }
        else {
            escaped[0] = (char) *p;
            escaped[1] = '\0';
        }

        if (!appendText(buffer, capacity, used, escaped)) {
            return 0;
        }
    }

    return appendText(buffer, capacity, used, "\"");
}

// Return all students as JSON
static void sendStudents(struct mg_connection *c) {

    char json[32768] = "";
    char item[256];
    size_t used = 0;
    int written = 0;

    if (!appendText(json, sizeof(json), &used,
                    "{\"status\":1,\"students\":[")) {
        mg_http_reply(c, 500, JSON_HEADERS,
                      "{\"status\":0,\"message\":\"JSON error\"}\n");
        return;
    }

    int count = getStudentCount();

    for (int i = 0; i < count; i++) {

        Student *student = getStudentAt(i);

        if (student == NULL) {
            continue;
        }

        if (written > 0) {
            if (!appendText(json, sizeof(json), &used, ",")) {
                mg_http_reply(c, 500, JSON_HEADERS,
                              "{\"status\":0,\"message\":\"JSON buffer full\"}\n");
                return;
            }
        }

        snprintf(item, sizeof(item),
                 "{\"id\":%d,\"age\":%d,\"grades\":[%.2f,%.2f,%.2f],\"average\":%.2f,\"name\":",
                 student->id,
                 student->age,
                 student->grades[0],
                 student->grades[1],
                 student->grades[2],
                 calculateAverage(student));

        if (!appendText(json, sizeof(json), &used, item) ||
            !appendJsonString(json, sizeof(json), &used,
                              student->name) ||
            !appendText(json, sizeof(json), &used, "}")) {

            mg_http_reply(c, 500, JSON_HEADERS,
                          "{\"status\":0,\"message\":\"JSON buffer full\"}\n");
            return;
        }

        written++;
    }

    if (!appendText(json, sizeof(json), &used, "]}")) {
        mg_http_reply(c, 500, JSON_HEADERS,
                      "{\"status\":0,\"message\":\"JSON buffer full\"}\n");
        return;
    }

    mg_http_reply(c, 200, JSON_HEADERS, "%s\n", json);
}

// Handle HTTP requests
static void eventHandler(struct mg_connection *c,
                         int ev, void *ev_data) {

    if (ev != MG_EV_HTTP_MSG) {
        return;
    }

    struct mg_http_message *hm =
        (struct mg_http_message *) ev_data;

    // Handle CORS preflight requests
    if (mg_strcasecmp(hm->method, mg_str("OPTIONS")) == 0) {
        mg_http_reply(c, 204, CORS_HEADERS, "");
        return;
    }

    // Test endpoint
    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {

        mg_http_reply(
            c,
            200,
            JSON_HEADERS,
            "{\"status\":1,\"message\":\"C backend is running\"}\n"
        );
    }

    // Students endpoint
    else if (mg_match(hm->uri, mg_str("/api/students"), NULL)) {

        if (mg_strcasecmp(hm->method, mg_str("GET")) != 0) {
            mg_http_reply(c, 405, JSON_HEADERS,
                          "{\"status\":0,\"message\":\"Method not allowed\"}\n");
            return;
        }

        sendStudents(c);
    }

    // Unknown endpoint
    else {
        mg_http_reply(
            c,
            404,
            JSON_HEADERS,
            "{\"status\":0,\"message\":\"Not found\"}\n"
        );
    }
}

int main(void) {

    // Create sample students
    Student student1 = {
        101, "Anna Smith", 16, {9, 10, 8}
    };

    Student student2 = {
        102, "David Johnson", 17, {10, 9, 10}
    };

    addStudent(student1);
    addStudent(student2);

    // Read the port provided by Render
    const char *port_env = getenv("PORT");
    int port = 10000;

    if (port_env != NULL) {
        char *end;
        long value = strtol(port_env, &end, 10);

        if (*end != '\0' || value < 1 || value > 65535) {
            printf("Invalid PORT value\n");
            return 1;
        }

        port = (int) value;
    }

    // Initialize HTTP server
    struct mg_mgr mgr;
    char listen_url[64];

    mg_mgr_init(&mgr);

    snprintf(listen_url, sizeof(listen_url),
             "http://0.0.0.0:%d", port);

    struct mg_connection *server = mg_http_listen(
        &mgr,
        listen_url,
        eventHandler,
        NULL
    );

    if (server == NULL) {
        printf("Failed to start server!\n");
        mg_mgr_free(&mgr);
        return 1;
    }

    printf("Student Management System server started!\n");
    printf("Listening on port %d\n", port);
    printf("API: /api/students\n");

    // Keep server running
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 0;
}
