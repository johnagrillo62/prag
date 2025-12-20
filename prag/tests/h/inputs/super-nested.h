struct Item
{
}

// Response types
struct ApiResponse {
    int status;
    
    struct Error {           
        int code;
        std::string message;
    };
    
    struct Data {            
        std::vector<Item> items;
    };
};

// Builder pattern
struct ConfigBuilder {
    struct DatabaseConfig {  
        std::string host;
        int port;
    };
    
    struct CacheConfig {     
        int size;
        int ttl;
    };
};

// Nested state machines
struct Game {
    struct Player {          
        std::string name;
        int score;
    };
    
    struct Enemy {           
        int health;
        int damage;
    };
};

