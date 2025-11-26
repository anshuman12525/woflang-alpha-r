// ==================================================
// trig_ops.cpp - Trigonometric operations for Woflang (v10.1.1 API)
// ==================================================

#ifndef WOFLANG_PLUGIN_EXPORT
#  ifdef _WIN32
#    define WOFLANG_PLUGIN_EXPORT __declspec(dllexport)
#  else
#    define WOFLANG_PLUGIN_EXPORT __attribute__((visibility("default")))
#  endif
#endif

#include "../../src/core/woflang.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

using woflang::WofValue;
using woflang::WoflangInterpreter;

// Bring the stack adapter template into easy reach
template <typename Stack>
using WofStackAdapter = woflang::WofStackAdapter<Stack>;

namespace {

// Pop a numeric from the stack, using the canonical WofValue API
template <typename Stack>
double pop_numeric(WofStackAdapter<Stack>& S, const char* ctx) {
    if (S.empty()) {
        throw std::runtime_error(
            std::string("pop_numeric: stack underflow in ") + ctx
        );
    }
    const WofValue& v = S.top();
    // This method name is inferred from runtime error messages:
    // "WofValue::as_numeric: value is not numeric"
    double d = v.as_numeric(ctx);
    S.pop();
    return d;
}

template <typename Stack>
void push_double(WofStackAdapter<Stack>& S, double x) {
    S.push(WofValue::make_double(x));
}

// Generic unary trig wrapper: pop one numeric, apply f, push result
template <typename F, typename Stack>
void apply_unary_trig(WofStackAdapter<Stack>& S, const char* name, F f) {
    double x = pop_numeric(S, name);
    push_double(S, f(x));
}

// atan2 helper: pop y then x, compute atan2(y, x)
template <typename Stack>
void apply_atan2(WofStackAdapter<Stack>& S) {
    double y = pop_numeric(S, "atan2.y");
    double x = pop_numeric(S, "atan2.x");
    push_double(S, std::atan2(y, x));
}

} // namespace

extern "C" WOFLANG_PLUGIN_EXPORT void register_plugin(WoflangInterpreter& interp) {

    // Constants: pi and e
    interp.register_op("pi", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        push_double(S, 3.141592653589793238462643383279502884L);
    });

    interp.register_op("e", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        push_double(S, 2.718281828459045235360287471352662497L);
    });

    // Basic trig
    interp.register_op("sin", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "sin",
                         static_cast<double(*)(double)>(std::sin));
    });

    interp.register_op("cos", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "cos",
                         static_cast<double(*)(double)>(std::cos));
    });

    interp.register_op("tan", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "tan",
                         static_cast<double(*)(double)>(std::tan));
    });

    // Inverse trig
    interp.register_op("asin", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "asin",
                         static_cast<double(*)(double)>(std::asin));
    });

    interp.register_op("acos", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "acos",
                         static_cast<double(*)(double)>(std::acos));
    });

    interp.register_op("atan", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "atan",
                         static_cast<double(*)(double)>(std::atan));
    });

    // atan2: expects y then x on stack (standard math convention atan2(y, x))
    interp.register_op("atan2", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_atan2(S);
    });

    // Hyperbolic trig
    interp.register_op("sinh", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "sinh",
                         static_cast<double(*)(double)>(std::sinh));
    });

    interp.register_op("cosh", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "cosh",
                         static_cast<double(*)(double)>(std::cosh));
    });

    interp.register_op("tanh", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        apply_unary_trig(S, "tanh",
                         static_cast<double(*)(double)>(std::tanh));
    });

    // Optional: degrees/radians helpers if you ever use them
    interp.register_op("deg->rad", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        double d = pop_numeric(S, "deg->rad");
        push_double(S, d * (3.141592653589793238462643383279502884L / 180.0));
    });

    interp.register_op("rad->deg", [](WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
        double r = pop_numeric(S, "rad->deg");
        push_double(S, r * (180.0 / 3.141592653589793238462643383279502884L));
    });
}
