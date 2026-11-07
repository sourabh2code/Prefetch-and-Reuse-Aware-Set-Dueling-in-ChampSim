#ifndef REPLACEMENT_REUSE_DRRIP_H
#define REPLACEMENT_REUSE_DRRIP_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "cache.h"
#include "modules.h"
#include "msl/stat_methods.h"

struct reuse_drrip : public champsim::modules::replacement {
private:
  unsigned& get_rrpv(long set, long way);
  std::size_t leader_category(long set) const;

public:
  static constexpr unsigned maxRRPV = 3;
  static constexpr unsigned BRRIP_MAX = 32;
  static constexpr unsigned PSEL_WIDTH = 10;

  enum class set_type { follower, brrip_leader, srrip_leader };

  long NUM_SET, NUM_WAY;

  unsigned brrip_counter;

  std::vector<unsigned> rrpv;
  std::vector<champsim::msl::dscounter<long, PSEL_WIDTH>> PSEL;

  // Tracks how many demand hits a resident line receives before eviction.
  std::vector<uint32_t> line_reuse_count;

  // Statistics
  std::array<uint64_t, 2> reused_evictions{};
  std::array<uint64_t, 2> unreused_evictions{};

  uint64_t follower_brrip_selections = 0;
  uint64_t follower_srrip_selections = 0;

  void replacement_final_stats();

  reuse_drrip(CACHE* cache);

  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                   champsim::address full_addr, access_type type);
  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                              access_type type);
  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                access_type type, uint8_t hit);

  void update_brrip(long set, long way);
  void update_srrip(long set, long way);
};

#endif