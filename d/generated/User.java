
import bhw.MetaTuple;

import com.opencsv.bean.CsvBindByName;
import java.util.List;
import java.util.ArrayList;

public class User {

    // Fields
    @CsvBindByName(column = "name")
    private String name;
    @CsvBindByName(column = "age")
    private Integer age;
    @CsvBindByName(column = "id")
    private String id;
    @CsvBindByName(column = "data")
    private String data;
    @CsvBindByName(column = "contact")
    private ContactInfo contact;
    @CsvBindByName(column = "employer")
    private Company employer;
    @CsvBindByName(column = "projects")
    private List<Project> projects;
    @CsvBindByName(column = "metadata")
    private Map<String, String> metadata;
    @CsvBindByName(column = "investments")
    private Map<String, Company> investments;
    @CsvBindByName(column = "nested")
    private Map<Map<String, String>, Integer> nested;

    // Constructors
    public User() {}

    public User(String name, Integer age, String id, String data, ContactInfo contact, Company employer, List<Project> projects, Map<String, String> metadata, Map<String, Company> investments, Map<Map<String, String>, Integer> nested) {
        this.name = name;
        this.age = age;
        this.id = id;
        this.data = data;
        this.contact = contact;
        this.employer = employer;
        this.projects = projects;
        this.metadata = metadata;
        this.investments = investments;
        this.nested = nested;
    }

    // Getters and Setters
    public String getName() {
        return name;
    }

    public void setName(String value) {
        this.name = value;
    }

    public Integer getAge() {
        return age;
    }

    public void setAge(Integer value) {
        this.age = value;
    }

    public String getId() {
        return id;
    }

    public void setId(String value) {
        this.id = value;
    }

    public String getData() {
        return data;
    }

    public void setData(String value) {
        this.data = value;
    }

    public ContactInfo getContact() {
        return contact;
    }

    public void setContact(ContactInfo value) {
        this.contact = value;
    }

    public Company getEmployer() {
        return employer;
    }

    public void setEmployer(Company value) {
        this.employer = value;
    }

    public List<Project> getProjects() {
        return projects;
    }

    public void setProjects(List<Project> value) {
        this.projects = value;
    }

    public Map<String, String> getMetadata() {
        return metadata;
    }

    public void setMetadata(Map<String, String> value) {
        this.metadata = value;
    }

    public Map<String, Company> getInvestments() {
        return investments;
    }

    public void setInvestments(Map<String, Company> value) {
        this.investments = value;
    }

    public Map<Map<String, String>, Integer> getNested() {
        return nested;
    }

    public void setNested(Map<Map<String, String>, Integer> value) {
        this.nested = value;
    }

    // Meta Tuples - Runtime field metadata
    public List<MetaTuple<?>> metaTuples() {
        List<MetaTuple<?>> tuples = new ArrayList<>();
        tuples.add(new MetaTuple<>("name", "name", "String", this.name));
        tuples.add(new MetaTuple<>("age", "age", "Integer", this.age));
        tuples.add(new MetaTuple<>("id", "id", "String", this.id));
        tuples.add(new MetaTuple<>("data", "data", "String", this.data));
        tuples.add(new MetaTuple<>("contact", "contact", "ContactInfo", this.contact));
        tuples.add(new MetaTuple<>("employer", "employer", "Company", this.employer));
        tuples.add(new MetaTuple<>("projects", "projects", "List<Project>", this.projects));
        tuples.add(new MetaTuple<>("metadata", "metadata", "Map<String, String>", this.metadata));
        tuples.add(new MetaTuple<>("investments", "investments", "Map<String, Company>", this.investments));
        tuples.add(new MetaTuple<>("nested", "nested", "Map<Map<String, String>, Integer>", this.nested));
        return tuples;
    }

    // Table metadata
    public static String tableName() {
        return "user";
    }
}
