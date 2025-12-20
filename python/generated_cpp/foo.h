struct Foo {
    std::vector<Bar> bar_items;
    std::unique_ptr<Bar> maybe_bar;
    std::map<std::string, std::string> lookup;
    Optional created_at;

    std::vector<Bar> getBarItems() const { return bar_items; }
    void setBarItems(const std::vector<Bar>& value) { bar_items = value; }
    std::unique_ptr<Bar> getMaybeBar() const { return maybe_bar; }
    void setMaybeBar(const std::unique_ptr<Bar>& value) { maybe_bar = value; }
    std::map<std::string, std::string> getLookup() const { return lookup; }
    void setLookup(const std::map<std::string, std::string>& value) { lookup = value; }
    Optional getCreatedAt() const { return created_at; }
    void setCreatedAt(const Optional& value) { created_at = value; }
};