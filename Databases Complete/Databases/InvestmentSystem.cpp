#include "InvestmentSystem.h"
#include "Database.h"

#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace std;

// Single DB instance for this translation unit
static std::unique_ptr<Database> g_db;

// =====================
// Constructor
// =====================
InvestmentSystem::InvestmentSystem() {
    cout << "Starting Investment System...\n";

    try {
        g_db = std::make_unique<Database>("investments.db");
        refreshCustomers();
        //cout << "Database initialized successfully.\n";
    }
    catch (const std::exception& e) {
        cerr << "Fatal error initializing database: " << e.what() << '\n';
        throw; // hard fail — no fallback
    }
}

// =====================
// Load Customers From DB
// =====================
void InvestmentSystem::refreshCustomers() {
    customers.clear();

    auto records = g_db->loadCustomers();
    for (const auto& r : records) {
        customers.emplace_back(
            r.id,
            r.firstName,
            r.lastName,
            r.street,
            r.city,
            r.state,
            r.zipCode,
            r.phone,
            r.serviceType
        );
    }

    //cout << "Loaded " << customers.size() << " customers from database.\n";
}

// =====================
// Authentication
// =====================
bool InvestmentSystem::authenticate() {
    string inputPassword;

    cout << "Enter username: ";
    cin >> username;
    cout << "Enter password: ";
    cin >> inputPassword;

    auto userOpt = g_db->getUserByUsername(username);
    if (!userOpt) {
        cerr << "DEBUG: User '" << username << "' not found in database.\n";
        return false;
    }

   // cout << "DEBUG: Found user. Expected password: " << userOpt->passwordHash << "\n";
    //cout << "DEBUG: Entered password: " << inputPassword << "\n";

    // NOTE: Replace with real hash comparison later
    return userOpt->passwordHash == inputPassword;
}

// =====================
// Display Customers
// =====================
void InvestmentSystem::displayInfo() const {
    cout << "\n--- Customer Information ---\n";
    cout << "Logged in as: " << username << '\n';
    cout << "Service Types: 1 = Brokerage, 2 = Retirement\n";

    for (size_t i = 0; i < customers.size(); ++i) {
        const auto& c = customers[i];
        cout << "Customer " << (i + 1)
            << " [DB id=" << c.id << "]\n"
            << "  Name: " << c.firstName << " " << c.lastName << '\n'
            << "  Service: " << getServiceTypeName(c.serviceType) << '\n'
            << "  Address: " << c.street << ", "
            << c.city << ", " << c.state << " " << c.zipCode << '\n'
            << "  Phone: " << c.phone << "\n\n";
    }
}

// =====================
// Add Customer
// =====================
void InvestmentSystem::addCustomer() {
    string first, last, street, city, state, zip, phone;
    int service;

    cout << "First name: "; cin >> first;
    cout << "Last name: ";  cin >> last;

    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Street: "; getline(cin, street);
    cout << "City: ";   getline(cin, city);
    cout << "State: ";  getline(cin, state);
    cout << "ZIP: ";    getline(cin, zip);
    cout << "Phone: ";  getline(cin, phone);

    cout << "Service (1=Brokerage, 2=Retirement): ";
    if (!(cin >> service) || !isValidServiceType(service)) {
        cout << "Invalid service type.\n";
        return;
    }

    g_db->addCustomer(
        first, last, street, city, state, zip, phone,
        static_cast<ServiceType>(service)
    );

    refreshCustomers();
    cout << "Customer added successfully.\n";
}

