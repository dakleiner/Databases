#include "Database.h"
#include "sqlite3.h"
#include <iostream>
#include <fstream>


using std::string;
using std::vector;
using std::optional;
using std::nullopt;

// ============================================
// HELPER FUNCTIONS
// ============================================

static vector<string> parseCSVLine(const string& line) {
    vector<string> result;
    string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            }
            else {
                inQuotes = !inQuotes;
            }
        }
        else if (c == ',' && !inQuotes) {
            result.push_back(field);
            field.clear();
        }
        else {
            field += c;
        }
    }

    result.push_back(field);
    return result;
}

static string trim(const string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// ============================================
// PIMPL IMPLEMENTATION
// ============================================

struct Database::Impl {
    sqlite3* db = nullptr;

    ~Impl() {
        if (db) {
            sqlite3_close(db);
        }
    }

    bool exec(const string& sql) const {
        char* errMsg = nullptr;
        bool ok = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) == SQLITE_OK;
        if (!ok && errMsg) {
            std::cerr << "SQL Error: " << errMsg << '\n';
            sqlite3_free(errMsg);
        }
        return ok;
    }

    sqlite3_stmt* prepare(const string& sql) const {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Prepare failed: " << sql << '\n';
            return nullptr;
        }
        return stmt;
    }

    void bind(sqlite3_stmt* stmt, int index, const string& value) const {
        sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
    }

    string getText(sqlite3_stmt* stmt, int col) const {
        const unsigned char* text = sqlite3_column_text(stmt, col);
        return text ? reinterpret_cast<const char*>(text) : "";
    }
};

// ============================================
// CONSTRUCTOR
// ============================================

Database::Database(const string& dbFile)
    : pImpl(std::make_unique<Impl>()) {

    // char buffer[FILENAME_MAX];
    // #ifdef _WIN32
    // _getcwd(buffer, FILENAME_MAX);
    // #else
    // getcwd(buffer, FILENAME_MAX);
    // #endif
    // std::cout << "DEBUG: Working directory: " << buffer << '\n';

    if (sqlite3_open(dbFile.c_str(), &pImpl->db) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << dbFile << '\n';
        pImpl->db = nullptr;
        return;
    }

    pImpl->exec("PRAGMA foreign_keys = ON");
    createTables();

    int customerCount = getCustomerCount();
    // std::cout << "DEBUG: Found " << customerCount << " customers in database.\n";
    
    if (customerCount == 0) {
        // std::cout << "DEBUG: No customers found. Attempting to import CSV...\n";
        if (!importCSV()) {
            // std::cout << "DEBUG: CSV import failed. Seeding default customer data...\n";
            seedDefaultData();
        }
    }

    // Import users from CSV if no users exist
    int userCount = getUserCount();
    // std::cout << "DEBUG: Found " << userCount << " users in database.\n";
    
    if (userCount == 0) {
        // std::cout << "DEBUG: No users found. Attempting to import user CSV...\n";
        if (!importUserCSV()) {
            // std::cout << "DEBUG: User CSV import failed. Seeding default user data...\n";
            seedDefaultUser();
        }
    }
}

Database::~Database() = default;

// ============================================
// TABLE CREATION
// ============================================

void Database::createTables() {
    if (!pImpl->db) return;

    pImpl->exec(R"(
        CREATE TABLE IF NOT EXISTS customers (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            first_name TEXT NOT NULL,
            last_name TEXT NOT NULL,
            street TEXT,
            city TEXT,
            state TEXT,
            zip_code TEXT,
            phone TEXT,
            service_type INTEGER DEFAULT 0
        )
    )");

    pImpl->exec(R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT DEFAULT 'user'
        )
    )");

    // Add missing columns to users table if they don't exist
    // Suppress errors if columns already exist (expected behavior)
    char* errMsg = nullptr;
    sqlite3_exec(pImpl->db, "ALTER TABLE users ADD COLUMN first_name TEXT", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    
    sqlite3_exec(pImpl->db, "ALTER TABLE users ADD COLUMN last_name TEXT", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
    
    sqlite3_exec(pImpl->db, "ALTER TABLE users ADD COLUMN employee_number TEXT", nullptr, nullptr, &errMsg);
    if (errMsg) sqlite3_free(errMsg);
}

// ============================================
// CUSTOMER OPERATIONS
// ============================================

vector<CustomerRecord> Database::getAllCustomers() const {
    vector<CustomerRecord> customers;
    if (!pImpl->db) return customers;

    auto* stmt = pImpl->prepare(
        "SELECT id, first_name, last_name, street, city, state, "
        "zip_code, phone, service_type FROM customers ORDER BY id"
    );

    if (!stmt) return customers;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        customers.emplace_back(
            sqlite3_column_int(stmt, 0),
            pImpl->getText(stmt, 1),
            pImpl->getText(stmt, 2),
            pImpl->getText(stmt, 3),
            pImpl->getText(stmt, 4),
            pImpl->getText(stmt, 5),
            pImpl->getText(stmt, 6),
            pImpl->getText(stmt, 7),
            static_cast<ServiceType>(sqlite3_column_int(stmt, 8))
        );
    }

    sqlite3_finalize(stmt);
    return customers;
}

