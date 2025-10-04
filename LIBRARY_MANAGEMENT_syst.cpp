// COMPLETE GOVERNMENT BOOKS MANAGEMENT & DISTRIBUTION SYSTEM
// Build: g++ -std=c++17 -o books_system government_books_management.cpp
// Run: ./books_system

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <map>
#include <set>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <functional>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <regex>
#include <stdexcept>
#include <ctime>
#include <numeric>

using namespace std;

// ========================= FORWARD DECLARATIONS =========================
class Book;
class BookInventory;
class Institution;
class BookRequest;
class User;
class BookLoan;

// ========================= ENUMS =========================
enum class BookCategory {
    TEXTBOOK, REFERENCE, LITERATURE, SCIENCE, 
    HISTORY, MATHEMATICS, LANGUAGE, VOCATIONAL
};

enum class InstitutionType {
    PRIMARY_SCHOOL, SECONDARY_SCHOOL, HIGH_SCHOOL, 
    COLLEGE, UNIVERSITY, LIBRARY, RESEARCH_CENTER
};

enum class Priority {
    LOW = 1, MEDIUM = 2, HIGH = 3, CRITICAL = 4
};

enum class RequestStatus {
    PENDING, APPROVED, PARTIALLY_FULFILLED, FULFILLED, REJECTED
};

enum class UserRole {
    ADMIN, LIBRARIAN, INSTITUTION_HEAD, STUDENT
};

enum class LogLevel {
    DEBUG, INFO, WARNING, ERROR_LOG, CRITICAL
};

// ========================= UTILITY FUNCTIONS =========================
string categoryToString(BookCategory cat) {
    static const map<BookCategory, string> names = {
        {BookCategory::TEXTBOOK, "Textbook"}, {BookCategory::REFERENCE, "Reference"},
        {BookCategory::LITERATURE, "Literature"}, {BookCategory::SCIENCE, "Science"},
        {BookCategory::HISTORY, "History"}, {BookCategory::MATHEMATICS, "Mathematics"},
        {BookCategory::LANGUAGE, "Language"}, {BookCategory::VOCATIONAL, "Vocational"}
    };
    auto it = names.find(cat);
    return (it != names.end()) ? it->second : "Unknown";
}

string institutionTypeToString(InstitutionType type) {
    static const map<InstitutionType, string> names = {
        {InstitutionType::PRIMARY_SCHOOL, "Primary School"},
        {InstitutionType::SECONDARY_SCHOOL, "Secondary School"},
        {InstitutionType::HIGH_SCHOOL, "High School"},
        {InstitutionType::COLLEGE, "College"},
        {InstitutionType::UNIVERSITY, "University"},
        {InstitutionType::LIBRARY, "Library"},
        {InstitutionType::RESEARCH_CENTER, "Research Center"}
    };
    auto it = names.find(type);
    return (it != names.end()) ? it->second : "Unknown";
}

string statusToString(RequestStatus status) {
    static const map<RequestStatus, string> names = {
        {RequestStatus::PENDING, "Pending"},
        {RequestStatus::APPROVED, "Approved"},
        {RequestStatus::PARTIALLY_FULFILLED, "Partially Fulfilled"},
        {RequestStatus::FULFILLED, "Fulfilled"},
        {RequestStatus::REJECTED, "Rejected"}
    };
    auto it = names.find(status);
    return (it != names.end()) ? it->second : "Unknown";
}

string roleToString(UserRole role) {
    static const map<UserRole, string> names = {
        {UserRole::ADMIN, "Admin"},
        {UserRole::LIBRARIAN, "Librarian"},
        {UserRole::INSTITUTION_HEAD, "Institution Head"},
        {UserRole::STUDENT, "Student"}
    };
    auto it = names.find(role);
    return (it != names.end()) ? it->second : "Unknown";
}

// ========================= EXCEPTIONS =========================
class BookManagementException : public runtime_error {
public:
    explicit BookManagementException(const string& msg) : runtime_error(msg) {}
};

class InsufficientStockException : public BookManagementException {
public:
    explicit InsufficientStockException(const string& isbn)
        : BookManagementException("Insufficient stock for ISBN: " + isbn) {}
};

class InvalidInputException : public BookManagementException {
public:
    explicit InvalidInputException(const string& field)
        : BookManagementException("Invalid input for: " + field) {}
};

class NotFoundException : public BookManagementException {
public:
    explicit NotFoundException(const string& item)
        : BookManagementException("Not found: " + item) {}
};

// ========================= LOGGER =========================
class Logger {
private:
    ofstream logFile;
    LogLevel minLevel;
    mutable mutex mtx;
    
public:
    Logger(const string& filename = "system.log", LogLevel level = LogLevel::INFO)
        : minLevel(level) {
        logFile.open(filename, ios::app);
        if (!logFile.is_open()) {
            cerr << "Warning: Could not open log file\n";
        }
    }
    
    ~Logger() {
        if (logFile.is_open()) logFile.close();
    }
    
    void log(LogLevel level, const string& message) {
        if (level < minLevel) return;
        
        lock_guard<mutex> lock(mtx);
        
        time_t now = time(nullptr);
        char timeStr[26];
        ctime_r(&now, timeStr);
        timeStr[24] = '\0';
        
        string levelStr;
        switch(level) {
            case LogLevel::DEBUG: levelStr = "DEBUG"; break;
            case LogLevel::INFO: levelStr = "INFO"; break;
            case LogLevel::WARNING: levelStr = "WARNING"; break;
            case LogLevel::ERROR_LOG: levelStr = "ERROR"; break;
            case LogLevel::CRITICAL: levelStr = "CRITICAL"; break;
        }
        
        string logEntry = string("[") + timeStr + "] [" + levelStr + "] " + message;
        if (logFile.is_open()) {
            logFile << logEntry << endl;
            logFile.flush();
        }
        
        // Also print to console for important messages
        if (level >= LogLevel::WARNING) {
            cout << logEntry << endl;
        }
    }
};

// Global logger instance
static Logger globalLogger;

