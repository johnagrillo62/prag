data User = User
  { name :: String
  , age :: Int
  , id :: String
  , data :: String
  , contact :: ContactInfo
  , employer :: Company
  , projects :: [Project]
  , metadata :: Map String String
  , investments :: Map String Company
  , nested :: Map Map String String Int
  } deriving (Show, Eq)
