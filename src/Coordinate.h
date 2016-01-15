#pragma once

/* Coordinate data type */
struct Coordinate {
  Coordinate() = default;
  Coordinate(int x, int y);

  ~Coordinate() = default;
  Coordinate(Coordinate const&) = default;
  Coordinate& operator=(Coordinate const&) = default;
  Coordinate& operator=(Coordinate&&) = default;

  bool operator==(Coordinate const&) const;

  int x;
  int y;
};