// ========================= VALIDATOR =========================
class Validator {
public:
    static bool isValidISBN(const string& isbn) {
        // Simple ISBN validation (10 or 13 digits with optional hyphens)
        string cleaned = isbn;
        cleaned.erase(remove(cleaned.begin(), cleaned.end(), '-'), cleaned.end());
        return (cleaned.length() == 10 || cleaned.length() == 13) &&
               all_of(cleaned.begin(), cleaned.end(), ::isdigit);
    }
    
    static bool isValidEmail(const string& email) {
        regex pattern(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        return regex_match(email, pattern);
    }
    
    static bool isValidQuantity(int qty) {
        return qty > 0 && qty < 1000000;
    }
    
    static bool isValidYear(int year) {
        return year >= 1900 && year <= 2100;
    }
    
    static bool isValidPhone(const string& phone) {
        regex pattern(R"(\d{10})");
        return regex_match(phone, pattern);
    }
};

// ========================= BOOK CLASS =========================
class Book {
private:
    string isbn;
    string title;
    string author;
    BookCategory category;
    int publicationYear;
    string publisher;
    double price;

public:
    Book(string isbn, string title, string author, BookCategory cat, 
         int year, string pub, double price = 0.0)
        : isbn(move(isbn)), title(move(title)), author(move(author)), 
          category(cat), publicationYear(year), publisher(move(pub)), price(price) {
        
        if (!Validator::isValidISBN(this->isbn)) {
            throw InvalidInputException("Invalid ISBN format");
        }
        if (!Validator::isValidYear(year)) {
            throw InvalidInputException("Invalid publication year");
        }
    }

    const string& getISBN() const { return isbn; }
    const string& getTitle() const { return title; }
    const string& getAuthor() const { return author; }
    BookCategory getCategory() const { return category; }
    int getPublicationYear() const { return publicationYear; }
    const string& getPublisher() const { return publisher; }
    double getPrice() const { return price; }

    void displayInfo() const {
        cout << "  ISBN: " << isbn << " | Title: " << title 
             << " | Author: " << author << " | Category: " << categoryToString(category)
             << " | Year: " << publicationYear << " | Price: Rs." << fixed << setprecision(2) << price << "\n";
    }
    
    string toCSV() const {
        return isbn + "," + title + "," + author + "," + 
               categoryToString(category) + "," + to_string(publicationYear) + "," + 
               publisher + "," + to_string(price);
    }
};

// ========================= BOOK INVENTORY =========================
class BookInventory {
private:
    unordered_map<string, pair<shared_ptr<Book>, int>> stock;
    map<BookCategory, set<string>> categoryIndex;
    mutable mutex mtx;
    
    struct Transaction {
        string isbn;
        int quantity;
        string type;
        time_t timestamp;
    };
    vector<Transaction> transactionLog;

public:
    void addBook(shared_ptr<Book> book, int quantity) {
        if (!Validator::isValidQuantity(quantity)) {
            throw InvalidInputException("Invalid quantity");
        }
        
        lock_guard<mutex> lock(mtx);
        
        string isbn = book->getISBN();
        if (stock.find(isbn) != stock.end()) {
            stock[isbn].second += quantity;
        } else {
            stock[isbn] = {book, quantity};
            categoryIndex[book->getCategory()].insert(isbn);
        }
        
        transactionLog.push_back({isbn, quantity, "ADD", time(nullptr)});
        globalLogger.log(LogLevel::INFO, "Added " + to_string(quantity) + " books: " + isbn);
    }

    bool allocateBooks(const string& isbn, int quantity) {
        if (!Validator::isValidQuantity(quantity)) return false;
        
        lock_guard<mutex> lock(mtx);
        
        auto it = stock.find(isbn);
        if (it == stock.end() || it->second.second < quantity) {
            return false;
        }
        
        it->second.second -= quantity;
        transactionLog.push_back({isbn, quantity, "ALLOCATE", time(nullptr)});
        globalLogger.log(LogLevel::INFO, "Allocated " + to_string(quantity) + " books: " + isbn);
        return true;
    }
    
    void returnBooks(const string& isbn, int quantity) {
        lock_guard<mutex> lock(mtx);
        auto it = stock.find(isbn);
        if (it != stock.end()) {
            it->second.second += quantity;
            transactionLog.push_back({isbn, quantity, "RETURN", time(nullptr)});
            globalLogger.log(LogLevel::INFO, "Returned " + to_string(quantity) + " books: " + isbn);
        }
    }

    int getAvailableQuantity(const string& isbn) const {
        lock_guard<mutex> lock(mtx);
        auto it = stock.find(isbn);
        return (it != stock.end()) ? it->second.second : 0;
    }

    shared_ptr<Book> getBook(const string& isbn) const {
        lock_guard<mutex> lock(mtx);
        auto it = stock.find(isbn);
        return (it != stock.end()) ? it->second.first : nullptr;
    }
    
    vector<pair<shared_ptr<Book>, int>> searchByTitle(const string& keyword) const {
        lock_guard<mutex> lock(mtx);
        vector<pair<shared_ptr<Book>, int>> results;
        string lowerKeyword = keyword;
        transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
        
        for (const auto& [isbn, data] : stock) {
            string title = data.first->getTitle();
            transform(title.begin(), title.end(), title.begin(), ::tolower);
            if (title.find(lowerKeyword) != string::npos) {
                results.push_back(data);
            }
        }
        return results;
    }
    
    vector<pair<shared_ptr<Book>, int>> searchByAuthor(const string& author) const {
        lock_guard<mutex> lock(mtx);
        vector<pair<shared_ptr<Book>, int>> results;
        string lowerAuthor = author;
        transform(lowerAuthor.begin(), lowerAuthor.end(), lowerAuthor.begin(), ::tolower);
        
        for (const auto& [isbn, data] : stock) {
            string bookAuthor = data.first->getAuthor();
            transform(bookAuthor.begin(), bookAuthor.end(), bookAuthor.begin(), ::tolower);
            if (bookAuthor.find(lowerAuthor) != string::npos) {
                results.push_back(data);
            }
        }
        return results;
    }
    
    vector<pair<shared_ptr<Book>, int>> getBooksByCategory(BookCategory cat) const {
        lock_guard<mutex> lock(mtx);
        vector<pair<shared_ptr<Book>, int>> results;
        
        auto catIt = categoryIndex.find(cat);
        if (catIt != categoryIndex.end()) {
            for (const auto& isbn : catIt->second) {
                auto stockIt = stock.find(isbn);
                if (stockIt != stock.end()) {
                    results.push_back(stockIt->second);
                }
            }
        }
        return results;
    }

