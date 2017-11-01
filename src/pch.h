#pragma once

#include <tchar.h>
#include <windows.h>

#include "../deps/Detours/src/detours.h"
#include "../deps/mhcode/src/mhcode.h"
#include "../deps/json-parser/json.h"

#include <memory>
#include <functional>
#include <string>
#include <list>
#include <set>
#include <vector>
#include <mutex>
typedef std::string String;
typedef std::wstring WString;
typedef std::string Buffer;
typedef std::function<void(const Buffer&)> MessageHandler;






