// government_books_management.cpp
// Advanced Government Books Management & Distribution System
// Build: g++ -std=c++17 government_books_management.cpp -o books_system && ./books_system

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
#include <utility>
#include <numeric>
#include <ctime>

using namespace std;
using namespace std::chrono_literals;

// ========================= ENUMS & CONSTANTS =========================
enum class BookCategory {
    TEXTBOOK,
    REFERENCE,
    LITERATURE,
    SCIENCE,
    HISTORY,
    MATHEMATICS,
    LANGUAGE,
    VOCATIONAL
};

enum class InstitutionType {
    PRIMARY_SCHOOL,
    SECONDARY_SCHOOL,
    HIGH_SCHOOL,
    COLLEGE,
    UNIVERSITY,
    LIBRARY,
    RESEARCH_CENTER
};

enum class Priority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

enum class RequestStatus {
    PENDING,
    APPROVED,
    PARTIALLY_FULFILLED,
    FULFILLED,
    REJECTED
};

string categoryToString(BookCategory cat) {
    switch(cat) {
        case BookCategory::TEXTBOOK: return "Textbook";
        case BookCategory::REFERENCE: return "Reference";
        case BookCategory::LITERATURE: return "Literature";
        case BookCategory::SCIENCE: return "Science";
        case BookCategory::HISTORY: return "History";
        case BookCategory::MATHEMATICS: return "Mathematics";
        case BookCategory::LANGUAGE: return "Language";
        case BookCategory::VOCATIONAL: return "Vocational";
        default: return "Unknown";
    }
}

string institutionTypeToString(InstitutionType type) {
    switch(type) {
        case InstitutionType::PRIMARY_SCHOOL: return "Primary School";
        case InstitutionType::SECONDARY_SCHOOL: return "Secondary School";
        case InstitutionType::HIGH_SCHOOL: return "High School";
        case InstitutionType::COLLEGE: return "College";
        case InstitutionType::UNIVERSITY: return "University";
        case InstitutionType::LIBRARY: return "Library";
        case InstitutionType::RESEARCH_CENTER: return "Research Center";
        default: return "Unknown";
    }
}

// ========================= BOOK CLASS =========================
class Book {
private:
    string isbn;
    string title;
    string author;
    BookCategory category;
    int publicationYear;
    string publisher;

public:
    Book(string isbn, string title, string author, BookCategory cat, int year, string pub)
        : isbn(move(isbn)), title(move(title)), author(move(author)), 
          category(cat), publicationYear(year), publisher(move(pub)) {}

    const string& getISBN() const { return isbn; }
    const string& getTitle() const { return title; }
    const string& getAuthor() const { return author; }
    BookCategory getCategory() const { return category; }
    int getPublicationYear() const { return publicationYear; }
    const string& getPublisher() const { return publisher; }

    void displayInfo() const {
        cout << "  ISBN: " << isbn << " | Title: " << title 
             << " | Author: " << author << " | Category: " << categoryToString(category) << "\n";
    }
};

// ========================= INVENTORY (Single Responsibility) =========================
class BookInventory {
private:
    // ISBN -> (Book, Quantity)
    unordered_map<string, pair<shared_ptr<Book>, int>> stock;
    // Category -> set of ISBNs (for efficient category-based queries)
    map<BookCategory, set<string>> categoryIndex;
    mutable std::mutex mtx;

    // Transaction log (Observer pattern could be applied here)
    struct Transaction {
        string isbn;
        int quantity;
        string type; // "ADD" or "ALLOCATE"
        time_t timestamp;
    };
    vector<Transaction> transactionLog;

public:
    void addBook(shared_ptr<Book> book, int quantity) {
        if (quantity <= 0) return;
        lock_guard<std::mutex> lock(mtx);
        
        string isbn = book->getISBN();
        if (stock.find(isbn) != stock.end()) {
            stock[isbn].second += quantity;
        } else {
            stock[isbn] = {book, quantity};
            categoryIndex[book->getCategory()].insert(isbn);
        }
        
        transactionLog.push_back({isbn, quantity, "ADD", time(nullptr)});
    }

    bool allocateBooks(const string& isbn, int quantity) {
        if (quantity <= 0) return false;
        lock_guard<std::mutex> lock(mtx);
        
        auto it = stock.find(isbn);
        if (it == stock.end() || it->second.second < quantity) {
            return false;
        }
        
        it->second.second -= quantity;
        transactionLog.push_back({isbn, quantity, "ALLOCATE", time(nullptr)});
        return true;
    }

