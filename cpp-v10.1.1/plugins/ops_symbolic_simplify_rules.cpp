// ==================================================
// ops_symbolic_simplify_rules.cpp - Symbolic Simplification Rules (v10.1.1)
// ==================================================

#ifndef WOFLANG_PLUGIN_EXPORT
# ifdef _WIN32
#  define WOFLANG_PLUGIN_EXPORT __declspec(dllexport)
# else
#  define WOFLANG_PLUGIN_EXPORT __attribute__((visibility("default")))
# endif
#endif

#include "../../src/core/woflang.hpp"

#include <iostream>
#include <stdexcept>

using woflang::WoflangInterpreter;
using woflang::WofValue;
using woflang::WofType;

class SymbolicSimplifyRulesPlugin : public woflang::WoflangPlugin {
public:
    void register_ops(WoflangInterpreter& interp) override {
        // Rule: X + X -> 2 * X
        interp.register_op("simplify_sum", [](WoflangInterpreter& interp) {
            if (interp.stack.size() < 2) {
                std::cout << "[simplify_sum] needs at least 2 values\n";
                return;
            }

            auto second = interp.stack.back();
            interp.stack.pop_back();
            auto first = interp.stack.back();
            interp.stack.pop_back();

            // Check if both are same symbol type
            if (first.type == WofType::Symbol && second.type == WofType::Symbol) {
                // If they're the same, replace with 2 * X
                std::string first_sym = std::get<std::string>(first.value);
                std::string second_sym = std::get<std::string>(second.value);

                if (first_sym == second_sym) {
                    std::cout << "[simplify_sum] " << first_sym << " + " << second_sym
                              << " => 2 * " << first_sym << "\n";
                    interp.stack.push_back(WofValue::make_int(2));
                    interp.stack.push_back(first);
                    // (caller would apply * operator)
                } else {
                    interp.stack.push_back(first);
                    interp.stack.push_back(second);
                }
            } else {
                interp.stack.push_back(first);
                interp.stack.push_back(second);
            }
        });

        // Rule: X * 1 -> X
        interp.register_op("simplify_mul_one", [](WoflangInterpreter& interp) {
            if (interp.stack.size() < 2) {
                std::cout << "[simplify_mul_one] needs at least 2 values\n";
                return;
            }

            auto multiplier = interp.stack.back();
            interp.stack.pop_back();
            auto value = interp.stack.back();
            interp.stack.pop_back();

            // Check if multiplier is 1
            if (multiplier.type == WofType::Integer &&
                std::get<int64_t>(multiplier.value) == 1) {
                std::cout << "[simplify_mul_one] X * 1 => X\n";
                interp.stack.push_back(value);
            } else {
                interp.stack.push_back(value);
                interp.stack.push_back(multiplier);
            }
        });

        // Rule: X + 0 -> X
        interp.register_op("simplify_add_zero", [](WoflangInterpreter& interp) {
            if (interp.stack.size() < 2) {
                std::cout << "[simplify_add_zero] needs at least 2 values\n";
                return;
            }

            auto addend = interp.stack.back();
            interp.stack.pop_back();
            auto value = interp.stack.back();
            interp.stack.pop_back();

            // Check if addend is 0
            if (addend.type == WofType::Integer &&
                std::get<int64_t>(addend.value) == 0) {
                std::cout << "[simplify_add_zero] X + 0 => X\n";
                interp.stack.push_back(value);
            } else {
                interp.stack.push_back(value);
                interp.stack.push_back(addend);
            }
        });

        // Rule: X * 0 -> 0
        interp.register_op("simplify_mul_zero", [](WoflangInterpreter& interp) {
            if (interp.stack.size() < 2) {
                std::cout << "[simplify_mul_zero] needs at least 2 values\n";
                return;
            }

            auto multiplier = interp.stack.back();
            interp.stack.pop_back();
            auto value = interp.stack.back();
            interp.stack.pop_back();

            // Check if multiplier is 0
            if (multiplier.type == WofType::Integer &&
                std::get<int64_t>(multiplier.value) == 0) {
                std::cout << "[simplify_mul_zero] X * 0 => 0\n";
                interp.stack.push_back(WofValue::make_int(0));
            } else {
                interp.stack.push_back(value);
                interp.stack.push_back(multiplier);
            }
        });

        std::cout << "[simplify_rules] Plugin loaded.\n";
    }
};

extern "C" WOFLANG_PLUGIN_EXPORT void
register_plugin(WoflangInterpreter& interp) {
    static SymbolicSimplifyRulesPlugin plugin;
    plugin.register_ops(interp);
}
