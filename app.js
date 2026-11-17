
// ==========================================
// 1. STUDENT DATA
// ==========================================

let students = [];
let editingStudentID = null;

const API_URL =
    "https://student-management-system-4smk.onrender.com/api";

// ==========================================
// 2. DOM ELEMENTS
// ==========================================

const studentForm = document.getElementById("studentForm");
const studentModal = document.getElementById("studentModal");

const submitButton = document.getElementById("submitButton");
const openModalButton = document.getElementById("openModal");
const closeModalButton = document.getElementById("closeModal");


// ==========================================
// 3. LOAD STUDENTS FROM C BACKEND
// ==========================================

async function loadStudents() {
    try {
        const response = await fetch(`${API_URL}/students`);
        if (!response.ok) {
            throw new Error("Failed to load students");
        }

        const data = await response.json();

        if (data.status !== 1 || !Array.isArray(data.students)) {
            throw new Error("Invalid student data");
        }

        students = data.students;

        displayStudents();
        updateDashboard();
        updateAnalytics();

        console.log("Students loaded from C backend:", students);

    } catch (error) {
        console.error("Error loading students:", error);

        alert("Cannot connect to C backend. Make sure the server is running.");
    }
}


// ==========================================
// 4. CALCULATE STUDENT AVERAGE
// ==========================================

function calculateAverage(student) {
    return student.grades.reduce((sum, grade) => sum + grade, 0)
        / student.grades.length;
}


// ==========================================
// 5. DISPLAY STUDENTS
// ==========================================

function displayStudents(list = students) {
    const table = document.getElementById("studentTable");
    const emptyMessage = document.getElementById("emptyMessage");

    table.innerHTML = "";

    if (list.length === 0) {
        emptyMessage.style.display = "block";
        return;
    }

    emptyMessage.style.display = "none";

    list.forEach(student => {
        const average = calculateAverage(student);

        const row = document.createElement("tr");

        row.innerHTML = `
            <td>${student.id}</td>
            <td>${escapeHTML(student.name)}</td>
            <td>${student.age}</td>
            <td>${student.grades[0].toFixed(2)}</td>
            <td>${student.grades[1].toFixed(2)}</td>
            <td>${student.grades[2].toFixed(2)}</td>
            <td>${average.toFixed(2)}</td>
            <td>
                <button class="edit-btn"
                    onclick="editStudent(${student.id})">
                    Edit
                </button>
            </td>
        `;

        table.appendChild(row);
    });
}


// Escape names before inserting them into HTML
function escapeHTML(value) {
    return String(value).replace(/[&<>"']/g, character => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;"
    })[character]);
}


// ==========================================
// 6. UPDATE DASHBOARD
// ==========================================

function updateDashboard() {
    document.getElementById("totalStudents").textContent = students.length;

    if (students.length === 0) {
        document.getElementById("averageGrade").textContent = "0.00";
        document.getElementById("topStudent").textContent = "N/A";
        return;
    }

    let total = 0;
    let topStudent = students[0];

    students.forEach(student => {
        const average = calculateAverage(student);

        total += average;

        if (average > calculateAverage(topStudent)) {
            topStudent = student;
        }
    });

    document.getElementById("averageGrade").textContent =
        (total / students.length).toFixed(2);

    document.getElementById("topStudent").textContent = topStudent.name;
}


// ==========================================
// 7. UPDATE ANALYTICS
// ==========================================

function updateAnalytics() {
    const subjects = ["math", "english", "physics"];

    if (students.length === 0) {
        subjects.forEach(subject => {
            document.getElementById(subject + "Average").textContent = "0.00";
            document.getElementById(subject + "Bar").style.width = "0%";
            document.getElementById(subject + "Percent").textContent = "0%";
        });

        return;
    }

    const totals = [0, 0, 0];

    students.forEach(student => {
        student.grades.forEach((grade, index) => {
            totals[index] += grade;
        });
    });

    subjects.forEach((subject, index) => {
        const average = totals[index] / students.length;
        const percentage = average * 10;

        document.getElementById(subject + "Average").textContent =
            average.toFixed(2);

        document.getElementById(subject + "Bar").style.width =
            percentage + "%";

        document.getElementById(subject + "Percent").textContent =
            percentage.toFixed(0) + "%";
    });
}