    int getAvailableQuantity(const string& isbn) const {
        lock_guard<std::mutex> lock(mtx);
        auto it = stock.find(isbn);
        return (it != stock.end()) ? it->second.second : 0;
    }

    vector<pair<shared_ptr<Book>, int>> getBooksByCategory(BookCategory cat) const {
        lock_guard<std::mutex> lock(mtx);
        vector<pair<shared_ptr<Book>, int>> result;
        
        auto catIt = categoryIndex.find(cat);
        if (catIt != categoryIndex.end()) {
            for (const auto& isbn : catIt->second) {
                auto stockIt = stock.find(isbn);
                if (stockIt != stock.end()) {
                    result.push_back(stockIt->second);
                }
            }
        }
        return result;
    }

    int getTotalBooks() const {
        lock_guard<std::mutex> lock(mtx);
        return accumulate(stock.begin(), stock.end(), 0,
            [](int sum, const auto& p) { return sum + p.second.second; });
    }

    void displayInventory() const {
        lock_guard<std::mutex> lock(mtx);
        cout << "\n=== CENTRAL INVENTORY ===\n";
        cout << "Total Books: " << getTotalBooks() << "\n";
        cout << "Unique Titles: " << stock.size() << "\n\n";
        
        for (const auto& [isbn, data] : stock) {
            data.first->displayInfo();
            cout << "    Available Quantity: " << data.second << "\n";
        }
    }

    shared_ptr<Book> getBook(const string& isbn) const {
        lock_guard<std::mutex> lock(mtx);
        auto it = stock.find(isbn);
        return (it != stock.end()) ? it->second.first : nullptr;
    }
};

// ========================= USER/STUDENT (for admission tracking) =========================
class User {
private:
    string userId;
    string name;
    string email;
    InstitutionType affiliatedInstitution;
    
public:
    User(string id, string name, string email, InstitutionType inst)
        : userId(move(id)), name(move(name)), email(move(email)), 
          affiliatedInstitution(inst) {}

    const string& getUserId() const { return userId; }
    const string& getName() const { return name; }
    InstitutionType getInstitution() const { return affiliatedInstitution; }
};

// ========================= BOOK REQUEST (Value Object) =========================
class BookRequest {
private:
    string requestId;
    string isbn;
    int quantityRequested;
    int quantityFulfilled;
    Priority priority;
    RequestStatus status;
    time_t requestDate;

public:
    BookRequest(string reqId, string isbn, int qty, Priority prio)
        : requestId(move(reqId)), isbn(move(isbn)), quantityRequested(qty),
          quantityFulfilled(0), priority(prio), status(RequestStatus::PENDING),
          requestDate(time(nullptr)) {}

    const string& getRequestId() const { return requestId; }
    const string& getISBN() const { return isbn; }
    int getQuantityRequested() const { return quantityRequested; }
    int getQuantityFulfilled() const { return quantityFulfilled; }
    int getRemainingQuantity() const { return quantityRequested - quantityFulfilled; }
    Priority getPriority() const { return priority; }
    RequestStatus getStatus() const { return status; }

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

// ========================= INSTITUTION (Composite Pattern) =========================
class Institution {
private:
    string institutionId;
    string name;
    InstitutionType type;
    string location;
    int studentCount;
    
    // Current book holdings
    unordered_map<string, int> currentBooks; // ISBN -> quantity
    
    // Request queue
    vector<shared_ptr<BookRequest>> requests;
    mutable std::mutex mtx;

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
        lock_guard<std::mutex> lock(mtx);
        requests.push_back(req);
    }

    vector<shared_ptr<BookRequest>> getPendingRequests() const {
        lock_guard<std::mutex> lock(mtx);
        vector<shared_ptr<BookRequest>> pending;
        for (const auto& req : requests) {
            if (req->getStatus() == RequestStatus::PENDING || 
                req->getStatus() == RequestStatus::PARTIALLY_FULFILLED) {
                pending.push_back(req);
            }
        }
        return pending;
    }

    void receiveBooks(const string& isbn, int quantity) {
        lock_guard<std::mutex> lock(mtx);
        currentBooks[isbn] += quantity;
    }

    int getCurrentStock(const string& isbn) const {
        lock_guard<std::mutex> lock(mtx);
        auto it = currentBooks.find(isbn);
        return (it != currentBooks.end()) ? it->second : 0;
    }

