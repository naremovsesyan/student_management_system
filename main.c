
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

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

    appendText(
        json,
        sizeof(json),
        &used,
        "{\"status\":1,\"students\":["
    );

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
// 6. READ STUDENT JSON
// ==========================================

static int readStudent(
    struct mg_http_message *hm,
    Student *student
) {
    char name[50];

    double id = mg_json_get_num(hm->body, "$.id");
    double age = mg_json_get_num(hm->body, "$.age");

    double math = mg_json_get_num(hm->body, "$.grades[0]");
    double english = mg_json_get_num(hm->body, "$.grades[1]");
    double physics = mg_json_get_num(hm->body, "$.grades[2]");

    size_t nameLength = mg_json_get_str(
        hm->body,
        "$.name",
        name,
        sizeof(name)
    );

    if (
        nameLength == 0 ||
        name[0] == '\0' ||
        strlen(name) >= sizeof(student->name) ||
        !isfinite(id) ||
        !isfinite(age) ||
        !isfinite(math) ||
        !isfinite(english) ||
        !isfinite(physics) ||
        id < 1 || id > INT_MAX ||
        floor(id) != id ||
        age < 1 || age > 120 ||
        floor(age) != age ||
        math < 0 || math > 10 ||
        english < 0 || english > 10 ||
        physics < 0 || physics > 10
    ) {
        return 0;
    }

    student->id = (int) id;
    student->age = (int) age;

    snprintf(student->name, sizeof(student->name), "%s", name);

    student->grades[0] = (float) math;
    student->grades[1] = (float) english;
    student->grades[2] = (float) physics;

    return 1;
}


// ==========================================
// 7. GET ID FROM QUERY
// ==========================================

static int readStudentID(
    struct mg_http_message *hm,
    int *id
) {
    char idBuffer[32];

    int length = mg_http_get_var(
        hm->query,
        "id",
        idBuffer,
        sizeof(idBuffer)
    );

    if (length <= 0) {
        return 0;
    }

    char *end;
    long value = strtol(idBuffer, &end, 10);

    if (
        end == idBuffer ||
        *end != '\0' ||
        value < 1 ||
        value > INT_MAX
    ) {
        return 0;
    }

    *id = (int) value;

    return 1;
}


// ==========================================
// 8. ADD STUDENT
// ==========================================

static void handleAddStudent(
    struct mg_connection *c,
    struct mg_http_message *hm
) {
    Student student = {0};

    if (!readStudent(hm, &student)) {
        sendError(c, 400, "Invalid student information");
        return;
    }

    if (!addStudent(student)) {
        sendError(c, 409, "Student ID already exists or limit reached");
        return;
    }

    sendSuccess(c, "Student added successfully");
}


// ==========================================
// 9. UPDATE STUDENT
// ==========================================

static void handleUpdateStudent(
    struct mg_connection *c,
    struct mg_http_message *hm
) {
    int oldID;

    if (!readStudentID(hm, &oldID)) {
        sendError(c, 400, "Valid student ID is required");
        return;
    }

    Student student = {0};

    if (!readStudent(hm, &student)) {
        sendError(c, 400, "Invalid student information");
        return;
    }

    if (findStudent(oldID) == NULL) {
        sendError(c, 404, "Student not found");
        return;
    }

    if (!updateStudent(oldID, student)) {
        sendError(c, 409, "Student ID already exists");
        return;
    }

    sendSuccess(c, "Student updated successfully");
}


// ==========================================
// 10. DELETE STUDENT
// ==========================================

static void handleDeleteStudent(
    struct mg_connection *c,
    struct mg_http_message *hm
) {
    int id;

    if (!readStudentID(hm, &id)) {
        sendError(c, 400, "Valid student ID is required");
        return;
    }

    if (!deleteStudent(id)) {
        sendError(c, 404, "Student not found");
        return;
    }

    sendSuccess(c, "Student deleted successfully");
}


// ==========================================
// 11. HTTP EVENT HANDLER
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


    // CORS PREFLIGHT

    if (mg_strcasecmp(hm->method, mg_str("OPTIONS")) == 0) {
        mg_http_reply(c, 204, CORS_HEADERS, "");
        return;
    }


    // HEALTH CHECK

    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {

        if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {

            mg_http_reply(
                c,
                200,
                JSON_HEADERS,
                "{\"status\":1,"
                "\"message\":\"C backend is running\"}\n"
            );

        } else {
            sendError(c, 405, "Method not allowed");
        }

        return;
    }


    // STUDENTS API

    if (mg_match(hm->uri, mg_str("/api/students"), NULL)) {

        if (mg_strcasecmp(hm->method, mg_str("GET")) == 0) {
            sendStudents(c);
            return;
        }

        if (mg_strcasecmp(hm->method, mg_str("POST")) == 0) {
            handleAddStudent(c, hm);
            return;
        }

        if (mg_strcasecmp(hm->method, mg_str("PUT")) == 0) {
            handleUpdateStudent(c, hm);
            return;
        }

        if (mg_strcasecmp(hm->method, mg_str("DELETE")) == 0) {
            handleDeleteStudent(c, hm);
            return;
        }

        sendError(c, 405, "Method not allowed");
        return;
    }


    // UNKNOWN ENDPOINT

    sendError(c, 404, "Not found");
}


// ==========================================
// 12. MAIN FUNCTION
// ==========================================

int main(void) {

    // INITIAL SAMPLE STUDENTS

    Student student1 = {
        101, "Anna Smith", 16, {9, 10, 8}
    };

    Student student2 = {
        102, "David Johnson", 17, {10, 9, 10}
    };

    addStudent(student1);
    addStudent(student2);


    // RENDER PORT

    const char *port_env = getenv("PORT");

    int port = 10000;

    if (port_env != NULL) {

        char *end;
        long value = strtol(port_env, &end, 10);

        if (
            end == port_env ||
            *end != '\0' ||
            value < 1 ||
            value > 65535
        ) {
            printf("Invalid PORT value\n");
            return 1;
        }

        port = (int) value;
    }


    // INITIALIZE SERVER

    struct mg_mgr mgr;

    char listen_url[64];

    mg_mgr_init(&mgr);

    snprintf(
        listen_url,
        sizeof(listen_url),
        "http://0.0.0.0:%d",
        port
    );


    // START HTTP SERVER

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

    printf("GET    /api/students\n");
    printf("POST   /api/students\n");
    printf("PUT    /api/students?id=ID\n");
    printf("DELETE /api/students?id=ID\n");


    // KEEP SERVER RUNNING

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    mg_mgr_free(&mgr);

    return 0;
}
