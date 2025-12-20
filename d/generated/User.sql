-- Table: users
-- Generated from D struct: User

CREATE TABLE public.users (
    full_name VARCHAR(255),
    user_age INTEGER,
    user_id VARCHAR(255),
    user_data VARCHAR(255),
    contact_info TEXT,
    employer_info TEXT,
    user_projects TEXT,
    user_metadata TEXT,
    investment_portfolio TEXT,
    investment_portfolio TEXT
);

-- Indexes for users
-- CREATE INDEX idx_users_id ON users(id);

-- Column descriptions for users:
-- full_name: string
-- user_age: int
-- user_id: string
-- user_data: string
-- contact_info: ContactInfo
-- employer_info: Company
-- user_projects: Project[]
-- user_metadata: string[string]
-- investment_portfolio: Company[string]
-- investment_portfolio: int[const(string)[string]]