    void displayStatus() const {
        cout << "\nInstitution: " << name << " (" << institutionTypeToString(type) << ")\n";
        cout << "Location: " << location << " | Students: " << studentCount << "\n";
        cout << "Pending Requests: " << getPendingRequests().size() << "\n";
    }

    // Calculate need based on student count and current stock
    int calculateNeed(const string& isbn, int booksPerStudent = 1) const {
        int needed = studentCount * booksPerStudent;
        int current = getCurrentStock(isbn);
        return max(0, needed - current);
    }
};

// ========================= DISTRIBUTION STRATEGY (Strategy Pattern) =========================
class IDistributionStrategy {
public:
    virtual ~IDistributionStrategy() = default;
    virtual void distribute(BookInventory& inventory, 
                          vector<shared_ptr<Institution>>& institutions) = 0;
    virtual string getStrategyName() const = 0;
};

// Priority-based distribution (uses priority queue)
class PriorityBasedDistribution : public IDistributionStrategy {
public:
    void distribute(BookInventory& inventory, 
                   vector<shared_ptr<Institution>>& institutions) override {
        
        // Priority queue: higher priority first, then FIFO
        auto cmp = [](const tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>& a,
                     const tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>& b) {
            return static_cast<int>(get<0>(a)->getPriority()) < 
                   static_cast<int>(get<0>(b)->getPriority());
        };
        
        priority_queue<tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>,
                      vector<tuple<shared_ptr<BookRequest>, shared_ptr<Institution>, int>>,
                      decltype(cmp)> pq(cmp);

        // Collect all pending requests
        for (auto& inst : institutions) {
            auto requests = inst->getPendingRequests();
            for (auto& req : requests) {
                pq.push({req, inst, req->getRemainingQuantity()});
            }
        }

        // Process requests by priority
        while (!pq.empty()) {
            auto [req, inst, needed] = pq.top();
            pq.pop();

            int available = inventory.getAvailableQuantity(req->getISBN());
            if (available <= 0) continue;

            int allocate = min(needed, available);
            if (inventory.allocateBooks(req->getISBN(), allocate)) {
                inst->receiveBooks(req->getISBN(), allocate);
                req->fulfillPartial(allocate);
            }
        }
    }

    string getStrategyName() const override { return "Priority-Based Distribution"; }
};

// Need-based distribution (uses graph/tree structure concepts)
class NeedBasedDistribution : public IDistributionStrategy {
public:
    void distribute(BookInventory& inventory, 
                   vector<shared_ptr<Institution>>& institutions) override {
        
        // Build need map: ISBN -> vector<(Institution, need)>
        map<string, vector<pair<shared_ptr<Institution>, int>>> needMap;
        
        for (auto& inst : institutions) {
            auto requests = inst->getPendingRequests();
            for (auto& req : requests) {
                int need = req->getRemainingQuantity();
                if (need > 0) {
                    needMap[req->getISBN()].push_back({inst, need});
                }
            }
        }

        // Distribute each book type proportionally
        for (auto& [isbn, needs] : needMap) {
            int available = inventory.getAvailableQuantity(isbn);
            if (available <= 0) continue;

            int totalNeed = accumulate(needs.begin(), needs.end(), 0,
                [](int sum, const auto& p) { return sum + p.second; });

            for (auto& [inst, need] : needs) {
                // Proportional allocation
                int allocate = (totalNeed > 0) ? 
                    min(need, (available * need) / totalNeed) : 0;
                
                if (allocate > 0 && inventory.allocateBooks(isbn, allocate)) {
                    inst->receiveBooks(isbn, allocate);
                    
                    // Update request status
                    auto requests = inst->getPendingRequests();
                    for (auto& req : requests) {
                        if (req->getISBN() == isbn) {
                            req->fulfillPartial(allocate);
                            break;
                        }
                    }
                }
            }
        }
    }

    string getStrategyName() const override { return "Need-Based Proportional Distribution"; }
};

