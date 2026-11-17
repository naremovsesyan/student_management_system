
#include <stdio.h>
#include <string.h>
#include "student.h"

static Student students[MAX_STUDENTS];
static int studentCount = 0;

// Add a student
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

// Find a student by ID
Student *findStudent(int id) {
    for (int i = 0; i < studentCount; i++) {
        if (students[i].id == id) {
            return &students[i];
        }
    }

    return NULL;
}

// Update a student
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

// Calculate average grade
float calculateAverage(const Student *student) {
    float total = 0;

    for (int i = 0; i < 3; i++) {
        total += student->grades[i];
    }

    return total / 3;
}

// Get total number of students
int getStudentCount(void) {
    return studentCount;
}

// Get student by position
Student *getStudentAt(int index) {
    if (index < 0 || index >= studentCount) {
        return NULL;
    }

    return &students[index];
}
