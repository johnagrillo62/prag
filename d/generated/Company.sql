-- Table: companies
-- Generated from D struct: Company

CREATE TABLE business.companies (
    company_name VARCHAR(255),
    headquarters_address TEXT,
    tax_identifier VARCHAR(255),
    office_locations TEXT
);

-- Indexes for companies
-- CREATE INDEX idx_companies_id ON companies(id);

-- Column descriptions for companies:
-- company_name: string
-- headquarters_address: Address
-- tax_identifier: string
-- office_locations: Address[string]
