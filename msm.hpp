#pragma once

namespace msm {
namespace aux {
struct none_type {};
template <class...> struct type {};
template <class...> struct type_list { using type = type_list; };
template <class... Ts> struct inherit : Ts... { using type = inherit; };
template <class T> T &&declval();
template <class T, T V> struct integral_constant {
  using type = integral_constant;
  static constexpr T value = V;
};
using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;
template <bool B, class T, class F> struct conditional { using type = T; };
template <class T, class F> struct conditional<false, T, F> { using type = F; };
template <bool B, class T, class F>
using conditional_t = typename conditional<B, T, F>::type;
template <bool B, class T = void> struct enable_if {};
template <class T> struct enable_if<true, T> { using type = T; };
template <bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;
template <class, class> struct is_same : false_type {};
template <class T> struct is_same<T, T> : true_type {};
template <class T, class U>
struct is_base_of : integral_constant<bool, __is_base_of(T, U)> {};
template <class T> struct remove_qualifiers { using type = T; };
template <class T> struct remove_qualifiers<const T> { using type = T; };
template <class T> struct remove_qualifiers<T &> { using type = T; };
template <class T> struct remove_qualifiers<const T &> { using type = T; };
template <class T> struct remove_qualifiers<T *> { using type = T; };
template <class T> struct remove_qualifiers<const T *> { using type = T; };
template <class T> struct remove_qualifiers<T *const &> { using type = T; };
template <class T> struct remove_qualifiers<T *const> { using type = T; };
template <class T> struct remove_qualifiers<const T *const> { using type = T; };
template <class T> struct remove_qualifiers<T &&> { using type = T; };
template <class T>
using remove_qualifiers_t = typename remove_qualifiers<T>::type;
template <class> struct function_traits;
template <class R, class... TArgs> struct function_traits<R (*)(TArgs...)> {
  using args = type_list<TArgs...>;
};
template <class R, class... TArgs> struct function_traits<R(TArgs...)> {
  using args = type_list<TArgs...>;
};
template <class R, class T, class... TArgs>
struct function_traits<R (T::*)(TArgs...)> {
  using args = type_list<TArgs...>;
};
template <class R, class T, class... TArgs>
struct function_traits<R (T::*)(TArgs...) const> {
  using args = type_list<TArgs...>;
};
template <class T> using function_traits_t = typename function_traits<T>::args;
template <class...> struct join;
template <> struct join<> { using type = type_list<>; };
template <class... TArgs> struct join<type_list<TArgs...>> {
  using type = type_list<TArgs...>;
};
template <class... TArgs1, class... TArgs2>
struct join<type_list<TArgs1...>, type_list<TArgs2...>> {
  using type = type_list<TArgs1..., TArgs2...>;
};
template <class... TArgs1, class... TArgs2, class... Ts>
struct join<type_list<TArgs1...>, type_list<TArgs2...>, Ts...> {
  using type = typename join<type_list<TArgs1..., TArgs2...>, Ts...>::type;
};
template <class... TArgs> using join_t = typename join<TArgs...>::type;
template <class, class...> struct unique_impl;
template <class T1, class T2, class... Rs, class... Ts>
struct unique_impl<type<T1, Rs...>, T2, Ts...>
    : conditional_t<
          is_base_of<type<T2>, T1>::value,
          unique_impl<type<inherit<T1>, Rs...>, Ts...>,
          unique_impl<type<inherit<T1, type<T2>>, Rs..., T2>, Ts...>> {};
template <class T1, class... Rs>
struct unique_impl<type<T1, Rs...>> : type_list<Rs...> {};
template <class... Ts> struct unique : unique_impl<type<none_type>, Ts...> {};
template <class T> struct unique<T> : type_list<T> {};
template <class... Ts> using unique_t = typename unique<Ts...>::type;
template <template <class...> class, class> struct apply;
template <template <class...> class T, template <class...> class U, class... Ts>
struct apply<T, U<Ts...>> : T<Ts...> {};
template <template <class...> class T, class D>
using apply_t = typename apply<T, D>::type;
} // aux

struct _ {};
struct type_op {};
struct state_base {};
struct event_base {};
struct anonymous {
  static constexpr auto id = -1;
  anonymous(...) {}
};
struct otherwise {
  static constexpr auto id = -2;
  otherwise(...) {}
};
struct always {
  auto operator()() { return true; }
};
struct none {
  void operator()() {}
};

template <class T> struct opaque {
  using type = T;
  T object;
};

template <class... Ts> struct pool : Ts... {
  using type = pool;
  explicit pool(Ts... args) noexcept : Ts(args)... {}
};

template <class... Ts> auto make_transition_table(Ts... ts) noexcept {
  return pool<Ts...>{ts...};
}

template <class...> struct transition;
template <class, class> struct transition_g;
template <class, class> struct transition_a;
template <class> struct state_properties {};
template <template <class...> class TState, class... Ts>
struct state_properties<TState<Ts...>> : Ts... {};

template <class TState>
struct state_impl : state_properties<TState>, state_base {
  template <class T> auto operator==(const T &t) const noexcept {
    return transition<state_impl, T>{*this, t};
  }

