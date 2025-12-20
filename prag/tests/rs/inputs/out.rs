#![allow(dead_code)]

pub mod demo {
  #[derive(Debug, Clone)]
  pub struct ComplexRow {
    pub id: i32,
    pub name: String,
    pub scores: Vec<i32>,
    pub tags: Vec<String>,
    pub matrix: Vec<Vec<i32>>,
    pub categories: Vec<Vec<String>>,
  }
} // mod demo


