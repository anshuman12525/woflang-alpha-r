#include "woflang.hpp"

#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifndef _WIN32
#  include <dlfcn.h>
#endif

namespace woflang {

// ======== WofValue implementation ========

WofValue WofValue::make_int(std::int64_t v) {
    WofValue w;
    w.type = WofType::Integer;
    w.value = v;
    return w;
}

WofValue WofValue::make_double(double v) {
    WofValue w;
    w.type = WofType::Double;
    w.value = v;
    return w;
}

WofValue WofValue::make_string(const std::string& s) {
    WofValue w;
    w.type = WofType::String;
    w.value = s;
    return w;
}

WofValue WofValue::make_symbol(const std::string& s) {
    WofValue w;
    w.type = WofType::Symbol;
    w.value = s;
    return w;
}

bool WofValue::operator==(const WofValue& other) const {
    if (type != other.type) {
        return false;
    }
    if (unit && other.unit) {
        if (unit->name != other.unit->name || unit->scale != other.unit->scale) {
            return false;
        }
    } else if (unit || other.unit) {
        return false;
    }
    return value == other.value;
}

bool WofValue::operator!=(const WofValue& other) const {
    return !(*this == other);
}

std::string WofValue::to_string() const {
    std::ostringstream oss;
    switch (type) {
        case WofType::Integer:
            oss << std::get<std::int64_t>(value);
            break;
        case WofType::Double:
            oss << std::get<double>(value);
            break;
        case WofType::String:
        case WofType::Symbol:
            oss << std::get<std::string>(value);
            break;
        case WofType::Unknown:
        default:
            oss << "<unknown>";
            break;
    }
    if (unit) {
        oss << " " << unit->name;
    }
    return oss.str();
}

double WofValue::as_numeric() const {
    if (type == WofType::Integer) {
        return static_cast<double>(std::get<std::int64_t>(value));
    }
    if (type == WofType::Double) {
        return std::get<double>(value);
    }
    throw std::runtime_error("WofValue::as_numeric: value is not numeric");
}

bool WofValue::is_numeric() const {
    return type == WofType::Integer || type == WofType::Double;
}

// ======== WoflangInterpreter implementation ========

namespace {

std::string trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

bool is_integer_token(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    std::size_t pos = 0;
    if (token[pos] == '+' || token[pos] == '-') {
        ++pos;
    }
    if (pos >= token.size()) {
        return false;
    }
    for (; pos < token.size(); ++pos) {
        if (!std::isdigit(static_cast<unsigned char>(token[pos]))) {
            return false;
        }
    }
    return true;
}

bool is_float_token(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    bool seen_dot = false;
    std::size_t pos = 0;
    if (token[pos] == '+' || token[pos] == '-') {
        ++pos;
    }
    bool any_digit = false;
    for (; pos < token.size(); ++pos) {
        char c = token[pos];
        if (std::isdigit(static_cast<unsigned char>(c))) {
            any_digit = true;
        } else if (c == '.') {
            if (seen_dot) {
                return false;
            }
            seen_dot = true;
        } else {
            return false;
        }
    }
    return any_digit && seen_dot;
}

// Simple whitespace tokenizer that keeps quoted strings together.
std::vector<std::string> simple_tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_quotes = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            current.push_back(c);
            if (in_quotes) {
                tokens.push_back(current);
                current.clear();
            }
            in_quotes = !in_quotes;
        } else if (std::isspace(static_cast<unsigned char>(c)) && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current.push_back(c);
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

} // anonymous namespace

WoflangInterpreter::WoflangInterpreter() {
    // Core arithmetic operations
    register_op("+", [](WoflangInterpreter& interp) {
        double b = interp.pop_numeric();
        double a = interp.pop_numeric();
        interp.push(WofValue::make_double(a + b));
    });

    register_op("-", [](WoflangInterpreter& interp) {
        double b = interp.pop_numeric();
        double a = interp.pop_numeric();
        interp.push(WofValue::make_double(a - b));
    });

    register_op("*", [](WoflangInterpreter& interp) {
        double b = interp.pop_numeric();
        double a = interp.pop_numeric();
        interp.push(WofValue::make_double(a * b));
    });

    register_op("/", [](WoflangInterpreter& interp) {
        double b = interp.pop_numeric();
        double a = interp.pop_numeric();
        if (b == 0.0) {
            interp.error("division by zero");
        }
        interp.push(WofValue::make_double(a / b));
    });

    // Stack manipulation
    register_op("dup", [](WoflangInterpreter& interp) {
        if (!interp.stack_has(1)) {
            interp.error("dup requires at least one value on the stack");
        }
        const auto& stk = interp.get_stack();
        WofValue v = stk.back();
        interp.push(v);
    });

    register_op("drop", [](WoflangInterpreter& interp) {
        if (!interp.stack_has(1)) {
            interp.error("drop requires at least one value on the stack");
        }
        (void)interp.pop();
    });

    register_op("swap", [](WoflangInterpreter& interp) {
        if (!interp.stack_has(2)) {
            interp.error("swap requires at least two values on the stack");
        }
        auto& stk = const_cast<std::vector<WofValue>&>(interp.get_stack());
        std::swap(stk[stk.size() - 1], stk[stk.size() - 2]);
    });

    register_op("print", [](WoflangInterpreter& interp) {
        if (!interp.stack_has(1)) {
            std::cout << "(stack empty)" << std::endl;
            return;
        }
        const auto& stk = interp.get_stack();
        std::cout << stk.back().to_string() << std::endl;
    });

    register_op(".s", [](WoflangInterpreter& interp) {
        interp.print_stack();
    });
}

WoflangInterpreter::~WoflangInterpreter() {
    for (auto handle : plugin_handles) {
#ifdef _WIN32
        if (handle != nullptr) {
            FreeLibrary(handle);
        }
#else
        if (handle != nullptr) {
            dlclose(handle);
        }
#endif
    }
    plugin_handles.clear();
}

void WoflangInterpreter::register_op(const std::string& name, WofOpHandler handler) {
    ops[name] = std::move(handler);
}

void WoflangInterpreter::load_plugin(const std::filesystem::path& dll_path) {
    if (!std::filesystem::exists(dll_path)) {
        // Silently ignore missing plugin path; caller usually iterates directory.
        return;
    }

#ifdef _WIN32
    PluginHandle handle = LoadLibraryA(dll_path.string().c_str());
    if (!handle) {
        std::cerr << "Failed to load plugin: " << dll_path.string() << std::endl;
        return;
    }

    using RegisterPluginFunc = void (*)(WoflangInterpreter&);
    auto* raw = GetProcAddress(handle, "register_plugin");
    if (!raw) {
        std::cerr << "Plugin " << dll_path.string() << " has no register_plugin symbol" << std::endl;
        FreeLibrary(handle);
        return;
    }

    auto register_func = reinterpret_cast<RegisterPluginFunc>(raw);
    register_func(*this);
    plugin_handles.push_back(handle);
#else
    PluginHandle handle = dlopen(dll_path.c_str(), RTLD_LAZY);
    if (!handle) {
        std::cerr << "Failed to load plugin: " << dll_path.string()
                  << " (" << dlerror() << ")" << std::endl;
        return;
    }

    using RegisterPluginFunc = void (*)(WoflangInterpreter&);
    dlerror(); // clear
    auto* raw = dlsym(handle, "register_plugin");
    const char* err = dlerror();
    if (err != nullptr || !raw) {
        std::cerr << "Plugin " << dll_path.string()
                  << " has no register_plugin symbol (" << (err ? err : "") << ")"
                  << std::endl;
        dlclose(handle);
        return;
    }

    auto register_func = reinterpret_cast<RegisterPluginFunc>(raw);
    register_func(*this);
    plugin_handles.push_back(handle);
#endif
}

void WoflangInterpreter::load_plugins(const std::filesystem::path& plugin_dir) {
    if (!std::filesystem::exists(plugin_dir) || !std::filesystem::is_directory(plugin_dir)) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(plugin_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        const auto& path = entry.path();
#ifdef _WIN32
        if (path.extension() == ".dll")
#else
        if (path.extension() == ".so" || path.extension() == ".dylib")
#endif
        {
            load_plugin(path);
        }
    }
}

void WoflangInterpreter::dispatch_token(const std::string& token) {
    // Comment support: ignore everything after '#'
    if (!token.empty() && token[0] == '#') {
        return;
    }

    // Quoted string
    if (token.size() >= 2 && token.front() == '"' && token.back() == '"') {
        std::string inner = token.substr(1, token.size() - 2);
        push(WofValue::make_string(inner));
        return;
    }

    // Integers / floats
    if (is_integer_token(token)) {
        std::int64_t v = std::stoll(token);
        push(WofValue::make_int(v));
        return;
    }
    if (is_float_token(token)) {
        double v = std::stod(token);
        push(WofValue::make_double(v));
        return;
    }

    // Known operator?
    auto it = ops.find(token);
    if (it != ops.end()) {
        it->second(*this);
        return;
    }

    // Fallback: treat as symbol and push onto the stack.
    push(WofValue::make_symbol(token));
}

void WoflangInterpreter::exec_line(const std::string& line) {
    const std::string trimmed = trim(line);
    if (trimmed.empty()) {
        return;
    }
    auto tokens = simple_tokenize(trimmed);
    for (const auto& tok : tokens) {
        dispatch_token(tok);
    }
}

void WoflangInterpreter::exec_script(const std::filesystem::path& filename) {
    std::ifstream in(filename);
    if (!in) {
        error("failed to open script: " + filename.string());
    }
    std::string line;
    while (std::getline(in, line)) {
        exec_line(line);
    }
}

void WoflangInterpreter::repl() {
    std::cout << "Woflang REPL. Ctrl+D or Ctrl+Z to exit." << std::endl;
    std::string line;
    while (true) {
        std::cout << "wofl> " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }
        try {
            exec_line(line);
        } catch (const std::exception& ex) {
            std::cerr << "Error: " << ex.what() << std::endl;
        }
    }
}

