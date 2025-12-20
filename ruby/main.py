from src.python.tm import meta
from src.python.tm import team
from src.python.tm import result

years = {2005, 2006, 2007, 2008, 2009,
         2010, 2011, 2012, 2013, 2014,
         2015, 2016, 2017, 2018, 2019,
         2022, 2023, 2024}

from enum import Enum
from collections import defaultdict

class MeetScore(Enum):
    Dual = 1
    Mock = 2
    Div = 3

def get_points_per_event(meet_score: MeetScore):
    # Dual points
    dual_points = {e: [500, 300, 100] for e in range(1, 69)}
    for e in [31, 32, 33, 34, 67, 68]:
        dual_points[e] = [800, 400, 200]

    # Mock points
    mock_points = {e: [500, 300, 100] for e in range(1, 69)}
    for e in [31, 32, 33, 34, 67, 68]:
        mock_points[e] = [800, 400]

    # Div points
    div_points = {e: [700, 500, 400, 300, 200, 100] for e in range(1, 69)}
    for e in [31, 32, 33, 34, 67, 68]:
        div_points[e] = [1000, 700, 400, 200]

    # Return the appropriate points based on the meet score
    if meet_score == MeetScore.Dual:
        return dual_points
    elif meet_score == MeetScore.Mock:
        return mock_points
    elif meet_score == MeetScore.Div:
        return div_points
    else:
        return dual_points  # Default case for safety

    

def score(entries, event_points):
    
    events = defaultdict(list)
    for e in entries:
        if e.mt_event in event_points.keys():
            events[e.mt_event].append(e)
    
    scores = defaultdict(int)

    for e in sorted(events.keys()):
        times = defaultdict(list)
        for r in events[e]:
            score = r.score * 100;
            time = score
            if r.course == "Y":
                time = int(score * 1.11)
            times[time].append(r)

        place = 0
        for t in sorted(times.keys()):
            
            count = len(times[t])
            endp = min(count, max(0, len(event_points[e]) - place))
            total_points = 0
            for i in range(place, place + endp):
                total_points += event_points[e][i]
            pts = int(total_points/count)
            place += count

            for r in times[t]:
                scores[r.team] += pts

    return scores


from collections import defaultdict
mock = defaultdict(lambda: defaultdict(list))

with open("../mdb/fssl/csv/mock.csv") as f:
    for line in f:
        year, tm, *weeks = line.strip().split(",")
        if len(weeks) == 6:
            mock[int(year)][tm.strip()]. extend(int(w) for w in weeks if w.strip().isdigit())

for y in years:
    results = result.Result.from_csv(f"../mdb/fssl/csv/{y}.Result.csv")
    print(f"{y} has {len(results)} ")

    teams = {}
    for t in team.Team.from_csv(f"../mdb/fssl/csv/{y}.Team.csv"):
        teams[t.t_code] = t.team
    
    # six weeks of meets
    
    ts = mock[y].keys()

    print(y)
    for wk in range(0, 6):
        print(f"   {wk}")        
        for t1 in teams:
            for t2 in teams:
                if t1 != t2:
                    in_meet = {teams[t1], teams[t2]}
                    #print(f"{y} {wk+1} {t1}-{mock[y][t1][wk]} {t2}-{mock[y][t2][wk]}")
                    entries = []
                    for r in results:
                        if r.place > 0:
                            if r.team in in_meet:
                            
                                entries.append(r)

                    scores = score(entries, get_points_per_event(MeetScore.Mock))




                            
                    
                    



                    
    


    

    