vector<CustomerRecord> Database::loadCustomers() const {
    // Simply delegates to getAllCustomers
    return getAllCustomers();
}

optional<CustomerRecord> Database::getCustomer(int id) const {
    if (!pImpl->db) return nullopt;

    auto* stmt = pImpl->prepare(
        "SELECT id, first_name, last_name, street, city, state, "
        "zip_code, phone, service_type FROM customers WHERE id = ?"
    );

    if (!stmt) return nullopt;

    sqlite3_bind_int(stmt, 1, id);

    optional<CustomerRecord> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = CustomerRecord(
            sqlite3_column_int(stmt, 0),
            pImpl->getText(stmt, 1),
            pImpl->getText(stmt, 2),
            pImpl->getText(stmt, 3),
            pImpl->getText(stmt, 4),
            pImpl->getText(stmt, 5),
            pImpl->getText(stmt, 6),
            pImpl->getText(stmt, 7),
            static_cast<ServiceType>(sqlite3_column_int(stmt, 8))
        );
    }

    sqlite3_finalize(stmt);
    return result;
}

vector<CustomerRecord> Database::searchCustomers(const string& searchTerm) const {
    vector<CustomerRecord> results;
    if (!pImpl->db) return results;

    auto* stmt = pImpl->prepare(
        "SELECT id, first_name, last_name, street, city, state, "
        "zip_code, phone, service_type FROM customers "
        "WHERE first_name LIKE ? OR last_name LIKE ? "
        "ORDER BY last_name, first_name"
    );

    if (!stmt) return results;

    string pattern = "%" + searchTerm + "%";
    pImpl->bind(stmt, 1, pattern);
    pImpl->bind(stmt, 2, pattern);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.emplace_back(
            sqlite3_column_int(stmt, 0),
            pImpl->getText(stmt, 1),
            pImpl->getText(stmt, 2),
            pImpl->getText(stmt, 3),
            pImpl->getText(stmt, 4),
            pImpl->getText(stmt, 5),
            pImpl->getText(stmt, 6),
            pImpl->getText(stmt, 7),
            static_cast<ServiceType>(sqlite3_column_int(stmt, 8))
        );
    }

    sqlite3_finalize(stmt);
    return results;
}

