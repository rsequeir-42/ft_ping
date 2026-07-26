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

### DD-001 - Console conformance for the network output
- Status: open
- Date: 2026-06-03 (CLI surface delivered #36, 2026-06-21)
- Current choice: the conformance suite covers the CLI surface (help screens, errors) -- a self-contained shell harness comparing the binary's output to snapshots established against the etalon, forced `LC_ALL=C`, `make conformance` inside `check`. The network output (reply lines, statistics) is not covered yet.
- Why temporary: `ft_ping` emits no packets or statistics yet, so there is nothing to compare for that part.
- Review trigger: the first network sprint where `ft_ping` prints reply/stat lines. Extend the suite to them, normalizing variable fields (RTT, times).

### DD-003 - Parser fuzzing
- Status: open
- Date: 2026-06-03
- Current choice: not set up.
- Why temporary: the reply parser does not exist; there is nothing to fuzz.
- Review trigger: the `receive`/parse module. Planned shape: libFuzzer (`libfuzzer-19-dev`, clang-19) on the pure parsing function, a dedicated `fuzz` target kept out of `check`.

### DD-004 - Packet-level conformance
- Status: open
- Date: 2026-06-03 (icmp-packet reached 2026-07-25)
- Current choice: the built packet is asserted byte-for-byte in a UNIT test (test_icmp), but nothing is captured on the wire.
- Why temporary: `ft_ping` now builds a packet but does not transmit it yet, so there is nothing to capture.
- Review trigger: the `send-receive` sprint, where packets are actually emitted. Planned shape: capture with tcpdump in a netns, assert ICMP fields/checksum with scapy; a separate `check-wire` target.

### DD-005 - Property-based testing
- Status: open
- Date: 2026-06-03 (checksum reached 2026-07-26)
- Current choice: `checksum` is covered by vectors verified against RFC 1071 and scapy, plus the `recompute == 0` property and edge cases -- 100% covered. No generative PBT.
- Why temporary: `theft` (vendored) plus a separate `test_pbt` binary is a fixed setup cost, not justified for a single pure function already exhaustively covered by verified vectors.
- Review trigger: the `stats` sprint, which adds a second pure function with algebraic invariants (min/avg/max/stddev). Reassess then whether `theft` amortizes across `checksum` + `stats`. Planned shape unchanged: theft (vendored), a separate `test_pbt` binary, properties such as RFC 1071 invariants and "never crashes on arbitrary input".

### DD-007 - trixie runs only the check job
- Status: open
- Date: 2026-06-10
- Current choice: the Debian trixie container runs only the `check` job; `analyze` and `memcheck` run on the ubuntu runner only.
- Why temporary: on a stub, duplicating the informative jobs inside the trixie container costs install time for no added signal.
- Review trigger: when `analyze`/`memcheck` become meaningful (real code); extend the matrix to trixie if a divergence on the grading OS would matter.

### DD-008 - No CI dependency cache
- Status: open
- Date: 2026-06-10
- Current choice: each CI job installs the apt toolchain from scratch (well under 90s); nothing is cached.
- Why temporary: caching the apt fileset is fragile for `libcriterion-dev` (its `.pc`/`.so` and `ldconfig` step), and the install is currently cheap; reproducibility is favored over speed.
- Review trigger: if install time becomes a bottleneck. Then a pre-provisioned Docker image (sharing the VM's toolchain) rather than an apt fileset cache.

### DD-009 - Open Graph meta tags for the journal
- Status: open
- Date: 2026-06-14
- Current choice: the journal ships mdBook's default `<head>` (a meta description, no Open Graph tags).
- Why temporary: social-share cards (`og:title`/`og:description`/`og:locale`) would need a `theme/head.hbs` override, one more theme file to maintain against mdBook updates -- not worth it for now.
- Review trigger: when the journal is shared widely enough that clean link previews matter. Then add a small `theme/head.hbs`.

### DD-010 - Dead-link checking for the journal
- Status: open
- Date: 2026-06-14
- Current choice: links in the journal (mostly the "Sources" sections) are checked by hand.
- Why temporary: `mdbook-linkcheck2` is another binary to install and pin, and web-link checking is flaky (a momentarily unreachable site is not a dead link); there are few links to watch.
- Review trigger: when the journal grows link-heavy enough that manual checking becomes unreliable.

### DD-011 - Non-root minimum interval for -i
- Status: open
- Date: 2026-06-19
- Current choice: `-i` validates its conversion, sign and overflow and stores the interval in milliseconds, but the 200 ms minimum for a non-root user (inetutils' `PING_MIN_USER_INTERVAL`) is not enforced; `-i 0` is currently accepted.
- Why temporary: that floor depends on `is_root` (`getuid() == 0`). Enforcing it at parse time mixes parsing with privilege and makes the unit tests UID-dependent (the CI may run as root), whereas inetutils computes `is_root` in `main` and applies the floor when it actually paces the pings -- i.e. in the network stage.
- Review trigger: the sprint that actually paces the probes. `raw-socket` did not introduce `is_root` after all -- the setuid drop was dropped (see DD-013), so nothing computes `getuid()` yet. Enforce `interval < PING_MIN_USER_INTERVAL` (200 ms) for a non-root user when the send loop honors `-i`.

### DD-013 - Effective drop of CAP_NET_RAW
- Status: open
- Date: 2026-07-25
- Current choice: after opening the socket, `ft_ping` does not drop the capability. inetutils' `setuid(getuid())` is a no-op under our deployment models (sudo: the real UID is already 0; file capability: no transition from UID 0, so the kernel does not clear `CAP_NET_RAW`), so we do not reproduce it.
- Why temporary: the real hardening -- removing `CAP_NET_RAW` from the effective set via libcap (`cap_set_proc`) -- would add a `-lcap` dependency for little gain on a single-capability binary.
- Review trigger: if the binary gains other capabilities, or if a security audit requires an effective drop.
