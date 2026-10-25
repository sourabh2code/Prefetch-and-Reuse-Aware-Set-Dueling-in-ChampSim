#ifndef CS683_STREAM_TRACKER_H
#define CS683_STREAM_TRACKER_H
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

// Experimental address-only detector: one directional stream per physical page.
class stream_tracker {
public:
  static constexpr unsigned TABLE_SIZE = 32;
  static constexpr unsigned MAX_GAP = 4;
  static constexpr unsigned CONFIRMATIONS = 2;
  static constexpr unsigned DISTANCE = 4;
  static constexpr unsigned DEGREE = 2;

  std::vector<uint64_t> observe(uint64_t line, uint64_t page, uint64_t lines_per_page)
  {
    ++clock;
    entry* selected = nullptr;
    for (auto& e : table)
      if (e.valid && e.page == page) { selected = &e; break; }
    if (!selected) {
      selected = &table[0];
      for (auto& e : table) {
        if (!e.valid) { selected = &e; break; }
        if (e.age < selected->age) selected = &e;
      }
      *selected = {true, page, line, clock, 0, 0, false, 0};
      return {};
    }
    auto& e = *selected;
    e.age = clock;
    if (line == e.last) return {};
    const int direction = line > e.last ? 1 : -1;
    const uint64_t gap = line > e.last ? line - e.last : e.last - line;
    e.last = line;
    if (gap > MAX_GAP) {
      e.direction = 0; e.confidence = 0; e.frontier_valid = false;
      return {};
    }
    if (direction != e.direction) {
      e.direction = direction; e.confidence = 1; e.frontier_valid = false;
    } else if (e.confidence < CONFIRMATIONS) {
      ++e.confidence;
    }
    if (e.confidence < CONFIRMATIONS) return {};
    std::vector<uint64_t> result;
    for (unsigned i = 0; i < DEGREE; ++i) {
      const uint64_t step = DISTANCE + i;
      if ((direction > 0 && line > std::numeric_limits<uint64_t>::max() - step)
          || (direction < 0 && line < step)) continue;
      const uint64_t target = direction > 0 ? line + step : line - step;
      if (target / lines_per_page != page) continue;
      if (e.frontier_valid && (direction > 0 ? target <= e.frontier : target >= e.frontier)) continue;
      result.push_back(target);
    }
    return result;
  }

  // Advance only after queue acceptance, so a rejected target is not suppressed.
  void accepted(uint64_t page, uint64_t target)
  {
    for (auto& e : table)
      if (e.valid && e.page == page) { e.frontier = target; e.frontier_valid = true; return; }
  }
private:
  struct entry {
    bool valid = false;
    uint64_t page = 0, last = 0, age = 0;
    int direction = 0;
    unsigned confidence = 0;
    bool frontier_valid = false;
    uint64_t frontier = 0;
  };
  std::array<entry, TABLE_SIZE> table{};
  uint64_t clock = 0;
};
#endif
