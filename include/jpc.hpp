#ifndef JPC_HPP
#define JPC_HPP

#include <cassert>
#include <iterator>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <jpc/detail/apply.hpp>
#include <jpc/detail/invoke.hpp>

#include <google/protobuf/message.h>

#include <meta/meta.hpp>

namespace jpc {

  struct Json {};
  struct Protobuf {};
  struct Cpp {};

  static constexpr Json json{};
  static constexpr Protobuf protobuf{};
  static constexpr Cpp cpp{};

  namespace detail {

    /* schemas. */

    class boolean;
    class enumeration;
    class number;
    class string;

    template <typename Schema>
    class array;

    template <typename F, typename Schema>
    class field;

    template <typename Schema>
    class optional;

    template <typename Object, typename Message, typename... Fields>
    class object;

    #define RETURN(...) -> decltype(__VA_ARGS__) { return __VA_ARGS__; }

    namespace adl {

      using std::begin;
      using std::end;

      template <typename T>
      auto adl_begin(T &&t) RETURN(begin(std::forward<T>(t)))

      template <typename T>
      auto adl_end(T &&t) RETURN(end(std::forward<T>(t)))

    }  // namespace adl

    namespace json {

      template <typename T, typename Schema>
      class string;

      template <>
      class string<bool, boolean> {
        public:
        bool value_;

        private:
        string(const string &) = default;
        string(string &&) = default;

        friend class boolean;

        friend std::ostream &operator<<(std::ostream &strm,
                                        const string &that) {
          std::ios_base::fmtflags flags = strm.setf(std::ios_base::boolalpha);
          strm << that.value_;
          strm.flags(flags);
          return strm;
        }
      };

      template <typename Enum>
      class string<Enum, enumeration> {
        public:
        static_assert(std::is_enum<Enum>{}, "");

        Enum value_;

        private:
        string(const string &) = default;
        string(string &&) = default;

        friend class enumeration;

        friend std::ostream &operator<<(std::ostream &strm,
                                        const string &that) {
          return strm << that.value_;
        }
      };

      template <typename Arithmetic>
      class string<Arithmetic, number> {
        public:
        static_assert(std::is_arithmetic<Arithmetic>{}, "");

        Arithmetic value_;

        private:
        string(const string &) = default;
        string(string &&) = default;

        friend class number;

        friend std::ostream &operator<<(std::ostream &strm,
                                        const string &that) {
          return strm << that.value_;
        }
      };

      template <>
      class string<std::string, detail::string> {
        public:
        const std::string &value_;

        private:
        string(const string &) = default;
        string(string &&) = default;

        friend class detail::string;

        friend std::ostream &operator<<(std::ostream &strm,
                                        const string &that) {
          return strm << '"' << that.value_ << '"';
        }
      };

      template <typename Iter, typename Schema>
      class string<Iter, array<Schema>> {
        public:
        Iter begin_;
        Iter end_;
        Schema schema_;

        private:
        string(const string &) = default;
        string(string &&) = default;

        friend class array<Schema>;

        friend std::ostream &operator<<(std::ostream &strm,
                                        const string &that) {
          strm << '[';
          if (that.begin_ != that.end_) {
            auto iter = that.begin_;
            strm << that.schema_.json(*iter++);
            while (iter != that.end_) {
              strm << ',' << that.schema_.json(*iter++);
            }
          }  // for
          strm << ']';
          return strm;
        }
      };

      template <typename Optional, typename Schema>
      class string<Optional, optional<Schema>> {
        public:
        const Optional &value_;
        Schema schema_;

        private:
        string(const string &) = default;
        string(string &&) = default;

        friend class optional<Schema>;

        friend std::ostream &operator<<(std::ostream &strm,
                                        const string &that) {
          if (!that.value_) {
            return strm << "null";
          }
          return strm << that.schema_.json(*that.value_);
        }
      };

      template <typename Object,
                typename Message,
                typename... Fields>
      class string<Object, object<Object, Message, Fields...>> {
        public:
        const Object &value_;
        std::tuple<Fields...> fields_;

        private:
        string(const string &) = default;
        string(string &&) = default;

        friend class object<Object, Message, Fields...>;

        friend std::ostream &operator<<(std::ostream &strm,
                                        const string &that) {
          strm << '{';
          detail::apply(
              [&](const auto &field, const auto &... fields) {
                field.json_object(strm, that.value_);
                int for_each[] = {[&] {
                  strm << ',';
                  fields.json_object(strm, that.value_);
                  return 0;
                }()...};
                (void)for_each;
              },
              that.fields_);
          strm << '}';
          return strm;
        }
      };

    }  // namespace json

