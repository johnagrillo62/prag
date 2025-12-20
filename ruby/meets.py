from src.python.tm import meta
from src.python.tm import team
from src.python.tm import result


import mock
years = {#2005, 2006, 2007, 2008, #2009,
         #2010, 2011, 2012, 2013, 2014,
         #2015, 2016, 2017, 2018, 2019,
         #2022, 2023,
         2024}

from collections import defaultdict
mock_meets = defaultdict(lambda: defaultdict(list))
import threading

import time

# Shared scoreboard
scoreboard = {}
lock = threading.Lock()


with open("../mdb/fssl/csv/mock.csv") as f:
    for line in f:
        year, tm, *weeks = line.strip().split(",")
        if len(weeks) == 6:
            mock_meets[int(year)][tm.strip()].extend(int(w) for w in weeks if w.strip().isdigit())

def process_year(y):

    with lock:
       scoreboard[y] = "started"
       print(f"{y}: started")


    results = result.Result.from_csv(f"../mdb/fssl/csv/{y}.Result.csv")
    teams = {}
    to_t_code = {}
    for t in team.Team.from_csv(f"../mdb/fssl/csv/{y}.Team.csv"):
        teams[t.t_code] = t.team
        to_t_code[t.team] = t.t_code
    
    ts = mock_meets[y].keys()

    for t1 in sorted(teams):
        print(f"{t1:<5}")
        for t2 in sorted(teams):
            if t1 != t2:
                print(f" --> {t2:<5}", end=" ")
                wins = 0

                for wk in range(0, 6):
                    #with lock:
                    #    print(f" {wk}",end = "" )
                    
                    #print(f"{y} {wk+1} {t1}-{mock[y][t1][wk]} {t2}-{mock[y][t2][wk]}")
                    entries = []

                    t1_meet = mock_meets[y][t1][wk]
                    t2_meet = mock_meets[y][t2][wk]
                    
                    for r in results:
                        if r.place > 0:
                            if r.team == teams[t1] and r.meet == t1_meet:
                                entries.append(r)
                            if r.team == teams[t2] and r.meet == t2_meet:                               
                                entries.append(r)                                
                    scores = mock.score(entries, mock.get_points_per_event(mock.MeetScore.Mock))

                    scores_by_t_code = {}
                    
                    for t,s in scores.items():
                        scores_by_t_code[to_t_code[t]] = s


                    #print(scores_by_t_code, end=" ")


                    char = "-"
                    if scores_by_t_code[t1] > scores_by_t_code[t2]:
                        char = "W"
                        wins += 1
                    elif scores_by_t_code[t1] < scores_by_t_code[t2]:
                        char = "L"
                    else:
                        char = "T"

                    str = t2 + "-" + char    
                    print(f"{char}", end=" ")
                print(wins)

            #print("")        


    with lock:
        scoreboard[y] = "done"
        print(f"{y}: done")

                            
#threads = []                    
for y in years:

    #t = threading.Thread(target=process_year, args=(y,))

    process_year(y)
 #   threads.append(t)
 #   t.start()

#for t in threads:
#   t.join()

#print("\n--- Final Scoreboard ---")
#for year in sorted(scoreboard):
#    print(f"{year}: {scoreboard[year]}")
    



                    
    


    

    


