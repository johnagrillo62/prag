
import bhw.MetaTuple;

import com.opencsv.bean.CsvBindByName;
import java.util.List;
import java.util.ArrayList;

public class Address {

    // Fields
    @CsvBindByName(column = "street")
    private String street;
    @CsvBindByName(column = "city")
    private String city;
    @CsvBindByName(column = "zipCode")
    private String zipCode;
    @CsvBindByName(column = "country")
    private String country;

    // Constructors
    public Address() {}

    public Address(String street, String city, String zipCode, String country) {
        this.street = street;
        this.city = city;
        this.zipCode = zipCode;
        this.country = country;
    }

    // Getters and Setters
    public String getStreet() {
        return street;
    }

    public void setStreet(String value) {
        this.street = value;
    }

    public String getCity() {
        return city;
    }

    public void setCity(String value) {
        this.city = value;
    }

    public String getZipcode() {
        return zipCode;
    }

    public void setZipcode(String value) {
        this.zipCode = value;
    }

    public String getCountry() {
        return country;
    }

    public void setCountry(String value) {
        this.country = value;
    }

    // Meta Tuples - Runtime field metadata
    public List<MetaTuple<?>> metaTuples() {
        List<MetaTuple<?>> tuples = new ArrayList<>();
        tuples.add(new MetaTuple<>("street", "street", "String", this.street));
        tuples.add(new MetaTuple<>("city", "city", "String", this.city));
        tuples.add(new MetaTuple<>("zipCode", "zipCode", "String", this.zipCode));
        tuples.add(new MetaTuple<>("country", "country", "String", this.country));
        return tuples;
    }

    // Table metadata
    public static String tableName() {
        return "address";
    }
}
