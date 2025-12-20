from dataclasses import dataclass
from typing import Optional
from langs.utils import table_meta, field_meta
from datetime import datetime


# ----------------------
# Classes
# ----------------------
@table_meta(table_name="ATHLETE", class_name="athlete")
@dataclass
class Athlete:
    athlete: int = field_meta(
        csv_column="Athlete",
        table_column="Athlete",
        member_name="athlete",
        primary_key=True,
        size=32,
        signed=True
    )

    team1: Optional[int] = field_meta(
        csv_column="Team1",
        table_column="Team1",
        member_name="team 1",
        size=32,
        signed=True
    )

    last: Optional[str] = field_meta(
        csv_column="Last",
        table_column="Last",
        member_name="last"
    )

    first: Optional[str] = field_meta(
        csv_column="First",
        table_column="First",
        member_name="first"
    )

    initial: Optional[str] = field_meta(
        csv_column="Initial",
        table_column="Initial",
        member_name="initial"
    )

    sex: str = field_meta(
        csv_column="Sex",
        table_column="Sex",
        member_name="sex"
    )

    birth: datetime = field_meta(
        csv_column="Birth",
        table_column="Birth",
        member_name="birth"
    )

    age: int = field_meta(
        csv_column="Age",
        table_column="Age",
        member_name="age",
        size=8,
        signed=False
    )

    id_no: str = field_meta(
        csv_column="ID_NO",
        table_column="ID_NO",
        member_name="id no"
    )

    inactive: bool = field_meta(
        csv_column="Inactive",
        table_column="Inactive",
        member_name="inactive"
    )


@table_meta(table_name="RESULT", class_name="result")
@dataclass
class Result:
    meet: int = field_meta(
        csv_column="MEET",
        table_column="MEET",
        member_name="meet"
    )

    athlete: int = field_meta(
        csv_column="ATHLETE",
        table_column="ATHLETE",
        member_name="athlete"
    )

    ind_or_rel: str = field_meta(
        csv_column="I_R",
        table_column="I_R",
        member_name="ind or rel"
    )

    team: int = field_meta(
        csv_column="TEAM",
        table_column="TEAM",
        member_name="team"
    )

    score: int = field_meta(
        csv_column="SCORE",
        table_column="SCORE",
        member_name="score"
    )

    exhibit: str = field_meta(
        csv_column="EX",
        table_column="EX",
        member_name="exhibit"
    )

    nt: int = field_meta(
        csv_column="NT",
        table_column="NT",
        member_name="nt"
    )

    result: int = field_meta(
        csv_column="RESULT",
        table_column="RESULT",
        member_name="result"
    )

    age: int = field_meta(
        csv_column="AGE",
        table_column="AGE",
        member_name="age"
    )

    distance: int = field_meta(
        csv_column="DISTANCE",
        table_column="DISTANCE",
        member_name="distance"
    )

    stroke: int = field_meta(
        csv_column="STROKE",
        table_column="STROKE",
        member_name="stroke"
    )

    mt_event: int = field_meta(
        csv_column="MTEVENT",
        table_column="MTEVENT",
        member_name="mt event"
    )

    points: int = field_meta(
        csv_column="POINTS",
        table_column="POINTS",
        member_name="points"
    )

    place: int = field_meta(
        csv_column="PLACE",
        table_column="PLACE",
        member_name="place"
    )

    course: str = field_meta(
        csv_column="COURSE",
        table_column="COURSE",
        member_name="course"
    )


@table_meta(table_name="RELAY", class_name="relay")
@dataclass
class Relay:
    relay: int = field_meta(
        csv_column="RELAY",
        table_column="RELAY",
        member_name="relay"
    )

    meet: int = field_meta(
        csv_column="MEET",
        table_column="MEET",
        member_name="meet"
    )

    lo_hi: int = field_meta(
        csv_column="LO_HI",
        table_column="LO_HI",
        member_name="lo hi"
    )

    team: int = field_meta(
        csv_column="TEAM",
        table_column="TEAM",
        member_name="team"
    )

    letter: str = field_meta(
        csv_column="LETTER",
        table_column="LETTER",
        member_name="letter"
    )

    age_range: int = field_meta(
        csv_column="AGE_RANGE",
        table_column="AGE_RANGE",
        member_name="age range"
    )

    sex: str = field_meta(
        csv_column="SEX",
        table_column="SEX",
        member_name="sex"
    )

    ath1: int = field_meta(
        csv_column="ATH(1)",
        table_column="ATH(1)",
        member_name="ath 1"
    )

    ath2: int = field_meta(
        csv_column="ATH(2)",
        table_column="ATH(2)",
        member_name="ath 2"
    )

    ath3: int = field_meta(
        csv_column="ATH(3)",
        table_column="ATH(3)",
        member_name="ath 3"
    )

    ath4: int = field_meta(
        csv_column="ATH(4)",
        table_column="ATH(4)",
        member_name="ath 4"
    )

    ath5: int = field_meta(
        csv_column="ATH(5)",
        table_column="ATH(5)",
        member_name="ath 5"
    )

    ath6: int = field_meta(
        csv_column="ATH(6)",
        table_column="ATH(6)",
        member_name="ath 6"
    )

    ath7: int = field_meta(
        csv_column="ATH(7)",
        table_column="ATH(7)",
        member_name="ath 7"
    )

    ath8: int = field_meta(
        csv_column="ATH(8)",
        table_column="ATH(8)",
        member_name="ath 8"
    )

    distance: int = field_meta(
        csv_column="DISTANCE",
        table_column="DISTANCE",
        member_name="distance"
    )

    stroke: int = field_meta(
        csv_column="STROKE",
        table_column="STROKE",
        member_name="stroke"
    )

    relay_age: str = field_meta(
        csv_column="RELAYAGE",
        table_column="RELAYAGE",
        member_name="relay age"
    )


