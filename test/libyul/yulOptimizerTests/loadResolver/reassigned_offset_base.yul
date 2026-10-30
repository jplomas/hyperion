{
    let t := calldataload(0)
    // Only becomes a copy of `t` after the common subexpression eliminator, so `c` does not reference `t` directly.
    let a := add(t, 0)
    let c := add(a, 2)
    let v := calldataload(128)
    let old := sload(c)
    sstore(c, v)
    // `c` still refers to the old value of `t`.
    t := calldataload(64)
    let x := sload(t)
    let w := calldataload(192)
    // `c` and `t` can be equal, so this store invalidates what is known about `sload(t)`.
    sstore(c, w)
    mstore(0, sload(t))
    return(0, 64)
}
// ----
// step: loadResolver
//
// {
//     {
//         let _1 := 0
//         let t := calldataload(_1)
//         let c := add(t, 2)
//         sstore(c, calldataload(128))
//         let _5 := 64
//         t := calldataload(_5)
//         sstore(c, calldataload(192))
//         mstore(_1, sload(t))
//         return(_1, _5)
//     }
// }
