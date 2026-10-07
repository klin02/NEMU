# Fast shared reference

Fast mode advances a speculative reference in batches. A separate slow
checker must confirm its result before it is treated as checked execution.
The APIs in this change support that integration; the fork scheduler and
packet transport belong to DiffTest.

## Build and execution modes

```sh
make NEMU_HOME="$PWD" git_commit= riscv64-xs-fastref_defconfig
make NEMU_HOME="$PWD" git_commit= -j8
```

The XS fastref configuration builds one shared reference that supports
both FAST and SLOW, with PERF_OPT_SHARE and STORE_LOG_HASH enabled. There are no
batch-execution configuration switches: the execution count comes from
`difftest_exec(n)`, and the runtime mode selects batching. Shared PERF_OPT_SHARE
uses exact per-instruction counts. Direct host-pointer HostTLB remains
disabled for shared execution. The separate FAST S/U translation cache is
not included here. Standalone configuration defaults are unchanged.

The default runtime mode is SLOW. Select FAST explicitly with
`difftest_set_exec_mode(DIFFTEST_EXEC_FAST)`. SLOW splits multi-instruction
requests into existing one-instruction calls. FAST executes in batches,
suppresses committed-store queue recording and bypasses PMP/PMA permission
checks, including those used during page-table walks. PMP/PMA checks remain
compiled according to the existing configuration and resume in SLOW.
CSR updates, page-table semantics and other memory-access checks remain
active. FAST is speculative: bypassing permissions can change access-fault
behavior, and a slow checker must independently confirm every segment.

Mode queries use inline helpers. A mode transition resets the committed-store
queue and refreshes MMU/PMP/PMA derived state and translation/trace caches.
Switch at a consumed boundary: pending committed-store queue contents must
not be relied on afterward. A forked worker still needs the boundary refresh
API before resuming, even if its requested mode already matches.

FAST batch execution returns at instruction boundaries. Compare
`difftest_get_instr_count()` before and after `difftest_exec(n)` to obtain
actual progress; the caller must handle a short request or zero progress
at exceptions and stops. FAST internal batches are capped at INT_MAX because
the interpreter batch counter is an int. Shared trace-cache state is refreshed
when the external PC or cache-flush state changes. REF logging remains
controlled by the existing runtime debug flag.

FAST calls `cpu_exec(n)` once and executes the requested instructions
continuously inside the interpreter. The per-instruction budget decrement
provides an exact stopping boundary; it does not copy register state or call
`regcpy`. Execution mode and the internal batch limit are selected once per
`cpu_exec` call. SLOW retains its existing sequence of `cpu_exec(1)` calls.
Use `difftest_regcpy` explicitly when a state snapshot is needed, for example
at a batch endpoint, an architectural event or a fork boundary. Ordinary
stretches can accumulate into one request; skip instructions and external
events must still be applied at their exact positions.

## Boundary APIs

- `difftest_get_pc()` reads the current reference PC.
- `difftest_skip_one()` advances the PC and optionally supplies a register
  writeback for a skipped instruction. It does not execute its side effects.
- `difftest_exec_skip()` executes one instruction and then supplies the
  requested register writeback.
- `difftest_flush_state()` rebuilds MMU/PMP/PMA derived state and invalidates
  translation and trace caches before a forked slow worker resumes.
- `difftest_state_hash()` returns register-state and ordered scalar-store
  digests. Synchronize the architectural state through regcpy before hashing.

## Independent interpreter and store configurations

- `PERF_OPT` selects the original standalone optimized interpreter.
- `PERF_OPT_SHARE` selects the optimized shared REF interpreter. It supports
  both FAST and SLOW and requires exact per-instruction counting.
- `STORE_LOG` enables rollback support; the existing runtime
  `enable_store_log` flag controls retaining old memory values.
- `STORE_LOG_HASH` independently enables an ordered scalar RAM-write digest.
  It does not require STORE_LOG or runtime rollback logging.

The common interpreter is compiled when either performance option is enabled.
There is no separate backend configuration.

FAST mode and STORE_LOG_HASH exclude LightQS and RV_AME configurations;
PERF_OPT_SHARE excludes LightQS and shared controller builds. Unsupported
configurations retain their established execution paths and reject FAST
mode selection. Enabling the optimized backend does not select FAST.
The separate FAST S/U translation cache belongs to a later change.

## Incremental store digest

This change depends on NEMU store-hash PR #1231 at
`0edb58ee607397e227c9b28ca325754adac3bdcb`. Both committed-store queue checking
and boundary digests reuse its version-1 address/data/mask hash utility.
A scalar write is normalized into one or two aligned 8-byte records;
masked-off bytes are zero. Their order is preserved in the rolling digest.

The boundary path updates the digest at each scalar RAM write and queries
it directly at checkpoints. Its state is 24 bytes: two hash words and a
record count. It retains no effect-record vector, scans no queue, and reads
no old memory solely for hashing. With rollback support compiled, another
24-byte digest checkpoint accompanies the existing rollback interval.
Old values (`orig_data`) remain in rollback logs only when rollback is active.
The committed-store queue keeps its existing prefix checking and failure
records; SLOW still produces queue entries, while FAST suppresses them.

Collection starts enabled in difftest_init when STORE_LOG_HASH is compiled.
`difftest_store_hash_version()` returns protocol version 1, or 0 when absent.
`difftest_store_hash_enabled()` reports the current collection flag.
`difftest_set_store_hash(false)` pauses collection without clearing the digest;
`true` resumes it. `difftest_store_hash_reset()` starts an empty digest and
is a no-op when the feature is absent. Check both version and collection
state before relying on the result. Zero records alone do not prove coverage.

A fork inherits the same digest prefix. Parent FAST and child SLOW can extend
that prefix independently and compare at the same instruction endpoint.
The caller can instead reset at an agreed segment origin. Hash queries and
committed-store queue consumption do not reset the rolling digest.

The first logged rollback write snapshots the current digest. Restoring that
contiguous interval restores both old memory and the digest; an empty restore
leaves the digest untouched. Do not interleave unlogged writes with a pending
rollback interval. The existing store-log reset clears both the log and digest.

The five-word state-hash output layout is unchanged, but its store fields
are incompatible with the earlier experimental address/data/mask/orig_data
hash. Use matching library versions on both sides. Removing orig_data from
the digest removes its pre-write-value coverage. These hashes do not compare
all guest memory, MMIO or matrix effects. Physical-write capture includes
RAM writes such as AMOs even when the committed-store queue configuration
filters AMOs; the two streams must have matching policies before comparison.
The skip helpers retain their existing integer-register writeback interface.
