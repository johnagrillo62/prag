pub const Address = struct {
    street: []const u8,
    city: []const u8,
    zipCode: []const u8,
    country: []const u8,

    pub fn init() Address {
        return .{
            .street = "",
            .city = "",
            .zipCode = "",
            .country = "",
        };
    }
};