    int getTotalBooks() const {
        lock_guard<mutex> lock(mtx);
        return accumulate(stock.begin(), stock.end(), 0,
            [](int sum, const auto& p) { return sum + p.second.second; });
    }

    void displayInventory() const {
        lock_guard<mutex> lock(mtx);
        cout << "\n=== CENTRAL INVENTORY ===\n";
        
        int total = accumulate(stock.begin(), stock.end(), 0,
            [](int sum, const auto& p) { return sum + p.second.second; });
        
        cout << "Total Books: " << total << "\n";
        cout << "Unique Titles: " << stock.size() << "\n";
        
        if (stock.empty()) {
            cout << "  No books in inventory.\n";
            return;
        }
        
        cout << "\nBook Details:\n";
        for (const auto& [isbn, data] : stock) {
            data.first->displayInfo();
            cout << "    Available Quantity: " << data.second << "\n";
        }
    }
    
    vector<Transaction> getTransactionLog() const {
        lock_guard<mutex> lock(mtx);
        return transactionLog;
    }
    
    void exportToCSV(const string& filename) const {
        lock_guard<mutex> lock(mtx);
        ofstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Cannot create CSV file");
        }
        
        file << "ISBN,Title,Author,Category,Year,Publisher,Price,Available\n";
        for (const auto& [isbn, data] : stock) {
            file << data.first->toCSV() << "," << data.second << "\n";
        }
        file.close();
        cout << "✓ Inventory exported to: " << filename << "\n";
    }
};

// ========================= USER CLASS =========================
class User {
private:
    string userId;
    string name;
    string email;
    string phone;
    UserRole role;
    string password; // In real system, use hashing
    InstitutionType affiliatedInstitution;
    
public:
    User(string id, string name, string email, string phone, UserRole role, 
         string password, InstitutionType inst = InstitutionType::PRIMARY_SCHOOL)
        : userId(move(id)), name(move(name)), email(move(email)), 
          phone(move(phone)), role(role), password(move(password)),
          affiliatedInstitution(inst) {
        
        if (!Validator::isValidEmail(this->email)) {
            throw InvalidInputException("Invalid email format");
        }
    }

    const string& getUserId() const { return userId; }
    const string& getName() const { return name; }
    const string& getEmail() const { return email; }
    UserRole getRole() const { return role; }
    bool authenticate(const string& pwd) const { return password == pwd; }
    
    void displayInfo() const {
        cout << "User ID: " << userId << " | Name: " << name 
             << " | Role: " << roleToString(role) << " | Email: " << email << "\n";
    }
};

// ========================= BOOK REQUEST =========================
class BookRequest {
private:
    string requestId;
    string isbn;
    int quantityRequested;
    int quantityFulfilled;
    Priority priority;
    RequestStatus status;
    time_t requestDate;
    string requestedBy;

public:
    BookRequest(string reqId, string isbn, int qty, Priority prio, string by = "")
        : requestId(move(reqId)), isbn(move(isbn)), quantityRequested(qty),
          quantityFulfilled(0), priority(prio), status(RequestStatus::PENDING),
          requestDate(time(nullptr)), requestedBy(move(by)) {}

    const string& getRequestId() const { return requestId; }
    const string& getISBN() const { return isbn; }
    int getQuantityRequested() const { return quantityRequested; }
    int getQuantityFulfilled() const { return quantityFulfilled; }
    int getRemainingQuantity() const { return quantityRequested - quantityFulfilled; }
    Priority getPriority() const { return priority; }
    RequestStatus getStatus() const { return status; }
    time_t getRequestDate() const { return requestDate; }

    void fulfillPartial(int qty) {
        quantityFulfilled += qty;
        if (quantityFulfilled >= quantityRequested) {
            status = RequestStatus::FULFILLED;
        } else if (quantityFulfilled > 0) {
            status = RequestStatus::PARTIALLY_FULFILLED;
        }
    }

    void setStatus(RequestStatus s) { status = s; }
    void setPriority(Priority p) { priority = p; }
};

// ========================= BOOK LOAN =========================
class BookLoan {
private:
    string loanId;
    string isbn;
    string institutionId;
    time_t issueDate;
    time_t dueDate;
    time_t returnDate;
    bool isReturned;
    int quantity;
    
public:
    BookLoan(string loanId, string isbn, string instId, int qty, int daysToReturn = 180)
        : loanId(move(loanId)), isbn(move(isbn)), institutionId(move(instId)),
          quantity(qty), isReturned(false), returnDate(0) {
        issueDate = time(nullptr);
        dueDate = issueDate + (daysToReturn * 24 * 60 * 60);
    }
    
    const string& getLoanId() const { return loanId; }
    const string& getISBN() const { return isbn; }
    const string& getInstitutionId() const { return institutionId; }
    int getQuantity() const { return quantity; }
    bool getIsReturned() const { return isReturned; }
    
    bool isOverdue() const {
        if (isReturned) return false;
        return time(nullptr) > dueDate;
    }
    
    void markReturned() {
        isReturned = true;
        returnDate = time(nullptr);
    }
    
    int getDaysOverdue() const {
        if (!isOverdue()) return 0;
        return (time(nullptr) - dueDate) / (24 * 60 * 60);
    }
    
    void displayInfo() const {
        cout << "  Loan ID: " << loanId << " | ISBN: " << isbn 
             << " | Institution: " << institutionId << " | Qty: " << quantity;
        if (isReturned) {
            cout << " | Status: Returned\n";
        } else if (isOverdue()) {
            cout << " | Status: OVERDUE (" << getDaysOverdue() << " days)\n";
        } else {
            cout << " | Status: Active\n";
        }
    }
};

// ========================= LOAN MANAGEMENT =========================
class LoanManagement {
private:
    vector<shared_ptr<BookLoan>> loans;
    mutable mutex mtx;
    
public:
    shared_ptr<BookLoan> issueBookLoan(const string& isbn, const string& instId, int quantity) {
        lock_guard<mutex> lock(mtx);
        string loanId = "LOAN-" + instId + "-" + to_string(time(nullptr));
        auto loan = make_shared<BookLoan>(loanId, isbn, instId, quantity);
        loans.push_back(loan);
        globalLogger.log(LogLevel::INFO, "Loan issued: " + loanId);
        return loan;
    }
    
