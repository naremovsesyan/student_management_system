
#ifndef STUDENT_H
#define STUDENT_H

#define MAX_STUDENTS 100

typedef struct {
    int id;
    char name[50];
    int age;
    float grades[3];
} Student;

// Student management functions
int addStudent(Student student);
Student *findStudent(int id);
int updateStudent(int oldID, Student updatedStudent);
float calculateAverage(const Student *student);
int deleteStudent(int id);

// Get student information
int getStudentCount(void);
Student *getStudentAt(int index);

#endif
