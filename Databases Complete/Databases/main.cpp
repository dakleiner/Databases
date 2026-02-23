#include "InvestmentSystem.h"
#include <iostream>

int main() {
    InvestmentSystem app;

    std::cout << "Welcome to the Investment System\n";

    if (!app.authenticate()) {
        std::cout << "Authentication failed. Exiting.\n";
        return 0;
    }

    while (true) {
        int choice = app.getMenuChoice();
        switch (choice) {
            case 1:
                app.displayInfo();
                break;
            case 2:
                app.addCustomer();
                break;
            case 3:
                app.updateCustomer();
                break;
            case 4:
                app.changeCustomerService();
                break;
            case 5:
                app.deleteCustomer();
                break;
            case 6:
                app.searchCustomer();
                break;
            case 7:
                std::cout << "Goodbye.\n";
                return 0;
            default:
                std::cout << "Invalid choice. Try again.\n";
                break;
        }
    }

    return 0;
}
