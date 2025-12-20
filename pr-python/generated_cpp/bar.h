struct Bar {
    std::unique_ptr<Baz> baz_item;
    Optional optional_val;
    std::map<std::string, std::string> scores;
    std::chrono::system_clock::time_point timestamp;

    std::unique_ptr<Baz> getBazItem() const { return baz_item; }
    void setBazItem(const std::unique_ptr<Baz>& value) { baz_item = value; }
    Optional getOptionalVal() const { return optional_val; }
    void setOptionalVal(const Optional& value) { optional_val = value; }
    std::map<std::string, std::string> getScores() const { return scores; }
    void setScores(const std::map<std::string, std::string>& value) { scores = value; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp; }
    void setTimestamp(const std::chrono::system_clock::time_point& value) { timestamp = value; }
};