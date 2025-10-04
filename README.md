# 📚 Government Books Management & Distribution System (CLI)

A **C++17 console-based application** designed for managing, distributing, and tracking government-provided books across **schools, colleges, universities, libraries, and research centers**.  
The project uses **Object-Oriented Programming principles, design patterns, concurrency control, and CLI-based interaction** to simulate a real-world distribution system.

---

## ✨ Features (Elaborated)

### 📖 Book Inventory Management
- Add new books with ISBN, title, author, category, publication year, and publisher.  
- Track **quantity per ISBN** and **category-wise distribution**.  
- Allocate and return books to/from institutions.  
- Export full inventory into CSV format.  
- Transaction logs maintained for all add/allocate operations.

### 🏫 Institutions & Users
- Register different institution types (Primary Schools, Colleges, Universities, Libraries, Research Centers).  
- Each institution maintains:  
  - Student count  
  - Current book stock  
  - Pending and fulfilled requests  
- Register and authenticate **users**:  
  - Roles: Admin, Librarian, Institution Head, Student  
  - Default Admin account: `admin/admin123`  

### 📦 Book Requests & Loans
- Institutions can submit book requests with **priority levels**: Low, Medium, High, Critical.  
- Requests may be **fulfilled, partially fulfilled, or queued**.  
- Issued requests are tracked as **loans**.  
- Manage **overdue loans** and enforce returns.  
- A **waiting list system** handles cases when demand > supply.

### ⚖️ Distribution Strategies
Implements the **Strategy Pattern** to provide multiple distribution approaches:
1. **Priority-Based** – Serve critical requests first using a priority queue.  
2. **Need-Based** – Proportional allocation depending on institution need and student count.  
3. **Equal Distribution** – Distribute equally among all institutions that request a book.

The distribution strategy can be switched at runtime.

### 📊 Analytics & Reporting
- Institution-level analytics: student needs vs allocated books.  
- Central analytics engine to generate distribution reports.  
- Track **fulfilled, partially fulfilled, and pending requests**.  
- Export reports (CSV) for further processing or auditing.

### 🔒 User Authentication & Security
- Login system with role-based interaction.  
- Admins can manage users and institutions.  
- Librarians and Heads handle requests.  
- Students primarily issue and return books.

### 🛠️ Utilities & Extras
- **Logger** (Singleton) with levels: INFO, WARNING, ERROR – stored in `system.log`.  
- **Persistence**: Save/load system state via `system_state.txt`.  
- **Notifications**: Print alerts for overdue loans and request approvals.  

---

## 🖥️ CLI Menu

```
============================================================
         GOVERNMENT BOOKS MANAGEMENT SYSTEM
============================================================
1.  Add Book to Inventory
2.  Register Institution
3.  Register User
4.  Submit Book Request
5.  Choose Distribution Strategy
6.  Run Distribution Cycle
7.  Display System Status
8.  Search Books
9.  View All Loans
10. View Overdue Loans
11. Return Books
12. View Waiting List
13. View Transaction Log
14. Export Reports (CSV)
15. User Login
q.  Quit
============================================================
```

---

## 🚀 Getting Started

### 🔧 Build & Run

```bash
# Compile
g++ -std=c++17 complete.cpp -o books_system

# Run
./books_system
```

### 🧑 Default Admin
- **User ID:** `admin`  
- **Password:** `admin123`  

---

## 📊 Example Workflow

1. Admin logs in and **adds books** to the inventory.  
2. Admin registers **institutions** (schools, colleges, libraries).  
3. Institutions submit book requests with defined priorities.  
4. Admin selects a **distribution strategy** (priority/need/equal).  
5. Run a **distribution cycle** to allocate books.  
6. View or export **distribution reports**.  
7. Manage **loans and overdue returns**.  
8. Save/export reports for auditing and future analysis.

---

## 🏗️ Design Patterns Implemented

- **Strategy Pattern** → Distribution algorithms (Priority, Need, Equal).  
- **Observer-like Logging** → Transaction logs & notifications.  
- **Singleton Pattern** → Centralized Logger instance.  
- **Facade Pattern** → `GovernmentBooksManagementSystem` acts as main entry point.  
- **Composite Pattern** → Institution manages multiple requests and holdings.

---

## 📂 Project Structure

```
complete.cpp             # Main project source file
system.log               # Auto-generated runtime logs
inventory_report.csv     # Exported inventory report
distribution_report.csv  # Exported distribution report
system_state.txt         # Saved system state (persistence)
README.md                # Project documentation
```

---

 

 
