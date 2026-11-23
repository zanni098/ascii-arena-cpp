// Scoreboard.h - persistent top scores stored in a plain text file.
#pragma once

#include <string>
#include <vector>

namespace aa {

struct ScoreEntry {
    std::string initials = "---";
    int score = 0;
    int wave = 0;
};

class Scoreboard {
public:
    void load(const std::string& path);
    void save() const;

    // Inserts the result if it makes the table. Returns the 0-based rank, or -1.
    int submit(const std::string& initials, int score, int wave);

    bool qualifies(int score) const;
    const std::vector<ScoreEntry>& entries() const { return entries_; }
    static std::size_t maxEntries() { return 8; }

private:
    std::string path_ = "highscores.txt";
    std::vector<ScoreEntry> entries_;
};

}  // namespace aa