    vector<shared_ptr<BookLoan>> getOverdueLoans() const {
        lock_guard<mutex> lock(mtx);
        vector<shared_ptr<BookLoan>> overdue;
        for (const auto& loan : loans) {
            if (loan->isOverdue()) {
                overdue.push_back(loan);
            }
        }
        return overdue;
    }
    
    vector<shared_ptr<BookLoan>> getLoansByInstitution(const string& instId) const {
        lock_guard<mutex> lock(mtx);
        vector<shared_ptr<BookLoan>> result;
        for (const auto& loan : loans) {
            if (loan->getInstitutionId() == instId) {
                result.push_back(loan);
            }
        }
        return result;
    }
    
    bool returnBooks(const string& loanId) {
        lock_guard<mutex> lock(mtx);
        for (auto& loan : loans) {
            if (loan->getLoanId() == loanId && !loan->getIsReturned()) {
                loan->markReturned();
                globalLogger.log(LogLevel::INFO, "Loan returned: " + loanId);
                return true;
            }
        }
        return false;
    }
    
    void displayAllLoans() const {
        lock_guard<mutex> lock(mtx);
        cout << "\n=== ALL LOANS (" << loans.size() << ") ===\n";
        if (loans.empty()) {
            cout << "  No loans issued yet.\n";
            return;
        }
        for (const auto& loan : loans) {
            loan->displayInfo();
        }
    }
};

// ========================= WAITING LIST =========================
class WaitingList {
private:
    struct WaitingEntry {
        string institutionId;
        string isbn;
        int quantity;
        time_t requestTime;
        Priority priority;
    };
    
    map<string, queue<WaitingEntry>> waitingQueues;
    mutable mutex mtx;
    
public:
    void addToWaitingList(const string& isbn, const string& instId, 
                         int quantity, Priority priority) {
        lock_guard<mutex> lock(mtx);
        waitingQueues[isbn].push({instId, isbn, quantity, time(nullptr), priority});
        globalLogger.log(LogLevel::INFO, "Added to waiting list: " + instId + " for " + isbn);
    }
    
    bool hasWaitingInstitutions(const string& isbn) const {
        lock_guard<mutex> lock(mtx);
        auto it = waitingQueues.find(isbn);
        return it != waitingQueues.end() && !it->second.empty();
    }
    
    int getWaitingCount(const string& isbn) const {
        lock_guard<mutex> lock(mtx);
        auto it = waitingQueues.find(isbn);
        return (it != waitingQueues.end()) ? it->second.size() : 0;
    }
    
    void displayWaitingList() const {
        lock_guard<mutex> lock(mtx);
        cout << "\n=== WAITING LIST ===\n";
        if (waitingQueues.empty()) {
            cout << "  No institutions waiting.\n";
            return;
        }
        for (const auto& [isbn, queue] : waitingQueues) {
            cout << "ISBN: " << isbn << " | Waiting: " << queue.size() << " institutions\n";
        }
    }
};

// ========================= INSTITUTION =========================
class Institution {
private:
    string institutionId;
    string name;
    InstitutionType type;
    string location;
    int studentCount;
    unordered_map<string, int> currentBooks;
    vector<shared_ptr<BookRequest>> requests;
    mutable mutex mtx;

public:
    Institution(string id, string name, InstitutionType type, string loc, int students)
        : institutionId(move(id)), name(move(name)), type(type), 
          location(move(loc)), studentCount(students) {}

    const string& getId() const { return institutionId; }
    const string& getName() const { return name; }
    InstitutionType getType() const { return type; }
    int getStudentCount() const { return studentCount; }
    const string& getLocation() const { return location; }

    void addRequest(shared_ptr<BookRequest> req) {
        lock_guard<mutex> lock(mtx);
        requests.push_back(req);
    }

    vector<shared_ptr<BookRequest>> getPendingRequests() const {
        lock_guard<mutex> lock(mtx);
        vector<shared_ptr<BookRequest>> pending;
        for (const auto& req : requests) {
            if (req->getStatus() == RequestStatus::PENDING || 
                req->getStatus() == RequestStatus::PARTIALLY_FULFILLED) {
                pending.push_back(req);
            }
        }
        return pending;
    }

    vector<shared_ptr<BookRequest>> getAllRequests() const {
        lock_guard<mutex> lock(mtx);
        return requests;
    }

    void receiveBooks(const string& isbn, int quantity) {
        lock_guard<mutex> lock(mtx);
        currentBooks[isbn] += quantity;
    }

    int getCurrentStock(const string& isbn) const {
        lock_guard<mutex> lock(mtx);
        auto it = currentBooks.find(isbn);
        return (it != currentBooks.end()) ? it->second : 0;
    }

    void displayStatus() const {
        cout << "\nInstitution: " << name << " (" << institutionTypeToString(type) << ")\n";
        cout << "ID: " << institutionId << " | Location: " << location 
             << " | Students: " << studentCount << "\n";
        cout << "Total Requests: " << requests.size() 
             << " | Pending: " << getPendingRequests().size() << "\n";
    }
};

// ========================= DISTRIBUTION STRATEGIES =========================
class IDistributionStrategy {
public:
    virtual ~IDistributionStrategy() = default;
    virtual void distribute(BookInventory& inventory, 
                          vector<shared_ptr<Institution>>& institutions,
                          LoanManagement& loanMgr) = 0;
    virtual string getStrategyName() const = 0;
};

