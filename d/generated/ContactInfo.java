
import bhw.MetaTuple;

import com.opencsv.bean.CsvBindByName;
import java.util.List;
import java.util.ArrayList;

public class Contactinfo {

    // Fields
    @CsvBindByName(column = "email")
    private String email;
    @CsvBindByName(column = "phone")
    private String phone;
    @CsvBindByName(column = "address")
    private Address address;
    @CsvBindByName(column = "previousAddresses")
    private List<Address> previousAddresses;

    // Constructors
    public Contactinfo() {}

    public Contactinfo(String email, String phone, Address address, List<Address> previousAddresses) {
        this.email = email;
        this.phone = phone;
        this.address = address;
        this.previousAddresses = previousAddresses;
    }

    // Getters and Setters
    public String getEmail() {
        return email;
    }

    public void setEmail(String value) {
        this.email = value;
    }

    public String getPhone() {
        return phone;
    }

    public void setPhone(String value) {
        this.phone = value;
    }

    public Address getAddress() {
        return address;
    }

    public void setAddress(Address value) {
        this.address = value;
    }

    public List<Address> getPreviousaddresses() {
        return previousAddresses;
    }

    public void setPreviousaddresses(List<Address> value) {
        this.previousAddresses = value;
    }

    // Meta Tuples - Runtime field metadata
    public List<MetaTuple<?>> metaTuples() {
        List<MetaTuple<?>> tuples = new ArrayList<>();
        tuples.add(new MetaTuple<>("email", "email", "String", this.email));
        tuples.add(new MetaTuple<>("phone", "phone", "String", this.phone));
        tuples.add(new MetaTuple<>("address", "address", "Address", this.address));
        tuples.add(new MetaTuple<>("previousAddresses", "previousAddresses", "List<Address>", this.previousAddresses));
        return tuples;
    }

    // Table metadata
    public static String tableName() {
        return "contactinfo";
    }
}
