pub const Company = struct {
    name: []const u8,
    headquarters: Address,
    taxId: []const u8,
    offices: std.AutoHashMap([]const u8, Address),

    pub fn init() Company {
        return .{
            .name = "",
            .taxId = "",
            .offices = null,
        };
    }
};
