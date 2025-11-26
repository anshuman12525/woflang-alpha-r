#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#ifdef _WIN32
#  include <windows.h>
#endif

namespace woflang {

// Forward declaration so we can use it in function types.
class WoflangInterpreter;

// Value type used on the Woflang stack.
enum class WofType {
    Unknown = 0,
    Integer,
    Double,
    String,
    Symbol
};

struct UnitInfo {
    std::string name;
    double scale{1.0};
};

class WofValue {
public:
    using Storage = std::variant<std::monostate, std::int64_t, double, std::string>;

    WofType type{WofType::Unknown};
    Storage value{};
    std::shared_ptr<UnitInfo> unit{};

    WofValue() = default;

    static WofValue make_int(std::int64_t v);
    static WofValue make_double(double v);
    static WofValue make_string(const std::string& s);
    static WofValue make_symbol(const std::string& s);

    bool operator==(const WofValue& other) const;
    bool operator!=(const WofValue& other) const;

    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] double as_numeric() const;
    [[nodiscard]] bool is_numeric() const;
};

// Function signature for primitive operations and plugin ops.
using WofOpHandler = std::function<void(WoflangInterpreter&)>;

// Interpreter core.
class WoflangInterpreter {
public:
    WoflangInterpreter();
    ~WoflangInterpreter();

    std::vector<WofValue> stack;

    // Registration of primitive operations
    void register_op(const std::string& name, WofOpHandler handler);

    // Dynamic plugin loading
    void load_plugin(const std::filesystem::path& dll_path);
    void load_plugins(const std::filesystem::path& plugin_dir);

    // Execution entry points
    void exec_line(const std::string& line);
    void exec_script(const std::filesystem::path& filename);
    void repl();

    // Stack helpers
    template <typename T>
    void push(T&& v) {
        stack.emplace_back(std::forward<T>(v));
    }

    WofValue pop();
    std::int64_t pop_int();
    double pop_double();
    double pop_numeric();
    std::string pop_string();
    std::string pop_symbol();
    bool pop_bool();

    [[nodiscard]] bool stack_has(std::size_t n) const {
        return stack.size() >= n;
    }

    [[nodiscard]] const std::vector<WofValue>& get_stack() const {
        return stack;
    }

    void print_stack() const;
    void clear_stack();

private:
#ifdef _WIN32
    using PluginHandle = HMODULE;
#else
    using PluginHandle = void*;
#endif

    std::unordered_map<std::string, WofOpHandler> ops;
    std::vector<PluginHandle> plugin_handles;

    void dispatch_token(const std::string& token);
    [[noreturn]] void error(const std::string& msg);
};

} // namespace woflang
