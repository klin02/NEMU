#include <memory/cmo_write_watch.h>

#ifdef CMO_WRITE_WATCH
#include <algorithm>
#include <unordered_map>

static constexpr unsigned line_size = 64;
static std::unordered_map<paddr_t, uint64_t> pending_lines;

bool cmo_write_watch_active = false;

void difftest_cmo_inval_begin(paddr_t line) {
  pending_lines[line] = 0;
  cmo_write_watch_active = true;
}

uint64_t difftest_cmo_inval_end(paddr_t line) {
  const auto entry = pending_lines.find(line);
  if (entry == pending_lines.end()) {
    return 0;
  }
  const uint64_t mask = entry->second;
  pending_lines.erase(entry);
  cmo_write_watch_active = !pending_lines.empty();
  return mask;
}

void cmo_write_watch_record(paddr_t addr, int len) {
  while (len > 0) {
    const unsigned offset = addr % line_size;
    const paddr_t line = addr - offset;
    const unsigned count = std::min<unsigned>(len, line_size - offset);
    const auto entry = pending_lines.find(line);
    if (entry != pending_lines.end()) {
      entry->second |= (UINT64_MAX >> (line_size - count)) << offset;
    }
    addr += count;
    len -= count;
  }
}

void cmo_write_watch_reset(void) {
  pending_lines.clear();
  cmo_write_watch_active = false;
}
#endif
