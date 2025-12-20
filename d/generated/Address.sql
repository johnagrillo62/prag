-- Table: addresses
-- Generated from D struct: Address

CREATE TABLE public.addresses (
    street_address VARCHAR(255),
    city_name VARCHAR(255),
    postal_code VARCHAR(255),
    country_code VARCHAR(255)
);

-- Indexes for addresses
-- CREATE INDEX idx_addresses_id ON addresses(id);

-- Column descriptions for addresses:
-- street_address: string
-- city_name: string
-- postal_code: string
-- country_code: string
