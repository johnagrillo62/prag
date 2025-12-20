-- Table: contact_information
-- Generated from D struct: ContactInfo

CREATE TABLE contact_information (
    email_address VARCHAR(255),
    phone_number VARCHAR(255),
    primary_address TEXT,
    address_history TEXT
);

-- Indexes for contact_information
-- CREATE INDEX idx_contact_information_id ON contact_information(id);

-- Column descriptions for contact_information:
-- email_address: string
-- phone_number: string
-- primary_address: Address
-- address_history: Address[]
