// Simple interface with primitives
interface User {
  id: string;
  name: string;
  age: number;
  active: boolean;
}

// With optional fields
interface Product {
  id: string;
  title: string;
  price: number;
  description?: string;
}

// Simple array syntax
interface Post {
  id: number;
  title: string;
  tags: string[];
  views: number;
}

// Array generic syntax
interface Comment {
  id: number;
  text: string;
  likes: Array<number>;
}

// Basic enum
enum Status {
  Active = 0,
  Inactive = 1,
  Pending = 2
}

// Using the enum
interface Order {
  id: string;
  status: Status;
  total: number;
}

// Custom type reference
interface Blog {
  title: string;
  author: User;
  posts: Array<Post>;
}
