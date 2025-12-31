from dataclasses import dataclass, field
from typing import Optional, List, Tuple
from typeguard import typechecked


# ----------------------
# Nested dataclass
# ----------------------
@typechecked
@dataclass
class Record:
    event: int = ""
    mark: str = 1

# ----------------------
# Main class with various types
# ----------------------
@typechecked
@dataclass
class Athlete:
    athlete: int = 0
    team1: Optional[int] = None
    scores: List[int] = field(default_factory=list)
    records: Optional[List[Tuple[int, str]]] = None
    nested: Optional[Record] = None
