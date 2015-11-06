#ifndef DETAIL_APPLY_HPP
#define DETAIL_APPLY_HPP

#include <array>
#include <tuple>
#include <utility>

#include <jpc/detail/invoke.hpp>

namespace jpc {

  namespace detail {

    /* `std::apply` */

    template <typename F, typename Tuple, size_t... Is>
    decltype(auto) apply_impl(F &&f, Tuple &&tuple, std::index_sequence<Is...>) {
      return invoke(std::forward<F>(f),
                    std::get<Is>(std::forward<Tuple>(tuple))...);
    }

    template <typename F, typename Tuple>
    decltype(auto) apply(F &&f, Tuple &&tuple) {
      return apply_impl(
          std::forward<F>(f),
          std::forward<Tuple>(tuple),
          std::make_index_sequence<std::tuple_size<std::decay_t<Tuple>>{}>{});
    }

  }  // namespace detail

}  // namespace jpc

#endif  // VARIANT_DETAIL_APPLY_HPP
