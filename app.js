
// ==========================================
// 1. STUDENT DATA & API
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
// 3. API HELPER
// ==========================================

async function apiRequest(endpoint, method = "GET", body = null) {

    const options = {
        method,
        headers: {
            "Content-Type": "application/json"
        }
    };

    if (body !== null) {
        options.body = JSON.stringify(body);
    }

    const response = await fetch(`${API_URL}${endpoint}`, options);

    const data = await response.json();

    if (!response.ok || data.status !== 1) {
        throw new Error(data.message || `Request failed: ${response.status}`);
    }

    return data;
}


// ==========================================
// 4. LOAD STUDENTS FROM C BACKEND
// ==========================================

async function loadStudents() {

    try {

        const data = await apiRequest("/students");

        if (!Array.isArray(data.students)) {
            throw new Error("Invalid student data received from server.");
        }

        students = data.students;

        refreshApplication();

        console.log("Students loaded:", students);

    } catch (error) {

        console.error("Loading students failed:", error);

        alert("Cannot load students: " + error.message);
    }
}


// ==========================================
// 5. REFRESH APPLICATION
// ==========================================

function refreshApplication() {

    displayStudents();
    updateDashboard();
    updateAnalytics();
}


// ==========================================
// 6. CALCULATE STUDENT AVERAGE
// ==========================================

function calculateAverage(student) {

    if (!Array.isArray(student.grades) || student.grades.length === 0) {
        return 0;
    }

    const total = student.grades.reduce(
        (sum, grade) => sum + Number(grade),
        0
    );

    return total / student.grades.length;
}


// ==========================================
// 7. ESCAPE HTML
// ==========================================

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
// 8. DISPLAY STUDENTS
// ==========================================

function displayStudents(list = students) {

    const table = document.getElementById("studentTable");
    const emptyMessage = document.getElementById("emptyMessage");

    if (!table) {
        console.error("HTML element #studentTable not found!");
        return;
    }

    table.innerHTML = "";

    if (list.length === 0) {

        if (emptyMessage) {
            emptyMessage.style.display = "block";
        }

        return;
    }

    if (emptyMessage) {
        emptyMessage.style.display = "none";
    }

    list.forEach(student => {

        const grades = student.grades || [0, 0, 0];
        const average = calculateAverage(student);

        const row = document.createElement("tr");

        row.innerHTML = `
            <td>${student.id}</td>

            <td>${escapeHTML(student.name)}</td>

            <td>${student.age}</td>

            <td>${Number(grades[0]).toFixed(2)}</td>

            <td>${Number(grades[1]).toFixed(2)}</td>

            <td>${Number(grades[2]).toFixed(2)}</td>

            <td>${average.toFixed(2)}</td>

            <td class="action-buttons">

                <button
                    class="edit-btn"
                    onclick="editStudent(${student.id})">
                    Edit
                </button>

                <button
                    class="delete-btn"
                    onclick="deleteStudent(${student.id})">
                    Delete
                </button>

            </td>
        `;

        table.appendChild(row);
    });
}


// ==========================================
// 9. UPDATE DASHBOARD
// ==========================================