// Equal distribution strategy
class EqualDistribution : public IDistributionStrategy {
public:
    void distribute(BookInventory& inventory, 
                   vector<shared_ptr<Institution>>& institutions) override {
        
        set<string> allISBNs;
        for (auto& inst : institutions) {
            auto requests = inst->getPendingRequests();
            for (auto& req : requests) {
                allISBNs.insert(req->getISBN());
            }
        }

        for (const auto& isbn : allISBNs) {
            int available = inventory.getAvailableQuantity(isbn);
            if (available <= 0) continue;

            // Count institutions needing this book
            vector<shared_ptr<Institution>> needingInsts;
            for (auto& inst : institutions) {
                auto requests = inst->getPendingRequests();
                for (auto& req : requests) {
                    if (req->getISBN() == isbn && req->getRemainingQuantity() > 0) {
                        needingInsts.push_back(inst);
                        break;
                    }
                }
            }

            if (needingInsts.empty()) continue;

            int perInst = available / needingInsts.size();
            if (perInst <= 0) continue;

            for (auto& inst : needingInsts) {
                if (inventory.allocateBooks(isbn, perInst)) {
                    inst->receiveBooks(isbn, perInst);
                    
                    auto requests = inst->getPendingRequests();
                    for (auto& req : requests) {
                        if (req->getISBN() == isbn) {
                            req->fulfillPartial(perInst);
                            break;
                        }
                    }
                }
            }
        }
    }

    string getStrategyName() const override { return "Equal Distribution"; }
};

// ========================= ANALYTICS & REPORTING (Single Responsibility) =========================
class AnalyticsEngine {
public:
    static void generateDistributionReport(const vector<shared_ptr<Institution>>& institutions) {
        cout << "\n=== DISTRIBUTION ANALYTICS REPORT ===\n";
        
        int totalRequests = 0;
        int fulfilledRequests = 0;
        int partiallyFulfilled = 0;
        int pendingRequests = 0;

        for (const auto& inst : institutions) {
            auto requests = inst->getPendingRequests();
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
        
        auto requests = inst->getPendingRequests();
        if (!requests.empty()) {
            cout << "Request Details:\n";
            for (const auto& req : requests) {
                cout << "  - ISBN: " << req->getISBN() 
                     << " | Requested: " << req->getQuantityRequested()
                     << " | Fulfilled: " << req->getQuantityFulfilled()
                     << " | Status: " << static_cast<int>(req->getStatus()) << "\n";
            }
        }
    }
};

// ========================= MAIN MANAGEMENT SYSTEM (Facade Pattern) =========================
class GovernmentBooksManagementSystem {
private:
    BookInventory centralInventory;
    unordered_map<string, shared_ptr<Institution>> institutions;
    unordered_map<string, shared_ptr<User>> users;
    unique_ptr<IDistributionStrategy> distributionStrategy;
    mutable std::mutex systemMtx;

public:
    GovernmentBooksManagementSystem(unique_ptr<IDistributionStrategy> strategy)
        : distributionStrategy(move(strategy)) {}

    // Book Management
    void addBookToInventory(shared_ptr<Book> book, int quantity) {
        centralInventory.addBook(book, quantity);
        cout << "✓ Added " << quantity << " copies of '" << book->getTitle() << "'\n";
    }

    // Institution Management
    void registerInstitution(shared_ptr<Institution> inst) {
        lock_guard<std::mutex> lock(systemMtx);
        institutions[inst->getId()] = inst;
        cout << "✓ Registered: " << inst->getName() << "\n";
    }

    // User/Student Management
    void registerUser(shared_ptr<User> user) {
        lock_guard<std::mutex> lock(systemMtx);
        users[user->getUserId()] = user;
    }

    // Request Management
    void submitBookRequest(const string& instId, const string& isbn, 
                          int quantity, Priority priority) {
        auto inst = getInstitution(instId);
        if (!inst) {
            cout << "✗ Institution not found: " << instId << "\n";
            return;
        }

        string reqId = "REQ-" + instId + "-" + to_string(time(nullptr));
        auto request = make_shared<BookRequest>(reqId, isbn, quantity, priority);
        inst->addRequest(request);
        
        cout << "✓ Request submitted: " << reqId << "\n";
    }

    // Distribution
    void executeDistribution() {
        lock_guard<std::mutex> lock(systemMtx);
        
        vector<shared_ptr<Institution>> instList;
        for (auto& [id, inst] : institutions) {
            instList.push_back(inst);
        }

        cout << "\n=== Executing Distribution: " 
             << distributionStrategy->getStrategyName() << " ===\n";
        
        distributionStrategy->distribute(centralInventory, instList);
        
        cout << "✓ Distribution completed\n";
    }

