#ifndef DETAIL_INVOKE_HPP
#define DETAIL_INVOKE_HPP

#include <utility>

namespace jpc {

  namespace detail {

    /* `std::invoke` */

    #define RETURN(...) -> decltype(__VA_ARGS__) { return __VA_ARGS__; }

    template <typename F, typename... As>
    constexpr auto invoke(F &&f, As &&... as)
      RETURN(std::forward<F>(f)(std::forward<As>(as)...))

    template <typename B, typename T, typename D>
    constexpr auto invoke(T B::*pmv, D &&d)
      RETURN(std::forward<D>(d).*pmv)

    template <typename Pmv, typename Ptr>
    constexpr auto invoke(Pmv pmv, Ptr &&ptr)
      RETURN((*std::forward<Ptr>(ptr)).*pmv)

    template <typename B, typename T, typename D, typename... As>
    constexpr auto invoke(T B::*pmf, D &&d, As &&... as)
      RETURN((std::forward<D>(d).*pmf)(std::forward<As>(as)...))

    template <typename Pmf, typename Ptr, typename... As>
    constexpr auto invoke(Pmf pmf, Ptr &&ptr, As &&... as)
      RETURN(((*std::forward<Ptr>(ptr)).*pmf)(std::forward<As>(as)...))

    #undef RETURN

  }  // namespace detail

}  // namespace jpc

#endif  // DETAIL_INVOKE_HPP
