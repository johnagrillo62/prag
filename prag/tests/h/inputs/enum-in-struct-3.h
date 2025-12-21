struct Car
{
  int miles;
  struct {
    int x;
    enum class Color {
      Red,
      Green,
      Blue
    };
    Color color ;
  } a;
};
