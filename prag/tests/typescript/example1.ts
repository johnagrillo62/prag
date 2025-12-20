// Basic enum
enum UserRole {
  Admin = 0,
  Moderator = 1,
  User = 2,
  Guest = 3
}

// String enum
enum HttpStatus {
  OK = "200",
  NotFound = "404",
  ServerError = "500"
}

// Simple interface with arrays
interface User {
  id: string;
  username: string;
  email: string;
  roles: UserRole[];
  tags: string[];
  scores: number[];
  active: boolean;
}

// Generics and optionals
interface ApiResponse {
  data: Array<User>;
  metadata: Map<string, string>;
  errors?: Array<string>;
  timestamp?: number;
}

// Union types
interface Post {
  id: string | number;
  title: string;
  content: string;
  status: HttpStatus | UserRole;
  author: User;
  views: number;
}

// Nested types and complex generics
interface Blog {
  id: string;
  name: string;
  owner: User;
  posts: Array<Post>;
  subscribers: Array<User>;
  settings: Map<string, string>;
  moderators?: Array<User>;
}

// Optional everything
interface Comment {
  id?: number;
  text?: string;
  author?: User;
  post?: Post;
  likes?: number;
  replies?: Array<Comment>;
}

// Mixed arrays and unions
interface Notification {
  id: string;
  type: string | number;
  recipient: User;
  content: string;
  read: boolean;
  metadata?: Map<string, string>;
  relatedPosts?: Array<Post>;
}

// Complex nested structure
interface Organization {
  id: string;
  name: string;
  admins: Array<User>;
  members: Array<User>;
  teams: Map<string, Array<User>>;
  projects: Array<Blog>;
  settings: Map<string, string>;
  createdAt?: number;
  updatedAt?: number;
}

// Union with multiple types
interface SearchResult {
  query: string;
  users: Array<User>;
  posts: Array<Post>;
  blogs: Array<Blog>;
  total: number;
  facets?: Map<string, number>;
}

// Deep nesting
interface ActivityFeed {
  user: User;
  activities: Array<Post>;
  notifications: Array<Notification>;
  recommendations: Array<Blog>;
  stats: Map<string, number>;
  preferences?: Map<string, string>;
}

