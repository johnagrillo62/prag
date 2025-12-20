pub const Project = struct {
    name: []const u8,
    description: []const u8,
    tags: [][]const u8,

    pub fn init() Project {
        return .{
            .name = "",
            .description = "",
            .tags = null,
        };
    }
};
