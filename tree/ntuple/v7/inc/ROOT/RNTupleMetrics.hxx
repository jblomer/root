/// \file ROOT/RNTupleMetrics.hxx
/// \ingroup NTuple ROOT7
/// \author Jakob Blomer <jblomer@cern.ch>
/// \date 2019-08-27
/// \warning This is part of the ROOT 7 prototype! It will change without notice. It might trigger earthquakes. Feedback
/// is welcome!

/*************************************************************************
 * Copyright (C) 1995-2019, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#ifndef ROOT7_RNTupleMetrics
#define ROOT7_RNTupleMetrics

#include <TError.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime> // for CPU time measurement with clock()
#include <memory>
#include <ostream>
#include <string>
#include <vector>
#include <utility>

namespace ROOT {
namespace Experimental {
namespace Detail {

// clang-format off
/**
\class ROOT::Experimental::Detail::RNTuplePerfCounter
\ingroup NTuple
\brief A performance counter with a name and a unit, which can be activated on demand

Derived classes decide on the counter type and implement printing of the value.
*/
// clang-format on
class RNTuplePerfCounter {
private:
   std::string fName;
   std::string fUnit;
   std::string fDescription;
   bool fIsActive = false;

public:
   RNTuplePerfCounter(const std::string &name, const std::string &unit, const std::string &desc)
      : fName(name), fUnit(unit), fDescription(desc) {}
   void Activate() { fIsActive = true; }
   bool IsActive() const { return fIsActive; }
   std::string GetName() const { return fName; }
   std::string GetDescription() const { return fDescription; }
   std::string GetUnit() const { return fUnit; }

   virtual std::string ToString() const = 0;
};


// clang-format off
/**
\class ROOT::Experimental::Detail::RNTuplePlainCounter
\ingroup NTuple
\brief A non thread-safe integral performance counter
*/
// clang-format on
class RNTuplePlainCounter : public RNTuplePerfCounter {
private:
   std::int64_t fCounter = 0;

public:
   RNTuplePlainCounter(const std::string &name, const std::string &unit, const std::string &desc)
      : RNTuplePerfCounter(name, unit, desc)
   {
      // There is no performance gain in checking the active status
      Activate();
   }

   void Inc() { ++fCounter; }
   void Dec() { --fCounter; }
   void Add(int64_t delta) { fCounter += delta; }
   int64_t GetValue() const { return fCounter; }
   void SetValue(int64_t val) { fCounter = val; }
   std::string ToString() const override { return std::to_string(fCounter); }
};


// clang-format off
/**
\class ROOT::Experimental::Detail::RNTupleAtomicCounter
\ingroup NTuple
\brief A thread-safe integral performance counter
*/
// clang-format on
class RNTupleAtomicCounter : public RNTuplePerfCounter {
private:
   std::atomic<std::int64_t> fCounter = 0;

public:
   RNTupleAtomicCounter(const std::string &name, const std::string &unit, const std::string &desc)
      : RNTuplePerfCounter(name, unit, desc) { }

   void Inc() {
      if (IsActive())
         ++fCounter;
   }
   void Dec() {
      if (IsActive())
         --fCounter;
   }
   void Add(int64_t delta) {
      if (IsActive())
         fCounter += delta;
   }
   int64_t XAdd(int64_t delta) {
      if (IsActive())
         return fCounter.fetch_add(delta);
      return 0;
   }
   int64_t GetValue() const {
      if (IsActive())
         return fCounter.load();
      return 0;
   }
   void SetValue(int64_t val) {
      if (IsActive())
         fCounter.store(val);
   }
   std::string ToString() const override { return std::to_string(GetValue()); }
};


// clang-format off
/**
\class ROOT::Experimental::Detail::RNTupleTickCounter
\ingroup NTuple
\brief An either thread-safe or non thread safe counter for CPU ticks

On print, the value is converted from ticks to ns.
*/
// clang-format on
template <typename BaseCounterT>
class RNTupleTickCounter : public BaseCounterT {
public:
   RNTupleTickCounter(const std::string &name, const std::string &unit, const std::string &desc)
      : BaseCounterT(name, unit, desc)
   {
      R__ASSERT(unit == "ns");
   }

   std::string ToString() const final {
      auto ticks = BaseCounterT::GetValue();
      return std::to_string(std::uint64_t(
         (double(ticks) / double(CLOCKS_PER_SEC)) / (1000. * 1000. * 1000.)));
   }
};


// clang-format off
/**
\class ROOT::Experimental::Detail::RNTupleTimer
\ingroup NTuple
\brief Record wall time and CPU time between construction and destruction

Uses RAII as a stop watch. Only the wall time counter is used to determine whether the timer is active.
*/
// clang-format on
template <typename WallTimeT, typename CpuTimeT>
class RNTupleTimer {
private:
   using Clock_t = std::chrono::steady_clock;

   WallTimeT &fCtrWallTime;
   CpuTimeT &fCtrCpuTicks;
   /// Wall clock time
   Clock_t::time_point fStartTime;
   /// CPU time
   clock_t fStartTicks;

public:
   RNTupleTimer(WallTimeT &ctrWallTime, CpuTimeT &ctrCpuTicks)
      : fCtrWallTime(ctrWallTime), fCtrCpuTicks(ctrCpuTicks)
   {
      if (!fCtrWallTime.IsActive())
         return;
      fStartTime = Clock_t::now();
      fStartTicks = clock();
   }

   ~RNTupleTimer() {
      if (!fCtrWallTime.IsActive())
         return;
      auto wallTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock_t::now() - fStartTime);
      fCtrWallTime.Add(wallTimeNs.count());
      fCtrCpuTicks.Add(clock() - fStartTicks);
   }

   RNTupleTimer(const RNTupleTimer &other) = delete;
   RNTupleTimer &operator =(const RNTupleTimer &other) = delete;
};

using RNTuplePlainTimer = RNTupleTimer<RNTuplePlainCounter, RNTupleTickCounter<RNTuplePlainCounter>>;
using RNTupleAtomicTimer = RNTupleTimer<RNTupleAtomicCounter, RNTupleTickCounter<RNTupleAtomicCounter>>;


// clang-format off
/**
\class ROOT::Experimental::Detail::RNTupleMetrics
\ingroup NTuple
\brief A collection of Counter objects with a name, a unit, and a description.

The class owns the counters; on registration of a new
*/
// clang-format on
class RNTupleMetrics {
private:
   std::vector<std::unique_ptr<RNTuplePerfCounter>> fCounters;
   std::string fName;
   bool fIsActive = false;

   bool Contains(const std::string &name) const;

public:
   explicit RNTupleMetrics(const std::string &name) : fName(name) {}
   RNTupleMetrics(const RNTupleMetrics &other) = delete;
   RNTupleMetrics & operator=(const RNTupleMetrics &other) = delete;
   ~RNTupleMetrics() = default;

   template <typename CounterT>
   void MakeCounter(const std::string &name, const std::string &unit, const std::string &desc,
                    CounterT *&ptrCounter)
   {
      R__ASSERT(!Contains(name));
      auto counter = std::make_unique<CounterT>(name, unit, desc);
      ptrCounter = counter.get();
      fCounters.emplace_back(std::move(counter));
   }

   void Print(std::ostream &output) const;
   void Activate();
   bool IsActive() const { return fIsActive; }
};

} // namespace Detail
} // namespace Experimental
} // namespace ROOT

#endif
