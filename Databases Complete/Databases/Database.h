#pragma once

#include "ServiceType.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>

struct UserRecord {
    int id = 0;
    std::string username;
    std::string passwordHash;
    std::string firstName;
    std::string lastName;
    std::string employeeNumber;
    std::string role;
};

struct CustomerRecord {
    int id = 0;
    std::string firstName;
    std::string lastName;
    std::string street;
    std::string city;
    std::string state;
    std::string zipCode;
    std::string phone;
    ServiceType serviceType = ServiceType::Invalid;

    CustomerRecord() = default;

    CustomerRecord(
        int id_,
        std::string first,
        std::string last,
        std::string street_,
        std::string city_,
        std::string state_,
        std::string zip,
        std::string phone_,
        ServiceType svc
    )
        : id(id_)
        , firstName(std::move(first))
        , lastName(std::move(last))
        , street(std::move(street_))
        , city(std::move(city_))
        , state(std::move(state_))
        , zipCode(std::move(zip))
        , phone(std::move(phone_))
        , serviceType(svc) {
    }
};

class Database {
public:
    explicit Database(const std::string& dbFile = "investments.db");
    ~Database(); // Must be declared in header but defined in .cpp for Pimpl

    // Non-copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Customer operations
    std::vector<CustomerRecord> loadCustomers() const;
    std::vector<CustomerRecord> getAllCustomers() const;
    std::optional<CustomerRecord> getCustomer(int id) const;
    void addCustomer(
        const std::string& firstName,
        const std::string& lastName,
        const std::string& street,
        const std::string& city,
        const std::string& state,
        const std::string& zipCode,
        const std::string& phone,
        ServiceType serviceType
    );
    bool updateCustomer(
        int id,
        const std::string& firstName,
        const std::string& lastName,
        const std::string& street,
        const std::string& city,
        const std::string& state,
        const std::string& zipCode,
        const std::string& phone,
        ServiceType serviceType
    );
    void updateCustomerService(int id, ServiceType serviceType);
    bool deleteCustomer(int id);
    std::vector<CustomerRecord> searchCustomers(const std::string& searchTerm) const;

    // User operations
    std::optional<UserRecord> getUserByUsername(const std::string& username) const;
    std::optional<UserRecord> getUser(const std::string& username) const;
    bool addUser(
        const std::string& username,
        const std::string& passwordPlaintext,
        const std::string& firstName = "",
        const std::string& lastName = "",
        const std::string& employeeNumber = "",
        const std::string& role = "user"
    );

    // Utility
    bool importCSV(const std::string& csvPath = "customer_data.csv");
    bool importUserCSV(const std::string& csvPath = "user_data.csv");
    int getCustomerCount() const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl;

    void createTables();
    void seedDefaultData();
    int getUserCount() const;
    void seedDefaultUser();
};