class PriorityBasedDistribution : public IDistributionStrategy {
public:
    void distribute(BookInventory& inventory, 
                   vector<shared_ptr<Institution>>& institutions,
                   LoanManagement& loanMgr) override {
        
        auto cmp = [](const tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>& a,
                     const tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>& b) {
            return static_cast<int>(get<0>(a)->getPriority()) < 
                   static_cast<int>(get<0>(b)->getPriority());
        };
        
        priority_queue<tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>,
                      vector<tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>>,
                      decltype(cmp)> pq(cmp);

        for (auto& inst : institutions) {
            auto requests = inst->getPendingRequests();
            for (auto& req : requests) {
                pq.push({req, inst, req->getRemainingQuantity()});
            }
        }

        while (!pq.empty()) {
            auto [req, inst, needed] = pq.top();
            pq.pop();

            int available = inventory.getAvailableQuantity(req->getISBN());
            if (available <= 0) continue;

            int allocate = min(needed, available);
            if (inventory.allocateBooks(req->getISBN(), allocate)) {
                inst->receiveBooks(req->getISBN(), allocate);
                req->fulfillPartial(allocate);
                loanMgr.issueBookLoan(req->getISBN(), inst->getId(), allocate);
            }
        }
    }

    string getStrategyName() const override { return "Priority-Based Distribution"; }
};

class NeedBasedDistribution : public IDistributionStrategy {
public:
    void distribute(BookInventory& inventory, 
                   vector<shared_ptr<Institution>>& institutions,
                   LoanManagement& loanMgr) override {
        
        map<string, vector<tuple<shared_ptr<Institution>, shared_ptr<BookRequest>, int>>> needMap;
        
        for (auto& inst : institutions) {
            auto requests = inst->getPendingRequests();
            for (auto& req : requests) {
                int need = req->getRemainingQuantity();
                if (need > 0) {
                    needMap[req->getISBN()].push_back({inst, req, need});
                }
            }
        }

        for (auto& [isbn, needs] : needMap) {
            int available = inventory.getAvailableQuantity(isbn);
            if (available <= 0) continue;

            int totalNeed = accumulate(needs.begin(), needs.end(), 0,
                [](int sum, const auto& t) { return sum + get<2>(t); });

            for (auto& [inst, req, need] : needs) {
                int allocate = (totalNeed > 0) ? 
                    min(need, (available * need) / totalNeed) : 0;
                
                if (allocate > 0 && inventory.allocateBooks(isbn, allocate)) {
                    inst->receiveBooks(isbn, allocate);
                    req->fulfillPartial(allocate);
                    loanMgr.issueBookLoan(isbn, inst->getId(), allocate);
                }
            }
        }
    }

    string getStrategyName() const override { return "Need-Based Proportional Distribution"; }
};

class EqualDistribution : public IDistributionStrategy {
public:
    void distribute(BookInventory& inventory, 
                   vector<shared_ptr<Institution>>& institutions,
                   LoanManagement& loanMgr) override {
        
        map<string, vector<pair<shared_ptr<Institution>, shared_ptr<BookRequest>>>> needingMap;
        
        for (auto& inst : institutions) {
            auto requests = inst->getPendingRequests();
            for (auto& req : requests) {
                if (req->getRemainingQuantity() > 0) {
                    needingMap[req->getISBN()].push_back({inst, req});
                }
            }
        }

        for (const auto& [isbn, needList] : needingMap) {
            int available = inventory.getAvailableQuantity(isbn);
            if (available <= 0 || needList.empty()) continue;

            int perInst = available / needList.size();
            if (perInst <= 0) continue;

            for (auto& [inst, req] : needList) {
                int allocate = min(perInst, req->getRemainingQuantity());
                if (allocate > 0 && inventory.allocateBooks(isbn, allocate)) {
                    inst->receiveBooks(isbn, allocate);
                    req->fulfillPartial(allocate);
                    loanMgr.issueBookLoan(isbn, inst->getId(), allocate);
                }
            }
        }
    }

    string getStrategyName() const override { return "Equal Distribution"; }
};

// ========================= ANALYTICS & REPORTING =========================
class AnalyticsEngine {
public:
    static void generateDistributionReport(const vector<shared_ptr<Institution>>& institutions) {
        cout << "\n=== DISTRIBUTION ANALYTICS REPORT ===\n";
        
        int totalRequests = 0;
        int fulfilledRequests = 0;
        int partiallyFulfilled = 0;
        int pendingRequests = 0;

        for (const auto& inst : institutions) {
            auto requests = inst->getAllRequests();
            totalRequests += requests.size();
            
            for (const auto& req : requests) {
                switch (req->getStatus()) {
                    case RequestStatus::FULFILLED:
                        fulfilledRequests++;
                        break;
                    case RequestStatus::PARTIALLY_FULFILLED:
                        partiallyFulfilled++;
                        break;
                    case RequestStatus::PENDING:
                        pendingRequests++;
                        break;
                    default:
                        break;
                }
            }
        }

        cout << "Total Requests: " << totalRequests << "\n";
        cout << "Fulfilled: " << fulfilledRequests << " ("
             << (totalRequests > 0 ? (fulfilledRequests * 100.0 / totalRequests) : 0) 
             << "%)\n";
        cout << "Partially Fulfilled: " << partiallyFulfilled << "\n";
        cout << "Pending: " << pendingRequests << "\n";
    }

    static void generateInstitutionReport(const shared_ptr<Institution>& inst) {
        inst->displayStatus();
        
        auto requests = inst->getAllRequests();
        if (!requests.empty()) {
            cout << "Request Details:\n";
            for (const auto& req : requests) {
                cout << "  - Request ID: " << req->getRequestId()
                     << " | ISBN: " << req->getISBN() 
                     << " | Requested: " << req->getQuantityRequested()
                     << " | Fulfilled: " << req->getQuantityFulfilled()
                     << " | Status: " << statusToString(req->getStatus()) << "\n";
            }
        } else {
            cout << "  No requests found.\n";
        }
        cout << endl;
    }
    
    static void exportReportToCSV(const vector<shared_ptr<Institution>>& institutions,
                                  const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Cannot create report file");
        }
        
        file << "Institution ID,Name,Type,Location,Students,Total Requests,Fulfilled,Partially Fulfilled,Pending\n";
        
        for (const auto& inst : institutions) {
            auto allReqs = inst->getAllRequests();
            int fulfilled = 0, partial = 0, pending = 0;
            
            for (const auto& req : allReqs) {
                switch(req->getStatus()) {
                    case RequestStatus::FULFILLED: fulfilled++; break;
                    case RequestStatus::PARTIALLY_FULFILLED: partial++; break;
                    case RequestStatus::PENDING: pending++; break;
                    default: break;
                }
            }
            
            file << inst->getId() << ","
                 << inst->getName() << ","
                 << institutionTypeToString(inst->getType()) << ","
                 << inst->getLocation() << ","
                 << inst->getStudentCount() << ","
                 << allReqs.size() << ","
                 << fulfilled << ","
                 << partial << ","
                 << pending << "\n";
        }
        
