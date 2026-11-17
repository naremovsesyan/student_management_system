
#ifndef STUDENT_H
#define STUDENT_H

#define MAX_STUDENTS 100

typedef struct {
    int id;
    char name[50];
    int age;
    float grades[3];
} Student;

// Student management
int addStudent(Student student);
Student *findStudent(int id);
int updateStudent(int oldID, Student updatedStudent);
int deleteStudent(int id);

// Calculations
float calculateAverage(const Student *student);

// Student information
int getStudentCount(void);
Student *getStudentAt(int index);

#endif
