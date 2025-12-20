#![allow(dead_code)]

pub mod demo {
  #[derive(Debug, Clone)]
  pub struct MyClass {
  }

  #[derive(Debug, Clone)]
  pub struct Complex {
    pub id: i32,
    pub string: String,
    pub vector: Vec<MyClass>,
    pub stringStringMap: std::collections::HashMap<String, std::collections::HashMap<String, String>>,
  }

} // mod demo

