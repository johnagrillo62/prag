pub const User = struct {
    name: []const u8,
    age: i32,
    id: []const u8,
    data: []const u8,
    contact: ContactInfo,
    employer: Company,
    projects: []Project,
    metadata: std.AutoHashMap([]const u8, []const u8),
    investments: std.AutoHashMap([]const u8, Company),
    nested: std.AutoHashMap(std.AutoHashMap([]const u8, []const u8), i32),

    pub fn init() User {
        return .{
            .name = "",
            .age = 0,
            .id = "",
            .data = "",
            .projects = null,
            .metadata = null,
            .investments = null,
            .nested = null,
        };
    }
};
