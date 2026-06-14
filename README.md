# ft_ping

[![CI](https://github.com/rsequeir-42/ft_ping/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/rsequeir-42/ft_ping/actions/workflows/ci.yml) [![Pages](https://github.com/rsequeir-42/ft_ping/actions/workflows/pages.yml/badge.svg)](https://rsequeir-42.github.io/ft_ping/)

A from-scratch reimplementation of the `ping` command in C, reproducing the behaviour and output of GNU inetutils-2.0. Built for the 42 cursus: it sends ICMP Echo Request packets over a raw socket and measures the round-trip time to a host.

## Development journal

The making of `ft_ping` is documented as it goes, in a series of short essays — in French — on each piece of groundwork laid before the program itself. Read them online: **[ft_ping — journal de bord](https://rsequeir-42.github.io/ft_ping/)**.

## Build

```sh
make          # release build -> ./ft_ping
make debug    # debug build (ASan + UBSan)
make re       # clean rebuild
make clean    # remove object files
make fclean   # remove objects and the binary
```

## Reproducible environment

`ft_ping` is meant to run in a Debian 13 (trixie) VM, created and provisioned on demand so the toolchain is identical on every machine.

### Prerequisites

- **VirtualBox**, in a version whose `vboxdrv` module builds against your kernel (for Linux 6.17: `>= 7.1.14` or `>= 7.2.0`; 7.1.10 fails). Your user must belong to the `vboxusers` group.
- Host tools (apt): `xorriso`, `curl`, `openssl`, `cpio`, `openssh-client`, `ansible`.

### Targets

```sh
make vm            # create + install + provision a Debian trixie VM (headless); prints the SSH command
make bootstrap     # provision the CURRENT machine instead (no VM; toolchain via Ansible)
make check-env     # check the quality toolchain is present (no install, no sudo)
make lint-playbook # lint the Ansible provisioning
```

`make vm` builds a throwaway VM; `make bootstrap` is for when you are already on a Debian/Ubuntu host and just want the toolchain locally.

### Notes

- The VM installs in **`en_US.UTF-8`**: the serial console used to log the install only offers ASCII locales. Locale-sensitive output (e.g. the decimal separator) is tested by pinning both `ft_ping` and the reference to the same locale.
- The ISO and the disk live **outside** the repository, under `FTPING_VM_DIR` (default `~/.cache/ftping-vm`; on a 42 workstation, set it to `~/goinfre/...`). The VM is disposable: rerun `make vm` to rebuild it from scratch.
- Tunable via environment variables: `FTPING_VM_DIR`, `FTPING_VM_NAME`, `FTPING_VM_RAM`, `FTPING_VM_CPUS`, `FTPING_VM_DISK`, `FTPING_VM_SSH_PORT`, `FTPING_VM_USER`, `FTPING_VM_MIRROR`.

## Quality tooling

Formatting, static analysis and runtime checks, orchestrated by one gate. The tools are installed by `make bootstrap` / `make vm`.

```sh
make format        # apply clang-format in place
make format-check  # verify formatting only (used by the hook and by check)
make lint          # clang-tidy + cppcheck
make analyze       # gcc -fanalyzer (deeper, slower; run on demand, not in check)
make memcheck      # valgrind on the binary (run on demand, not in check)
make coverage      # structure only; measurement deferred (see DEFERRED.md)
make check         # the gate: check-env + format-check + lint + build + tests
```

The git hooks in `.githooks/` are versioned. **Run `make hooks` once after cloning** to activate them via `core.hooksPath`: `pre-commit` verifies formatting and lints staged C sources, `pre-push` runs `make check`. Both are bypassable with `--no-verify`; CI is the real gate.

## Testing

Unit tests use [Criterion](https://github.com/Snaipe/Criterion) (installed by `make bootstrap`).

```sh
make test   # build the test binary (debug + ASan/UBSan) and run it
```

The test binary links the module objects without `main.o` (Criterion provides its own entry point) and is built with sanitizers, so `make test` also catches memory bugs in the tested logic. `make test` runs inside `make check`.

The conformance suite compares `ft_ping` to the reference `ping` from GNU inetutils 2.0. That reference is built, isolated, by:

```sh
tests/install-etalon.sh     # builds inetutils-2.0 ping into ~/.local/bin/inetutils-ping (never touch the system ping)
```

Override the install path with `FT_PING_ETALON`. The script verifies the tarball's GPG signature, is idempotent, and grants `cap_net_raw` so the reference runs without sudo.

Other test types (console and packet-level conformance, fuzzing, property-based testing, real coverage) are scaffolded as they become relevant; see `DEFERRED.md`.

## Continuous integration

Every pull request replays `make check` through GitHub Actions, on the development OS (Ubuntu) and inside a Debian trixie container — the OS the project is graded on — so a green check proves the gate passes where it counts. Two informative jobs run `gcc -fanalyzer` and valgrind. The `check` job is a required status check, so `main` only advances through a green pull request: that is the real gate the bypassable git hooks cannot enforce. Action versions are pinned by commit SHA and kept current by Dependabot. See `.github/workflows/ci.yml`.

## Academic integrity

> This repository is a 42 school project, published as a portfolio piece. It is shared to be read and studied, not copied. If you follow the 42 cursus (or a similar one), copying this solution violates your school's integrity rules and deprives you of the learning; submissions are screened for plagiarism. Take inspiration from the approach, but write your own code.

## License

MIT. See [LICENSE](LICENSE).
