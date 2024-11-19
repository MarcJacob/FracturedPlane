// Subsystem.h
// Declarations for common types used by Server Subsystems.

#pragma once

template<typename FuncPtrType, int Size>
struct CallbackTable
{
    // Functions to be called back.
    FuncPtrType CallbackFunctions[Size];
    // Arbitrary data accompanying any registration. Usually pointing to a data structure with needed contextual data.
    void* RegistrationContexts[Size];
    // Number of registrations. When at the maximum size, further Registration calls will fail.
    size_t CallbackFunctionCount = 0;
    
    bool RegisterCallback(FuncPtrType Func, void* Context = nullptr)
    {
        if (CallbackFunctionCount >= Size)
        {
            return false;
        }

        RegistrationContexts[CallbackFunctionCount] = Context;
        CallbackFunctions[CallbackFunctionCount++] = Func;
        return true;
    }

    template<typename... Args>
    void TriggerCallbacks(Args... FuncPtrArgs)
    {
        for(int CallbackIndex = 0; CallbackIndex < CallbackFunctionCount; ++CallbackIndex)
        {
            CallbackFunctions[CallbackIndex](FuncPtrArgs..., RegistrationContexts[CallbackIndex]);
        }
    }
};