#include "prefetch_drrip.h"

#include <algorithm>
#include <cassert>
#include <random>
#include <utility>

#include "champsim.h"

prefetch_drrip::prefetch_drrip(CACHE* cache)
    : replacement(cache), NUM_SET(cache->NUM_SET), NUM_WAY(cache->NUM_WAY), brrip_counter(0), rrpv(static_cast<std::size_t>(NUM_SET * NUM_WAY)),
      PSEL(NUM_CPUS, champsim::msl::dscounter<long, PSEL_WIDTH>(champsim::msl::get_sample_rate(NUM_SET))), pending_prefetch(
    static_cast<std::size_t>(NUM_SET * NUM_WAY), 0)
{
}

unsigned& prefetch_drrip::get_rrpv(long set, long way) { return rrpv.at(static_cast<std::size_t>(set * NUM_WAY + way)); }

void prefetch_drrip::update_brrip(long set, long way)
{
  get_rrpv(set, way) = maxRRPV;

  brrip_counter++;
  if (brrip_counter == BRRIP_MAX) {
    brrip_counter = 0;
    get_rrpv(set, way) = maxRRPV - 1;
  }
}

void prefetch_drrip::update_srrip(long set, long way) { get_rrpv(set, way) = maxRRPV - 1; }

// called on every cache hit and cache fill
void prefetch_drrip::update_replacement_state(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                     champsim::address victim_addr, access_type type, uint8_t hit)
{
  if (hit && (type == access_type::LOAD || type == access_type::RFO)) {

    auto index = static_cast<std::size_t>(set * NUM_WAY + way);

    if (pending_prefetch.at(index)) {
        // First demand use of this prefetched line
        pending_prefetch[index] = 0;

        // Reward the corresponding leader policy
        PSEL[triggering_cpu].update_good(set);
    }
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

void prefetch_drrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr, champsim::address ip,
                                   champsim::address victim_addr, access_type type)
{
  auto index = static_cast<std::size_t>(set * NUM_WAY + way);

    // If the previous line was prefetched
    // but never used, count an unused eviction.
    if (pending_prefetch.at(index)) {
        PSEL[triggering_cpu].update_bad(set);
        pending_prefetch[index] = 0;
    }

    // Track whether the incoming line is a prefetch.
    pending_prefetch[index] =
        (type == access_type::PREFETCH) ? 1 : 0;

    // Preserve original DRRIP writeback handling.
    if (type == access_type::WRITE) {
        get_rrpv(set, way) = maxRRPV - 1;
        return;
    }

    // Preserve original policy selection.
    if (PSEL[triggering_cpu].decide(set)) {
        update_brrip(set, way);
    } else {
        update_srrip(set, way);
    }
}

// find replacement victim
long prefetch_drrip::find_victim(uint32_t triggering_cpu, uint64_t instr_id, long set, const champsim::cache_block* current_set, champsim::address ip,
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
