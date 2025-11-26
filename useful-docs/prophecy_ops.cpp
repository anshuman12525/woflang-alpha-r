// ==================================================
// prophecy_ops.cpp - Mystical Prophecy Operations
// ==================================================

#ifndef WOFLANG_PLUGIN_EXPORT
# ifdef _WIN32
#  define WOFLANG_PLUGIN_EXPORT __declspec(dllexport)
# else
#  define WOFLANG_PLUGIN_EXPORT __attribute__((visibility("default")))
# endif
#endif

#include "../../src/core/woflang.hpp"
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <iostream>
#include <cmath>

using woflang::WofValue;
using woflang::WoflangInterpreter;

extern "C" WOFLANG_PLUGIN_EXPORT void 
namespace {
    template<typename Container>
    struct WofStackAdapter {
        Container& v;
        auto size() const { return v.size(); }
        woflang::WofValue& top() { return v.back(); }
        const woflang::WofValue& top() const { return v.back(); }
        void pop() { v.pop_back(); }
        void push(const woflang::WofValue& x) { v.push_back(x); }
    };
}

extern "C" WOFLANG_PLUGIN_EXPORT void register_plugin(woflang::WoflangInterpreter& interp) {
    using namespace woflang;
    interp.register_op("prophecy", [](woflang::WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
                static const std::vector<std::string> prophecies = {
                    "The stack shall overflow with wisdom.",
                    "A great recursion approaches.",
                    "Beware the null pointer of destiny.",
                    "The garbage collector comes for us all.",
                    "In the end, all returns to void.",
                    "The algorithm of fate is O(∞).",
                    "Your code compiles, but at what cost?",
                    "The segfault was within you all along.",
                    "Stack and heap, forever in balance.",
                    "The undefined behavior defines us."
                };
                
                static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
                std::uniform_int_distribution<> dis(0, prophecies.size() - 1);
                
                std::cout << "\n🔮 The Oracle speaks:\n";
                std::cout << "  \"" << prophecies[dis(gen)] << "\"\n\n";
                
                WofValue val;
                val.d = 42.0;
                S.push(val);
            
    });
    interp.register_op("oracle", [](woflang::WoflangInterpreter& ip) {
        WofStackAdapter<decltype(ip.stack)> S{ip.stack};
                if (S.empty()) {
                    std::cout << "The Oracle requires an offering.\n";
                    return;
                }
                
                auto offering = S.top(); S.pop();
                std::cout << "The Oracle contemplates your offering of "
                          << offering.d << "...\n";
                
                double divination = offering.d;
                divination = std::sin(divination) * std::cos(divination * 3.14159);
                
                std::cout << "The Oracle reveals: " << divination << "\n";
                
                WofValue result;
                result.d = divination;
                S.push(result);
            
    });
}