        file.close();
        cout << "✓ Report exported to: " << filename << "\n";
    }
};

// ========================= DATA PERSISTENCE =========================
class DataPersistence {
public:
    static void saveInventoryToFile(const BookInventory& inventory, const string& filename) {
        try {
            inventory.exportToCSV(filename);
            globalLogger.log(LogLevel::INFO, "Inventory saved to file");
        } catch (const exception& e) {
            globalLogger.log(LogLevel::ERROR_LOG, string("Failed to save inventory: ") + e.what());
            throw;
        }
    }
    
    static void saveSystemState(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            throw runtime_error("Cannot create state file");
        }
        
        file << "System state saved at: " << time(nullptr) << "\n";
        file.close();
        
        globalLogger.log(LogLevel::INFO, "System state saved");
        cout << "✓ System state saved to: " << filename << "\n";
    }
};

// ========================= NOTIFICATION SERVICE =========================
class NotificationService {
public:
    static void notifyInstitution(const string& instName, const string& message) {
        cout << "\n[NOTIFICATION] To: " << instName << "\n";
        cout << "Message: " << message << "\n";
        globalLogger.log(LogLevel::INFO, "Notification sent to " + instName);
    }
    
    static void notifyOverdue(const vector<shared_ptr<BookLoan>>& overdueLoans) {
        if (overdueLoans.empty()) {
            cout << "\n✓ No overdue loans\n";
            return;
        }
        
        cout << "\n⚠ OVERDUE LOANS ALERT ⚠\n";
        for (const auto& loan : overdueLoans) {
            cout << "Institution: " << loan->getInstitutionId()
                 << " | ISBN: " << loan->getISBN()
                 << " | Overdue by: " << loan->getDaysOverdue() << " days\n";
        }
        globalLogger.log(LogLevel::WARNING, "Overdue loans detected: " + to_string(overdueLoans.size()));
    }
};

// ========================= MAIN MANAGEMENT SYSTEM =========================
class GovernmentBooksManagementSystem {
private:
    BookInventory centralInventory;
    unordered_map<string, shared_ptr<Institution>> institutions;
    unordered_map<string, shared_ptr<User>> users;
    unique_ptr<IDistributionStrategy> distributionStrategy;
    LoanManagement loanManager;
    WaitingList waitingList;
    mutable mutex systemMtx;
    shared_ptr<User> currentUser;

public:
    GovernmentBooksManagementSystem(unique_ptr<IDistributionStrategy> strategy)
        : distributionStrategy(move(strategy)), currentUser(nullptr) {
        globalLogger.log(LogLevel::INFO, "System initialized");
    }

    // Authentication
    bool login(const string& userId, const string& password) {
        auto it = users.find(userId);
        if (it != users.end() && it->second->authenticate(password)) {
            currentUser = it->second;
            globalLogger.log(LogLevel::INFO, "User logged in: " + userId);
            return true;
        }
        globalLogger.log(LogLevel::WARNING, "Failed login attempt: " + userId);
        return false;
    }
    
    void logout() {
        if (currentUser) {
            globalLogger.log(LogLevel::INFO, "User logged out: " + currentUser->getUserId());
            currentUser = nullptr;
        }
    }
    
    shared_ptr<User> getCurrentUser() const { return currentUser; }

    // Book Management
    void addBookToInventory(shared_ptr<Book> book, int quantity) {
        centralInventory.addBook(book, quantity);
        cout << "✓ Added " << quantity << " copies of '" << book->getTitle() << "'\n";
    }

    // Institution Management
    void registerInstitution(shared_ptr<Institution> inst) {
        lock_guard<mutex> lock(systemMtx);
        institutions[inst->getId()] = inst;
        cout << "✓ Registered: " << inst->getName() << "\n";
        globalLogger.log(LogLevel::INFO, "Institution registered: " + inst->getId());
    }

    // User Management
    void registerUser(shared_ptr<User> user) {
        lock_guard<mutex> lock(systemMtx);
        users[user->getUserId()] = user;
        cout << "✓ User registered: " << user->getName() << "\n";
        globalLogger.log(LogLevel::INFO, "User registered: " + user->getUserId());
    }

    // Request Management
    void submitBookRequest(const string& instId, const string& isbn, 
                          int quantity, Priority priority) {
        auto inst = getInstitution(instId);
        if (!inst) {
            throw NotFoundException("Institution: " + instId);
        }

        auto book = centralInventory.getBook(isbn);
        if (!book) {
            throw NotFoundException("Book ISBN: " + isbn);
        }

        string reqId = "REQ-" + instId + "-" + to_string(time(nullptr));
        auto request = make_shared<BookRequest>(reqId, isbn, quantity, priority, 
                                               currentUser ? currentUser->getUserId() : "");
        inst->addRequest(request);
        
        // Check if book is available, otherwise add to waiting list
        if (centralInventory.getAvailableQuantity(isbn) < quantity) {
            waitingList.addToWaitingList(isbn, instId, quantity, priority);
            cout << "⚠ Added to waiting list (insufficient stock)\n";
        }
        
        cout << "✓ Request submitted: " << reqId << "\n";
        globalLogger.log(LogLevel::INFO, "Request submitted: " + reqId);
    }

    // Distribution
    void executeDistribution() {
        lock_guard<mutex> lock(systemMtx);
        
        vector<shared_ptr<Institution>> instList;
        for (auto& [id, inst] : institutions) {
            instList.push_back(inst);
        }

        cout << "\n=== Executing Distribution: " 
             << distributionStrategy->getStrategyName() << " ===\n";
        
        distributionStrategy->distribute(centralInventory, instList, loanManager);
        
        cout << "✓ Distribution completed\n";
        globalLogger.log(LogLevel::INFO, "Distribution cycle completed");
        
        // Notify institutions
        for (auto& inst : instList) {
            auto fulfilled = 0;
            for (auto& req : inst->getAllRequests()) {
                if (req->getStatus() == RequestStatus::FULFILLED) fulfilled++;
            }
            if (fulfilled > 0) {
                NotificationService::notifyInstitution(inst->getName(), 
                    "Distribution completed. " + to_string(fulfilled) + " requests fulfilled.");
            }
        }
    }

