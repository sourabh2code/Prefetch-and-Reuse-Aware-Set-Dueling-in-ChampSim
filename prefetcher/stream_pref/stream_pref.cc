#include "stream_pref.h"
#include "champsim.h"

uint32_t stream_pref::prefetcher_cache_operate(champsim::address addr, champsim::address,
    uint8_t, bool, access_type type, uint32_t metadata_in)
{
  // Do not let generated prefetches recursively train the detector.
  // Train on demand LOADs (both hits and misses); exclude writebacks and RFOs.
  if (type != access_type::LOAD) return metadata_in;
  // These configuration values are runtime constants in this ChampSim version.
  if (LOG2_PAGE_SIZE <= LOG2_BLOCK_SIZE || LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE >= 64)
    return metadata_in;
  const auto line = champsim::block_number{addr}.to<uint64_t>();
  const auto page = champsim::page_number{addr}.to<uint64_t>();
  const uint64_t lines_per_page = uint64_t{1} << (LOG2_PAGE_SIZE - LOG2_BLOCK_SIZE);
  for (auto target : tracker.observe(line, page, lines_per_page)) {
    // true requests allocation in the cache hosting this module: the LLC.
    if (!prefetch_line(champsim::address{champsim::block_number{target}}, true, metadata_in)) break;
    tracker.accepted(page, target);
  }
  return metadata_in;
}
