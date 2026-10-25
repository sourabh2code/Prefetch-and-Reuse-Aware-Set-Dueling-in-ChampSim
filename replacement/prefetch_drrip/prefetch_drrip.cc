#include "prefetch_drrip.h"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <random>
#include <utility>

#include "champsim.h"

prefetch_drrip::prefetch_drrip(CACHE* cache)
    : replacement(cache), NUM_SET(cache->NUM_SET), NUM_WAY(cache->NUM_WAY), brrip_counter(0), rrpv(static_cast<std::size_t>(NUM_SET * NUM_WAY)),
      PSEL(NUM_CPUS, champsim::msl::dscounter<long, PSEL_WIDTH>(champsim::msl::get_sample_rate(NUM_SET), category_projector<long>{}, 512)), pending_prefetch(
    static_cast<std::size_t>(NUM_SET * NUM_WAY), 0)
{
}

std::size_t prefetch_drrip::leader_category(long set) const
{
    champsim::msl::categorizer<long> sampler(
        champsim::msl::get_sample_rate(NUM_SET)
    );

    return sampler.get_sample_category(set);
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

        auto category = leader_category(set);

        if (category < 2) {
            useful_prefetches[category]++;
            positive_feedback[category]++;

            PSEL[triggering_cpu].update_good(set);
        }
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

void prefetch_drrip::replacement_cache_fill(uint32_t triggering_cpu, long set, long way, champsim::address full_addr,
                                  champsim::address ip, champsim::address victim_addr, access_type type) 
{
    auto index = static_cast<std::size_t>(
        set * NUM_WAY + way
    );

    auto category = leader_category(set);

    // Track an outstanding prefetched line
    // being replaced before demand use.
    if (pending_prefetch.at(index)) {

        if (category < 2) {
            unused_evictions[category]++;
            negative_feedback[category]++;

            PSEL[triggering_cpu].update_bad(set);
        }

        pending_prefetch[index] = 0;
    }

    // Track new prefetch fills.
    pending_prefetch[index] = (type == access_type::PREFETCH) ? 1 : 0;

    if (type == access_type::PREFETCH && category < 2) {
        prefetch_fills[category]++;
    }

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

void prefetch_drrip::replacement_final_stats()
{
    std::cout << "\n========== PREFETCH-DRRIP STATISTICS ==========\n";

    std::cout << "Initial PSEL: 512\n";
    std::cout << "Final PSEL: " << PSEL[0].value() << '\n';

    std::cout << "BRRIP leader prefetch fills: "
              << prefetch_fills[0] << '\n';

    std::cout << "SRRIP leader prefetch fills: "
              << prefetch_fills[1] << '\n';

    std::cout << "BRRIP useful prefetches: "
              << useful_prefetches[0] << '\n';

    std::cout << "SRRIP useful prefetches: "
              << useful_prefetches[1] << '\n';

    std::cout << "BRRIP unused evictions: "
              << unused_evictions[0] << '\n';

    std::cout << "SRRIP unused evictions: "
              << unused_evictions[1] << '\n';

    std::cout << "BRRIP positive feedback: "
              << positive_feedback[0] << '\n';

    std::cout << "SRRIP positive feedback: "
              << positive_feedback[1] << '\n';

    std::cout << "BRRIP negative feedback: "
              << negative_feedback[0] << '\n';

    std::cout << "SRRIP negative feedback: "
              << negative_feedback[1] << '\n';

    std::cout << "Follower BRRIP selections: "
              << follower_brrip_selections << '\n';

    std::cout << "Follower SRRIP selections: "
              << follower_srrip_selections << '\n';

    std::cout << "===============================================\n";
}
