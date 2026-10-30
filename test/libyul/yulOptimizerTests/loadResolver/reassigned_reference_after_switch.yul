{
    let i := 0
    switch calldataload(0) case 0 { i := add(i, 1) }
    // Only becomes a copy of `i` after the expression simplifier, so `k` does not reference `i` directly.
    let i1 := add(i, 0)
    let k := add(i1, 2)
    sstore(k, add(sload(k), calldataload(128)))
    // The value of `i` is unknown both before and after this.
    switch calldataload(64) case 0 { i := add(i, 2) }
    sstore(i, calldataload(192))
    // `i` and `k` can be equal, so this cannot be resolved.
    mstore(0, sload(k))
    return(0, 64)
}
// ----
// step: loadResolver
//
// {
//     {
//         let i := 0
//         switch calldataload(i)
//         case 0 { i := 1 }
//         let _4 := 0
//         let i1 := i
//         let _5 := 2
//         let k := add(i1, _5)
//         sstore(k, add(sload(k), calldataload(128)))
//         let _10 := 64
//         switch calldataload(_10)
//         case 0 { i := add(i, _5) }
//         sstore(i, calldataload(192))
//         mstore(_4, sload(k))
//         return(_4, _10)
//     }
// }