    namespace protobuf {

      using google::protobuf::FieldDescriptor;
      using google::protobuf::Message;
      using google::protobuf::Reflection;
      using google::protobuf::RepeatedField;
      using google::protobuf::RepeatedPtrField;

      /* is_message */

      template <typename T>
      struct is_message : std::is_convertible<T *, Message *> {};

      /* cpp_type */

      template <typename T, typename E = void>
      struct CppType;

#define CPP_TYPE(TYPE, type)                             \
  template <>                                            \
  struct CppType<type>                                   \
      : std::integral_constant<FieldDescriptor::CppType, \
                               FieldDescriptor::CPPTYPE_##TYPE> {};
      CPP_TYPE(BOOL, bool)
      CPP_TYPE(INT32, int32_t)
      CPP_TYPE(INT64, int64_t)
      CPP_TYPE(UINT32, uint32_t)
      CPP_TYPE(UINT64, uint64_t)
      CPP_TYPE(DOUBLE, double)
      CPP_TYPE(FLOAT, float)
      CPP_TYPE(STRING, std::string)
#undef CPP_TYPE

      template <typename Enum>
      struct CppType<Enum, meta::if_<std::is_enum<Enum>>>
          : std::integral_constant<FieldDescriptor::CppType,
                                   FieldDescriptor::CPPTYPE_ENUM> {};

      template <typename Message>
      struct CppType<Message, meta::if_<is_message<Message>>>
          : std::integral_constant<FieldDescriptor::CppType,
                                   FieldDescriptor::CPPTYPE_MESSAGE> {};

      template <typename T>
      static constexpr FieldDescriptor::CppType cpp_type = CppType<T>{};

      /* repeated */

      template <typename T, typename E = void>
      struct Repeated;

      template <typename Arithmetic>
      struct Repeated<Arithmetic, meta::if_<std::is_arithmetic<Arithmetic>>>
          : meta::id<RepeatedField<Arithmetic>> {};

      template <typename Enum>
      struct Repeated<Enum, meta::if_<std::is_enum<Enum>>>
          : meta::id<RepeatedField<int>> {};

      template <>
      struct Repeated<std::string> : meta::id<RepeatedPtrField<std::string>> {};

      template <typename Message>
      struct Repeated<Message, meta::if_<is_message<Message>>>
          : meta::id<RepeatedPtrField<Message>> {};

      template <typename T>
      using repeated = meta::_t<Repeated<T>>;

      template <typename T, typename E = void>
      struct TypeCheck;

      template <typename T>
      struct TypeCheck<T,
                       meta::if_<meta::or_<std::is_arithmetic<T>,
                                           std::is_enum<T>,
                                           std::is_same<T, std::string>,
                                           is_message<T>>>> {
        bool operator()(const FieldDescriptor *field_descriptor) const {
          return (field_descriptor->is_required() ||
                  field_descriptor->is_optional()) &&
                 field_descriptor->cpp_type() == cpp_type<T>;
        }
      };

      template <typename T>
      struct TypeCheck<boost::optional<T>> {
        bool operator()(const FieldDescriptor *field_descriptor) const {
          return field_descriptor->is_optional() &&
                 field_descriptor->cpp_type() == cpp_type<T>;
        }
      };

      template <typename T>
      struct TypeCheck<RepeatedField<T>> {
        bool operator()(const FieldDescriptor *field_descriptor) const {
          return field_descriptor->is_repeated() &&
                 field_descriptor->cpp_type() == cpp_type<T>;
        }
      };

      template <typename T>
      struct TypeCheck<RepeatedPtrField<T>> {
        bool operator()(const FieldDescriptor *field_descriptor) const {
          return field_descriptor->is_repeated() &&
                 field_descriptor->cpp_type() == cpp_type<T>;
        }
      };

      template <typename T>
      bool type_check(const FieldDescriptor *field_descriptor) {
        return TypeCheck<T>{}(field_descriptor);
      }

      /* set_elem */

      template <typename T>
      meta::if_<is_message<T>,
      void> set_elem(T *elem, meta::id_t<T> &&value) {
        elem->CopyFrom(std::move(value));
      }

      template <typename T>
      meta::if_<meta::not_<is_message<T>>,
      void> set_elem(T *elem, meta::id_t<T> &&value) {
        *elem = std::move(value);
      }

      /* set_field */

