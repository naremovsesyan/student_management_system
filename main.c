
#include <stdio.h>
#include <string.h>
#include "mongoose.h"
#include "student.h"

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

    appendText(json, sizeof(json), &used,
               "{\"status\":1,\"students\":[");

    int count = getStudentCount();

    for (int i = 0; i < count; i++) {

        Student *student = getStudentAt(i);

        if (student == NULL) {
            continue;
        }

        if (i > 0) {
            if (!appendText(json, sizeof(json), &used, ",")) {
                mg_http_reply(c, 500, "", "JSON buffer full\n");
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

            mg_http_reply(c, 500, "", "JSON buffer full\n");
            return;
        }
    }

    appendText(json, sizeof(json), &used, "]}");

    mg_http_reply(
        c,
        200,
        "Content-Type: application/json\r\n",
        "%s\n",
        json
    );
}

// Handle HTTP requests
static void eventHandler(struct mg_connection *c,
                         int ev, void *ev_data) {

    if (ev != MG_EV_HTTP_MSG) {
        return;
    }

    struct mg_http_message *hm =
        (struct mg_http_message *) ev_data;

    // Test endpoint
    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {

        mg_http_reply(
            c,
            200,
            "Content-Type: application/json\r\n",
            "{\"status\":1,\"message\":\"C backend is running\"}\n"
        );
    }

    // Students endpoint
    else if (mg_match(hm->uri, mg_str("/api/students"), NULL)) {

        sendStudents(c);
    }

    // Serve website files
    else {

        struct mg_http_serve_opts opts = {
            .root_dir = "."
        };

        mg_http_serve_dir(c, hm, &opts);
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

    // Initialize HTTP server
    struct mg_mgr mgr;

    mg_mgr_init(&mgr);

    struct mg_connection *server = mg_http_listen(
        &mgr,
        "http://127.0.0.1:8000",
        eventHandler,
        NULL
    );

    if (server == NULL) {
        printf("Failed to start server!\n");
        mg_mgr_free(&mgr);
        return 1;
    }

    printf("Student Management System server started!\n");
    printf("Website: http://localhost:8000\n");
    printf("API: http://localhost:8000/api/students\n");

    // Keep server running
    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 0;
}
