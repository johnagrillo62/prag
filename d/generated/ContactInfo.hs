data Contactinfo = Contactinfo
  { email :: String
  , phone :: String
  , address :: Address
  , previousAddresses :: [Address]
  } deriving (Show, Eq)
