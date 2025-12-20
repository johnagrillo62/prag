
# lib/from_csv.py

import datetime
import csv
from typing import List, Optional
from dataclasses import fields
from typing import get_type_hints

def from_csv(cls):
    """
    A decorator that will allow the class to read from a CSV string.
    """
    def from_csv_method(csv_data: str) -> List:
        reader = csv.DictReader(csv_data.splitlines())
        instances = []

        for row in reader:
            row_data = {}

            # Get the type hints for the dataclass (used for checking optional/required)
            type_hints = get_type_hints(cls)

            for field in fields(cls):
                field_name = field.name  # Original dataclass field name
                # Get the custom CSV field name from metadata
                csv_column = field.metadata.get('csv_field', field_name)
                # Check if the CSV column exists and map it to the correct field
                if csv_column in row:
                    
                    val = row[csv_column]

                    # Handle empty values or convert types as needed
                    if val == "":
                        val = None
                    elif field.type == int:
                        val = int(val) if val else None
                    elif field.type == bool:
                        val = bool(int(val)) if val is not None else None
                    elif field.type == datetime.datetime:
                        if val:
                            val = datetime.datetime.strptime(val, "%m/%d/%y %H:%M:%S")
                        else:
                            val = None
                    
                    row_data[field_name] = val
                else:
                    # If the CSV doesn't contain a value for this field, set it to None
                    row_data[field_name] = None

            # **Required Fields Validation**:
            # For fields that are NOT Optional, check if they are None or empty
            for field in fields(cls):
                field_name = field.name
                field_type = type_hints.get(field_name)
                
                # Check if the field is required (not Optional)
                if field_type != Optional[field_type] and row_data.get(field_name) is None:
                    raise ValueError(f"Missing required field '{field_name}' in CSV row: {row}")

            # Create an instance of the class and add it to the list
            instance = cls(**row_data)
            instances.append(instance)

        return instances

    cls.from_csv = staticmethod(from_csv_method)
    return cls

