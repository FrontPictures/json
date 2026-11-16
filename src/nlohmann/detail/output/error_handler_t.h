#ifndef ERROR_HANDLER_T_H
#define ERROR_HANDLER_T_H

namespace nlohmann {
namespace detail {

enum class error_handler_t { strict, replace, ignore };

}
} // namespace nlohmann

#endif // ERROR_HANDLER_T_H