function updateDashboard() {

    const totalElement = document.getElementById("totalStudents");
    const averageElement = document.getElementById("averageGrade");
    const topElement = document.getElementById("topStudent");

    if (!totalElement || !averageElement || !topElement) {
        console.error("Dashboard HTML elements are missing!");
        return;
    }

    totalElement.textContent = students.length;

    if (students.length === 0) {

        averageElement.textContent = "0.00";
        topElement.textContent = "N/A";

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

    averageElement.textContent =
        (total / students.length).toFixed(2);

    topElement.textContent = topStudent.name;
}


// ==========================================
// 10. UPDATE ANALYTICS
// ==========================================

function updateAnalytics() {

    const subjects = ["math", "english", "physics"];

    if (students.length === 0) {

        subjects.forEach(subject => {

            const averageElement =
                document.getElementById(subject + "Average");

            const barElement =
                document.getElementById(subject + "Bar");

            const percentElement =
                document.getElementById(subject + "Percent");

            if (averageElement) averageElement.textContent = "0.00";
            if (barElement) barElement.style.width = "0%";
            if (percentElement) percentElement.textContent = "0%";

        });

        return;
    }

    const totals = [0, 0, 0];

    students.forEach(student => {

        student.grades.forEach((grade, index) => {

            if (index < 3) {
                totals[index] += Number(grade);
            }

        });
    });

    subjects.forEach((subject, index) => {

        const average = totals[index] / students.length;
        const percentage = (average / 10) * 100;

        const averageElement =
            document.getElementById(subject + "Average");

        const barElement =
            document.getElementById(subject + "Bar");

        const percentElement =
            document.getElementById(subject + "Percent");

        if (averageElement) {
            averageElement.textContent = average.toFixed(2);
        }

        if (barElement) {
            barElement.style.width = percentage + "%";
        }

        if (percentElement) {
            percentElement.textContent =
                percentage.toFixed(0) + "%";
        }

    });
}


// ==========================================
// 11. SEARCH STUDENTS
// ==========================================

const searchInput = document.getElementById("searchInput");

if (searchInput) {

    searchInput.addEventListener("input", function() {

        const searchValue = this.value.toLowerCase();

        const filteredStudents = students.filter(student =>

            student.name.toLowerCase().includes(searchValue) ||

            student.id.toString().includes(searchValue)

        );

        displayStudents(filteredStudents);

    });
}


// ==========================================
// 12. OPEN AND CLOSE MODAL
// ==========================================

if (openModalButton) {

    openModalButton.addEventListener("click", function() {

        editingStudentID = null;

        studentForm.reset();

        submitButton.textContent = "Add Student";

        studentModal.style.display = "flex";

    });
}


if (closeModalButton) {

    closeModalButton.addEventListener("click", function() {

        studentModal.style.display = "none";

    });
}


// ==========================================
// 13. EDIT STUDENT
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
// 14. DELETE STUDENT
// ==========================================

async function deleteStudent(id) {

    const student = students.find(s => s.id === id);

    if (!student) {

        alert("Student not found!");

        return;
    }

    const confirmed = confirm(
        `Are you sure you want to delete ${student.name}?`
    );

    if (!confirmed) return;

    try {

        await apiRequest(`/students?id=${id}`, "DELETE");

        alert("Student deleted successfully!");

        await loadStudents();

    } catch (error) {

        console.error("Delete error:", error);

        alert(error.message);
    }
}


// ==========================================
// 15. ADD AND EDIT STUDENT
// ==========================================

if (studentForm) {

    studentForm.addEventListener("submit", async function(event) {

        event.preventDefault();

        const id = Number(
            document.getElementById("studentID").value
        );

        const name =
            document.getElementById("studentName").value.trim();

        const age = Number(
            document.getElementById("studentAge").value
        );

        const math = Number(
            document.getElementById("math").value
        );

        const english = Number(
            document.getElementById("english").value
        );

        const physics = Number(
            document.getElementById("physics").value
        );


        // Validate information

        if (
            !Number.isInteger(id) || id < 1 ||
            name === "" ||
            !Number.isInteger(age) || age < 1 || age > 120 ||
            ![math, english, physics].every(
                grade =>
                    Number.isFinite(grade) &&
                    grade >= 0 &&
                    grade <= 10
            )
        ) {

            alert("Please enter valid student information!");

            return;
        }


        // Check duplicate ID

        const duplicateID = students.some(student =>

            student.id === id &&
            student.id !== editingStudentID

        );

        if (duplicateID) {

            alert("Student ID already exists!");

            return;
        }


        // Prepare student data

        const studentData = {

            id,
            name,
            age,

            grades: [math, english, physics]

        };


        try {

            submitButton.disabled = true;

            submitButton.textContent = "Saving...";


            if (editingStudentID !== null) {

                // UPDATE STUDENT

                await apiRequest(
                    `/students?id=${editingStudentID}`,
                    "PUT",
                    studentData
                );

                alert("Student updated successfully!");

            } else {

                // ADD STUDENT

                await apiRequest(
                    "/students",
                    "POST",
                    studentData
                );

                alert("Student added successfully!");

            }


            editingStudentID = null;

            studentForm.reset();

            studentModal.style.display = "none";

            await loadStudents();

        } catch (error) {

            console.error("Save error:", error);

            alert(error.message);

        } finally {

            submitButton.disabled = false;

            submitButton.textContent = "Add Student";

        }

    });
}


// ==========================================
// 16. NAVIGATION
// ==========================================

const navigationLinks =
    document.querySelectorAll(".nav-link");

const pageSections =
    document.querySelectorAll(".page-section");


function showPage(pageID) {

    pageSections.forEach(section => {

        section.hidden = true;

    });

    const selectedPage =
        document.getElementById(pageID);

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

        const pageID =
            this.getAttribute("href").substring(1);

        showPage(pageID);

        history.replaceState(null, "", "#" + pageID);

    });

});


// ==========================================
// 17. INITIALIZE APPLICATION
// ==========================================

const initialPage =
    window.location.hash.substring(1) || "dashboard";

showPage(initialPage);

loadStudents();
