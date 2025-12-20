module models;

import core.types;

// ============================================================================
// DEMONSTRATION: REAL-WORLD DOMAIN MODEL
// ============================================================================

@table("addresses", "public")
@typeStyle(TypeStyle.ValueObject)
struct Address {
    @field(AccessLevel.Public, "street_address") string street;
    @field(AccessLevel.Public, "city_name") string city;
    @field(AccessLevel.Public, "postal_code") string zipCode;
    @field(AccessLevel.ReadOnly, "country_code") string country;
}

@table("contact_information") 
@typeStyle(TypeStyle.ValueObject)
struct ContactInfo {
    @field(AccessLevel.Private, "email_address") string email;
    @field(AccessLevel.Public, "phone_number") string phone;
    @field(AccessLevel.Public, "primary_address") Address address;
    @field(AccessLevel.Public, "address_history") Address[] previousAddresses;
}

@table("companies", "business")
@typeStyle(TypeStyle.ValueObject)
struct Company {
    @field(AccessLevel.ReadOnly, "company_name") string name;
    @field(AccessLevel.Public, "headquarters_address") Address headquarters;
    @field(AccessLevel.Private, "tax_identifier") string taxId;
    @field(AccessLevel.Public, "office_locations") Address[string] offices;
}

@typeStyle(TypeStyle.ValueObject)
struct Project {
    @field(AccessLevel.Public, "project_name") string name;
    @field(AccessLevel.Public, "project_description") string description;
    @field(AccessLevel.Public, "project_tags") string[] tags;
}

@typeStyle(TypeStyle.ValueObject)
@table("users", "public")
struct User {
    @field(AccessLevel.Private, "full_name") string name;
    @field(AccessLevel.Public, "user_age") int age;
    @field(AccessLevel.ReadOnly, "user_id") string id;
    @field(AccessLevel.Private, FieldModifier.Unique, "user_data") string data;
    @field(AccessLevel.Public, "contact_info") ContactInfo contact;
    @field(AccessLevel.Public, FieldModifier.Optional, "employer_info") Company employer;
    @field(AccessLevel.Public, "user_projects") Project[] projects;
    @field(AccessLevel.Public, "user_metadata") string[string] metadata;
    @field(AccessLevel.Public, "investment_portfolio") Company[string] investments;
    @field(AccessLevel.Public, "investment_portfolio") int[string[string]] nested;    
}