  template <class T> auto operator+(const T &t) const noexcept {
    return transition<state_impl, T>{*this, t};
  }

  template <class T> auto operator[](const T &t) const noexcept {
    return transition_g<state_impl, T>{*this, t};
  }

  template <class T> auto operator/(const T &t) const noexcept {
    return transition_a<state_impl, T>{*this, t};
  }
};

struct state : state_impl<state> {};
struct init_state : state_impl<init_state> {};
template <class... Ts>
struct property_state : state_impl<property_state<Ts...>> {};

template <class TEvent> struct event_impl : event_base {
  template <class T> auto operator[](const T &t) const noexcept {
    return transition<event_impl, T>{*this, t};
  }
};

template <class TEvent> event_impl<TEvent> event{};

template <class E, class G> struct transition<event_impl<E>, G> {
  template <class T> auto operator/(const T &t) const noexcept {
    return transition<event_impl<E>, G, T>{e, g, t};
  }
  event_impl<E> e;
  G g;
};

template <class E, class G, class A> struct transition<event_impl<E>, G, A> {
  event_impl<E> e;
  G g;
  A a;
};

template <class S2, class G, class A> struct transition<state_impl<S2>, G, A> {
  const state_impl<S2> &s2;
  G g;
  A a;
};

template <class S1, class S2>
struct transition<state_impl<S1>, state_impl<S2>> {
  const state_impl<S1> &s1;
  const state_impl<S2> &s2;
};

template <class S2, class G> struct transition_g<state_impl<S2>, G> {
  template <class T> auto operator/(const T &t) const noexcept {
    return transition<state, G, T>{s2, g, t};
  }
  const state_impl<S2> &s2;
  G g;
};

template <class S2, class A> struct transition_a<state_impl<S2>, A> {
  const state_impl<S2> &s2;
  A a;
};

template <class S2, class E> struct transition<state_impl<S2>, event_impl<E>> {
  const state_impl<S2> &s2;
  event_impl<E> e;
};

template <class T, class E> auto args__(int) -> aux::function_traits_t<T>;
template <class T, class E>
auto args__(int)
    -> aux::function_traits_t<decltype(&T::template operator() < E > )>;
template <class T, class>
auto args__(int) -> aux::function_traits_t<decltype(&T::operator())>;
template <class, class> aux::type_list<> args__(...);
template <class T, class E> using args_t = decltype(args__<T, E>(0));

template <class T, class E, class = void> struct get_deps {
  using type = args_t<T, E>;
};

template <class T, class E> using get_deps_t = typename get_deps<T, E>::type;

template <template <class...> class T, class... Ts, class E>
struct get_deps<T<Ts...>, E,
                aux::enable_if_t<aux::is_base_of<type_op, T<Ts...>>::value>> {
  using type = aux::join_t<get_deps_t<Ts, E>...>;
};

template <class T, class TEvent, class TDeps>
auto call(T object, const TEvent &event, const TDeps &deps) noexcept {
  return call_impl(args_t<T, TEvent>{}, object, event, deps);
}

template <
    class T, class TEvent, class TDeps,
    aux::enable_if_t<!aux::is_same<TEvent, aux::remove_qualifiers_t<T>>::value,
                     int> = 0>
decltype(auto) get(const TEvent &, const TDeps &deps) noexcept {
  return static_cast<opaque<T>>(deps).object;
}

template <
    class T, class TEvent, class TDeps,
    aux::enable_if_t<aux::is_same<TEvent, aux::remove_qualifiers_t<T>>::value,
                     int> = 0>
decltype(auto) get(const TEvent &event, const TDeps &) noexcept {
  return event;
}

template <class... Ts, class T, class TEvent, class TDeps,
          aux::enable_if_t<!aux::is_base_of<type_op, T>::value, int> = 0>
auto call_impl(const aux::type_list<Ts...> &, T object, const TEvent &event,
               const TDeps &deps) noexcept {
  return object(get<Ts>(event, deps)...);
}

template <class... Ts, class T, class TEvent, class TDeps,
          aux::enable_if_t<aux::is_base_of<type_op, T>::value, int> = 0>
auto call_impl(const aux::type_list<Ts...> &, T object, const TEvent &event,
               const TDeps &deps) noexcept {
  return object(event, deps);
}

template <class... Ts> struct seq_ : type_op {
  explicit seq_(Ts... ts) noexcept : a(ts...) {}

  template <class TEvent, class TDeps>
  void operator()(const TEvent &event, const TDeps &deps) noexcept {
    int _[]{0, (call(static_cast<Ts>(a), event, deps), 0)...};
    (void)_;
  }

  pool<Ts...> a;
};

template <class T1, class T2>
auto operator, (const T1 &t1, const T2 &t2) noexcept {
  return seq_<T1, T2>(t1, t2);
}

template <class... Ts> struct and_ : type_op {
  explicit and_(Ts... ts) noexcept : g(ts...) {}

  template <class TEvent, class TDeps>
  bool operator()(const TEvent &event, const TDeps &deps) noexcept {
    auto result = true;
    for (auto r : {call(static_cast<Ts>(g), event, deps)...})
      result &= r;
    return result;
  }

  pool<Ts...> g;
};

template <class T1, class T2>
auto operator&&(const T1 &t1, const T2 &t2) noexcept {
  return and_<T1, T2>(t1, t2);
}

template <class... Ts> struct or_ : type_op {
  explicit or_(Ts... ts) noexcept : g(ts...) {}

  template <class TEvent, class TDeps>
  bool operator()(const TEvent &event, const TDeps &deps) noexcept {
    auto result = false;
    for (auto r : {call(static_cast<Ts>(g), event, deps)...})
      result |= r;
    return result;
  }

  pool<Ts...> g;
};

template <class T1, class T2>
auto operator||(const T1 &t1, const T2 &t2) noexcept {
  return or_<T1, T2>(t1, t2);
}

template <class T> struct not_ : type_op {
  explicit not_(T t) noexcept : g(t) {}

  template <class TEvent, class TDeps>
  bool operator()(const TEvent &event, const TDeps &deps) noexcept {
    return !call(g, event, deps);
  }

  T g;
};

template <class T> auto operator!(const T &t) noexcept { return not_<T>(t); }

template <class S1, class S2, class E, class G, class A>
struct transition<state_impl<S1>, state_impl<S2>, event_impl<E>, G, A> {
  using src_state = S1;
  using dst_state = S2;
  using event = E;
  using deps =
      aux::apply_t<aux::unique_t, aux::join_t<get_deps_t<G, event_impl<E>>,
                                              get_deps_t<A, event_impl<E>>>>;

