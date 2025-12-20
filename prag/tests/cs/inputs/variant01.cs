using System;
using System.Collections.Generic;

public class Employee
{
  public string Name { get; set; }
  public Employment_type Employment_type { get; set; }
}

public class Person
{
  public string Name { get; set; }
  public int Id { get; set; }
  public Contact Contact { get; set; }
}

public class Company
{
  public string Name { get; set; }
  public List<Employee> Employees { get; set; }
}

public class Entity
{
  public Entity_type Entity_type { get; set; }
}

public abstract record Employment_type;
public record Employment_typeFull_time(bool Value) : Employment_type;
public record Employment_typeContractor(string Value) : Employment_type;

public abstract record Contact;
public record ContactEmail(string Value) : Contact;
public record ContactPhone(string Value) : Contact;

public abstract record Entity_type;
public record Entity_typePerson(Person Value) : Entity_type;
public record Entity_typeCompany(Company Value) : Entity_type;


