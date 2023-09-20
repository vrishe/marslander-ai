#include <google/protobuf/repeated_field.h>

#include <iterator>
#include <type_traits>

namespace marslander::pb {

template<typename T, template<class> typename TField,
  std::enable_if_t<
    std::disjunction_v<
      std::is_same<TField<T>, ::google::protobuf::RepeatedField<T>>,
      std::is_same<TField<T>, ::google::protobuf::RepeatedPtrField<T>>
    >,
    bool> = true>
struct inserter {

  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = void;
  using pointer = void;
  using reference = T&;
  using container_type = TField<T>;

  inserter(container_type* f)
    : _f(f) {}

  inserter(const inserter& ) = default;
  inserter(      inserter&&) = default;

  inserter& operator=(const inserter& ) = default;
  inserter& operator=(      inserter&&) = default;

  inserter& operator++(   ) { return *this; }
  inserter& operator++(int) { return *this; }

  reference operator*() { return *_f->Add(); }

private:
  container_type* _f;
};

} // namespace marslander::iterators
