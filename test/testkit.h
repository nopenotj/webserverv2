
#include <format>
#include <stdexcept>
#include <utility>

namespace testkit {

template <typename T, typename S> void is_equal(T left, S right) {
  if (left != right) {
    throw std::logic_error(
        std::format("Compared values are not equal. Left = '{}', Right = '{}'",
                    left, right));
  }
}
} // namespace testkit
