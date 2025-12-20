from typing import List, Dict, Optional, Any
from dataclasses import dataclass, field

@dataclass
class Company:
    name: str = ''
    headquarters: Address
    taxId: str = ''
    offices: Dict[str, Address] = None

    def __post_init__(self):
        if self.offices is None:
            self.offices = {}
