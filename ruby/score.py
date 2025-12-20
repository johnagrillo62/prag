from collections import defaultdict
from typing import List, Dict

def score(entries, event_points: Dict[int, List[int]]) -> Dict[str, int]:
    """
    Scores meet entries based on event-specific point allocations and performance.
    
    Args:
        entries: List of result objects, each with `mt_event`, `score`, `course`, and `team` attributes.
        event_points: A mapping of event ID to a list of point values (ordered by placement).

    Returns:
        A dictionary mapping team names to total scored points.
    """
    events = defaultdict(list)
    
    # Group entries by valid events
    for entry in entries:
        if entry.mt_event in event_points:
            events[entry.mt_event].append(entry)
    
    team_scores = defaultdict(int)
    
    # Process each event
    for event_id in sorted(events):
        time_buckets = defaultdict(list)
        
        # Normalize times and group results into time buckets
        for result in events[event_id]:
            time = int(result.score * 100)
            if result.course == "Y":
                time = int(time * 1.11)
            time_buckets[time].append(result)
        
        place = 0
        for time in sorted(time_buckets):
            tied_results = time_buckets[time]
            count = len(tied_results)
            max_places = max(0, len(event_points[event_id]) - place)
            endp = min(count, max_places)
            
            total_points = sum(event_points[event_id][i] for i in range(place, place + endp))
            average_points = int(total_points / count) if count > 0 else 0
            place += count

            for result in tied_results:
                team_scores[result.team] += average_points
    
    return dict(team_scores)
