
#include <stdio.h>
#include <string.h>

#include "student.h"

static Student students[MAX_STUDENTS];
static int studentCount = 0;


// ==========================================
// ADD STUDENT
// ==========================================

int addStudent(Student student) {

    if (studentCount >= MAX_STUDENTS) {
        return 0;
    }

    if (findStudent(student.id) != NULL) {
        return 0;
    }

    students[studentCount] = student;
    studentCount++;

    return 1;
}


// ==========================================
// FIND STUDENT
// ==========================================

Student *findStudent(int id) {

    for (int i = 0; i < studentCount; i++) {

        if (students[i].id == id) {
            return &students[i];
        }
    }

    return NULL;
}


// ==========================================
// UPDATE STUDENT
// ==========================================

int updateStudent(int oldID, Student updatedStudent) {

    Student *student = findStudent(oldID);

    if (student == NULL) {
        return 0;
    }

    Student *duplicate = findStudent(updatedStudent.id);

    if (duplicate != NULL && duplicate != student) {
        return 0;
    }

    *student = updatedStudent;

    return 1;
}


// ==========================================
// DELETE STUDENT
// ==========================================

int deleteStudent(int id) {

    for (int i = 0; i < studentCount; i++) {

        if (students[i].id == id) {

            for (int j = i; j < studentCount - 1; j++) {
                students[j] = students[j + 1];
            }

            studentCount--;

            return 1;
        }
    }

    return 0;
}


// ==========================================
// CALCULATE AVERAGE
// ==========================================

float calculateAverage(const Student *student) {

    if (student == NULL) {
        return 0;
    }

    float total = 0;

    for (int i = 0; i < 3; i++) {
        total += student->grades[i];
    }

    return total / 3.0f;
}


// ==========================================
// GET STUDENT COUNT
// ==========================================

int getStudentCount(void) {
    return studentCount;
}


// ==========================================
// GET STUDENT BY INDEX
// ==========================================

Student *getStudentAt(int index) {

    if (index < 0 || index >= studentCount) {
        return NULL;
    }

    return &students[index];
}
