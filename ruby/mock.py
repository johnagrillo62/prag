from src.python.tm import team
from src.python.tm import result

from enum import Enum
from collections import defaultdict

class MeetScore(Enum):
    Dual = 1
    Mock = 2
    Div = 3

def get_points_per_event(meet_score: MeetScore):
    def generate(default, overrides):
        points = {e: default[:] for e in range(1, 69)}
        for e, value in overrides.items():
            points[e] = value[:]
        return points

    dual_overrides = {e: [800, 400, 200] for e in [31, 32, 33, 34, 67, 68]}
    mock_overrides = {e: [800, 400] for e in [31, 32, 33, 34, 67, 68]}
    div_overrides = {e: [1000, 700, 400, 200] for e in [31, 32, 33, 34, 67, 68]}

    match meet_score:
        case MeetScore.Dual:
            return generate([500, 300, 100], dual_overrides)
        case MeetScore.Mock:
            return generate([500, 300, 100], mock_overrides)
        case MeetScore.Div:
            return generate([700, 500, 400, 300, 200, 100], div_overrides)
        case _:
            return generate([500, 300, 100], dual_overrides)
                    
from collections import defaultdict

def split_points_evenly(points, count):
    """Splits points evenly, ensuring the total points are distributed correctly."""
    if count == 0:
        return []
    
    total = sum(points)  # Calculate the total points
    base = total // count  # The base points for each entry
    remainder = total % count  # How much "extra" needs to be distributed
    
    # Distribute the points to account for the remainder
    return [base + 1 if i < remainder else base for i in range(count)]

def score(entries, event_points):
    """Scores the event entries based on their adjusted times and points distribution."""
    events = defaultdict(list)
    
    # Group entries by their event
    for entry in entries:
        if entry.mt_event in event_points.keys():
            events[entry.mt_event].append(entry)
    
    scores = defaultdict(int)

    # Process each event
    for event_id, event_entries in events.items():
        times = defaultdict(list)
        
        # Calculate the adjusted times for each entry
        for entry in event_entries:
            base_time = int(entry.score * 100)  # converting to centiseconds
            if entry.course == "Y":
                # Adjust time for yard courses (multiplied by 1.11 as integer math)
                adjusted_time = (base_time * 111) // 100
            else:
                adjusted_time = base_time  # No adjustment for short course
            times[adjusted_time].append(entry)

        # Now calculate the points based on sorted adjusted times
        place = 0
        for adjusted_time in sorted(times.keys()):
            tied_entries = times[adjusted_time]
            count = len(tied_entries)
            remaining_points = len(event_points[event_id]) - place
            endp = min(count, max(0, remaining_points))
            available_points = event_points[event_id][place : place + endp]

            # Use the `split_points_evenly` function to fairly distribute points
            split = split_points_evenly(available_points, count)
            
            # Award points to the tied entries
            for entry, pts in zip(tied_entries, split):
                scores[entry.team] += pts

            place += count
    
    return dict(scores)
import unittest


# === Dummy class for testing ===
class Entry:
    def __init__(self, mt_event, score, team, course="S"):
        self.mt_event = mt_event
        self.score = score
        self.team = team
        self.course = course  # "S" for short course, "Y" for yard

# === Tests ===
class TestScoring(unittest.TestCase):

    def test_basic_scoring(self):
        points = get_points_per_event(MeetScore.Dual)
        entries = [
            Entry(mt_event=1, score=50.0, team='A'),
            Entry(mt_event=1, score=51.0, team='B'),
            Entry(mt_event=1, score=52.0, team='C'),
        ]
        result = score(entries, points)
        self.assertEqual(result['A'], 500)
        self.assertEqual(result['B'], 300)
        self.assertEqual(result['C'], 100)

    def test_tie_scoring(self):
        points = get_points_per_event(MeetScore.Dual)
        entries = [
            Entry(mt_event=2, score=50.0, team='A'),
            Entry(mt_event=2, score=50.0, team='B'),
            Entry(mt_event=2, score=52.0, team='C'),
        ]
        # A & B tie → share 500 + 300 = 800 -> 400 each
        result = score(entries, points)
        self.assertEqual(result['A'], 400)
        self.assertEqual(result['B'], 400)
        self.assertEqual(result['C'], 100)

    def test_tie_exceeds_points(self):
        points = {3: [500, 300]}  # Only 2 places scored
        entries = [
            Entry(mt_event=3, score=50.0, team='A'),
            Entry(mt_event=3, score=50.0, team='B'),
            Entry(mt_event=3, score=50.0, team='C'),  # Tie of 3 for 2 places
        ]
        result = score(entries, points)
        # Only 500 + 300 = 800 points to divide among 3 → 266 each
        self.assertEqual(result['A'], 267)
        self.assertEqual(result['B'], 267)
        self.assertEqual(result['C'], 266)

    def test_course_adjustment(self):
        points = get_points_per_event(MeetScore.Dual)
        entries = [
            Entry(mt_event=4, score=50.0, team='A', course='Y'),  # Gets adjusted
            Entry(mt_event=4, score=55.6, team='B'),
            Entry(mt_event=4, score=60.0, team='C'),
        ]
        result = score(entries, points)
        self.assertEqual(result['A'], 500)
        self.assertEqual(result['B'], 400)
        self.assertEqual(result['C'], 100)          

if __name__ == "__main__":
    unittest.main()

    