// ==========================================
// 8. SEARCH STUDENTS
// ==========================================

document.getElementById("searchInput").addEventListener("input", function() {
    const searchValue = this.value.toLowerCase();

    const filteredStudents = students.filter(student =>
        student.name.toLowerCase().includes(searchValue) ||
        student.id.toString().includes(searchValue)
    );

    displayStudents(filteredStudents);
});


// ==========================================
// 9. OPEN AND CLOSE MODAL
// ==========================================

openModalButton.addEventListener("click", function() {
    editingStudentID = null;
    studentForm.reset();

    submitButton.textContent = "Add Student";
    studentModal.style.display = "flex";
});

closeModalButton.addEventListener("click", function() {
    studentModal.style.display = "none";
});


// ==========================================
// 10. EDIT STUDENT
// ==========================================

function editStudent(id) {
    const student = students.find(s => s.id === id);

    if (!student) {
        alert("Student not found!");
        return;
    }

    document.getElementById("studentID").value = student.id;
    document.getElementById("studentName").value = student.name;
    document.getElementById("studentAge").value = student.age;

    document.getElementById("math").value = student.grades[0];
    document.getElementById("english").value = student.grades[1];
    document.getElementById("physics").value = student.grades[2];

    editingStudentID = id;

    submitButton.textContent = "Save Changes";
    studentModal.style.display = "flex";
}


// ==========================================
// 11. ADD AND EDIT STUDENT
// ==========================================

studentForm.addEventListener("submit", function(event) {
    event.preventDefault();

    const id = Number(document.getElementById("studentID").value);
    const name = document.getElementById("studentName").value.trim();
    const age = Number(document.getElementById("studentAge").value);

    const math = Number(document.getElementById("math").value);
    const english = Number(document.getElementById("english").value);
    const physics = Number(document.getElementById("physics").value);

    if (
        !Number.isInteger(id) || id < 1 ||
        name === "" ||
        !Number.isInteger(age) || age < 1 || age > 120 ||
        ![math, english, physics].every(
            grade => Number.isFinite(grade) && grade >= 0 && grade <= 10
        )
    ) {
        alert("Please enter valid student information!");
        return;
    }

    const duplicateID = students.some(student =>
        student.id === id && student.id !== editingStudentID
    );

    if (duplicateID) {
        alert("Student ID already exists!");
        return;
    }

    if (editingStudentID !== null) {
        const student = students.find(
            student => student.id === editingStudentID
        );

        if (!student) {
            alert("Student not found!");
            return;
        }

        student.id = id;
        student.name = name;
        student.age = age;
        student.grades = [math, english, physics];

        alert("Student updated locally!");

    } else {
        if (students.length >= 100) {
            alert("Maximum number of students reached!");
            return;
        }

        students.push({
            id,
            name,
            age,
            grades: [math, english, physics]
        });

        alert("Student added locally!");
    }

    editingStudentID = null;

    displayStudents();
    updateDashboard();
    updateAnalytics();

    studentForm.reset();
    submitButton.textContent = "Add Student";
    studentModal.style.display = "none";
});


const navigationLinks = document.querySelectorAll(".nav-link");

const pageSections = document.querySelectorAll(".page-section");

function showPage(pageID) {

    pageSections.forEach(section => {
        section.hidden = true;
    });

    const selectedPage = document.getElementById(pageID);

    if (!selectedPage) {
        console.error("Page not found:", pageID);
        return;
    }

    selectedPage.hidden = false;

    navigationLinks.forEach(link => {
        link.classList.toggle(
            "active",
            link.getAttribute("href") === "#" + pageID
        );
    });
}

navigationLinks.forEach(link => {

    link.addEventListener("click", function(event) {

        event.preventDefault();

        const pageID = this.getAttribute("href").substring(1);

        showPage(pageID);

        history.replaceState(null, "", "#" + pageID);
    });

});

showPage("dashboard");


// ==========================================
// 13. INITIALIZE APPLICATION
// ==========================================

loadStudents();
