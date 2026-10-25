### Unreleased

Breaking changes:
 * ABI coder: Remove v1 support. All contracts use ABI coder v2; `pragma abicoder v1` is rejected.

Bugfixes:
 * Code Generator: Convert `mldsa87verify` and `xmssverify` arguments in evaluation order in the IR code generator, so a storage `bytes` argument is copied to memory before later arguments can modify it, as the legacy code generator and the SMTChecker already do.
 * Test host: Verify real ML-DSA-87 signatures and reject weak public keys, matching go-qrl.
 * SMTChecker: Apply digest conversions to `bytes64` when modeling `mldsa87verify`, preventing incorrect proofs and errors on string literals.

Build System:
 * Build with current Apple toolchains (Apple Clang 21, libc++ 22), Boost 1.88+ (Boost.Process v1 headers) and Boost 1.89+ (no compiled Boost.System library).
 * Raise the macOS deployment target from 10.14 to 11.0, the lowest that libc++ 22 supports. The released `hypc-macos` binary requires macOS 11 or later.

### 0.8.24