void Database::addCustomer(
    const std::string& firstName,
    const std::string& lastName,
    const std::string& street,
    const std::string& city,
    const std::string& state,
    const std::string& zipCode,
    const std::string& phone,
    ServiceType serviceType
) {
    if (!pImpl->db) return;

    auto* stmt = pImpl->prepare(
        "INSERT INTO customers "
        "(first_name, last_name, street, city, state, zip_code, phone, service_type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
    );

    if (!stmt) return;

    pImpl->bind(stmt, 1, firstName);
    pImpl->bind(stmt, 2, lastName);
    pImpl->bind(stmt, 3, street);
    pImpl->bind(stmt, 4, city);
    pImpl->bind(stmt, 5, state);
    pImpl->bind(stmt, 6, zipCode);
    pImpl->bind(stmt, 7, phone);
    sqlite3_bind_int(stmt, 8, static_cast<int>(serviceType));

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool Database::updateCustomer(
    int id,
    const std::string& firstName,
    const std::string& lastName,
    const std::string& street,
    const std::string& city,
    const std::string& state,
    const std::string& zipCode,
    const std::string& phone,
    ServiceType serviceType
) {
    if (!pImpl->db) return false;

    auto* stmt = pImpl->prepare(
        "UPDATE customers SET "
        "first_name=?, last_name=?, street=?, city=?, state=?, "
        "zip_code=?, phone=?, service_type=? WHERE id=?"
    );

    if (!stmt) return false;

    pImpl->bind(stmt, 1, firstName);
    pImpl->bind(stmt, 2, lastName);
    pImpl->bind(stmt, 3, street);
    pImpl->bind(stmt, 4, city);
    pImpl->bind(stmt, 5, state);
    pImpl->bind(stmt, 6, zipCode);
    pImpl->bind(stmt, 7, phone);
    sqlite3_bind_int(stmt, 8, static_cast<int>(serviceType));
    sqlite3_bind_int(stmt, 9, id);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

void Database::updateCustomerService(int id, ServiceType serviceType) {
    if (!pImpl->db) return;

    auto* stmt = pImpl->prepare(
        "UPDATE customers SET service_type=? WHERE id=?"
    );

    if (!stmt) return;

    sqlite3_bind_int(stmt, 1, static_cast<int>(serviceType));
    sqlite3_bind_int(stmt, 2, id);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool Database::deleteCustomer(int id) {
    if (!pImpl->db) return false;

    auto* stmt = pImpl->prepare("DELETE FROM customers WHERE id=?");
    if (!stmt) return false;

    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

// ============================================
// USER OPERATIONS
// ============================================

optional<UserRecord> Database::getUser(const string& username) const {
    if (!pImpl->db) return nullopt;

    auto* stmt = pImpl->prepare(
        "SELECT id, username, password_hash, first_name, last_name, employee_number, role FROM users WHERE username=?"
    );

    if (!stmt) return nullopt;

    pImpl->bind(stmt, 1, username);

    optional<UserRecord> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = UserRecord{
            sqlite3_column_int(stmt, 0),
            pImpl->getText(stmt, 1),
            pImpl->getText(stmt, 2),
            pImpl->getText(stmt, 3),
            pImpl->getText(stmt, 4),
            pImpl->getText(stmt, 5),
            pImpl->getText(stmt, 6)
        };
    }

    sqlite3_finalize(stmt);
    return result;
}

std::optional<UserRecord> Database::getUserByUsername(const std::string& username) const {
    // Simply delegates to getUser
    return getUser(username);
}

bool Database::addUser(
    const string& username,
    const string& passwordPlaintext,
    const string& firstName,
    const string& lastName,
    const string& employeeNumber,
    const string& role
) {
    if (!pImpl->db) {
        // std::cerr << "DEBUG addUser: Database not open\n";
        return false;
    }

    auto* stmt = pImpl->prepare(
        "INSERT INTO users (username, password_hash, first_name, last_name, employee_number, role) VALUES (?, ?, ?, ?, ?, ?)"
    );

    if (!stmt) {
        // std::cerr << "DEBUG addUser: Failed to prepare statement\n";
        return false;
    }

    pImpl->bind(stmt, 1, username);
    pImpl->bind(stmt, 2, passwordPlaintext);
    pImpl->bind(stmt, 3, firstName);
    pImpl->bind(stmt, 4, lastName);
    pImpl->bind(stmt, 5, employeeNumber);
    pImpl->bind(stmt, 6, role);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        std::cerr << "DEBUG addUser: Insert failed for user '" << username 
                  << "' - SQLite error: " << sqlite3_errmsg(pImpl->db) << '\n';
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    // std::cout << "DEBUG addUser: Successfully added user '" << username << "'\n";
    return true;
}

// ============================================
// UTILITIES
// ============================================

int Database::getCustomerCount() const {
    if (!pImpl->db) return 0;

    auto* stmt = pImpl->prepare("SELECT COUNT(*) FROM customers");
    if (!stmt) return 0;

    int count = (sqlite3_step(stmt) == SQLITE_ROW)
        ? sqlite3_column_int(stmt, 0)
        : 0;

    sqlite3_finalize(stmt);
    return count;
}

int Database::getUserCount() const {
    if (!pImpl->db) return 0;

    auto* stmt = pImpl->prepare("SELECT COUNT(*) FROM users");
    if (!stmt) return 0;

    int count = (sqlite3_step(stmt) == SQLITE_ROW)
        ? sqlite3_column_int(stmt, 0)
        : 0;

    sqlite3_finalize(stmt);
    return count;
}

bool Database::importCSV(const string& csvPath) {
    if (!pImpl->db) return false;

    std::ifstream file(csvPath);
    if (!file) return false;

    pImpl->exec("BEGIN TRANSACTION");

    string header;
    std::getline(file, header);

    string line;
    int imported = 0;

    while (std::getline(file, line)) {
        if (trim(line).empty()) continue;

        auto fields = parseCSVLine(line);
        if (fields.size() < 8) continue;

        CustomerRecord c;
        c.firstName = trim(fields[0]);
        c.lastName = trim(fields[1]);
        c.street = trim(fields[2]);
        c.city = trim(fields[3]);
        c.state = trim(fields[4]);
        c.zipCode = trim(fields[5]);
        c.phone = trim(fields[6]);

        try {
            c.serviceType = static_cast<ServiceType>(std::stoi(fields[7]));
        }
        catch (...) {
            c.serviceType = ServiceType::Invalid;
        }

        addCustomer(
            c.firstName,
            c.lastName,
            c.street,
            c.city,
            c.state,
            c.zipCode,
            c.phone,
            c.serviceType
        );
        ++imported;
    }

    pImpl->exec("COMMIT");
    return imported > 0;
}

bool Database::importUserCSV(const string& csvPath) {
    if (!pImpl->db) {
        // std::cout << "DEBUG importUserCSV: Database not open\n";
        return false;
    }

    // std::cout << "DEBUG importUserCSV: Attempting to open " << csvPath << "\n";
    std::ifstream file(csvPath);
    if (!file) {
        // std::cout << "DEBUG importUserCSV: Could not open " << csvPath << "\n";
        return false;
    }
    // std::cout << "DEBUG importUserCSV: File opened successfully\n";

    pImpl->exec("BEGIN TRANSACTION");

    string header;
    std::getline(file, header);
    // std::cout << "DEBUG importUserCSV: Header: " << header << "\n";

    string line;
    int imported = 0;
    int lineNum = 1;

    while (std::getline(file, line)) {
        lineNum++;
        if (trim(line).empty()) {
            // std::cout << "DEBUG importUserCSV: Skipping empty line " << lineNum << "\n";
            continue;
        }

        // std::cout << "DEBUG importUserCSV: Processing line " << lineNum << ": " << line << "\n";

        auto fields = parseCSVLine(line);
        // std::cout << "DEBUG importUserCSV: Parsed " << fields.size() << " fields\n";
        
        if (fields.size() < 6) {
            // std::cerr << "DEBUG importUserCSV: Skipping invalid user line " << lineNum 
            //           << " (expected 6 fields, got " << fields.size() << ")\n";
            continue;
        }

        string username = trim(fields[0]);
        string password = trim(fields[1]);
        string firstName = trim(fields[2]);
        string lastName = trim(fields[3]);
        string employeeNumber = trim(fields[4]);
        string role = trim(fields[5]);

        // std::cout << "DEBUG importUserCSV: Attempting to add user: " << username << "\n";

        if (addUser(username, password, firstName, lastName, employeeNumber, role)) {
            ++imported;
        } else {
            // std::cerr << "DEBUG importUserCSV: Failed to add user: " << username << "\n";
        }
    }

    pImpl->exec("COMMIT");
    // std::cout << "DEBUG importUserCSV: Imported " << imported << " users from CSV.\n";
    return imported > 0;
}

void Database::seedDefaultData() {
    pImpl->exec(R"(
        INSERT INTO customers
        (first_name, last_name, street, city, state, zip_code, phone, service_type)
        VALUES
        ('Bob', 'Jones', '100 Main St', 'Springfield', 'IL', '62701', '217-555-0100', 1),
        ('Sarah', 'Davis', '200 Oak Ave', 'Columbus', 'OH', '43085', '614-555-0200', 2)
    )");
}

void Database::seedDefaultUser() {
    pImpl->exec(R"(
        INSERT INTO users (username, password_hash, first_name, last_name, employee_number, role)
        VALUES ('admin', 'admin123', 'System', 'Administrator', 'EMP001', 'admin')
    )");
}