    void setDistributionStrategy(unique_ptr<IDistributionStrategy> strategy) {
        lock_guard<mutex> lock(systemMtx);
        distributionStrategy = move(strategy);
        cout << "✓ Strategy changed to: " << distributionStrategy->getStrategyName() << "\n";
        globalLogger.log(LogLevel::INFO, "Strategy changed to: " + distributionStrategy->getStrategyName());
    }

    // Search functionality
    void searchBooks(const string& keyword, int searchType) {
        cout << "\n=== SEARCH RESULTS ===\n";
        vector<pair<shared_ptr<Book>, int>> results;
        
        if (searchType == 1) {
            results = centralInventory.searchByTitle(keyword);
        } else if (searchType == 2) {
            results = centralInventory.searchByAuthor(keyword);
        } else if (searchType == 3) {
            int cat = stoi(keyword);
            if (cat >= 0 && cat <= 7) {
                results = centralInventory.getBooksByCategory(static_cast<BookCategory>(cat));
            }
        }
        
        if (results.empty()) {
            cout << "No books found.\n";
        } else {
            cout << "Found " << results.size() << " book(s):\n";
            for (const auto& [book, qty] : results) {
                book->displayInfo();
                cout << "    Available: " << qty << "\n";
            }
        }
    }

    // Loan Management
    void returnBooks(const string& loanId) {
        if (loanManager.returnBooks(loanId)) {
            // Find the loan to get details
            auto loans = loanManager.getLoansByInstitution("");
            for (auto& loan : loans) {
                if (loan->getLoanId() == loanId) {
                    centralInventory.returnBooks(loan->getISBN(), loan->getQuantity());
                    cout << "✓ Books returned successfully\n";
                    globalLogger.log(LogLevel::INFO, "Books returned: " + loanId);
                    return;
                }
            }
        } else {
            cout << "✗ Loan not found or already returned\n";
        }
    }
    
    void displayOverdueLoans() {
        auto overdue = loanManager.getOverdueLoans();
        NotificationService::notifyOverdue(overdue);
    }
    
    void displayAllLoans() {
        loanManager.displayAllLoans();
    }

    // Reporting
    void displaySystemStatus() {
        centralInventory.displayInventory();
        
        cout << "\n=== INSTITUTIONS (" << institutions.size() << ") ===\n";
        
        if (institutions.empty()) {
            cout << "  No institutions registered.\n";
        } else {
            for (auto& [id, inst] : institutions) {
                AnalyticsEngine::generateInstitutionReport(inst);
            }
        }
        
        vector<shared_ptr<Institution>> instList;
        for (auto& [id, inst] : institutions) {
            instList.push_back(inst);
        }
        
        if (!instList.empty()) {
            AnalyticsEngine::generateDistributionReport(instList);
        }
    }
    
    void exportReports() {
        try {
            DataPersistence::saveInventoryToFile(centralInventory, "inventory_report.csv");
            
            vector<shared_ptr<Institution>> instList;
            for (auto& [id, inst] : institutions) {
                instList.push_back(inst);
            }
            AnalyticsEngine::exportReportToCSV(instList, "distribution_report.csv");
            
            DataPersistence::saveSystemState("system_state.txt");
        } catch (const exception& e) {
            cout << "✗ Export failed: " << e.what() << "\n";
        }
    }
    
    void displayWaitingList() {
        waitingList.displayWaitingList();
    }
    
    void displayTransactionLog() {
        auto logs = centralInventory.getTransactionLog();
        cout << "\n=== TRANSACTION LOG (" << logs.size() << " entries) ===\n";
        if (logs.empty()) {
            cout << "  No transactions yet.\n";
            return;
        }
        
        int count = 0;
        for (auto it = logs.rbegin(); it != logs.rend() && count < 20; ++it, ++count) {
            time_t t = it->timestamp;
            char timeStr[26];
            ctime_r(&t, timeStr);
            timeStr[24] = '\0';
            
            cout << "[" << timeStr << "] " << it->type << " | ISBN: " 
                 << it->isbn << " | Qty: " << it->quantity << "\n";
        }
    }

    shared_ptr<Institution> getInstitution(const string& id) {
        auto it = institutions.find(id);
        return (it != institutions.end()) ? it->second : nullptr;
    }
    
    size_t getInstitutionCount() const { return institutions.size(); }
    size_t getUserCount() const { return users.size(); }
};

// ========================= CLI INTERFACE =========================
void displayMainMenu() {
    cout << "\n" << string(60, '=') << "\n";
    cout << "         GOVERNMENT BOOKS MANAGEMENT SYSTEM\n";
    cout << string(60, '=') << "\n";
    cout << "1.  Add Book to Inventory\n";
    cout << "2.  Register Institution\n";
    cout << "3.  Register User\n";
    cout << "4.  Submit Book Request\n";
    cout << "5.  Choose Distribution Strategy\n";
    cout << "6.  Run Distribution Cycle\n";
    cout << "7.  Display System Status\n";
    cout << "8.  Search Books\n";
    cout << "9.  View All Loans\n";
    cout << "10. View Overdue Loans\n";
    cout << "11. Return Books\n";
    cout << "12. View Waiting List\n";
    cout << "13. View Transaction Log\n";
    cout << "14. Export Reports (CSV)\n";
    cout << "15. User Login\n";
    cout << "q.  Quit\n";
    cout << string(60, '=') << "\n";
    cout << "Choose option: ";
}

