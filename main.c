
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "mongoose.h"
#include "student.h"


// ==========================================
// 1. CORS CONFIGURATION
// ==========================================

#define CORS_HEADERS \
    "Access-Control-Allow-Origin: https://naremovsesyan.github.io\r\n" \
    "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n" \
    "Access-Control-Allow-Headers: Content-Type\r\n" \
    "Vary: Origin\r\n"

#define JSON_HEADERS \
    "Content-Type: application/json\r\n" \
    CORS_HEADERS


// ==========================================
// 2. ERROR RESPONSE
// ==========================================

static void sendError(
    struct mg_connection *c,
    int status,
    const char *message
) {
    mg_http_reply(
        c,
        status,
        JSON_HEADERS,
        "{\"status\":0,\"message\":\"%s\"}\n",
        message
    );
}


// ==========================================
// 3. SUCCESS RESPONSE
// ==========================================

static void sendSuccess(
    struct mg_connection *c,
    const char *message
) {
    mg_http_reply(
        c,
        200,
        JSON_HEADERS,
        "{\"status\":1,\"message\":\"%s\"}\n",
        message
    );
}


// ==========================================
// 4. JSON BUFFER HELPERS
// ==========================================

static int appendText(
    char *buffer,
    size_t capacity,
    size_t *used,
    const char *text
) {
    size_t length = strlen(text);

    if (*used + length >= capacity) {
        return 0;
    }

    memcpy(buffer + *used, text, length);

    *used += length;
    buffer[*used] = '\0';

    return 1;
}


static int appendJsonString(
    char *buffer,
    size_t capacity,
    size_t *used,
    const char *value
) {
    if (!appendText(buffer, capacity, used, "\"")) {
        return 0;
    }

    for (
        const unsigned char *p = (const unsigned char *) value;
        *p;
        p++
    ) {
        char escaped[8];

        if (*p == '"' || *p == '\\') {
            escaped[0] = '\\';
            escaped[1] = (char) *p;
            escaped[2] = '\0';
        }
        else if (*p < 0x20) {
            snprintf(
                escaped,
                sizeof(escaped),
                "\\u%04x",
                (unsigned int) *p
            );
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


// ==========================================
// 5. GET ALL STUDENTS
// ==========================================

static void sendStudents(struct mg_connection *c) {

    char json[32768] = "";
    char item[256];

    size_t used = 0;
    int written = 0;

    if (!appendText(
        json,
        sizeof(json),
        &used,
        "{\"status\":1,\"students\":["
    )) {
        sendError(c, 500, "JSON error");
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
                sendError(c, 500, "JSON buffer full");
                return;
            }
        }

        snprintf(
            item,
            sizeof(item),
            "{\"id\":%d,\"age\":%d,"
            "\"grades\":[%.2f,%.2f,%.2f],"
            "\"average\":%.2f,\"name\":",
            student->id,
            student->age,
            student->grades[0],
            student->grades[1],
            student->grades[2],
            calculateAverage(student)
        );

        if (
            !appendText(json, sizeof(json), &used, item) ||
            !appendJsonString(
                json,
                sizeof(json),
                &used,
                student->name
            ) ||
            !appendText(json, sizeof(json), &used, "}")
        ) {
            sendError(c, 500, "JSON buffer full");
            return;
        }

        written++;
    }

    if (!appendText(json, sizeof(json), &used, "]}")) {
        sendError(c, 500, "JSON buffer full");
        return;
    }

    mg_http_reply(c, 200, JSON_HEADERS, "%s\n", json);
}


// ==========================================
// 6. DELETE STUDENT
// ==========================================

static void handleDeleteStudent(
    struct mg_connection *c,
    struct mg_http_message *hm
) {
    char idBuffer[32];

    int length = mg_http_get_var(
        hm->query,
        "id",
        idBuffer,
        sizeof(idBuffer)
    );

    if (length <= 0) {
        sendError(c, 400, "Student ID is required");
        return;
    }

    char *end;
    long id = strtol(idBuffer, &end, 10);

    if (
        end == idBuffer ||
        *end != '\0' ||
        id < 1 ||
        id > INT_MAX
    ) {
        sendError(c, 400, "Invalid student ID");
        return;
    }

    if (!deleteStudent((int) id)) {
        sendError(c, 404, "Student not found");
        return;
    }

    sendSuccess(c, "Student deleted successfully");
}


// ==========================================
// 7. HTTP EVENT HANDLER
// ==========================================

static void eventHandler(
    struct mg_connection *c,
    int ev,
    void *ev_data
) {
    if (ev != MG_EV_HTTP_MSG) {
        return;
    }

    struct mg_http_message *hm =
        (struct mg_http_message *) ev_data;


    // CORS preflight

    if (
        mg_strcasecmp(hm->method, mg_str("OPTIONS")) == 0
    ) {
        mg_http_reply(c, 204, CORS_HEADERS, "");
        return;
    }


    // HEALTH CHECK

    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {

        if (
            mg_strcasecmp(hm->method, mg_str("GET")) == 0
        ) {
            mg_http_reply(
                c,
                200,
                JSON_HEADERS,
                "{\"status\":1,"
                "\"message\":\"C backend is running\"}\n"
            );
        }
        else {
            sendError(c, 405, "Method not allowed");
        }

        return;
    }


    // STUDENTS API

    if (mg_match(hm->uri, mg_str("/api/students"), NULL)) {

        // GET

        if (
            mg_strcasecmp(hm->method, mg_str("GET")) == 0
        ) {
            sendStudents(c);
            return;
        }


        // DELETE

        if (
            mg_strcasecmp(hm->method, mg_str("DELETE")) == 0
        ) {
            handleDeleteStudent(c, hm);
            return;
        }


        // POST and PUT are not implemented in this version.
        // Add them when connecting your student management functions.

        sendError(c, 405, "Method not allowed");
        return;
    }


    // UNKNOWN ENDPOINT

    sendError(c, 404, "Not found");
}


// ==========================================
// 8. MAIN FUNCTION
// ==========================================

int main(void) {

    // Sample students

    Student student1 = {
        101, "Anna Smith", 16, {9, 10, 8}
    };

    Student student2 = {
        102, "David Johnson", 17, {10, 9, 10}
    };

    addStudent(student1);
    addStudent(student2);


    // Render port

    const char *port_env = getenv("PORT");
    int port = 10000;

    if (port_env != NULL) {

        char *end;
        long value = strtol(port_env, &end, 10);

        if (
            *end != '\0' ||
            value < 1 ||
            value > 65535
        ) {
            printf("Invalid PORT value\n");
            return 1;
        }

        port = (int) value;
    }


    // Initialize server

    struct mg_mgr mgr;
    char listen_url[64];

    mg_mgr_init(&mgr);

    snprintf(
        listen_url,
        sizeof(listen_url),
        "http://0.0.0.0:%d",
        port
    );


    // Start HTTP server

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


    printf("Student Management System started!\n");
    printf("Listening on port %d\n", port);

    printf("GET    /api/hello\n");
    printf("GET    /api/students\n");
    printf("DELETE /api/students?id=ID\n");


    // Keep server running

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 0;
}
