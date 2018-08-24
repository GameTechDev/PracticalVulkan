////////////////////////////////////////////////////////////////////////////////
// Copyright 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
////////////////////////////////////////////////////////////////////////////////

#if !defined(TIMER_HEADER)
#define TIMER_HEADER

#include <chrono>
#include <array>

namespace ApiWithoutSecrets {

  // ************************************************************ //
  // TimerData                                                    //
  //                                                              //
  // Class for time and FPS measurements                          //
  // ************************************************************ //
  class TimerData {
  public:
    float                         GetTime() const;
    float                         GetDeltaTime() const;
    float                         GetAverageDeltaTime() const;
    std::array<float, 10> const & GetDeltaTimeHistogram() const;
    float                         GetAverageFPS() const;
    std::array<float, 10> const & GetFPSHistogram() const;

    void    Update();

    TimerData();
    ~TimerData();

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock> Time;
    float                                                       FloatTime;
    std::chrono::duration<float>                                DeltaTime;
    float                                                       FloatDeltaTime;
    float                                                       AverageDeltaTime;
    std::array<float, 10>                                       DeltaTimeHistogram;
    std::array<float, 10>                                       FPSHistogram;
    float                                                       AverageFPS;
    float                                                       CurrentSecondFPS;
  };

} // namespace ApiWithoutSecrets

#endif // TIMER_HEADER