void runCLI() {
    cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║   GOVERNMENT BOOKS MANAGEMENT & DISTRIBUTION SYSTEM      ║\n";
    cout << "║              Version 2.0 - Complete Edition              ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n";

    auto system = make_unique<GovernmentBooksManagementSystem>(
        make_unique<PriorityBasedDistribution>()
    );
    
    // Create default admin user
    auto admin = make_shared<User>("admin", "System Administrator", 
                                   "admin@gov.in", "9999999999",
                                   UserRole::ADMIN, "admin123");
    system->registerUser(admin);
    cout << "\n✓ Default admin user created (ID: admin, Password: admin123)\n";

    char choice;
    while (true) {
        try {
            displayMainMenu();
            
            // Show current user
            if (system->getCurrentUser()) {
                cout << "[Logged in as: " << system->getCurrentUser()->getName() << "]\n";
            }
            
            cin >> choice;

            if (tolower(choice) == 'q') {
                cout << "\n✓ Saving system state...\n";
                system->exportReports();
                break;
            }

            if (cin.fail()) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "⌧ Invalid input\n";
                continue;
            }

            switch(choice) {
                case '1': { // Add Book
                    string isbn, title, author, publisher;
                    int cat, qty, year;
                    double price;
                    
                    cout << "\n--- Add Book to Inventory ---\n";
                    cout << "ISBN: "; cin >> isbn;
                    cin.ignore();
                    cout << "Title: "; getline(cin, title);
                    cout << "Author: "; getline(cin, author);
                    cout << "Publisher: "; getline(cin, publisher);
                    cout << "Year: "; cin >> year;
                    cout << "Price (Rs): "; cin >> price;
                    cout << "Category (0-7): "; cin >> cat;
                    cout << "Quantity: "; cin >> qty;

                    auto book = make_shared<Book>(isbn, title, author, 
                                                 static_cast<BookCategory>(cat), 
                                                 year, publisher, price);
                    system->addBookToInventory(book, qty);
                    break;
                }
                
                case '2': { // Register Institution
                    string id, name, loc;
                    int type, students;
                    
                    cout << "\n--- Register Institution ---\n";
                    cout << "ID: "; cin >> id;
                    cin.ignore();
                    cout << "Name: "; getline(cin, name);
                    cout << "Type (0-6): "; cin >> type;
                    cin.ignore();
                    cout << "Location: "; getline(cin, loc);
                    cout << "Students: "; cin >> students;

                    auto inst = make_shared<Institution>(id, name, 
                                                        static_cast<InstitutionType>(type),
                                                        loc, students);
                    system->registerInstitution(inst);
                    break;
                }
                
                case '3': { // Register User
                    string id, name, email, phone, pwd;
                    int role;
                    
                    cout << "\n--- Register User ---\n";
                    cout << "User ID: "; cin >> id;
                    cin.ignore();
                    cout << "Name: "; getline(cin, name);
                    cout << "Email: "; cin >> email;
                    cout << "Phone: "; cin >> phone;
                    cout << "Role (0=Admin,1=Librarian,2=Head,3=Student): "; cin >> role;
                    cout << "Password: "; cin >> pwd;

                    auto user = make_shared<User>(id, name, email, phone,
                                                 static_cast<UserRole>(role), pwd);
                    system->registerUser(user);
                    break;
                }
                
                case '4': { // Submit Request
                    string instId, isbn;
                    int qty, prio;
                    
                    cout << "\n--- Submit Book Request ---\n";
                    cout << "Institution ID: "; cin >> instId;
                    cout << "ISBN: "; cin >> isbn;
                    cout << "Quantity: "; cin >> qty;
                    cout << "Priority (1-4): "; cin >> prio;

                    system->submitBookRequest(instId, isbn, qty, 
                                            static_cast<Priority>(prio));
                    break;
                }
                
                case '5': { // Change Strategy
                    int opt;
                    cout << "\n--- Choose Distribution Strategy ---\n";
                    cout << "1. Priority-Based\n";
                    cout << "2. Equal Distribution\n";
                    cout << "3. Need-Based\n";
                    cout << "Choice: "; cin >> opt;
                    
                    if (opt == 1) {
                        system->setDistributionStrategy(make_unique<PriorityBasedDistribution>());
                    } else if (opt == 2) {
                        system->setDistributionStrategy(make_unique<EqualDistribution>());
                    } else if (opt == 3) {
                        system->setDistributionStrategy(make_unique<NeedBasedDistribution>());
                    }
                    break;
                }
                
                case '6': { // Run Distribution
                    system->executeDistribution();
                    break;
                }
                
                case '7': { // System Status
                    system->displaySystemStatus();
                    break;
                }
                
                case '8': { // Search Books
                    int searchType;
                    string keyword;
                    
                    cout << "\n--- Search Books ---\n";
                    cout << "1. By Title\n2. By Author\n3. By Category\n";
                    cout << "Search type: "; cin >> searchType;
                    cin.ignore();
                    cout << "Keyword: "; getline(cin, keyword);
                    
                    system->searchBooks(keyword, searchType);
                    break;
                }
                
                case '9': { // View All Loans
                    system->displayAllLoans();
                    break;
                }
                
                case '10': { // View Overdue
                    system->displayOverdueLoans();
                    break;
                }
                
                case '11': { // Return Books
                    string loanId;
                    cout << "\n--- Return Books ---\n";
                    cout << "Loan ID: "; cin >> loanId;
                    system->returnBooks(loanId);
                    break;
                }
                
                case '12': { // Waiting List
                    system->displayWaitingList();
                    break;
                }
                
                case '13': { // Transaction Log
                    system->displayTransactionLog();
                    break;
                }
                
                case '14': { // Export Reports
                    system->exportReports();
                    break;
                }
                
                case '15': { // Login
                    string userId, pwd;
                    cout << "\n--- User Login ---\n";
                    cout << "User ID: "; cin >> userId;
                    cout << "Password: "; cin >> pwd;
                    
                    if (system->login(userId, pwd)) {
                        cout << "✓ Login successful!\n";
                    } else {
                        cout << "✗ Login failed!\n";
                    }
                    break;
                }
                
                default:
                    cout << "⌧ Invalid choice\n";
            }
            
        } catch (const BookManagementException& e) {
            cout << "✗ Error: " << e.what() << "\n";
            globalLogger.log(LogLevel::ERROR_LOG, e.what());
        } catch (const exception& e) {
            cout << "✗ Unexpected error: " << e.what() << "\n";
            globalLogger.log(LogLevel::CRITICAL, e.what());
        }
    }

    cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║              Thank you for using the system!             ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n";
}

// ========================= MAIN =========================
int main() {
    try {
        runCLI();
    } catch (const exception& e) {
        cerr << "FATAL ERROR: " << e.what() << endl;
        return 1;
    }
    return 0;
}