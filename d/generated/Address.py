from typing import List, Dict, Optional, Any
from dataclasses import dataclass, field

@dataclass
class Address:
    street: str = ''
    city: str = ''
    zipCode: str = ''
    country: str = ''

    def __post_init__(self):
        pass