@table_meta(table_name="MEET", class_name="meet")
@dataclass
class Meet:
    meet: int = field_meta(
        csv_column="Meet",
        table_column="Meet",
        member_name="meet"
    )

    m_name: str = field_meta(
        csv_column="MName",
        table_column="MName",
        member_name="m name"
    )

    start: datetime = field_meta(
        csv_column="Start",
        table_column="Start",
        member_name="start"
    )

    age_up: datetime = field_meta(
        csv_column="AgeUp",
        table_column="AgeUp",
        member_name="age up"
    )

    course: str = field_meta(
        csv_column="Course",
        table_column="Course",
        member_name="course"
    )

    location: str = field_meta(
        csv_column="Location",
        table_column="Location",
        member_name="location"
    )

    max_ind_ent: int = field_meta(
        csv_column="MaxIndEnt",
        table_column="MaxIndEnt",
        member_name="max ind ent"
    )

    max_rel_ent: int = field_meta(
        csv_column="MaxRelEnt",
        table_column="MaxRelEnt",
        member_name="max rel ent"
    )

    max_ent: int = field_meta(
        csv_column="MaxEnt",
        table_column="MaxEnt",
        member_name="max ent"
    )


@table_meta(table_name="TEAM", class_name="team")
@dataclass
class Team:
    team: int = field_meta(
        csv_column="Team",
        table_column="Team",
        member_name="team"
    )

    t_code: str = field_meta(
        csv_column="TCode",
        table_column="TCode",
        member_name="t code"
    )


@table_meta(table_name="ENTRY", class_name="entry")
@dataclass
class Entry:
    meet: int = field_meta(
        csv_column="Meet",
        table_column="Meet",
        member_name="meet"
    )

    athlete: int = field_meta(
        csv_column="Athlete",
        table_column="Athlete",
        member_name="athlete"
    )

    ind_or_real: str = field_meta(
        csv_column="I_R",
        table_column="I_R",
        member_name="ind or real"
    )

    team: int = field_meta(
        csv_column="Team",
        table_column="Team",
        member_name="team"
    )

    course: str = field_meta(
        csv_column="Course",
        table_column="Course",
        member_name="course"
    )

    score: int = field_meta(
        csv_column="Score",
        table_column="Score",
        member_name="score"
    )

    ex: str = field_meta(
        csv_column="Ex",
        table_column="Ex",
        member_name="ex"
    )

    mt_event: int = field_meta(
        csv_column="MtEvent",
        table_column="MtEvent",
        member_name="mt event"
    )

    misc: str = field_meta(
        csv_column="Misc",
        table_column="Misc",
        member_name="misc"
    )

    entry: int = field_meta(
        csv_column="Entry",
        table_column="Entry",
        member_name="entry"
    )

    division: str = field_meta(
        csv_column="Division",
        table_column="Division",
        member_name="division"
    )

    heat: int = field_meta(
        csv_column="HEAT",
        table_column="HEAT",
        member_name="heat"
    )

    lane: int = field_meta(
        csv_column="LANE",
        table_column="LANE",
        member_name="lane"
    )


@table_meta(table_name="MTEVENT", class_name="mt event")
@dataclass
class MtEvent:
    meet: int = field_meta(
        csv_column="Meet",
        table_column="Meet",
        member_name="meet"
    )

    mt_ev: int = field_meta(
        csv_column="MtEv",
        table_column="MtEv",
        member_name="mt ev"
    )

    lo_hi: int = field_meta(
        csv_column="Lo_Hi",
        table_column="Lo_Hi",
        member_name="lo hi"
    )

    course: str = field_meta(
        csv_column="Course",
        table_column="Course",
        member_name="course"
    )

    mt_event: int = field_meta(
        csv_column="MtEvent",
        table_column="MtEvent",
        member_name="mt event"
    )

    distance: int = field_meta(
        csv_column="Distance",
        table_column="Distance",
        member_name="distance"
    )

    stroke: int = field_meta(
        csv_column="Stroke",
        table_column="Stroke",
        member_name="stroke"
    )

    sex: str = field_meta(
        csv_column="Sex",
        table_column="Sex",
        member_name="sex"
    )

    ind_or_rel: str = field_meta(
        csv_column="I_R",
        table_column="I_R",
        member_name="ind or rel"
    )

    division: str = field_meta(
        csv_column="Division",
        table_column="Division",
        member_name="division"
    )


