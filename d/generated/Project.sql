-- Table: project
-- Generated from D struct: Project

CREATE TABLE project (
    project_name VARCHAR(255),
    project_description VARCHAR(255),
    project_tags TEXT
);

-- Indexes for project
-- CREATE INDEX idx_project_id ON project(id);

-- Column descriptions for project:
-- project_name: string
-- project_description: string
-- project_tags: string[]