WofValue WoflangInterpreter::pop() {
    if (stack.empty()) {
        error("stack underflow");
    }
    WofValue v = stack.back();
    stack.pop_back();
    return v;
}

std::int64_t WoflangInterpreter::pop_int() {
    WofValue v = pop();
    if (v.type == WofType::Integer) {
        return std::get<std::int64_t>(v.value);
    }
    if (v.type == WofType::Double) {
        return static_cast<std::int64_t>(std::llround(std::get<double>(v.value)));
    }
    error("pop_int: value is not numeric");
}

double WoflangInterpreter::pop_double() {
    WofValue v = pop();
    if (v.type == WofType::Double) {
        return std::get<double>(v.value);
    }
    if (v.type == WofType::Integer) {
        return static_cast<double>(std::get<std::int64_t>(v.value));
    }
    error("pop_double: value is not numeric");
}

double WoflangInterpreter::pop_numeric() {
    WofValue v = pop();
    if (v.type == WofType::Double) {
        return std::get<double>(v.value);
    }
    if (v.type == WofType::Integer) {
        return static_cast<double>(std::get<std::int64_t>(v.value));
    }
    error("pop_numeric: value is not numeric");
}

std::string WoflangInterpreter::pop_string() {
    WofValue v = pop();
    if (v.type == WofType::String || v.type == WofType::Symbol) {
        return std::get<std::string>(v.value);
    }
    error("pop_string: value is not a string");
}

std::string WoflangInterpreter::pop_symbol() {
    WofValue v = pop();
    if (v.type == WofType::Symbol) {
        return std::get<std::string>(v.value);
    }
    error("pop_symbol: value is not a symbol");
}

bool WoflangInterpreter::pop_bool() {
    WofValue v = pop();
    if (v.type == WofType::Integer) {
        return std::get<std::int64_t>(v.value) != 0;
    }
    if (v.type == WofType::Double) {
        return std::get<double>(v.value) != 0.0;
    }
    if (v.type == WofType::String || v.type == WofType::Symbol) {
        const auto& s = std::get<std::string>(v.value);
        return !s.empty() && s != "0" && s != "false" && s != "False";
    }
    return false;
}

void WoflangInterpreter::print_stack() const {
    std::cout << "Stack [" << stack.size() << "]" << std::endl;
    for (std::size_t i = 0; i < stack.size(); ++i) {
        const auto& v = stack[i];
        std::cout << "  [" << i << "] " << v.to_string() << std::endl;
    }
}

void WoflangInterpreter::clear_stack() {
    stack.clear();
}

[[noreturn]] void WoflangInterpreter::error(const std::string& msg) {
    throw std::runtime_error(msg);
}

} // namespace woflang
