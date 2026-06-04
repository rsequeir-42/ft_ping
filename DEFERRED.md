# Deferred decisions

This file records decisions taken provisionally: choices made "for now" that we already know are not the last word. Each entry states the current choice, why it is temporary, and the concrete condition that will trigger its review. The list is reviewed at every milestone; once a decision is settled, its entry is removed.

Status values: `open` (in effect), `triggered` (condition met, to revisit), `resolved`.

Entry format:

    ### DD-NNN - <short title>
    - Status: open
    - Date: YYYY-MM-DD
    - Current choice: <what we do for now>
    - Why temporary: <the constraint or reason>
    - Review trigger: <a concrete date, milestone, or event>

---

### DD-001 - Console conformance suite
- Status: open
- Date: 2026-06-03
- Current choice: none yet; the test harness ships unit tests (Criterion) and the etalon installer only.
- Why temporary: `ft_ping` produces no output yet, so there is nothing to compare against the reference.
- Review trigger: the first network sprint where `ft_ping` prints a line. Planned shape: a parameterized harness (`FT_PING=`, `REF_PING=`) driven by bats-core, normalizing variable fields and forcing `LC_ALL=C`.

### DD-002 - Real coverage and full-binary ASan run
- Status: open
- Date: 2026-06-03
- Current choice: `coverage` is a placeholder; `run-asan` is a placeholder; the test binary already runs under ASan.
- Why temporary: there is no real module code to cover, and `ft_ping` does nothing runnable yet.
- Review trigger: the first logic module (checksum/options/stats). Then `coverage` runs gcovr on the unit-test binary, and `run-asan` exercises the full `ft_ping` under ASan.

### DD-003 - Parser fuzzing
- Status: open
- Date: 2026-06-03
- Current choice: not set up.
- Why temporary: the reply parser does not exist; there is nothing to fuzz.
- Review trigger: the `receive`/parse module. Planned shape: libFuzzer (`libfuzzer-19-dev`, clang-19) on the pure parsing function, a dedicated `fuzz` target kept out of `check`.

### DD-004 - Packet-level conformance
- Status: open
- Date: 2026-06-03
- Current choice: not set up.
- Why temporary: `ft_ping` emits no packets yet.
- Review trigger: the `icmp-packet`/`checksum` modules. Planned shape: capture with tcpdump in a netns, assert ICMP fields/checksum with scapy; a separate `check-wire` target.

### DD-005 - Property-based testing
- Status: open
- Date: 2026-06-03
- Current choice: Criterion theories cover chosen vectors; no generative PBT.
- Why temporary: the pure functions to fuzz with properties (checksum, stats) do not exist yet.
- Review trigger: the `checksum`/`stats` modules. Planned shape: theft (vendored), a separate `test_pbt` binary, properties such as RFC 1071 algebraic invariants and "never crashes on arbitrary input".