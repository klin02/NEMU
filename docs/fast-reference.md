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
both FAST and SLOW, with PERF_OPT and STORE_LOG_HASH enabled. There are no
batch-execution configuration switches: the execution count comes from
`difftest_exec(n)`, and the runtime mode selects batching. Shared PERF_OPT
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

Batch execution returns at instruction boundaries. Compare
`difftest_get_instr_count()` before and after `difftest_exec(n)` to obtain
actual progress; the caller must handle a short request or zero progress
at exceptions and stops. FAST internal batches are capped at INT_MAX because
the interpreter batch counter is an int. Shared trace-cache state is refreshed
when the external PC or cache-flush state changes. REF logging remains
controlled by the existing runtime debug flag.

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

STORE_LOG_HASH adds ordered scalar-store hashes on top of STORE_LOG.
Collection requires both configuration options and the existing runtime
`enable_store_log` flag. With STORE_LOG_HASH disabled, rollback logging still
works and boundary hashes report zero store words/count. The checker must
confirm hash collection is enabled before relying on those fields; a zero
store count does not prove store-effect coverage. Hashes do not compare all
guest-memory bytes and do not cover matrix stores. Existing matrix rollback
logging retains its original entry point.

FAST mode and STORE_LOG_HASH exclude LightQS and RV_AME configurations;
shared PERF_OPT excludes LightQS and shared controller builds. Unsupported
configurations retain their established execution paths and reject FAST
mode selection. Neither enabling PERF_OPT nor using the fastref defconfig
selects FAST automatically.
