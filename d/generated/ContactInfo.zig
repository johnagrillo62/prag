pub const Contactinfo = struct {
    email: []const u8,
    phone: []const u8,
    address: Address,
    previousAddresses: []Address,

    pub fn init() Contactinfo {
        return .{
            .email = "",
            .phone = "",
            .previousAddresses = null,
        };
    }
};
