#pragma once

#include <vector>
#include <string>
#include <fstream>

#include "item.h"

class Wand : public Item {
public:
  enum Type {
    HasteMonster,
    SlowMonster,
    InvisibleOther,
    Light,
    MagicMissile,
    ElectricBolt,
    FireBolt,
    ColdBolt,
    Cancellation,
    Polymorph,
    TeleportTo,
    TeleportAway,
    DrainLife,

    NWANDS
  };

  ~Wand();
  Wand();     // Random wand
  explicit Wand(Type); // Wand of given type
  explicit Wand(std::ifstream&);
  explicit Wand(Wand const&) = default;

  Wand* clone() const override;
  Wand& operator=(Wand const&) = default;
  Wand& operator=(Wand&&) = default;


  // Modifier
  void modify_charges(int amount);

  // Setters
  void set_identified() override;
  void set_charges(int amount);

  // Getters
  std::string get_description() const override;
  bool        is_magic() const override;
  bool        is_identified() const override;
  int         get_value() const override;
  int         get_base_value() const override;
  std::string get_material() const;
  int         get_charges() const;

  void save(std::ofstream&) const override;
  bool load(std::ifstream&) override;

  // Static
  static void init_wands();
  static void save_wands(std::ofstream&);
  static void load_wands(std::ifstream&);
  static void free_wands();

  static std::string        name(Type subtype);
  static int                worth(Type subtype);
  static std::string&       guess(Type subtype);
  static bool               is_known(Type subtype);
  static void               set_known(Type subtype);
  static std::string const& material(Type subtype);

private:
  int charges;
  bool identified;
  Type subtype;

  static std::vector<std::string>* materials;
  static std::vector<std::string>* guesses;
  static std::vector<bool>*        known;

  static unsigned long long constexpr TAG_WANDS     = 0x4000000000000000ULL;
  static unsigned long long constexpr TAG_MATERIALS = 0x4000000000000001ULL;
  static unsigned long long constexpr TAG_KNOWN     = 0x4000000000000002ULL;
  static unsigned long long constexpr TAG_GUESSES   = 0x4000000000000003ULL;
};

/* Perform a zap with a wand */
bool wand_zap();
