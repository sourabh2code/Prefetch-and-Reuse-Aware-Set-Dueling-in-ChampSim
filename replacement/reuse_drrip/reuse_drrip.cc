#include "reuse_drrip.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <utility>

#include "champsim.h"

reuse_drrip::reuse_drrip(CACHE* cache)
    : replacement(cache), NUM_SET(cache->NUM_SET), NUM_WAY(cache->NUM_WAY), brrip_counter(0),
      rrpv(static_cast<std::size_t>(NUM_SET * NUM_WAY)),
      PSEL(NUM_CPUS, champsim::msl::dscounter<long, PSEL_WIDTH>(champsim::msl::get_sample_rate(NUM_SET), category_projector<long>{}, 512)),
      line_reuse_count(static_cast<std::size_t>(NUM_SET * NUM_WAY), 0)
{
}

std::size_t reuse_drrip::leader_category(long set) const
{
  champsim::msl::categorizer<long> sampler(
      champsim::msl::get_sample_rate(NUM_SET)
  );

  return sampler.get_sample_category(set);
}

unsigned& reuse_drrip::get_rrpv(long set, long way) { return rrpv.at(static_cast<std::size_t>(set * NUM_WAY + way)); }

void reuse_drrip::update_brrip(long set, long way)
{
  get_rrpv(set, way) = maxRRPV;

  brrip_counter++;
  if (brrip_counter == BRRIP_MAX) {
    brrip_counter = 0;
    get_rrpv(set, way) = maxRRPV - 1;
  }
}

void reuse_drrip::update_srrip(long set, long way) { get_rrpv(set, way) = maxRRPV - 1; }

// called on every cache hit and cache fill
void reuse_drrip::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                     champsim::address victim_addr, access_type type, uint8_t hit)
{
  if (hit && (type == access_type::LOAD || type == access_type::RFO)) {
    auto index = static_cast<std::size_t>(set * NUM_WAY + way);
    line_reuse_count.at(index)++;
  }

  // cache hit
  if (hit) {
    // do not update replacement state for writebacks
    if (access_type{type} == access_type::WRITE) {
      get_rrpv(set, way) = maxRRPV - 1;
      return;
    }
    get_rrpv(set, way) = 0; // for cache hit, DRRIP always promotes a cache line to the MRU position
  }
}

void reuse_drrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                  champsim::address ip, champsim::address victim_addr, access_type type)
{
  auto index = static_cast<std::size_t>(
      set * NUM_WAY + way
  );

  auto category = leader_category(set);
  uint32_t prior_reuses = line_reuse_count.at(index);

  if (category < 2) {
    if (prior_reuses > 0) {
      reused_evictions[category]++;
      PSEL[triggering_cpu].update_good(set);
    } else {
      unreused_evictions[category]++;
      PSEL[triggering_cpu].update_bad(set);
    }
  }

  line_reuse_count[index] = 0;

  // Preserve original writeback insertion.
  if (type == access_type::WRITE) {
    get_rrpv(set, way) = maxRRPV - 1;
    return;
  }

  bool use_brrip = PSEL[triggering_cpu].decide(set);

  // Record which policy follower sets select.
  if (category >= 2) {
    if (use_brrip)
      follower_brrip_selections++;
    else
      follower_srrip_selections++;
  }

  // Original DRRIP insertion behavior.
  if (use_brrip)
    update_brrip(set, way);
  else
    update_srrip(set, way);
}

// find replacement victim
long reuse_drrip::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
                        champsim::address full_addr, access_type type)
{
  // look for the maxRRPV line
  auto begin = std::next(std::begin(rrpv), set * NUM_WAY);
  auto end = std::next(begin, NUM_WAY);

  auto victim = std::max_element(begin, end);
  if (auto rrpv_update = maxRRPV - *victim; rrpv_update != 0)
    for (auto it = begin; it != end; ++it)
      *it += rrpv_update;

  assert(begin <= victim);
  assert(victim < end);
  return std::distance(begin, victim); // cast protected by assertions
}

void reuse_drrip::replacement_final_stats()
{
  std::cout << "\n========== REUSE-DRRIP STATISTICS ==========\n";

  std::cout << "Initial PSEL: 512\n";
  std::cout << "Final PSEL: " << PSEL[0].value() << '\n';

  std::cout << "BRRIP reused evictions: "
            << reused_evictions[0] << '\n';

  std::cout << "SRRIP reused evictions: "
            << reused_evictions[1] << '\n';

  std::cout << "BRRIP unreused evictions: "
            << unreused_evictions[0] << '\n';

  std::cout << "SRRIP unreused evictions: "
            << unreused_evictions[1] << '\n';

  std::cout << "Follower BRRIP selections: "
            << follower_brrip_selections << '\n';

  std::cout << "Follower SRRIP selections: "
            << follower_srrip_selections << '\n';

  std::cout << "============================================\n";
}