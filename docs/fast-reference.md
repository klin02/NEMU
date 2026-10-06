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

The opt-in XS configuration enables SHARE_BATCH_EXEC, SHARE_DYNAMIC_BATCH
and PERF_OPT. Shared PERF_OPT uses exact per-instruction counts and retains
the configuration's PMP/PMA checks. Direct host-pointer HostTLB is disabled
for shared execution, so accesses continue through the checked physical
memory path. Standalone configuration defaults are unchanged.

The default runtime mode is SLOW. Select FAST explicitly with
`difftest_set_exec_mode(DIFFTEST_EXEC_FAST)` in a batch-capable build. SLOW
splits multi-instruction requests into existing one-instruction calls.
FAST suppresses committed-store queue recording; switching modes resets
that queue and SLOW restores recording. Switch at a consumed boundary:
pending committed-store queue contents must not be relied on afterward.

Batch execution returns at architectural boundaries. Compare
`difftest_get_instr_count()` before and after `difftest_exec(n)` to obtain
actual progress; the caller must handle a short request or zero progress.
Dynamic batches are capped at INT_MAX because the interpreter batch counter
is an int. Shared trace-cache state is refreshed when the external PC or
cache-flush state changes. REF logging remains controlled by the existing
runtime debug flag.

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

Store effects are collected only with STORE_LOG compiled in and the existing
dynamic enable_store_log flag enabled. A zero store count does not prove
store-effect coverage. Digests do not compare all guest-memory bytes, and
scalar store-effect tracking does not cover matrix stores. Existing matrix
rollback logging retains its original entry point.

Batch mode excludes LightQS and RV_AME configurations; shared PERF_OPT
excludes LightQS and shared controller builds. Unsupported combinations
must retain their established execution paths. The separate FAST S/U
translation cache is not part of this change.
