
import bhw.MetaTuple;

import com.opencsv.bean.CsvBindByName;
import java.util.List;
import java.util.ArrayList;

public class Company {

    // Fields
    @CsvBindByName(column = "name")
    private String name;
    @CsvBindByName(column = "headquarters")
    private Address headquarters;
    @CsvBindByName(column = "taxId")
    private String taxId;
    @CsvBindByName(column = "offices")
    private Map<String, Address> offices;

    // Constructors
    public Company() {}

    public Company(String name, Address headquarters, String taxId, Map<String, Address> offices) {
        this.name = name;
        this.headquarters = headquarters;
        this.taxId = taxId;
        this.offices = offices;
    }

    // Getters and Setters
    public String getName() {
        return name;
    }

    public void setName(String value) {
        this.name = value;
    }

    public Address getHeadquarters() {
        return headquarters;
    }

    public void setHeadquarters(Address value) {
        this.headquarters = value;
    }

    public String getTaxid() {
        return taxId;
    }

    public void setTaxid(String value) {
        this.taxId = value;
    }

    public Map<String, Address> getOffices() {
        return offices;
    }

    public void setOffices(Map<String, Address> value) {
        this.offices = value;
    }

    // Meta Tuples - Runtime field metadata
    public List<MetaTuple<?>> metaTuples() {
        List<MetaTuple<?>> tuples = new ArrayList<>();
        tuples.add(new MetaTuple<>("name", "name", "String", this.name));
        tuples.add(new MetaTuple<>("headquarters", "headquarters", "Address", this.headquarters));
        tuples.add(new MetaTuple<>("taxId", "taxId", "String", this.taxId));
        tuples.add(new MetaTuple<>("offices", "offices", "Map<String, Address>", this.offices));
        return tuples;
    }

    // Table metadata
    public static String tableName() {
        return "company";
    }
}
