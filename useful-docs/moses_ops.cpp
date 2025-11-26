// ==================================================
// moses_ops.cpp - A Mystical Riddle Plugin
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
#include <random>
#include <chrono>
#include <thread>
#include <clocale>

#if defined(_MSC_VER)
# include <windows.h>
#endif

using woflang::WoflangInterpreter;

extern "C" WOFLANG_PLUGIN_EXPORT void register_plugin(WoflangInterpreter& interp) {

namespace woflang {

static bool hebrew_mode_active = false;

void setup_utf8_console() {
#if defined(_MSC_VER)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, "en_US.UTF-8");
}

} // namespace woflang

WOFLANG_PLUGIN_EXPORT void init_plugin(woflang::WoflangInterpreter::OpTable* op_table) {
    using namespace woflang;
    
    (*op_table)["那"] = [](std::stack<WofValue>& S) {
        static bool first_run = true;
        if (first_run) {
            setup_utf8_console();
            first_run = false;
        }
        
        static std::mt19937 gen(std::chrono::steady_clock::now().time_since_epoch().count());
        std::uniform_int_distribution<> dis(1, 100);
        
        if (!hebrew_mode_active && dis(gen) == 1) {
            hebrew_mode_active = true;
            std::cout << "\n那... How does Moses make his tea?\n";
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::seconds(3));
            
            std::cout << "\n...העולם השתנה\n";
            std::cout << "(The world has changed... type 'answer' to respond)\n";
        } else if (hebrew_mode_active) {
            std::cout << "אם אין אני לי, מי לי? וכשאני לעצמי, מה אני? ואם לא עכשיו, אימתי?" << std::endl;
            std::cout << "(If I am not for myself, who will be for me? And when I am for myself, what am 'I'? And if not now, when?)\n";
        } else {
            std::cout << "The tablets are yet unbroken.\n";
        }
    };
    
    (*op_table)["answer"] = [](std::stack<WofValue>& S) {
        if (hebrew_mode_active) {
            std::cout << "\nHe brews it.\n";
            std::cout << "הוא מכין תה... (He brews it.)\n\n";
        } else {
            std::cout << "There is no riddle to answer.\n";
        }
    };
    
    (*op_table)["reset"] = [](std::stack<WofValue>& S) {
        if (hebrew_mode_active) {
            hebrew_mode_active = false;
            std::cout << "The world returns to its former shape.\n";
        } else {
            std::cout << "Everything is already as it should be.\n";
        }
    };
}
}