  void init_state(const state_base *cs[], int &i) const noexcept {
    init_state_impl(S1{}, cs, i);
  }
  void init_state_impl(...) const noexcept {}
  void init_state_impl(const msm::init_state &, const state_base *cs[],
                       int &i) const noexcept {
    cs[i++] = &s1;
  }

  template <class TEvent, class TDeps,
            std::enable_if_t<!std::is_same<TEvent, E>::value, int> = 0>
  void process_event(const state_base **, const TEvent &, const TDeps &,
                     bool &) const noexcept {}

  template <class TEvent, class TDeps,
            std::enable_if_t<std::is_same<TEvent, E>::value, int> = 0>
  void process_event(const state_base **state, const TEvent &event,
                     const TDeps &deps, bool &handled) const noexcept {
    if (!handled && *state == &s1 && call(g, event, deps)) {
      call(a, event, deps);
      // on_entry
      *state = &s2;
      // on_exit
      handled = true;
    }
  }

  template <class TVisitor>
  void visit_state(const state_base *state, const TVisitor &visitor,
                   bool &visited) const noexcept {
    if (!visited && &s1 == state) {
      visitor(S1{});
      visited = true;
    }

    if (!visited && &s2 == state) {
      visitor(S2{});
      visited = true;
    }
  }

  const state_impl<S1> &s1;
  const state_impl<S2> &s2;
  event_impl<E> e;
  G g;
  A a;
};

template <class S1, template <class...> class Transition, class S2, class G,
          class A>
struct transition<state_impl<S1>, Transition<state_impl<S2>, G, A>>
    : transition<state_impl<S1>, state_impl<S2>, event_impl<anonymous>, G, A> {
  transition(const state_impl<S1> &s1,
             const Transition<state_impl<S2>, G, A> &t)
      : transition<state_impl<S1>, state_impl<S2>, event_impl<anonymous>, G, A>{
            s1, t.s2, event_impl<anonymous>{}, t.g, t.a} {}
};

template <class S1, template <class...> class Transition, class E, class G,
          class A>
struct transition<state_impl<S1>, Transition<event_impl<E>, G, A>>
    : transition<state_impl<S1>, state_impl<S1>, event_impl<E>, G, A> {
  transition(const state_impl<S1> &s1, const Transition<event_impl<E>, G, A> &t)
      : transition<state_impl<S1>, state_impl<S1>, event_impl<E>, G, A>{
            s1, s1, t.e, t.g, t.a} {}
};

template <class S1, template <class...> class Transition, class S2, class E>
struct transition<state_impl<S1>, Transition<state_impl<S2>, event_impl<E>>>
    : transition<state_impl<S1>, state_impl<S2>, event_impl<E>, always, none> {
  transition(const state_impl<S1> &s1,
             const Transition<state_impl<S2>, event_impl<E>> &t)
      : transition<state_impl<S1>, state_impl<S2>, event_impl<E>, always, none>{
            s1, t.s2, t.e, always{}, none{}} {}
};

template <class S1, class S2, class G>
struct transition<state_impl<S1>, transition_g<state_impl<S2>, G>>
    : transition<state_impl<S1>, state_impl<S2>, event_impl<anonymous>, G,
                 none> {
  transition(const state_impl<S1> &s1, const transition_g<state_impl<S2>, G> &t)
      : transition<state_impl<S1>, state_impl<S2>, event_impl<anonymous>, G,
                   none>{s1, t.s2, event_impl<anonymous>{}, t.g, none{}} {}
};

template <class S1, class S2, class A>
struct transition<state_impl<S1>, transition_a<state_impl<S2>, A>>
    : transition<state_impl<S1>, state_impl<S2>, event_impl<anonymous>, always,
                 A> {
  transition(const state_impl<S1> &s1, const transition_a<state_impl<S2>, A> &t)
      : transition<state_impl<S1>, state_impl<S2>, event_impl<anonymous>,
                   always, A>{s1, t.s2, event_impl<anonymous>{}, always{},
                              t.a} {}
};

template <class S1, template <class...> class Transition, class S2, class E,
          class G, class A>
struct transition<state_impl<S1>,
                  Transition<state_impl<S2>, Transition<event_impl<E>, G, A>>>
    : transition<state_impl<S1>, state_impl<S2>, event_impl<E>, G, A> {
  transition(
      const state_impl<S1> &s1,
      const Transition<state_impl<S2>, Transition<event_impl<E>, G, A>> &t)
      : transition<state_impl<S1>, state_impl<S2>, event_impl<E>, G, A>{
            s1, t.s2, t.e, t.g, t.a} {}
};

template <class... Ts> using add_opaque = aux::type_list<opaque<Ts>...>;

template <class T, class = void> struct ignore {
  using type = aux::type_list<T>;
};

template <class T>
struct ignore<T, aux::enable_if_t<aux::is_base_of<
                     event_base, aux::remove_qualifiers_t<T>>::value>> {
  using type = aux::type_list<>;
};

template <class... Ts>
using ignore_t = aux::join_t<typename ignore<Ts>::type...>;

template <class... Ts>
using merge_deps = aux::apply_t<
    pool, aux::apply_t<
              add_opaque,
              aux::apply_t<ignore_t,
                           aux::apply_t<aux::unique_t,
                                        aux::join_t<typename Ts::deps...>>>>>;

template <class...> struct sum_up;

template <class T, class... Ts>
struct sum_up<T, Ts...>
    : aux::integral_constant<int, T::value + sum_up<Ts...>::value> {};

template <class T> struct sum_up<T> : aux::integral_constant<int, T::value> {};

template <> struct sum_up<> : aux::integral_constant<int, 0> {};

template <class... Ts>
using init_states_nr = aux::apply_t<
    sum_up,
    aux::type_list<aux::integral_constant<
        int, aux::is_same<init_state, typename Ts::src_state>::value>...>>;

template <class, class> class sm_impl;

template <class T, class... TDeps> class sm_impl<T, pool<TDeps...>> {
  using transitions_t = decltype(aux::declval<T>().configure());
  static constexpr auto regions_nr =
      aux::apply_t<init_states_nr, transitions_t>::value;

public:
  explicit sm_impl(typename TDeps::type... deps) noexcept
      : deps_{opaque<decltype(deps)>{deps}...},
        transitions_(T{}.configure()) {
    init_states(transitions_);
  }

  template <class TEvent> void process_event(const TEvent &event) noexcept {
    if (!process_event_impl(transitions_, event)) {
      process_event_impl(transitions_, otherwise{});
    }
  }

  template <class TVisitor>
  void visit_current_states(const TVisitor &visitor) const noexcept {
    visit_current_states_impl(transitions_, visitor);
  };

private:
  template <class... Ts>
  void init_states(const pool<Ts...> &transitions) noexcept {
    auto i = 0;
    int _[]{
        0, (static_cast<const Ts &>(transitions).init_state(current_states_, i),
            0)...};
    (void)_;
  }

  template <class... Ts, class TEvent>
  bool process_event_impl(const pool<Ts...> &transitions,
                          const TEvent &event) noexcept {
    auto handled = false;
    for (auto i = 0; i < regions_nr; ++i) {
      int _[]{0,
              (static_cast<const Ts &>(transitions)
                   .process_event(&current_states_[i], event, deps_, handled),
               0)...};
      (void)_;
    }
    return handled;
  }

  template <class... Ts, class TVisitor>
  void visit_current_states_impl(const pool<Ts...> &transitions,
                                 const TVisitor &visitor) const noexcept {
    for (const auto *state : current_states_) {
      auto visited = false;
      auto i = 0;
      int _[]{0, (static_cast<const Ts &>(transitions)
                      .visit_state(state, visitor, visited),
                  0)...};
      (void)_;
    }
  }

  pool<TDeps...> deps_;
  transitions_t transitions_;
  const state_base *current_states_[regions_nr];
};

template <class T>
using sm =
    sm_impl<T,
            aux::apply_t<merge_deps, decltype(aux::declval<T>().configure())>>;

template <class TEvent, class TConfig> struct dispatcher {
  template <class T>
  static void dispatch_event(const TEvent &e, T &sm) noexcept {
    dispatch_event_impl(e, sm, typename T::events{});
  }

private:
  template <class T, class E, class... Ts>
  static void dispatch_event_impl(const TEvent &event, T &sm,
                                  const aux::type_list<E, Ts...> &) noexcept {
    if (TConfig::template get_id<E>() == TConfig::get_id(event)) {
      sm.process_event(TConfig::template get_event<E>(event));
    }
    dispatch_event_impl(event, sm, aux::type_list<Ts...>{});
  }

  template <class T>
  static void dispatch_event_impl(const TEvent &, T &,
                                  const aux::type_list<> &) noexcept {}
};

} // msm