#ifndef CS683_STREAM_PREF_H
#define CS683_STREAM_PREF_H
#include <cstdint>
#include "address.h"
#include "modules.h"
#include "stream_tracker.h"

struct stream_pref : champsim::modules::prefetcher {
  using prefetcher::prefetcher;
  stream_tracker tracker;
  uint32_t prefetcher_cache_operate(champsim::address addr, champsim::address ip,
      uint8_t cache_hit, bool useful_prefetch, access_type type, uint32_t metadata_in);
};
#endif
