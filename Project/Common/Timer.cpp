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

#include "Timer.h"

namespace ApiWithoutSecrets {

  float TimerData::GetTime() const {
    return FloatTime;
  }

  float TimerData::GetDeltaTime() const {
    return FloatDeltaTime;
  }

  float TimerData::GetAverageDeltaTime() const {
    return AverageDeltaTime;
  }

  std::array<float, 10> const & TimerData::GetDeltaTimeHistogram() const {
    return DeltaTimeHistogram;
  }

  float TimerData::GetAverageFPS() const {
    return AverageFPS;
  }

  std::array<float, 10> const & TimerData::GetFPSHistogram() const {
    return FPSHistogram;
  }

  void TimerData::Update() {
    {
      auto previous_time = Time;
      Time = std::chrono::high_resolution_clock::now();
      DeltaTime = std::chrono::high_resolution_clock::now() - previous_time;
    }
    {
      auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(Time.time_since_epoch()).count();
      FloatTime = static_cast<float>(milliseconds * 0.001f);
      FloatDeltaTime = DeltaTime.count();
    }
    {
      static size_t previous_second = 0;
      size_t current_second = static_cast<size_t>(FloatTime) % (2);

      if( current_second != previous_second ) {
        AverageFPS = CurrentSecondFPS;
        for( size_t i = 1; i < FPSHistogram.size(); ++i ) {
          AverageFPS += FPSHistogram[i];
          FPSHistogram[i - 1] = FPSHistogram[i];
          DeltaTimeHistogram[i - 1] = DeltaTimeHistogram[i];
        }
        FPSHistogram[9] = CurrentSecondFPS;
        DeltaTimeHistogram[9] = 1000.0f / CurrentSecondFPS;
        AverageFPS *= 0.1f;
        AverageDeltaTime = 1000.0f / AverageFPS;
        CurrentSecondFPS = 0.0f;
      }
      CurrentSecondFPS += 1.0f;
      previous_second = current_second;
    }
  }

  TimerData::TimerData() :
    Time( std::chrono::high_resolution_clock::now() ),
    DeltaTime( std::chrono::high_resolution_clock::now() - std::chrono::high_resolution_clock::now() ),
    FloatDeltaTime( 10.0f ),
    AverageDeltaTime( 10.0f ),
    AverageFPS( 10.0f ),
    CurrentSecondFPS( 10.0f ) {

    for( size_t i = 0; i < FPSHistogram.size(); ++i ) {
      FPSHistogram[i] = 10.0f;
      DeltaTimeHistogram[i] = 10.0f;
    }

    Update();
  }

  TimerData::~TimerData() {
  }

} // namespace ApiWithoutSecrets
