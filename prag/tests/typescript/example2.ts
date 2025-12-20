// Multiple union types
interface FlexibleData {
  id: string | number | boolean;
  value: string | number | User | Post;
  tags: Array<string> | Array<number>;
  metadata?: Map<string, string> | Map<string, number>;
}

// Nested generics
interface Container {
  items: Array<Array<string>>;
  mappings: Map<string, Array<User>>;
  optional?: Array<Map<string, number>>;
}

// Everything at once
interface MegaType {
  // Primitives
  id: string;
  count: number;
  active: boolean;
  
  // Arrays
  tags: string[];
  scores: number[];
  
  // Generics
  users: Array<User>;
  settings: Map<string, string>;
  
  // Unions
  status: string | number | HttpStatus;
  
  // Optionals
  description?: string;
  metadata?: Map<string, string>;
  
  // Nested
  author: User;
  posts: Array<Post>;
  
  // Nested optionals
  notifications?: Array<Notification>;
  
  // Complex nesting
  teams: Map<string, Array<User>>;
  history?: Array<Map<string, string>>;
}
