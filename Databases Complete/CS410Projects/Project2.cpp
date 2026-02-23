#include <iostream>
#include <string>
#include <limits> // For std::numeric_limits

using namespace std;

// Structure to hold customer data
struct Customer {
    string name;
    int service;
};

// Structure to hold user credentials
struct User {
    string username;
    string password;
};

// Function declarations
int CheckUserPermissionAccess(string& username);
void DisplayInfo(const Customer customers[], int customerCount);
void ChangeCustomerChoice(Customer customers[], int customerCount);
void DisplayMenu();

int main() {
    cout << "Hello! Welcome to our Investment Company" << endl;

    // Local variables instead of globals
    // VULNERABILITY FIX: Removed global variables to prevent unintended state sharing
    // and improve thread safety. Variables are now properly scoped.
    string username;
    int answer;
    int choice;

    // Initialize customers array
    // VULNERABILITY FIX: Used array structure instead of separate global variables
    // to prevent direct access and modification from anywhere in the program.
    Customer customers[5] = {
        {"Bob Jones", 1},
        {"Sarah Davis", 2},
        {"Amy Friendly", 1},
        {"Johnny Smith", 1},
        {"Carol Spears", 2}
    };
    const int customerCount = 5;

    // Authentication loop
    do {
        answer = CheckUserPermissionAccess(username);

        if (answer != 1) {
            cout << "Access Denied! Please try again." << endl;
        }
    } while (answer != 1);

    cout << "\nAccess Granted! Welcome, " << username << "!" << endl;

    // Main menu loop
    do {
        DisplayMenu();

        // VULNERABILITY FIX: Added proper input validation with error recovery
        // Prevents infinite loop when non-numeric input is entered (e.g., letters)
        if (!(cin >> choice)) {
            cin.clear();  // Clear error flags
            cin.ignore(numeric_limits<streamsize>::max(), '\n');  // Discard invalid input
            cout << "Invalid input. Please enter a number (1-3)." << endl;
            continue;  // Skip to next iteration
        }

        // VULNERABILITY FIX: Added range validation to prevent out-of-bounds menu choices
        // that could cause unexpected behavior
        if (choice < 1 || choice > 3) {
            cout << "Invalid choice. Please select 1-3." << endl;
            continue;
        }

        cout << "You selected: " << choice << endl;

        switch (choice) {
            case 1:
                DisplayInfo(customers, customerCount);
                break;
            case 2:
                ChangeCustomerChoice(customers, customerCount);
                break;
            case 3:
                cout << "\nExiting. Goodbye, " << username << "!" << endl;
                break;
        }

    } while (choice != 3);

    return 0;
}

int CheckUserPermissionAccess(string& username) {
    // Array of valid users
    // SECURITY NOTE: In production, passwords should be hashed (e.g., using SHA-256)
    // and stored securely, not in plaintext
    const int userCount = 3;
    User validUsers[userCount] = {
        {"admin", "admin123"},
        {"manager", "manage456"},
        {"user1", "pass789"},
    };
    
    string inputPassword;

    cout << "Enter username: ";
    cin >> username;

    cout << "Enter password: ";
    cin >> inputPassword;

    // Validate credentials
    for (int i = 0; i < userCount; i++) {
        if (username == validUsers[i].username && 
            inputPassword == validUsers[i].password) {
            return 1;  // Authentication successful
        }
    }
    
    return 2;  // Authentication failed
}

void DisplayInfo(const Customer customers[], int customerCount) {
    cout << "\n=== Customer Information ===" << endl;
    // VULNERABILITY FIX: Used loop with customerCount parameter instead of hardcoded values
    // Prevents accessing invalid array indices
    for (int i = 0; i < customerCount; i++) {
        cout << "Customer " << (i + 1) << ": " << customers[i].name 
             << " - Service: " << customers[i].service << endl;
    }
}

void ChangeCustomerChoice(Customer customers[], int customerCount) {
    int changechoice;
    int newservice;

    cout << "\nWhich customer do you want to change? (1-5): ";
    
    // VULNERABILITY FIX: Added input validation to handle non-numeric input
    // Prevents crash or infinite loop when user enters invalid data
    if (!(cin >> changechoice)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Operation cancelled." << endl;
        return;
    }

    // VULNERABILITY FIX: Added bounds checking to prevent array out-of-bounds access
    // Original code would silently fail or access invalid memory if choice was out of range
    if (changechoice < 1 || changechoice > customerCount) {
        cout << "Invalid customer number. Please select 1-" << customerCount << "." << endl;
        return;
    }

    cout << "Enter new service number (1 or 2): ";
    
    // VULNERABILITY FIX: Added input validation for service number
    // Prevents undefined behavior from invalid input types
    if (!(cin >> newservice)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input. Operation cancelled." << endl;
        return;
    }

    // VULNERABILITY FIX: Added validation to restrict service choices to only 1 or 2
    // Prevents invalid service numbers from being assigned
    if (newservice != 1 && newservice != 2) {
        cout << "Invalid service number. Please enter 1 or 2 only." << endl;
        return;
    }

    // VULNERABILITY FIX: Array-based access with validated index prevents
    // the need for multiple if-else statements that could be missed or incorrectly modified
    customers[changechoice - 1].service = newservice;
    
    cout << "Customer " << changechoice << " (" << customers[changechoice - 1].name 
         << ") service updated to " << newservice << endl;
}

void DisplayMenu() {
    cout << "\n=== Main Menu ===" << endl;
    cout << "1. Display Information" << endl;
    cout << "2. Change Customer Choice" << endl;
    cout << "3. Exit" << endl;
    cout << "Enter your choice (1-3): ";
}