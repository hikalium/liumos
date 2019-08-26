#pragma once

template <int hi, int lo, typename TReturn, typename Texpr>
constexpr TReturn GetBits(Texpr v) {
  assert(hi >= lo);
  return static_cast<TReturn>((v >> lo) & ((1 << (hi - lo + 1)) - 1));
}
