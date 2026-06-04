#include <criterion/criterion.h>
#include <criterion/new/assert.h>

/* Smoke test: proves the Criterion toolchain compiles, links and runs, and that
   `make test` is wired into the build. Real unit tests arrive with the modules. */
Test(smoke, toolchain_is_wired) {
    cr_assert(eq(int, 1, 1));
}
