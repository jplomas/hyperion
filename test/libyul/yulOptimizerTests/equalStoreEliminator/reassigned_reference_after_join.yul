{
    let t := sload(0)
    if calldataload(128) { t := sload(5) }
    let a := t
    let c := add(a, 2)
    let v := calldataload(0)
    let w := calldataload(64)
    let old := sload(c)
    sstore(c, v)
    // The value of `t` is unknown both before and after this.
    if calldataload(192) { t := sload(6) }
    sstore(t, w)
    // `t` and `c` can be equal, so this cannot be removed.
    sstore(c, v)
}
// ----
// step: equalStoreEliminator
//
// {
//     let t := sload(0)
//     if calldataload(128) { t := sload(5) }
//     let a := t
//     let c := add(a, 2)
//     let v := calldataload(0)
//     let w := calldataload(64)
//     let old := sload(c)
//     sstore(c, v)
//     if calldataload(192) { t := sload(6) }
//     sstore(t, w)
//     sstore(c, v)
// }
