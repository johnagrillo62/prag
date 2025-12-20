using System;
using System.Collections.Generic;

namespace ExampleStructs
{
    // Example struct with various types
    public struct User
    {
        public string Name { get; set; }
        public int Age { get; set; }
        public string? Email { get; set; }
        public List<string> Tags { get; set; }
    }

    public struct Product
    {
        public int Id;
        public string Title;
        public decimal Price;
        public List<string> Categories;
        public Dictionary<string, string> Metadata;
    }

    public struct Order
    {
        public int OrderId { get; set; }
        public List<Product> Items { get; set; }
        public DateTime CreatedAt { get; set; }
        public HashSet<string> SpecialRequests { get; set; }
    }

    public struct Response<T>
    {
        public bool Success { get; set; }
        public T? Data { get; set; }
        public string? ErrorMessage { get; set; }
    }
}
