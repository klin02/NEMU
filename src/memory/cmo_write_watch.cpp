#include <memory/cmo_write_watch.h>

#ifdef CMO_WRITE_WATCH
#include <algorithm>
#include <unordered_map>

static constexpr unsigned line_size = 64;
static constexpr size_t max_pending_lines = 4096;

struct CmoWriteWatch {
  uint64_t written;
  bool invalidated;
};

static std::unordered_map<paddr_t, CmoWriteWatch> pending_lines;

bool cmo_write_watch_active = false;

bool difftest_cmo_inval_arm(paddr_t line) {
  const auto entry = pending_lines.find(line);
  if (entry != pending_lines.end()) {
    if (!entry->second.invalidated) {
      return false;
    }
    entry->second = {};
    return true;
  }
  if (pending_lines.size() == max_pending_lines) {
    // Evicting a watch would lose the protection for a later refill.
    return false;
  }
  pending_lines.emplace(line, CmoWriteWatch{});
  cmo_write_watch_active = true;
  return true;
}

void cmo_write_watch_invalidate(paddr_t line) {
  const auto entry = pending_lines.find(line);
  if (entry != pending_lines.end()) {
    entry->second = {0, true};
  }
}

int difftest_cmo_inval_written(paddr_t line, uint64_t *written) {
  const auto entry = pending_lines.find(line);
  if (entry == pending_lines.end()) {
    return 0;
  }
  if (!entry->second.invalidated) {
    return -1;
  }
  *written = entry->second.written;
  return 1;
}

void difftest_cmo_inval_release(paddr_t line) {
  pending_lines.erase(line);
  cmo_write_watch_active = !pending_lines.empty();
}

void cmo_write_watch_record(paddr_t addr, int len) {
  while (len > 0) {
    const unsigned offset = addr % line_size;
    const paddr_t line = addr - offset;
    const unsigned count = std::min<unsigned>(len, line_size - offset);
    const auto entry = pending_lines.find(line);
    if (entry != pending_lines.end() && entry->second.invalidated) {
      entry->second.written |= (UINT64_MAX >> (line_size - count)) << offset;
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
