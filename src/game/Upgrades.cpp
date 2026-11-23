#include "game/Upgrades.h"

#include <algorithm>

#include "core/Rng.h"
#include "game/Player.h"

namespace aa {

const std::vector<Upgrade>& upgradePool() {
    static const std::vector<Upgrade> pool = {
        {UpgradeId::RapidFire, "RAPID FIRE", "Fire 15% faster"},
        {UpgradeId::HeavyRounds, "HEAVY ROUNDS", "+5 damage per shot"},
        {UpgradeId::Piercing, "PIERCING ROUNDS", "Shots pass through +1 enemy"},
        {UpgradeId::SwiftBoots, "SWIFT BOOTS", "+12% move speed"},
        {UpgradeId::Vitality, "VITALITY", "+25 max HP, healed by 25"},
        {UpgradeId::TwinCannons, "TWIN CANNONS", "+1 projectile per shot"},
        {UpgradeId::DashCoils, "DASH COILS", "Dash recharges 20% faster"},
        {UpgradeId::Repair, "FIELD REPAIR", "Restore 40 HP"},
    };
    return pool;
}

std::vector<Upgrade> rollUpgrades(Rng& rng, const Player& player, int count) {
    std::vector<Upgrade> candidates;
    for (const Upgrade& u : upgradePool()) {
        // Do not offer caps that are already reached.
        if (u.id == UpgradeId::RapidFire && player.fireInterval <= 0.13f) continue;
        if (u.id == UpgradeId::Piercing && player.pierce >= 4) continue;
        if (u.id == UpgradeId::TwinCannons && player.extraShots >= 3) continue;
        if (u.id == UpgradeId::DashCoils && player.dashCooldown <= 0.52f) continue;
        if (u.id == UpgradeId::Repair && player.hp >= player.maxHp - 1.0f) continue;
        candidates.push_back(u);
    }

    std::vector<Upgrade> result;
    for (int i = 0; i < count && !candidates.empty(); ++i) {
        int index = rng.rangeInt(0, static_cast<int>(candidates.size()) - 1);
        result.push_back(candidates[static_cast<std::size_t>(index)]);
        candidates.erase(candidates.begin() + index);
    }
    return result;
}

void applyUpgrade(Player& player, UpgradeId id) {
    switch (id) {
        case UpgradeId::RapidFire:
            player.fireInterval = std::max(0.12f, player.fireInterval * 0.85f);
            break;
        case UpgradeId::HeavyRounds:
            player.damage += 5;
            break;
        case UpgradeId::Piercing:
            player.pierce += 1;
            break;
        case UpgradeId::SwiftBoots:
            player.speed *= 1.12f;
            break;
        case UpgradeId::Vitality:
            player.maxHp += 25.0f;
            player.heal(25);
            break;
        case UpgradeId::TwinCannons:
            player.extraShots = std::min(3, player.extraShots + 1);
            break;
        case UpgradeId::DashCoils:
            player.dashCooldown = std::max(0.5f, player.dashCooldown * 0.8f);
            break;
        case UpgradeId::Repair:
            player.heal(40);
            break;
    }
}

}  // namespace aa
