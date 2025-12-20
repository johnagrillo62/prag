from typing import List, Dict, Optional, Any
from dataclasses import dataclass, field

@dataclass
class Project:
    name: str = ''
    description: str = ''
    tags: List[str] = None

    def __post_init__(self):
        if self.tags is None:
            self.tags = []