      struct set_field {
        set_field(Message *message, const FieldDescriptor *field_descriptor)
            : message_(message),
              field_descriptor_(field_descriptor),
              reflection_(message_->GetReflection()) {}

#define OP(Type, type)                                                     \
  void operator()(type value) const {                                      \
    reflection_->Set##Type(message_, field_descriptor_, std::move(value)); \
  }
        OP(Bool, bool)
        OP(Int32, int32_t)
        OP(Int64, int64_t)
        OP(UInt32, uint32_t)
        OP(UInt64, uint64_t)
        OP(Double, double)
        OP(Float, float)
        OP(String, std::string)
#undef OP

        template <typename Enum>
        meta::if_<std::is_enum<Enum>,
        void> operator()(Enum value) const {
          const auto *enum_value_descriptor =
              field_descriptor_->enum_type()->FindValueByNumber(value);
          reflection_->SetEnum(
              message_, field_descriptor_, enum_value_descriptor);
        }

        template <typename T>
        void operator()(boost::optional<T> &&value) const {
          if (value) {
            (*this)(*std::move(value));
          }  // if
        }

        template <typename Message>
        meta::if_<is_message<Message>,
        void> operator()(Message &&value) const {
          auto *message =
              reflection_->MutableMessage(message_, field_descriptor_);
          message->CopyFrom(std::move(value));
        }

        template <typename T>
        void operator()(RepeatedField<T> &&value) const {
          auto *repeated =
              reflection_->MutableRepeatedField<T>(message_, field_descriptor_);
          repeated->CopyFrom(std::move(value));
        }

        template <typename T>
        void operator()(RepeatedPtrField<T> &&value) const {
          auto *repeated = reflection_->MutableRepeatedPtrField<T>(
              message_, field_descriptor_);
          repeated->CopyFrom(std::move(value));
        }

        private:
        Message *message_;
        const FieldDescriptor *field_descriptor_;
        const Reflection *reflection_;
      };  // set_field

    }  // namespace protobuf

    /* schemas */

    class boolean {
      public:
      json::string<bool, boolean> json(bool value) const { return {value}; }
      bool protobuf(bool value) const { return value; }
    };  // boolean

    class enumeration {
      public:
      template <typename Enum>
      meta::if_<std::is_enum<Enum>,
      json::string<Enum, enumeration>> json(Enum value) const { return {value}; }

      template <typename Enum>
      meta::if_<std::is_enum<Enum>,
      Enum> protobuf(Enum value) const { return value; }
    };  // enumeration

    class number {
      public:
      json::string<int32_t, number> json(int32_t value) const { return {value}; }
      json::string<int64_t, number> json(int64_t value) const { return {value}; }
      json::string<uint32_t, number> json(uint32_t value) const { return {value}; }
      json::string<uint64_t, number> json(uint64_t value) const { return {value}; }
      json::string<double, number> json(double value) const { return {value}; }
      json::string<float, number> json(float value) const { return {value}; }

      int32_t protobuf(int32_t value) const { return value; }
      int64_t protobuf(int64_t value) const { return value; }
      uint32_t protobuf(uint32_t value) const { return value; }
      uint64_t protobuf(uint64_t value) const { return value; }
      double protobuf(double value) const { return value; }
      float protobuf(float value) const { return value; }
    };  // number

    class string {
      public:
      json::string<std::string, string> json(std::string value) const {
        return {value};
      }

      std::string protobuf(std::string value) const { return std::move(value); }
    };  // string

    template <typename Schema>
    class array {
      public:
      constexpr array(Schema schema) : schema_(std::move(schema)) {}

      private:
      Schema schema_;

      template <typename Iter>
      auto json(Iter first, Iter last) const -> meta::_t<decltype(
          first != last, ++first, meta::id<json::string<Iter, array>>{})> {
        return {first, last, schema_};
      }

      template <typename Iter>
      auto protobuf(Iter first, Iter last) const
          -> meta::_t<decltype(first != last,
                               ++first,
                               meta::id<protobuf::repeated<decltype(
                                   this->schema_.protobuf(*first))>>{})> {
        protobuf::repeated<decltype(schema_.protobuf(*first))> result;
        for (; first != last; ++first) {
          protobuf::set_elem(result.Add(), schema_.protobuf(*first));
        }  // for
        return result;
      }

      public:
      template <typename Iterable>
      auto json(const Iterable &value) const
        RETURN(this->json(adl::adl_begin(value), adl::adl_end(value)))

