/************************************************************************
* [# filter: thirdparty\easyprofiler #]
* file name         : profiler.cpp
* ----------------- :
* creation time     : 2018/05/06
* authors           : Sergey Yagovtsev, Victor Zarubkin
* emails            : yse.sey@gmail.com, v.s.zarubkin@gmail.com
* ----------------- :
* description       : The file contains implementation of Profile manager and implement access c-function
*                   :
* license           : Lightweight profiler library for c++
*                   : Copyright(C) 2016-2018  Sergey Yagovtsev, Victor Zarubkin
*                   :
*                   : Licensed under either of
*                   :     * MIT license (LICENSE.MIT or http://opensource.org/licenses/MIT)
*                   :     * Apache License, Version 2.0, (LICENSE.APACHE or http://www.apache.org/licenses/LICENSE-2.0)
*                   : at your option.
*                   :
*                   : The MIT License
*                   :
*                   : Permission is hereby granted, free of charge, to any person obtaining a copy
*                   : of this software and associated documentation files (the "Software"), to deal
*                   : in the Software without restriction, including without limitation the rights
*                   : to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
*                   : of the Software, and to permit persons to whom the Software is furnished
*                   : to do so, subject to the following conditions:
*                   :
*                   : The above copyright notice and this permission notice shall be included in all
*                   : copies or substantial portions of the Software.
*                   :
*                   : THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
*                   : INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
*                   : PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
*                   : LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*                   : TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
*                   : USE OR OTHER DEALINGS IN THE SOFTWARE.
*                   :
*                   : The Apache License, Version 2.0 (the "License")
*                   :
*                   : You may not use this file except in compliance with the License.
*                   : You may obtain a copy of the License at
*                   :
*                   : http://www.apache.org/licenses/LICENSE-2.0
*                   :
*                   : Unless required by applicable law or agreed to in writing, software
*                   : distributed under the License is distributed on an "AS IS" BASIS,
*                   : WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*                   : See the License for the specific language governing permissions and
*                   : limitations under the License.
************************************************************************/

#include "build.h"
#include "profiler.h"
#include "arbitrary_value.h"
#include "profile_manager.h"
#include "event_trace_win.h"
#include "current_time.h"

//////////////////////////////////////////////////////////////////////////

#define EASY_PROFILER_PRODUCT_VERSION "v" EASY_STRINGIFICATION(EASY_PROFILER_VERSION_MAJOR) "." \
                                          EASY_STRINGIFICATION(EASY_PROFILER_VERSION_MINOR) "." \
                                          EASY_STRINGIFICATION(EASY_PROFILER_VERSION_PATCH)

extern const uint32_t EASY_PROFILER_SIGNATURE = ('E' << 24) | ('a' << 16) | ('s' << 8) | 'y';
extern const uint32_t EASY_PROFILER_VERSION = (static_cast<uint32_t>(EASY_PROFILER_VERSION_MAJOR) << 24) |
                                              (static_cast<uint32_t>(EASY_PROFILER_VERSION_MINOR) << 16) |
                                               static_cast<uint32_t>(EASY_PROFILER_VERSION_PATCH);

#undef EASY_VERSION_INT

//////////////////////////////////////////////////////////////////////////

