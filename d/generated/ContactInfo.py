from typing import List, Dict, Optional, Any
from dataclasses import dataclass, field

@dataclass
class Contactinfo:
    email: str = ''
    phone: str = ''
    address: Address
    previousAddresses: List[Address] = None

    def __post_init__(self):
        if self.previousAddresses is None:
            self.previousAddresses = []
