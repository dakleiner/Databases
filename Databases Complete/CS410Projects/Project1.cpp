#include <iostream>
#include <string>


using namespace std;

int answer;
int choice;
int changechoice;
int newservice;

string username;  


string customer1 = "Bob Jones";
string customer2 = "Sarah Davis";
string customer3 = "Amy Friendly";
string customer4 = "Johnny Smith";
string customer5 = "Carol Spears";

int service1 = 1;
int service2 = 2;
int service3 = 1;
int service4 = 1;
int service5 = 2;


int CheckUserPermissionAccess();
void DisplayInfo();
void ChangeCustomerChoice();

int main() {
    cout << "Hello! Welcome to our Investment Company" << endl;

    do {
        answer = CheckUserPermissionAccess();

        if (answer != 1) {
            cout << "Access Denied! Please try again." << endl;
        }
    } while (answer != 1);

   

    do {
        
        cout << "1. Display Information" << endl;
        cout << "2. Change Customer Choice" << endl;
        cout << "3. Exit" << endl;

        
        while (!(cin >> choice)) {                    
            cout << "Invalid input. Enter a number (1-3): ";
        }
        cout << "You selected: " << choice << endl;

        if (choice == 1) {
            DisplayInfo();
        }
        else if (choice == 2) {
            ChangeCustomerChoice();
        }
        else if (choice == 3) {
            cout << "Exiting. Goodbye!" << endl;
        }
        else {
            cout << "Unknown option. Please select 1-3." << endl;
        }

    } while (choice != 3);

    return 0;
}

int CheckUserPermissionAccess() {
    const string correctPassword = "123";
    string inputPassword;

    cout << "Enter username: ";
    cin >> username;

    cout << "Enter password: ";
    cin >> inputPassword;

    if (inputPassword == correctPassword) {
        return 1;  
    }
    else {
        return 2; 
    }
}

void DisplayInfo() {
   

    cout << "Customer 1: " << customer1 << " - Service: " << service1 << endl;
    cout << "Customer 2: " << customer2 << " - Service: " << service2 << endl;
    cout << "Customer 3: " << customer3 << " - Service: " << service3 << endl;
    cout << "Customer 4: " << customer4 << " - Service: " << service4 << endl;
    cout << "Customer 5: " << customer5 << " - Service: " << service5 << endl;
}

void ChangeCustomerChoice() {
    cout << "Which customer do you want to change? (1-5): ";
    cin >> changechoice;

    cout << "Enter new service number: ";
    cin >> newservice;

   
    if (changechoice == 1) {
        service1 = newservice;
    }
    else if (changechoice == 2) {
        service2 = newservice;
    }
    else if (changechoice == 3) {
        service3 = newservice;
    }
    else if (changechoice == 4) {
        service4 = newservice;
    }
    else if (changechoice == 5) {
        service5 = newservice;
    }
     
}