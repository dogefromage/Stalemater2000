#pragma once
#include <iostream>

extern bool e_debug;
#define LOG(x) if (e_debug) { std::cout << x << std::endl; }

