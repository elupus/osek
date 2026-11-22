# Benchmarks: OSEK vs other RTOSes on RISC-V

The metric apps in `test/Os_Metric*` count how often a task runs, or how
often the kernel switches tasks, in one second of OS time. Each directory
here has the same apps written for another RTOS:

| Directory | RTOS | Version |
|---|---|---|
| `freertos/` | [FreeRTOS](https://github.com/FreeRTOS/FreeRTOS-Kernel) | V11.1.0 |
| `threadx/` | [Eclipse ThreadX](https://github.com/eclipse-threadx/threadx) | v6.5.2.202603b |

`compare.sh` builds OSEK and every RTOS here, runs them on the same machine
with the same compiler, flags and C library, and prints one table, so the
counts can be compared directly.

## Running

On Debian or Ubuntu:

```sh
sudo apt-get install gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf \
    cmake ninja-build qemu-system-riscv   # qemu-system-misc on older releases
benchmarks/compare.sh
```

The script configures and builds the OSEK apps with the `riscv32-qemu`
preset into `build/riscv32-qemu`, and each RTOS `benchmarks/<os>` into
`build/bench-<os>`. The RTOS sources are fetched by CMake's `FetchContent`
at a pinned release into that build directory; nothing from them is kept
in this repository. See the section of each RTOS below for how to build
against a local checkout instead.

All builds use `toolchain/Toolchain-riscv32-elf.cmake`: RV32IMAC
(`-march=rv32imac_zicsr -mabi=ilp32`), `-Os -DNDEBUG` (MinSizeRel),
picolibc, and QEMU's `virt` machine with output and exit through
semihosting.

## Method

Each app is run with

```sh
qemu-system-riscv32 -M virt -bios none -nographic -monitor none \
    -semihosting-config enable=on,target=native \
    -icount shift=0,sleep=off -kernel <elf>
```

With `-icount shift=0` QEMU advances virtual time by exactly 1 ns per
executed instruction, and `sleep=off` keeps it from tying virtual time to
the host clock. One second of virtual time is therefore exactly 10^9
instructions, the 10 ms tick fires every 10^7 instructions, and the results
are the same on every run and every host. Before `main` picolibc reads the
command line, which is the `-kernel` argument, through semihosting, so the
length of that path changes the start up slightly and can shift a count by
one; `compare.sh` runs each app from its own directory so that it does not
depend on where the repository is checked out.

Limits of this method:

- The numbers are **instruction counts, not cycles**. QEMU is not cycle
  accurate: it models no pipeline, caches, branch prediction or memory wait
  states, and a load, a divide or a CSR access all count as one
  instruction. On real hardware the trap entry and exit, CSR accesses and
  memory traffic of a context switch usually cost more than an ALU
  instruction, so a kernel that saves fewer registers or takes fewer traps
  may do better than its instruction count shows.
- Compressed and full size instructions count the same, so code size does
  not show up in the numbers.
- It measures one configuration of each kernel: OSEK with the options of
  the metric apps, and each other RTOS with the configuration described in
  its section below.

## The metrics

Every app has a supervisor task at the highest priority that starts the
test, waits one second plus a tick, and prints `Execution counts N (...)`.
"Instructions per operation" is 10^9 divided by the number of operations.

| Metric | OSEK app | One operation |
|---|---|---|
| Basic | `Os_MetricBasic` | One pass of a task's loop over a 1024 word array. No kernel calls; the only kernel cost is the tick interrupt. |
| Cooperative | `Os_MetricCooperative` | One task switch between five tasks of equal priority. OSEK: `ChainTask` to itself, which terminates the task and restarts it at its entry after the others. N counts per task, so operations are N × 5. |
| Preemptive | `Os_MetricPreemptive` | One round of a chain through five priorities: the lowest task wakes the next higher one, which preempts it and wakes the next, up to the highest. OSEK: `ActivateTask` of a basic task, which runs and `TerminateTask`s, and the lowest task restarts with `ChainTask`. A round is eight task switches. |
| Events | `Os_MetricEvents` | One round between two tasks: a low priority task wakes a higher one, which runs and blocks again. OSEK: `SetEvent` to an extended task in `WaitEvent`, which `ClearEvent`s and waits again. A round is two task switches. |

Basic only checks that all apps run the same loop at the same speed and
that the tick costs about the same. An OSEK **basic task activation**
(`ActivateTask`, then the task runs to `TerminateTask`) usually has no
direct equivalent in other RTOSes, whose tasks do not terminate and
restart; the sections below say how each RTOS models it. **The event
ping-pong is the fair comparison**: every kernel wakes a waiting task and
blocks it again, two switches per round with no task restart and the same
priorities.

## FreeRTOS

`freertos/FreeRTOS_Metric<Name>` is the counterpart of
`test/Os_Metric<Name>`, and `freertos/FreeRTOS_Metric.h` has the start up
they share. The project fetches FreeRTOS-Kernel at tag `V11.1.0`; to build
against a local checkout, pass
`-DFETCHCONTENT_SOURCE_DIR_FREERTOS_KERNEL=<path>` when configuring
`benchmarks/freertos`. It is configured by `freertos/FreeRTOSConfig.h`:
preemption without time slicing, as OSEK; no stack overflow checks, trace
or other unused features; generic task selection.

| Metric | FreeRTOS app | FreeRTOS primitives |
|---|---|---|
| Basic | `FreeRTOS_MetricBasic` | none |
| Cooperative | `FreeRTOS_MetricCooperative` | `taskYIELD` |
| Preemptive | `FreeRTOS_MetricPreemptive` | `xTaskNotifyGive` to a task blocked in `ulTaskNotifyTake` |
| Events | `FreeRTOS_MetricEvents` | `xTaskNotifyGive` and `ulTaskNotifyTake` |

How the kernels map:

- FreeRTOS models an OSEK **activation** with a task that loops, blocks on
  a **task notification** in place of terminating, and is woken with
  `xTaskNotifyGive` in place of being activated. That is the usual way to
  write the same thing in FreeRTOS, but OSEK does more work per
  activation: it starts the task afresh at its entry.
- **Cooperative is not like-for-like.** `ChainTask` terminates the task and
  restarts it, while `taskYIELD` only switches to the next ready task and
  later returns into the loop. The row shows what the idiomatic way to
  hand over between equal priority tasks costs in each kernel, not the
  cost of the same operation.
- In the **event ping-pong**, `Os_SetEvent`/`Os_WaitEvent` are the OSEK
  services closest to task notifications.

## ThreadX

`threadx/ThreadX_Metric<Name>` is the counterpart of `test/Os_Metric<Name>`,
`threadx/ThreadX_Metric.h` has the start up they share, and
`threadx/ThreadX_Metric_Port.S` what ThreadX leaves to the application:
`_tx_initialize_low_level`, the trap entry and the tick, a CLINT machine
timer interrupt every 10 ms that calls `_tx_timer_interrupt`, as in the
port's `qemu_virt` example. The project fetches ThreadX at tag
`v6.5.2.202603b_rel`, the latest release, and builds the generic sources
and the `ports/risc-v32/gnu` port; to build against a local checkout, pass
`-DFETCHCONTENT_SOURCE_DIR_THREADX=<path>` when configuring
`benchmarks/threadx`.

That port is a real RV32 port, so ThreadX runs on the same RV32IMAC target
as the others. It supports ilp32 (soft float, no FP state saved) as well as
ilp32d; its own example is built for `rv32gc`/`ilp32d`, but nothing in the
port needs more than RV32IMA and Zicsr.

It is configured by `threadx/tx_user.h`: 100 Hz tick, timers expired in the
tick interrupt (`TX_TIMER_PROCESS_IN_ISR`) as OSEK alarms and FreeRTOS
delays are, instead of in a timer thread, no parameter checking
(`TX_DISABLE_ERROR_CHECKING`, as OSEK without extended status), and no
preemption threshold, notify callbacks or stack filling. Threads are
created with `TX_NO_TIME_SLICE`. ThreadX priorities are reversed: the
supervisor runs at 0, the highest.

| Metric | ThreadX app | ThreadX primitives |
|---|---|---|
| Basic | `ThreadX_MetricBasic` | none |
| Cooperative | `ThreadX_MetricCooperative` | `tx_thread_relinquish` |
| Preemptive | `ThreadX_MetricPreemptive` | `tx_semaphore_put` to a thread blocked in `tx_semaphore_get` |
| Events | `ThreadX_MetricEvents` | `tx_event_flags_set` and `tx_event_flags_get` with `TX_OR_CLEAR` |

How the kernels map:

- ThreadX threads do not terminate and restart either, so an OSEK
  **activation** is modelled as for FreeRTOS: a thread that loops and
  blocks on a **semaphore** in place of terminating, and is woken with
  `tx_semaphore_put` in place of being activated. A counting semaphore is
  ThreadX's equivalent of a FreeRTOS task notification, and like an OSEK
  activation a put is not lost when the thread is not waiting yet.
  `tx_thread_resume`/`tx_thread_suspend` would be a little lighter, about
  636000 rounds instead of 611737, but a resume of a thread that is not suspended
  is an error and is lost, so it does not have the semantics of an
  activation.
- **Cooperative is not like-for-like**, as for FreeRTOS:
  `tx_thread_relinquish` only moves the thread behind the others of its
  priority, while `ChainTask` restarts it.
- The **event ping-pong** uses event flags, the ThreadX service closest to
  OSEK events: `tx_event_flags_set` is `SetEvent`, and
  `tx_event_flags_get` with `TX_OR_CLEAR` is `WaitEvent` followed by
  `ClearEvent`. Event flags are a separate object rather than part of the
  thread, and `tx_event_flags_set` has to search the group's waiting
  threads, so they cost a little more than a task notification.

## Results

Measured with `compare.sh` on OSEK master at a8d90eb, FreeRTOS V11.1.0,
ThreadX v6.5.2.202603b, GCC 14.2.0, picolibc and QEMU 10.0. Counts are per
second of virtual time, i.e. per 10^9 instructions.

| Metric | OSEK | FreeRTOS | ThreadX | OSEK ins/op | FreeRTOS ins/op | ThreadX ins/op |
|---|--:|--:|--:|--:|--:|--:|
| Basic | 140780 | 140782 | 140781 | 7103 | 7103 | 7103 |
| Cooperative | 686123 | 1765706 | 1819783 | 291 | 113 | 110 |
| Preemptive | 375874 | 523850 | 611737 | 2660 | 1909 | 1635 |
| Events | 1359319 | 2086747 | 1984245 | 736 | 479 | 504 |

Basic matches, so the kernels run the same task code with the same tick
overhead. In the event ping-pong FreeRTOS needs about two thirds of the
instructions OSEK does per round, and ThreadX about the same as FreeRTOS.
Cooperative and Preemptive include OSEK's task restarts, which the others
do not do, and so overstate the difference in switch cost. When a thread
blocks or wakes a higher priority one through a service call, ThreadX
switches without a trap and saves only the callee-saved registers, while
the FreeRTOS RISC-V port yields with an `ecall` and saves the full
context; that is likely most of ThreadX's lead in Preemptive.

## Adding an RTOS

`compare.sh` treats every directory `benchmarks/<os>/` with a
`CMakeLists.txt` as an RTOS to compare; there is no list to update. Such a
directory is a standalone CMake project that:

- configures with `-DCMAKE_TOOLCHAIN_FILE=toolchain/Toolchain-riscv32-elf.cmake`
  and defaults to `MinSizeRel`, like the `riscv32-qemu` preset;
- fetches the RTOS with `FetchContent` at a pinned release tag;
- has one directory `<Prefix>_Metric<Name>` per metric, with a target of
  the same name, where `<Name>` is that of `test/Os_Metric<Name>` and
  `<Prefix>` is the RTOS's usual spelling (`FreeRTOS`, `ThreadX`). The script takes
  the column title from the prefix and runs
  `build/bench-<os>/<Prefix>_Metric<Name>/<Prefix>_Metric<Name>`;
- prints `Execution counts N (...)` from a highest priority supervisor
  after one second plus a tick and exits through semihosting.

A metric the RTOS does not implement is shown as `-`. Add a section above
that maps each metric to the RTOS's primitives, and its numbers to the
results.

## On GitHub

The `Benchmark` workflow runs `compare.sh` and shows the table in the run's
summary. It is not part of the checks on each commit; start it by hand from
the Actions tab, or with `gh workflow run benchmark.yml --ref <branch>`.
