data Company = Company
  { name :: String
  , headquarters :: Address
  , taxId :: String
  , offices :: Map String Address
  } deriving (Show, Eq)
