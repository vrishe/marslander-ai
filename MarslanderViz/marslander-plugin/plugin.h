#pragma once

#ifndef PLUGIN_H_
#define PLUGIN_H_

#include <string>

#ifdef MARSLANDERPLUGIN_EXPORTS
#define DLL_API __declspec( dllexport )
#else
#define DLL_API __declspec( dllimport )
#endif // MARSLANDERPLUGIN_EXPORTS

#ifdef __cplusplus
extern "C" {
#endif

    DLL_API LPVOID BeginSimulation(LPCSTR);

    DLL_API VOID EndSimulation(LPVOID*);

    DLL_API marslander::game_turn_input GetTurnInput(LPVOID);

    DLL_API marslander::outcome StepSimulation(LPVOID, const marslander::game_turn_output &);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLUGIN_H_