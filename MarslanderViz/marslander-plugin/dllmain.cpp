// dllmain.cpp : Defines the entry point for the DLL application.
#include "framework.h"

#include "marslander/marslander.h"
#include "plugin.h"

#include <utility>

LPVOID BeginSimulation(LPCSTR state_base64) {
    auto state_ptr = new marslander::state();
    state_ptr->from_base64(state_base64);
    return state_ptr;
}

VOID EndSimulation(LPVOID* ctx) {
    delete static_cast<marslander::state *>(std::exchange(*ctx, nullptr));
}

marslander::game_turn_input GetTurnInput(LPVOID ctx) {
    return *static_cast<marslander::state *>(ctx);
}

marslander::outcome StepSimulation(LPVOID ctx,
        const marslander::game_turn_output &output) {
    auto& state = *static_cast<marslander::state *>(ctx);
    state.out = output;
    return marslander::simulate(state);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call,
        LPVOID lpReserved) {
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
