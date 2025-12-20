namespace demo
{
struct ComplexRow
{
  int id;
  std::string name;
  std::vector<int> scores;       
  std::vector<std::string> tags; 
  std::vector<std::vector<int>> matrix;
  std::vector<std::vector<std::string>> categories;
};
}
