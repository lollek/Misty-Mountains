#pragma once

/* Coordinate data type */
class Coordinate {
public:
  Coordinate() = default;
  Coordinate(int x, int y);

  ~Coordinate() = default;
  Coordinate(Coordinate const&) = default;
  Coordinate& operator=(Coordinate const&) = default;
  Coordinate& operator=(Coordinate&&) = default;

  bool operator==(Coordinate const&) const;

  int get_x() const;
  int get_y() const;

private:
  int x;
  int y;
};
