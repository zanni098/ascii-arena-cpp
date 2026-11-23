#include "game/Scoreboard.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace aa {

void Scoreboard::load(const std::string& path) {
    path_ = path;
    entries_.clear();

    std::ifstream in(path_);
    if (!in) return;

    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        ScoreEntry e;
        if (!(ss >> e.initials >> e.score >> e.wave)) continue;
        if (e.initials.empty()) e.initials = "---";
        entries_.push_back(e);
    }
    std::sort(entries_.begin(), entries_.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > maxEntries()) entries_.resize(maxEntries());
}

void Scoreboard::save() const {
    std::ofstream out(path_);
    if (!out) return;
    for (const ScoreEntry& e : entries_) {
        out << e.initials << ' ' << e.score << ' ' << e.wave << '\n';
    }
}

bool Scoreboard::qualifies(int score) const {
    if (entries_.size() < maxEntries()) return score > 0;
    return score > entries_.back().score;
}

int Scoreboard::submit(const std::string& initials, int score, int wave) {
    ScoreEntry entry;
    entry.initials = initials.empty() ? "---" : initials;
    if (entry.initials.size() > 3) entry.initials.resize(3);
    entry.score = score;
    entry.wave = wave;

    entries_.push_back(entry);
    std::stable_sort(entries_.begin(), entries_.end(),
                     [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > maxEntries()) entries_.resize(maxEntries());

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].initials == entry.initials && entries_[i].score == entry.score &&
            entries_[i].wave == entry.wave) {
            save();
            return static_cast<int>(i);
        }
    }
    return -1;
}

}  // namespace aa
