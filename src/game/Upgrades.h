// Upgrades.h - the level-up card pool. Data driven: adding a new upgrade means
// adding one table entry and one case in applyUpgrade().
#pragma once

#include <vector>

namespace aa {

class Rng;
class Player;

enum class UpgradeId {
    RapidFire,
    HeavyRounds,
    Piercing,
    SwiftBoots,
    Vitality,
    TwinCannons,
    DashCoils,
    Repair
};

struct Upgrade {
    UpgradeId id;
    const char* name;
    const char* description;
};

const std::vector<Upgrade>& upgradePool();

// Picks up to `count` distinct, currently useful upgrades for the player.
std::vector<Upgrade> rollUpgrades(Rng& rng, const Player& player, int count);

void applyUpgrade(Player& player, UpgradeId id);

}  // namespace aa
