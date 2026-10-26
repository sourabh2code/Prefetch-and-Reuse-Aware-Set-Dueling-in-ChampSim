#ifndef REPLACEMENT_PREFETCH_DRRIP_H
#define REPLACEMENT_PREFETCH_DRRIP_H

#include<cstddef>
#include <cstdint>
#include <array>
#include <vector>

#include "cache.h"
#include "modules.h"
#include "msl/stat_methods.h"

struct prefetch_drrip : public champsim::modules::replacement {
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

  // Tracks whether a prefetched line is still
  // waiting for its first demand use.
  std::vector<uint8_t> pending_prefetch;

  // Statistics
  std::array<uint64_t, 2> prefetch_fills{};
  std::array<uint64_t, 2> useful_prefetches{};
  std::array<uint64_t, 2> unused_evictions{};

  std::array<uint64_t, 2> positive_feedback{};
  std::array<uint64_t, 2> negative_feedback{};

  uint64_t follower_brrip_selections = 0;
  uint64_t follower_srrip_selections = 0;

  void replacement_final_stats();

  prefetch_drrip(CACHE* cache);

  // void initialize_replacement()
  long find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                   champsim::address full_addr, access_type type);
  void replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                              access_type type);
  void update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip, champsim::address victim_addr,
                                access_type type, uint8_t hit);

  // use this function to print out your own stats at the end of simulation
  // void replacement_final_stats() {}

  void update_brrip(long set, long way);
  void update_srrip(long set, long way);
};

#endif