      template <typename Iterable>
      auto protobuf(const Iterable &value) const
        RETURN(this->protobuf(adl::adl_begin(value), adl::adl_end(value)))
    };  // array

    template <typename Schema, typename F>
    class field {
      public:
      field(Schema schema, F f, const char *name)
          : schema_(std::move(schema)),
            f_(std::move(f)),
            name_(std::move(name)) {}

      private:
      template <typename Object>
      void json_object(std::ostream &strm, const Object &value) const {
        strm << string{}.json(name_) << ':'
             << schema_.json(detail::invoke(f_, value));
      }

      Schema schema_;
      F f_;
      const char *name_;

      template <typename, typename>
      friend class json::string;

      template <typename Object, typename Message, typename... Fields>
      friend class object;
    };  // field

    template <typename Schema>
    class optional {
      public:
      optional(Schema schema) : schema_(std::move(schema)) {}

      private:
      template <typename Optional>
      json::string<Optional, optional> json(const Optional &value) const {
        return {value, schema_};
      }

      template <typename Optional>
      auto protobuf(const Optional &value) const
        RETURN(value ? boost::make_optional(*value) : boost::none)

      Schema schema_;

      template <typename, typename>
      friend class field;

      template <typename Object, typename Message, typename... Fields>
      friend class object;
    };  // optional

    template <typename Object, typename Message, typename... Fields>
    class object {
      public:
      template <typename... Schemas, typename... Fs>
      constexpr object(field<Schemas, Fs>... fields)
          : fields_{std::move(fields)...} {}

      /* runtime type checkers */

      const object &json(Cpp) const {
        // TODO(mpark): json type_check.
        return *this;
      }

      const object &protobuf(Cpp) const {
        Message dummy;
        const auto *descriptor = dummy.GetDescriptor();
        detail::apply(
            [&](Fields... fields) {
              int for_each[] = {[&] {
                const auto *field_descriptor =
                    descriptor->FindFieldByName(fields.name_);
                if (!field_descriptor) {
                  throw std::runtime_error("missing field");
                }  // if
                using T = decltype(fields.schema_.protobuf(
                    detail::invoke(fields.f_, std::declval<Object>())));
                if (!protobuf::type_check<T>(field_descriptor)) {
                  throw std::runtime_error("type check failed");
                }  // if
                protobuf::set_field{&dummy, field_descriptor}(T{});
                return 0;
              }()...};
              (void)for_each;
            },
            fields_);
        // dummy.CheckInitialized();
        /*
        TODO(mpark): Throw the same exception as above error cases rather than
                     Google's `FatalException`.
        if (!dummy.IsInitialized()) {
          throw std::runtime_error("missing fields");
        }  // if
        */
        return *this;
      }

      /* converters */

      json::string<Object, object> json(const Object &value) const {
        return {value, fields_};
      }

      Message protobuf(const Object &value) const {
        Message result;
        const auto *descriptor = result.GetDescriptor();
        detail::apply(
            [&](Fields... fields) {
              int for_each[] = {[&] {
                using protobuf::type_check;
                using protobuf::set_field;
                const auto *field_descriptor =
                    descriptor->FindFieldByName(fields.name_);
                assert(field_descriptor);
                auto field =
                    fields.schema_.protobuf(detail::invoke(fields.f_, value));
                assert(type_check<decltype(field)>(field_descriptor));
                set_field{&result, field_descriptor}(std::move(field));
                return 0;
              }()...};
              (void)for_each;
            },
            fields_);
        assert(result.IsInitialized());
        return result;
      }

      private:
      std::tuple<Fields...> fields_;
    };  // object

    #undef RETURN

  }  // namespace detail

  constexpr detail::boolean boolean{};
  constexpr detail::enumeration enumeration{};
  constexpr detail::number number{};
  constexpr detail::string string{};

  template <typename Schema>
  constexpr auto array(Schema schema) {
    return detail::array<Schema>{std::move(schema)};
  }

  template <typename F, typename Schema>
  constexpr auto field(Schema schema, F f, const char *name) {
    return detail::field<Schema, F>{std::move(schema), std::move(f), name};
  }

  template <typename Schema>
  constexpr auto optional(Schema schema) {
    return detail::optional<Schema>{std::move(schema)};
  }

  template <typename Object,
            typename Message,
            typename... Schemas,
            typename... Fs>
  constexpr auto object(detail::field<Schemas, Fs>... fields) {
    return detail::object<Object, Message, detail::field<Schemas, Fs>...>{
        std::move(fields)...};
  }

}  // namespace jpc

#endif  // JPC_HPP