    // Change distribution strategy at runtime
    void setDistributionStrategy(unique_ptr<IDistributionStrategy> strategy) {
        lock_guard<std::mutex> lock(systemMtx);
        distributionStrategy = move(strategy);
        cout << "✓ Strategy changed to: " << distributionStrategy->getStrategyName() << "\n";
    }

    // Reporting
    void displaySystemStatus() {
        centralInventory.displayInventory();
        
        cout << "\n=== INSTITUTIONS (" << institutions.size() << ") ===\n";
        for (auto& [id, inst] : institutions) {
            inst->displayStatus();
        }
        
        vector<shared_ptr<Institution>> instList;
        for (auto& [id, inst] : institutions) {
            instList.push_back(inst);
        }
        AnalyticsEngine::generateDistributionReport(instList);
    }

    shared_ptr<Institution> getInstitution(const string& id) {
        auto it = institutions.find(id);
        return (it != institutions.end()) ? it->second : nullptr;
    }
};

// ========================= DEMONSTRATION =========================
void runGovernmentBooksDemo() {
    cout << "╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║   GOVERNMENT BOOKS MANAGEMENT & DISTRIBUTION SYSTEM      ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    // Initialize system with priority-based strategy
    auto system = make_unique<GovernmentBooksManagementSystem>(
        make_unique<PriorityBasedDistribution>()
    );

    // Add books to central inventory
    cout << "--- Adding Books to Central Inventory ---\n";
    system->addBookToInventory(
        make_shared<Book>("ISBN-001", "Mathematics Grade 10", "Dr. A. Kumar", 
                         BookCategory::MATHEMATICS, 2024, "NCERT"),
        500
    );
    system->addBookToInventory(
        make_shared<Book>("ISBN-002", "English Literature", "Prof. B. Singh", 
                         BookCategory::LITERATURE, 2024, "State Board"),
        400
    );
    system->addBookToInventory(
        make_shared<Book>("ISBN-003", "Science Fundamentals", "Dr. C. Patel", 
                         BookCategory::SCIENCE, 2024, "CBSE"),
        600
    );
    system->addBookToInventory(
        make_shared<Book>("ISBN-004", "Indian History", "Prof. D. Sharma", 
                         BookCategory::HISTORY, 2023, "NCERT"),
        300
    );

    // Register institutions
    cout << "\n--- Registering Educational Institutions ---\n";
    auto school1 = make_shared<Institution>(
        "INST-001", "Gandhi Memorial High School", 
        InstitutionType::HIGH_SCHOOL, "New Delhi", 350
    );
    auto school2 = make_shared<Institution>(
        "INST-002", "Nehru Public School", 
        InstitutionType::SECONDARY_SCHOOL, "Mumbai", 280
    );
    auto college1 = make_shared<Institution>(
        "INST-003", "National Science College", 
        InstitutionType::COLLEGE, "Bangalore", 500
    );
    auto library1 = make_shared<Institution>(
        "INST-004", "State Central Library", 
        InstitutionType::LIBRARY, "Chennai", 150
    );

    system->registerInstitution(school1);
    system->registerInstitution(school2);
    system->registerInstitution(college1);
    system->registerInstitution(library1);

    // Submit book requests with different priorities
    cout << "\n--- Submitting Book Requests ---\n";
    
    // High priority requests (new academic year)
    system->submitBookRequest("INST-001", "ISBN-001", 300, Priority::CRITICAL);
    system->submitBookRequest("INST-001", "ISBN-002", 300, Priority::HIGH);
    
    // Medium priority
    system->submitBookRequest("INST-002", "ISBN-001", 250, Priority::MEDIUM);
    system->submitBookRequest("INST-002", "ISBN-003", 250, Priority::MEDIUM);
    
    // College requests
    system->submitBookRequest("INST-003", "ISBN-003", 400, Priority::HIGH);
    system->submitBookRequest("INST-003", "ISBN-004", 200, Priority::MEDIUM);
    
    // Library requests
    system->submitBookRequest("INST-004", "ISBN-002", 100, Priority::LOW);
    system->submitBookRequest("INST-004", "ISBN-004", 100, Priority::LOW);

    // Execute first distribution
    cout << "\n--- ROUND 1: Priority-Based Distribution ---\n";
    system->executeDistribution();
    system->displaySystemStatus();

    // Change strategy and add more inventory
    cout << "\n--- Adding More Inventory and Changing Strategy ---\n";
    system->addBookToInventory(
        make_shared<Book>("ISBN-001", "Mathematics Grade 10", "Dr. A. Kumar", 
                         BookCategory::MATHEMATICS, 2024, "NCERT"),
        300
    );
    
    system->setDistributionStrategy(make_unique<NeedBasedDistribution>());

    // Execute second distribution
    cout << "\n--- ROUND 2: Need-Based Distribution ---\n";
    system->executeDistribution();
    system->displaySystemStatus();

    // Switch to equal distribution
    cout << "\n--- Switching to Equal Distribution Strategy ---\n";
    system->setDistributionStrategy(make_unique<EqualDistribution>());
    system->executeDistribution();
    system->displaySystemStatus();

    cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║              DEMONSTRATION COMPLETED                      ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n";
}

// int main() {
//     runGovernmentBooksDemo();
//     return 0;
// }

int main() {
    cout << "WELCOME TO GOVERNMENT BOOKS MANAGEMENT CLI APP" << endl;

    // Default strategy = Priority Based
    auto system = make_unique<GovernmentBooksManagementSystem>(
        make_unique<PriorityBasedDistribution>()
    );

    char choice;
    while (true) {
        cout << "\n===== MAIN MENU =====" << endl;
        cout << "1. Add Book to Inventory" << endl;
        cout << "2. Register Institution" << endl;
        cout << "3. Submit Book Request" << endl;
        cout << "4. Choose Distribution Strategy" << endl;
        cout << "5. Run Distribution Cycle" << endl;
        cout << "6. Display System Status" << endl;
        cout << "q. Quit" << endl;
        cout << "Choose option: ";
        cin >> choice;

        if (tolower(choice) == 'q') break;

        if (choice == '1') {
            string isbn, title, author, publisher;
            int cat, qty, year;
            cout << "Enter ISBN: "; cin >> isbn;
            cin.ignore();
            cout << "Enter Title: "; getline(cin, title);
            cout << "Enter Author: "; getline(cin, author);
            cout << "Enter Publisher: "; getline(cin, publisher);
            cout << "Publication Year: "; cin >> year;
            cout << "Category (0=TEXTBOOK,1=REFERENCE,2=LITERATURE,3=SCIENCE,4=HISTORY,5=MATHEMATICS,6=LANGUAGE,7=VOCATIONAL): ";
            cin >> cat;
            cout << "Enter Quantity: "; cin >> qty;

            system->addBookToInventory(
                make_shared<Book>(isbn, title, author, static_cast<BookCategory>(cat), year, publisher),
                qty
            );
        }
        else if (choice == '2') {
            string id, name, addr;
            int type, students;
            cout << "Enter Institution ID: "; cin >> id;
            cin.ignore();
            cout << "Enter Name: "; getline(cin, name);
            cout << "Type (0=PRIMARY,1=SECONDARY,2=HIGH,3=COLLEGE,4=UNIVERSITY,5=LIBRARY,6=RESEARCH): ";
            cin >> type;
            cin.ignore();
            cout << "Enter Location: "; getline(cin, addr);
            cout << "Enter Student Count: "; cin >> students;

            system->registerInstitution(
                make_shared<Institution>(id, name, static_cast<InstitutionType>(type), addr, students)
            );
        }
        else if (choice == '3') {
            string instId, isbn;
            int qty, prio;
            cout << "Enter Institution ID: "; cin >> instId;
            cout << "Enter ISBN: "; cin >> isbn;
            cout << "Enter Quantity: "; cin >> qty;
            cout << "Priority (1=LOW,2=MEDIUM,3=HIGH,4=CRITICAL): "; cin >> prio;

            system->submitBookRequest(instId, isbn, qty, static_cast<Priority>(prio));
        }
        else if (choice == '4') {
            int opt;
            cout << "Choose Strategy:" << endl;
            cout << "1. Priority-Based" << endl;
            cout << "2. Equal Distribution" << endl;
            cout << "3. Need-Based" << endl;
            cin >> opt;
            if (opt == 1) system->setDistributionStrategy(make_unique<PriorityBasedDistribution>());
            else if (opt == 2) system->setDistributionStrategy(make_unique<EqualDistribution>());
            else if (opt == 3) system->setDistributionStrategy(make_unique<NeedBasedDistribution>());
        }
        else if (choice == '5') {
            system->executeDistribution();
        }
        else if (choice == '6') {
            system->displaySystemStatus();
        }
        else {
            cout << "❌ Invalid choice" << endl;
        }
    }

    cout << "--------- Exited CLI ---------" << endl;
    return 0;
}