// =====================
// Update Customer
// =====================
void InvestmentSystem::updateCustomer() {
    int idx;
    cout << "Customer number to edit: ";
    if (!(cin >> idx) || idx < 1 || idx > static_cast<int>(customers.size())) {
        return;
    }

    auto& cust = customers[idx - 1];
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    string input;

    cout << "First name (" << cust.firstName << "): ";
    getline(cin, input); if (!input.empty()) cust.firstName = input;

    cout << "Last name (" << cust.lastName << "): ";
    getline(cin, input); if (!input.empty()) cust.lastName = input;

    cout << "Street (" << cust.street << "): ";
    getline(cin, input); if (!input.empty()) cust.street = input;

    cout << "City (" << cust.city << "): ";
    getline(cin, input); if (!input.empty()) cust.city = input;

    cout << "State (" << cust.state << "): ";
    getline(cin, input); if (!input.empty()) cust.state = input;

    cout << "ZIP (" << cust.zipCode << "): ";
    getline(cin, input); if (!input.empty()) cust.zipCode = input;

    cout << "Phone (" << cust.phone << "): ";
    getline(cin, input); if (!input.empty()) cust.phone = input;

    cout << "Service (1=Brokerage, 2=Retirement, blank to keep): ";
    getline(cin, input);
    if (!input.empty()) {
        int svc = stoi(input);
        if (isValidServiceType(svc)) {
            cust.serviceType = static_cast<ServiceType>(svc);
        }
    }

    g_db->updateCustomer(
        cust.id,
        cust.firstName,
        cust.lastName,
        cust.street,
        cust.city,
        cust.state,
        cust.zipCode,
        cust.phone,
        cust.serviceType
    );

    refreshCustomers();
    cout << "Customer updated.\n";
}

// =====================
// Change Service
// =====================
void InvestmentSystem::changeCustomerService() {
    int idx, svc;

    cout << "Customer number: ";
    if (!(cin >> idx) || idx < 1 || idx > static_cast<int>(customers.size())) {
        return;
    }

    cout << "New service (1=Brokerage, 2=Retirement): ";
    if (!(cin >> svc) || !isValidServiceType(svc)) {
        return;
    }

    g_db->updateCustomerService(
        customers[idx - 1].id,
        static_cast<ServiceType>(svc)
    );

    refreshCustomers();
    cout << "Service updated.\n";
}

// =====================
// Delete Customer
// =====================
void InvestmentSystem::deleteCustomer() {
    int idx;
    cout << "Customer number to delete: ";
    if (!(cin >> idx) || idx < 1 || idx > static_cast<int>(customers.size())) {
        return;
    }

    g_db->deleteCustomer(customers[idx - 1].id);
    refreshCustomers();

    cout << "Customer deleted.\n";
}

// =====================
// Search Customer
// =====================
void InvestmentSystem::searchCustomer() {
    string searchTerm;
    
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Enter name to search: ";
    getline(cin, searchTerm);

    if (searchTerm.empty()) {
        cout << "Search term cannot be empty.\n";
        return;
    }

    auto results = g_db->searchCustomers(searchTerm);

    if (results.empty()) {
        cout << "No customers found matching '" << searchTerm << "'.\n";
        return;
    }

    cout << "\n--- Search Results (" << results.size() << " found) ---\n";
    for (const auto& c : results) {
        cout << "[DB id=" << c.id << "] "
             << c.firstName << " " << c.lastName
             << " - " << getServiceTypeName(c.serviceType)
             << " - " << c.phone << '\n';
    }
    cout << '\n';
}

// =====================
// Menu Helpers
// =====================
int InvestmentSystem::getMenuChoice() {
    int choice;
    cout << "1. Display All Customers" << endl;
    cout << "2. Add Customer" << endl;
    cout << "3. Edit Customer" << endl; 
    cout << "4. Change Service" << endl;
    cout << "5. Delete Customer" << endl;
    cout << "6. Search Customer" << endl;
    cout << "7. Exit" << endl;
    cout << "Choice: ";
    if (!(cin >> choice)) return -1;
    return choice;
}

bool InvestmentSystem::isValidServiceType(int svc) const {
    return svc >= static_cast<int>(ServiceType::Brokerage)
        && svc <= static_cast<int>(ServiceType::Retirement);
}

string InvestmentSystem::getServiceTypeName(ServiceType type) const {
    switch (type) {
    case ServiceType::Brokerage:  return "Brokerage";
    case ServiceType::Retirement: return "Retirement";
    default: return "Unknown";
    }
}
