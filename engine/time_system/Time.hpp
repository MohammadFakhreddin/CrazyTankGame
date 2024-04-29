#pragma once

#include <memory>

namespace MFA
{

    class Time
    {
    public:

        static std::unique_ptr<Time> Instantiate(int maxFramerate);

        explicit Time(int maxFramerate);

        ~Time();

        void Update();

        static int DeltaTimeMs();

        static float DeltaTimeSec();

        static float NowSec();

    private:

        static inline Time * Instance = nullptr;
        
        int _startTimeMs{};
        int _nowMs{};
        int _minDeltaTimeMs{};
        
        int _deltaTimeMs {};
        float _deltaTimeSec {};
        float _timeSec {};

    };

}
