#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <jpc.hpp>

#include <google/protobuf/message.h>

#include "intro.pb.h"

#include <gtest/gtest.h>

namespace intro {

  class Bar {
    public:
    static const auto &full() {
      static const auto schema = jpc::object<Bar, BarInfo>(
          jpc::field(jpc::boolean              , &Bar::x_, "x"),
          jpc::field(jpc::number               , &Bar::y_, "y"),
          jpc::field(jpc::optional(jpc::string), &Bar::z_, "z")
      )
      .json(jpc::cpp)
      .protobuf(jpc::cpp);
      return schema;
    }

    bool x_;
    int64_t y_;
    boost::optional<std::string> z_;
  };

  class Foo {
    public:
    static const auto &full() {
      static const auto schema = jpc::object<Foo, FooInfo>(
          jpc::field(jpc::boolean              , &Foo::a_, "a"),
          jpc::field(jpc::number               , &Foo::b_, "b"),
          jpc::field(jpc::number               , &Foo::c_, "c"),
          jpc::field(jpc::number               , &Foo::d_, "d"),
          jpc::field(jpc::number               , &Foo::e_, "e"),
          jpc::field(jpc::number               , &Foo::f_, "f"),
          jpc::field(jpc::number               , &Foo::g_, "g"),
          jpc::field(jpc::enumeration          , &Foo::h_, "h"),
          jpc::field(jpc::string               , &Foo::i, "i" ),
          jpc::field(jpc::array(jpc::number)   , &Foo::j_, "j"),
          jpc::field(jpc::array(jpc::string)   , &Foo::k_, "k"),
          jpc::field(jpc::optional(jpc::number), &Foo::l_, "l"),
          jpc::field(jpc::optional(jpc::number), &Foo::m_, "m"),
          jpc::field(jpc::number               , &Foo::n_, "n"),
          jpc::field(jpc::optional(jpc::number), &Foo::o_, "o"),
          jpc::field(jpc::optional(jpc::number), &Foo::p_, "p"),
          jpc::field(Bar::full(),                &Foo::bar_, "bar")
      )
      .json(jpc::cpp)
      .protobuf(jpc::cpp);
      return schema;
    }

    Foo(bool a,
        int32_t b,
        int64_t c,
        uint32_t d,
        uint64_t e,
        double f,
        float g,
        FooInfo::H h,
        std::string i,
        std::vector<int32_t> j,
        std::set<std::string> k,
        boost::optional<int32_t> l,
        boost::optional<int32_t> m,
        int64_t n,
        std::unique_ptr<int32_t> o,
        std::unique_ptr<int32_t> p,
        Bar bar)
        : a_(std::move(a)),
          b_(std::move(b)),
          c_(std::move(c)),
          d_(std::move(d)),
          e_(std::move(e)),
          f_(std::move(f)),
          g_(std::move(g)),
          h_(std::move(h)),
          i_(std::move(i)),
          j_(std::move(j)),
          k_(std::move(k)),
          l_(std::move(l)),
          m_(std::move(m)),
          n_(std::move(n)),
          o_(std::move(o)),
          p_(std::move(p)),
          bar_(std::move(bar)) {}

    const std::string &i() const { return i_; }

    private:
    bool a_;
    int32_t b_;
    int64_t c_;
    uint32_t d_;
    uint64_t e_;
    double f_;
    float g_;
    FooInfo::H h_;
    std::string i_;
    std::vector<int32_t> j_;
    std::set<std::string> k_;
    boost::optional<int32_t> l_;
    boost::optional<int32_t> m_;
    int64_t n_;
    std::unique_ptr<int32_t> o_;
    std::unique_ptr<int32_t> p_;
    Bar bar_;
  };

}  // namespace intro

using namespace intro;

TEST(JPC, Intro) {
  std::vector<int32_t> v = {1, 2, 3};
  std::set<std::string> w = {"hello", "world"};
  Bar bar{false, 42, boost::none};
  Foo foo(true,
          101,
          202,
          303u,
          404u,
          1.1,
          2.2,
          FooInfo::X,
          "hello",
          v,
          w,
          505,
          boost::none,
          606,
          std::make_unique<int32_t>(707),
          nullptr,
          bar);
  {
    // Bar => BarInfo
    BarInfo bar_info = Bar::full().protobuf(bar);
    EXPECT_FALSE(bar_info.x());
    EXPECT_TRUE(bar_info.has_y());
    EXPECT_EQ(42, bar_info.y());
    EXPECT_FALSE(bar_info.has_z());
  }
  {
    // Foo => FooInfo
    FooInfo foo_info = Foo::full().protobuf(foo);
    EXPECT_EQ(true, foo_info.a());
    EXPECT_EQ(101, foo_info.b());
    EXPECT_EQ(202, foo_info.c());
    EXPECT_EQ(303u, foo_info.d());
    EXPECT_EQ(404u, foo_info.e());
    EXPECT_DOUBLE_EQ(1.1, foo_info.f());
    EXPECT_FLOAT_EQ(2.2, foo_info.g());
    EXPECT_EQ(FooInfo::X, foo_info.h());
    // EXPECT_EQ("hello", foo_info.i());
    const google::protobuf::RepeatedField<int32_t> &x = foo_info.j();
    EXPECT_EQ(v.size(), static_cast<std::size_t>(x.size()));
    for (std::size_t i = 0; i < v.size(); ++i) {
      EXPECT_EQ(v[i], x.Get(i));
    }  // for
    const google::protobuf::RepeatedPtrField<std::string> &y = foo_info.k();
    EXPECT_EQ(w.size(), static_cast<std::size_t>(y.size()));
    auto w_iter = std::begin(w);
    auto y_iter = std::begin(y);
    // std::vector<std::string> w_expected(, std::end(w));
    for (; w_iter != std::end(w); ++w_iter, ++y_iter) {
      EXPECT_EQ(*w_iter, *y_iter);
    }
    EXPECT_TRUE(foo_info.has_l());
    EXPECT_EQ(505, foo_info.l());
    EXPECT_FALSE(foo_info.has_m());
    EXPECT_EQ(0, foo_info.m());
    EXPECT_EQ(606, foo_info.n());
    EXPECT_TRUE(foo_info.has_o());
    EXPECT_EQ(707, foo_info.o());
    EXPECT_FALSE(foo_info.has_p());
    EXPECT_EQ(0, foo_info.p());

    EXPECT_TRUE(foo_info.has_bar());
    const BarInfo &bar_info = foo_info.bar();
    EXPECT_FALSE(bar_info.x());
    EXPECT_TRUE(bar_info.has_y());
    EXPECT_EQ(42, bar_info.y());
    EXPECT_FALSE(bar_info.has_z());
  }
  {
    // Bar => json::string
    std::ostringstream strm;
    strm << Bar::full().json(bar);
    EXPECT_EQ(R"~~({"x":false,"y":42,"z":null})~~", strm.str());
  }
  {
    // Foo => json::string
    std::ostringstream strm;
    strm << Foo::full().json(foo);
    EXPECT_EQ(
        R"~~({"a":true,"b":101,"c":202,"d":303,"e":404,"f":1.1,"g":2.2,)~~"
        R"~~("h":1,"i":"hello","j":[1,2,3],"k":["hello","world"],"l":505,)~~"
        R"~~("m":null,"n":606,"o":707,"p":null,)~~"
        R"~~("bar":{"x":false,"y":42,"z":null}})~~",
        strm.str());
  }
}
