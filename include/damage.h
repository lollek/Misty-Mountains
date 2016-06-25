#pragma once

struct damage {
  int dices;
  int sides;

  bool operator==(damage const&) const;
  bool operator!=(damage const&) const;
};