extern "C" {

uint8_t versionMajor()
{
    static_assert(0 <= EASY_PROFILER_VERSION_MAJOR && EASY_PROFILER_VERSION_MAJOR <= 255,
                  "EASY_PROFILER_VERSION_MAJOR must be defined in range [0, 255]");

    return EASY_PROFILER_VERSION_MAJOR;
}

uint8_t versionMinor()
{
    static_assert(0 <= EASY_PROFILER_VERSION_MINOR && EASY_PROFILER_VERSION_MINOR <= 255,
                  "EASY_PROFILER_VERSION_MINOR must be defined in range [0, 255]");

    return EASY_PROFILER_VERSION_MINOR;
}

uint16_t versionPatch()
{
    static_assert(0 <= EASY_PROFILER_VERSION_PATCH && EASY_PROFILER_VERSION_PATCH <= 65535,
                  "EASY_PROFILER_VERSION_PATCH must be defined in range [0, 65535]");

    return EASY_PROFILER_VERSION_PATCH;
}

uint32_t version()
{
    return EASY_PROFILER_VERSION;
}

const char* versionName()
{
#ifdef EASY_PROFILER_API_DISABLED
    return EASY_PROFILER_PRODUCT_VERSION "_disabled";
#else
    return EASY_PROFILER_PRODUCT_VERSION;
#endif
}

//////////////////////////////////////////////////////////////////////////

#if !defined(EASY_PROFILER_API_DISABLED)

profiler::timestamp_t now()
{
    return profiler::clock::now();
}

profiler::timestamp_t toNanoseconds(profiler::timestamp_t _ticks)
{
    return ProfileManager::instance().ticks2ns(_ticks);
}

profiler::timestamp_t toMicroseconds(profiler::timestamp_t _ticks)
{
    return ProfileManager::instance().ticks2us(_ticks);
}

const profiler::BaseBlockDescriptor*
registerDescription(profiler::EasyBlockStatus _status, const char* _autogenUniqueId, const char* _name,
                    const char* _filename, int _line, profiler::block_type_t _block_type, profiler::color_t _color,
                    bool _copyName)
{
    return ProfileManager::instance().addBlockDescriptor(_status, _autogenUniqueId, _name, _filename, _line,
                                                         _block_type, _color, _copyName);
}

void endBlock()
{
    ProfileManager::instance().endBlock();
}

void setEnabled(bool isEnable)
{
    ProfileManager::instance().setEnabled(isEnable);
}

bool isEnabled()
{
    return ProfileManager::instance().isEnabled();
}

void storeValue(const profiler::BaseBlockDescriptor* _desc, profiler::DataType _type, const void* _data,
                             uint16_t _size, bool _isArray, profiler::ValueId _vin)
{
    ProfileManager::instance().storeValue(_desc, _type, _data, _size, _isArray, _vin);
}

void storeEvent(const profiler::BaseBlockDescriptor* _desc, const char* _runtimeName)
{
    ProfileManager::instance().storeBlock(_desc, _runtimeName);
}

void storeBlock(const profiler::BaseBlockDescriptor* _desc, const char* _runtimeName,
                             profiler::timestamp_t _beginTime, profiler::timestamp_t _endTime)
{
    ProfileManager::instance().storeBlock(_desc, _runtimeName, _beginTime, _endTime);
}

void beginBlock(profiler::Block& _block)
{
    ProfileManager::instance().beginBlock(_block);
}

void beginNonScopedBlock(const profiler::BaseBlockDescriptor* _desc, const char* _runtimeName)
{
    ProfileManager::instance().beginNonScopedBlock(_desc, _runtimeName);
}

uint32_t dumpBlocksToFile(const char* filename)
{
    return ProfileManager::instance().dumpBlocksToFile(filename);
}

const char* registerThreadScoped(const char* name, profiler::ThreadGuard& threadGuard)
{
    return ProfileManager::instance().registerThread(name, threadGuard);
}

const char* registerThread(const char* name)
{
    return ProfileManager::instance().registerThread(name);
}

void setEventTracingEnabled(bool _isEnable)
{
    ProfileManager::instance().setEventTracingEnabled(_isEnable);
}

bool isEventTracingEnabled()
{
    return ProfileManager::instance().isEventTracingEnabled();
}

# ifdef _WIN32
void setLowPriorityEventTracing(bool _isLowPriority)
{
    EasyEventTracer::instance().setLowPriority(_isLowPriority);
}

bool isLowPriorityEventTracing()
{
    return EasyEventTracer::instance().isLowPriority();
}
# else
void setLowPriorityEventTracing(bool) { }
bool isLowPriorityEventTracing() { return false; }
# endif

void setContextSwitchLogFilename(const char* name)
{
    return ProfileManager::instance().setContextSwitchLogFilename(name);
}

const char* getContextSwitchLogFilename()
{
    return ProfileManager::instance().getContextSwitchLogFilename();
}

void startListen(uint16_t _port)
{
    return ProfileManager::instance().startListen(_port);
}

void stopListen()
{
    return ProfileManager::instance().stopListen();
}

bool isListening()
{
    return ProfileManager::instance().isListening();
}

bool isMainThread()
{
    return ProfileManager::isMainThread();
}

profiler::timestamp_t this_thread_frameTime(profiler::Duration _durationCast)
{
    return ProfileManager::this_thread_frameTime(_durationCast);
}

profiler::timestamp_t this_thread_frameTimeLocalMax(profiler::Duration _durationCast)
{
    return ProfileManager::this_thread_frameTimeLocalMax(_durationCast);
}

profiler::timestamp_t this_thread_frameTimeLocalAvg(profiler::Duration _durationCast)
{
    return ProfileManager::this_thread_frameTimeLocalAvg(_durationCast);
}

profiler::timestamp_t main_thread_frameTime(profiler::Duration _durationCast)
{
    return ProfileManager::main_thread_frameTime(_durationCast);
}

profiler::timestamp_t main_thread_frameTimeLocalMax(profiler::Duration _durationCast)
{
    return ProfileManager::main_thread_frameTimeLocalMax(_durationCast);
}

profiler::timestamp_t main_thread_frameTimeLocalAvg(profiler::Duration _durationCast)
{
    return ProfileManager::main_thread_frameTimeLocalAvg(_durationCast);
}

#else // EASY_PROFILER_API_DISABLED

profiler::timestamp_t now() { return 0; }
profiler::timestamp_t toNanoseconds(profiler::timestamp_t) { return 0; }
profiler::timestamp_t toMicroseconds(profiler::timestamp_t) { return 0; }

const profiler::BaseBlockDescriptor* registerDescription(profiler::EasyBlockStatus, const char*,
                                                                      const char*, const char*, int,
                                                                      profiler::block_type_t, profiler::color_t, bool)
{
    return reinterpret_cast<const BaseBlockDescriptor*>(0xbad);
}

void endBlock() { }
void setEnabled(bool) { }
bool isEnabled() { return false; }

void storeValue(const profiler::BaseBlockDescriptor*, profiler::DataType, const void*, uint16_t, bool,
                             profiler::ValueId)
{
}

void storeEvent(const profiler::BaseBlockDescriptor*, const char*) { }
void storeBlock(const profiler::BaseBlockDescriptor*, const char*, profiler::timestamp_t,
                             profiler::timestamp_t)
{
}

void beginBlock(profiler::Block&) { }
void beginNonScopedBlock(const profiler::BaseBlockDescriptor*, const char*) { }
uint32_t dumpBlocksToFile(const char*) { return 0; }
const char* registerThreadScoped(const char*, profiler::ThreadGuard&) { return ""; }
const char* registerThread(const char*) { return ""; }
void setEventTracingEnabled(bool) { }
bool isEventTracingEnabled() { return false; }
void setLowPriorityEventTracing(bool) { }
bool isLowPriorityEventTracing(bool) { return false; }
void setContextSwitchLogFilename(const char*) { }
const char* getContextSwitchLogFilename() { return ""; }
void startListen(uint16_t) { }
void stopListen() { }
bool isListening() { return false; }

bool isMainThread() { return false; }
profiler::timestamp_t this_thread_frameTime(profiler::Duration) { return 0; }
profiler::timestamp_t this_thread_frameTimeLocalMax(profiler::Duration) { return 0; }
profiler::timestamp_t this_thread_frameTimeLocalAvg(profiler::Duration) { return 0; }
profiler::timestamp_t main_thread_frameTime(profiler::Duration) { return 0; }
profiler::timestamp_t main_thread_frameTimeLocalMax(profiler::Duration) { return 0; }
profiler::timestamp_t main_thread_frameTimeLocalAvg(profiler::Duration) { return 0; }

#endif // EASY_PROFILER_API_DISABLED

} // end extern "C".
