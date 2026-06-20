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

### DD-006 - analyze and memcheck are informative, not required
- Status: open
- Date: 2026-06-10
- Current choice: in CI, the `check` job (both legs) is a required status check; `analyze` and `memcheck` run on every PR but are not required, so a red one does not block merging.
- Why temporary: the binary is a stub, so `analyze` (`-fanalyzer`) and `memcheck` (valgrind) exercise nothing real; making them required would block merges on noise or flakiness.
- Review trigger: the first real logic module. Then promote `analyze` and `memcheck` to required checks alongside `check`.

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
- Review trigger: the first network sprint (`raw-socket`), where `is_root` is computed as in inetutils. Then reject `interval < PING_MIN_USER_INTERVAL` (200 ms) for a non-root user.
