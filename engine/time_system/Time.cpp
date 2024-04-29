#include "Time.hpp"

#include "../bedrock/BedrockAssert.hpp"

#include <SDL2/SDL_timer.h>

namespace MFA
{
    
    //==========================================================

    std::unique_ptr<Time> Time::Instantiate(int maxFramerate)
    {
        return std::make_unique<Time>(maxFramerate);
    }

    //==========================================================

    Time::Time(int maxFramerate)
    {
        MFA_ASSERT(Instance == nullptr);
        Instance = this;
        _startTimeMs = SDL_GetTicks();
        _nowMs = _startTimeMs;
        _minDeltaTimeMs = static_cast<int>(1000.0f / static_cast<float>(maxFramerate));
        _deltaTimeSec = static_cast<float>(_minDeltaTimeMs) / 1000.0f;
    }
    
    //==========================================================
    
    Time::~Time()
    {
        MFA_ASSERT(Instance != nullptr);
        Instance = nullptr;
    }

    //==========================================================
    
    void Time::Update()
    {
        _deltaTimeMs = SDL_GetTicks() - _nowMs;
        if(_minDeltaTimeMs > _deltaTimeMs)
        {
            SDL_Delay(_minDeltaTimeMs - _deltaTimeMs);
        }

        _deltaTimeMs = SDL_GetTicks() - _nowMs;
		_deltaTimeSec = static_cast<float>(_deltaTimeMs) / 1000.0f;
        _timeSec += _deltaTimeSec;

        _nowMs = SDL_GetTicks();
    }

    //==========================================================

    int Time::DeltaTimeMs()
    {
        return Instance->_deltaTimeSec;
    }

    //==========================================================

    float Time::DeltaTimeSec()
    {
        return Instance->_deltaTimeSec;
    }

    //==========================================================
    
    float Time::NowSec()
    {
        return Instance->_timeSec;
    }
    
    //==========================================================
    
}