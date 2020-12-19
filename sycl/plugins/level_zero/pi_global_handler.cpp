//==--------- global_handler.cpp --- Global objects handler ----------------==//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "pi_global_handler.hpp"
#include "pi_level_zero.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <vector>

GlobalHandler::GlobalHandler() = default;
GlobalHandler::~GlobalHandler() = default;

GlobalHandler &GlobalHandler::instance() {
  static GlobalHandler *GlobalObjectsHandler = new GlobalHandler();
  return *GlobalObjectsHandler;
}

std::vector<_pi_platform*>& GlobalHandler::getPiPlatformsCache() {
  if (MPiPlatformsCache)
    return *MPiPlatformsCache;

  const std::lock_guard<SpinLock> Lock{MFieldsLock};
  if (!MPiPlatformsCache)
    MPiPlatformsCache = std::make_unique<std::vector<_pi_platform*>>();

  return *MPiPlatformsCache;
}

std::mutex& GlobalHandler::getPiPlatformsCacheMutex() {
  if (MPiPlatformsCacheMutex)
    return *MPiPlatformsCacheMutex;

  const std::lock_guard<SpinLock> Lock{MFieldsLock};
  if (!MPiPlatformsCacheMutex)
    MPiPlatformsCacheMutex = std::make_unique<std::mutex>();

  return *MPiPlatformsCacheMutex;
}

void shutdown() {
  std::vector<_pi_platform*>& PiPlatformsCache = GlobalHandler::instance().getPiPlatformsCache();
  //PiPlatformsCache.clear();
  while (!PiPlatformsCache.empty()) {
    _pi_platform* Platform = PiPlatformsCache.back();
    //std::cout << " >>> removing a platfrom from cache " << Platform << std::endl;
    delete Platform;
    PiPlatformsCache.pop_back();
  }
  delete &GlobalHandler::instance();
}

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
  // Perform actions based on the reason for calling.
  switch (fdwReason) {
  case DLL_PROCESS_DETACH:
    shutdown();
    break;
  case DLL_PROCESS_ATTACH:
  case DLL_THREAD_ATTACH:
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE; // Successful DLL_PROCESS_ATTACH.
}
#else
// Setting maximum priority on destructor ensures it runs after all other global
// destructors.
__attribute__((destructor(65535))) static void syclUnload() { shutdown(); }
#endif
