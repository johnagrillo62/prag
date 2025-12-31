// --- Example structs ---
#[allow(dead_code)]
        use std::collections::HashMap;
        use std::time::SystemTime;

        #[derive(Debug)]
        pub struct Athlete {
            pub athlete: i64,
            pub team1: Option<i64>,
            pub last: Option<String>,
            pub first: Option<String>,
            pub initial: Option<String>,
            pub sex: String,
            pub birth: SystemTime,
            pub age: i16,
            pub id_no: String,
            pub inactive: bool,
        }

        #[derive(Debug)]
        pub struct Team {
            pub team_id: i64,
            pub name: String,
            pub athletes: Vec<Athlete>,
        }

        #[derive(Debug)]
        pub struct Meet {
            pub meet_id: i64,
            pub name: String,
            pub teams: Vec<Team>,
            pub results: HashMap<i64, i64>,
        }
