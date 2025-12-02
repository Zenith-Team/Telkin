#pragma once

#include "PrivateInterface.h"
#include "Debug.h"
#include "HookApplicators.h"

#include <utility>

namespace tk {

    template <typename T>
    class MyVector {
    private:
        static constexpr u32 cGrowthFactor = 2;

    public:
        MyVector()
            : mBuffer(nullptr)
            , mCount(0)
            , mCapacity(0)
        { }
        
        ~MyVector() {
            if (mBuffer != nullptr)
                MEMFreeToDefaultHeap(mBuffer);
        }
        
        MyVector(const MyVector&) = delete;
        MyVector& operator=(const MyVector&) = delete;

        void add(T entry) {
            mCount++;
            
            if (mBuffer == nullptr) {
                mCapacity = mCount * cGrowthFactor;
                mBuffer = (T*)MEMAllocFromDefaultHeap(mCapacity * sizeof(T));
                
                if (mBuffer == nullptr) {
                    LOG("Sorry, out of memory. (A)");
                }
            } else if (mCount > mCapacity) {
                    mCapacity = mCapacity * cGrowthFactor;
                    
                    T* newBuffer = (T*)MEMAllocFromDefaultHeap(mCapacity * sizeof(T));
                    if (newBuffer == nullptr) {
                        LOG("Sorry, out of memory. (B)");
                    }
                    
                    OSBlockMove(newBuffer, mBuffer, mCount * sizeof(T), false);
                    
                    MEMFreeToDefaultHeap(mBuffer);
                    mBuffer = newBuffer;
            }
            
            mBuffer[mCount - 1] = entry;
        }
        
        [[nodiscard]] T* data() { return mBuffer; }
        [[nodiscard]] s32 count() const { return mCount; }
        
    private:
        T* mBuffer;    // Storage
        s32 mCount;    // Full slots
        s32 mCapacity; // Total slots we have
    };

    class HookList {
    public:
        void add(const void* hook, u32 startAddr, u32 endAddr) {
            const GenericHook* h = reinterpret_cast<const GenericHook*>(hook);
            
            const HookEntry entry = {
                .hook = h,
                .startAddr = startAddr,
                .endAddr = endAddr
            };
            
            mVector.add(entry);
        }
        
        bool validateRanges() {
            if (mVector.count() <= 1)
                return true;
            
            this->sort();
            
            for (s32 i = 1; i < mVector.count(); i++) {
                if (mVector.data()[i].startAddr < mVector.data()[i - 1].endAddr) {
                    // TODO: Better diagnostic here with mod blame and addrs/types
                    LOG("MOD INCOMPATIBILITY: Overlapping hooks found!");
                    return false;
                }
            }
            
            LOG("No hook conflicts found :)");
            return true;
        }
        
        bool applyAll() {
            for (s32 i = 0; i < mVector.count(); i++) {
                const HookEntry& entry = mVector.data()[i];
                const GenericHook* hook = entry.hook;
                
                switch (hook->magic) {
                    case tk::DataMagic::BranchHook: {
                        if (!tk::applyBranchHook(reinterpret_cast<const tk::BranchHook*>(hook)))
                            return false;
                        
                        break;
                    }
                    
                    case tk::DataMagic::PatchHook: {
                        if (!tk::applyPatchHook(reinterpret_cast<const tk::PatchHook*>(hook)))
                            return false;
                        
                        break;
                    }
                    
                    case tk::DataMagic::PointerHook: {
                        if (!tk::applyPointerHook(reinterpret_cast<const tk::PointerHook*>(hook)))
                            return false;
                        
                        break;
                    }
                }
            }
            
            return true;
        }
        
    private:
        void sort() {
            // TODO: This is bubble sort, optimize it later <3
            
            s32 n = mVector.count();
            do {
                s32 newN = 0;
                for (int i = 1; i <= (n-1); i++) {
                    if (mVector.data()[i-1].startAddr > mVector.data()[i].startAddr) {
                        std::swap(mVector.data()[i-1], mVector.data()[i]);
                        newN = i;
                    }
                }
                
                n = newN;
            } while (n > 1);
        }
    
    private:
        struct HookEntry {
            const GenericHook* hook;
            u32 startAddr;
            u32 endAddr;
        };
        
        MyVector<HookEntry> mVector;
    };
    
    class FunctionList {
    public:
        void add(tk::startfunc_t func) {
            mFuncs.add(func);
        }
        
        void callAll(u32 acquireAddr, u32 exportAddr) {
            for (s32 i = 0; i < mFuncs.count(); i++)
                mFuncs.data()[i](acquireAddr, exportAddr);
        }
        
    private:
        MyVector<tk::startfunc_t> mFuncs;
    };

} // namespace tk
