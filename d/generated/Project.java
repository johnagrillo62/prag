
import bhw.MetaTuple;

import com.opencsv.bean.CsvBindByName;
import java.util.List;
import java.util.ArrayList;

public class Project {

    // Fields
    @CsvBindByName(column = "name")
    private String name;
    @CsvBindByName(column = "description")
    private String description;
    @CsvBindByName(column = "tags")
    private List<String> tags;

    // Constructors
    public Project() {}

    public Project(String name, String description, List<String> tags) {
        this.name = name;
        this.description = description;
        this.tags = tags;
    }

    // Getters and Setters
    public String getName() {
        return name;
    }

    public void setName(String value) {
        this.name = value;
    }

    public String getDescription() {
        return description;
    }

    public void setDescription(String value) {
        this.description = value;
    }

    public List<String> getTags() {
        return tags;
    }

    public void setTags(List<String> value) {
        this.tags = value;
    }

    // Meta Tuples - Runtime field metadata
    public List<MetaTuple<?>> metaTuples() {
        List<MetaTuple<?>> tuples = new ArrayList<>();
        tuples.add(new MetaTuple<>("name", "name", "String", this.name));
        tuples.add(new MetaTuple<>("description", "description", "String", this.description));
        tuples.add(new MetaTuple<>("tags", "tags", "List<String>", this.tags));
        return tuples;
    }

    // Table metadata
    public static String tableName() {
        return "project";
    }
}
