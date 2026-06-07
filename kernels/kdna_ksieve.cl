__kernel void kdna_ksieve_kernel(
    __global const ulong *symbols,
    __global const ulong *edge_from,
    __global const ulong *edge_to,
    __global uchar *baseline_hit,
    __global uchar *grammar_hit,
    const ulong n,
    const ulong edge_n,
    const ulong train_n,
    const ulong test_trans,
    const ulong baseline
) {
    const ulong j = (ulong)get_global_id(0);
    if (j >= test_trans) return;
    const ulong i = train_n + j;
    if (i + 1UL >= n) {
        baseline_hit[j] = (uchar)0;
        grammar_hit[j] = (uchar)0;
        return;
    }

    const ulong from = symbols[i];
    const ulong to = symbols[i + 1UL];
    baseline_hit[j] = (to == baseline) ? (uchar)1 : (uchar)0;

    /* KGRAM edges are sorted lexicographically by (from,to). Binary search only,
       no learning, no prediction, no heuristic expansion. */
    ulong lo = 0UL;
    ulong hi = edge_n;
    while (lo < hi) {
        const ulong mid = lo + ((hi - lo) >> 1);
        const ulong ef = edge_from[mid];
        const ulong et = edge_to[mid];
        if (ef < from || (ef == from && et < to)) lo = mid + 1UL;
        else hi = mid;
    }

    grammar_hit[j] = (lo < edge_n && edge_from[lo] == from && edge_to[lo] == to) ? (uchar)1 : (uchar)0;
}
