from typing import List, Dict, Optional, Any
from dataclasses import dataclass, field

@dataclass
class User:
    name: str = ''
    age: int = 0
    id: str = ''
    data: str = ''
    contact: ContactInfo
    employer: Company
    projects: List[Project] = None
    metadata: Dict[str, str] = None
    investments: Dict[str, Company] = None
    nested: Dict[Dict[str, str], int] = None

    def __post_init__(self):
        if self.projects is None:
            self.projects = []
        if self.metadata is None:
            self.metadata = {}
        if self.investments is None:
            self.investments = {}
        if self.nested is None:
            self.nested = {}
