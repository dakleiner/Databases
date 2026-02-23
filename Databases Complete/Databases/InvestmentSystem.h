#pragma once

#include "ServiceType.h"
#include <string>
#include <vector>

// =====================
// Customer Model
// =====================
struct Customer {
    int id = 0;
    std::string firstName;
    std::string lastName;
    std::string street;
    std::string city;
    std::string state;
    std::string zipCode;
    std::string phone;
    ServiceType serviceType = ServiceType::Invalid;

    Customer() = default;

    Customer(int id_,
        std::string first,
        std::string last,
        std::string street_,
        std::string city_,
        std::string state_,
        std::string zip,
        std::string phone_,
        ServiceType svc)
        : id(id_),
        firstName(std::move(first)),
        lastName(std::move(last)),
        street(std::move(street_)),
        city(std::move(city_)),
        state(std::move(state_)),
        zipCode(std::move(zip)),
        phone(std::move(phone_)),
        serviceType(svc) {
    }
};

// =====================
// Investment System
// =====================
class InvestmentSystem {
public:
    // =====================
    // Lifecycle
    // =====================
    InvestmentSystem();

    // =====================
    // Authentication
    // =====================
    bool authenticate();

    // =====================
    // Menu Actions
    // =====================
    void displayInfo() const;
    void addCustomer();
    void updateCustomer();
    void changeCustomerService();
    void deleteCustomer();
    void searchCustomer();

    // =====================
    // Menu Helpers
    // =====================
    int getMenuChoice();

private:
    // =====================
    // Internal Helpers
    // =====================
    void refreshCustomers();
    bool isValidServiceType(int svc) const;
    std::string getServiceTypeName(ServiceType type) const;

private:
    // =====================
    // State
    // =====================
    std::string username;

    // Cached view of database customers
    std::vector<Customer> customers;
};
