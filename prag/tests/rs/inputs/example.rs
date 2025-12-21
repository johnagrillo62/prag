#[derive(Default)]
pub struct Comment {
    id: i32,
    content: String,
}

#[derive(Debug)]
pub enum Role {
    Admin,
    User,
    Guest,
}