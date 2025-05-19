/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Contains non-inline method definitions for the
 *          GenericPlatformManagerImpl_Nrfconnect<> template.
 */

#ifndef GENERIC_PLATFORM_MANAGER_IMPL_NRFCONNECT_CPP
#define GENERIC_PLATFORM_MANAGER_IMPL_NRFCONNECT_CPP

#include <platform/PlatformManager.h>
#include <platform/internal/CHIPDeviceLayerInternal.h>
#include <platform/internal/GenericPlatformManagerImpl_Nrfconnect.h>

// Include the non-inline definitions for the GenericPlatformManagerImpl<> template,
// from which the GenericPlatformManagerImpl_Zephyr<> template inherits.
#include <platform/internal/GenericPlatformManagerImpl.ipp>

#include <system/SystemError.h>
#include <system/SystemLayer.h>
#include <system/SystemStats.h>

#include <zephyr/sys/reboot.h>

// #ifdef CONFIG_CHIP_CRYPTO_PSA
// #include <psa/crypto.h>
// #endif

// #define DEFAULT_MIN_SLEEP_PERIOD (60 * 60 * 24 * 30) // Month [sec]

namespace chip {
namespace DeviceLayer {
namespace Internal {

// namespace {

// System::LayerSocketsLoop & SystemLayerSocketsLoop()
// {
//     return static_cast<System::LayerSocketsLoop &>(DeviceLayer::SystemLayer());
// }

// K_WORK_DEFINE(sSignalWork, [](k_work *) { SystemLayerSocketsLoop().Signal(); });

// } // anonymous namespace

template <class ImplClass>
CHIP_ERROR GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_InitChipStack(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    if (mInitialized)
        return err;

    k_mutex_init(&mChipStackLock);

    k_msgq_init(&mChipEventQueue, reinterpret_cast<char *>(&mChipEventRingBuffer), sizeof(ChipDeviceEvent),
                CHIP_DEVICE_CONFIG_MAX_EVENT_QUEUE_SIZE);

    mShouldRunEventLoop = false;

#ifdef CONFIG_CHIP_CRYPTO_PSA
    VerifyOrReturnError(psa_crypto_init() == PSA_SUCCESS, CHIP_ERROR_INTERNAL);
#endif

    // Call up to the base class _InitChipStack() to perform the bulk of the initialization.
    err = GenericPlatformManagerImpl<ImplClass>::_InitChipStack();
    SuccessOrExit(err);

    mInitialized = true;

exit:
    return err;
}

template <class ImplClass>
void GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_LockChipStack(void)
{
    k_mutex_lock(&mChipStackLock, K_FOREVER);
}

template <class ImplClass>
bool GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_TryLockChipStack(void)
{
    return k_mutex_lock(&mChipStackLock, K_NO_WAIT) == 0;
}

template <class ImplClass>
void GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_UnlockChipStack(void)
{
    k_mutex_unlock(&mChipStackLock);
}

template <class ImplClass>
CHIP_ERROR GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_StartChipTimer(System::Clock::Timeout delay)
{
    // mChipTimerActive = true;
    // vTaskSetTimeOutState(&mNextTimerBaseTime);
    // mNextTimerDurationTicks = pdMS_TO_TICKS(System::Clock::Milliseconds64(delay).count());

    // // If the platform timer is being updated by a thread other than the event loop thread,
    // // trigger the event loop thread to recalculate its wait time by posting a no-op event
    // // to the event queue.
    // if (xTaskGetCurrentTaskHandle() != mEventLoopTask)
    // {
    //     ChipDeviceEvent noop{ .Type = DeviceEventType::kNoOp };
    //     ReturnErrorOnFailure(Impl()->PostEvent(&noop));
    // }

//     assertChipStackLockedByCurrentThread();

//     VerifyOrReturnError(mLayerState.IsInitialized(), CHIP_ERROR_INCORRECT_STATE);

//     CHIP_SYSTEM_FAULT_INJECT(FaultInjection::kFault_TimeoutImmediate, delay = System::Clock::kZero);

//     CancelTimer(onComplete, appState);

//     TimerList::Node * timer = mTimerPool.Create(*this, SystemClock().GetMonotonicTimestamp() + delay, onComplete, appState);
//     VerifyOrReturnError(timer != nullptr, CHIP_ERROR_NO_MEMORY);

// #if CHIP_SYSTEM_CONFIG_USE_DISPATCH
//     dispatch_queue_t dispatchQueue = GetDispatchQueue();
//     if (dispatchQueue)
//     {
//         (void) mTimerList.Add(timer);
//         dispatch_source_t timerSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, DISPATCH_TIMER_STRICT, dispatchQueue);
//         VerifyOrDie(timerSource != nullptr);

//         timer->mTimerSource = timerSource;
//         dispatch_source_set_timer(
//             timerSource, dispatch_walltime(nullptr, static_cast<int64_t>(Clock::Milliseconds64(delay).count() * NSEC_PER_MSEC)),
//             DISPATCH_TIME_FOREVER, 2 * NSEC_PER_MSEC);
//         dispatch_source_set_event_handler(timerSource, ^{
//             dispatch_source_cancel(timerSource);
//             dispatch_release(timerSource);

//             this->HandleTimerComplete(timer);
//         });
//         dispatch_resume(timerSource);
//         return CHIP_NO_ERROR;
//     }
// #elif CHIP_SYSTEM_CONFIG_USE_LIBEV
//     VerifyOrDie(mLibEvLoopP != nullptr);
//     ev_timer_init(&timer->mLibEvTimer, &LayerImplSelect::HandleLibEvTimer, 1, 0);
//     timer->mLibEvTimer.data = timer;
//     auto t                  = Clock::Milliseconds64(delay).count();
//     // Note: libev uses the time when events started processing as the "now" reference for relative timers,
//     //   for efficiency reasons. This point in time is represented by ev_now().
//     //   The real time is represented by ev_time().
//     //   Without correction, this leads to timers firing a bit too early relative to the time StartTimer()
//     //   is called. So the relative value passed to ev_timer_set() is adjusted (increased) here.
//     // Note: Still, slightly early (and of course, late) firing timers are something the caller MUST be prepared for,
//     //   because edge cases like system clock adjustments may cause them even with the correction applied here.
//     ev_timer_set(&timer->mLibEvTimer, (static_cast<double>(t) / 1E3) + ev_time() - ev_now(mLibEvLoopP), 0.);
//     (void) mTimerList.Add(timer);
//     ev_timer_start(mLibEvLoopP, &timer->mLibEvTimer);
//     return CHIP_NO_ERROR;
// #endif
// #if !CHIP_SYSTEM_CONFIG_USE_LIBEV
//     // Note: dispatch based implementation needs this as fallback, but not LIBEV (and dead code is not allowed with -Werror)
//     if (mTimerList.Add(timer) == timer)
//     {
//         // The new timer is the earliest, so the time until the next event has probably changed.
//         Signal();
//     }
//     return CHIP_NO_ERROR;
// #endif // !CHIP_SYSTEM_CONFIG_USE_LIBEV

    return CHIP_NO_ERROR;
}

template <class ImplClass>
CHIP_ERROR GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_StopEventLoopTask(void)
{
    mShouldRunEventLoop = false;
    return CHIP_NO_ERROR;
}

template <class ImplClass>
void GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_Shutdown(void)
{
#ifdef CONFIG_REBOOT
    sys_reboot(SYS_REBOOT_WARM);
#else
    // NB: When this is implemented, |mInitialized| can be removed.
#endif
}

template <class ImplClass>
CHIP_ERROR GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_PostEvent(const ChipDeviceEvent * event)
{
    int status = k_msgq_put(&mChipEventQueue, event, K_NO_WAIT);
    if (status != 0)
    {
        ChipLogError(DeviceLayer, "Failed to post event to CHIP Platform event queue");
        return System::MapErrorZephyr(status);
    }

    return CHIP_NO_ERROR;
}

template <class ImplClass>
void GenericPlatformManagerImpl_Nrfconnect<ImplClass>::ProcessDeviceEvents()
{
    ChipDeviceEvent event;

    while (k_msgq_get(&mChipEventQueue, &event, K_NO_WAIT) == 0)
    {
        SYSTEM_STATS_DECREMENT(System::Stats::kPlatformMgr_NumEvents);
        Impl()->DispatchEvent(&event);
    }
}

template <class ImplClass>
void GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_RunEventLoop(void)
{
    Impl()->LockChipStack();

    if (mShouldRunEventLoop)
    {
        ChipLogError(DeviceLayer, "Error trying to run the event loop while it is already running");
        return;
    }
    mShouldRunEventLoop = true;

    // SystemLayerSocketsLoop().EventLoopBegins();
    while (mShouldRunEventLoop)
    {
        // SystemLayerSocketsLoop().PrepareEvents();

        // Impl()->UnlockChipStack();
        // SystemLayerSocketsLoop().WaitForEvents();
        // Impl()->LockChipStack();

        // SystemLayerSocketsLoop().HandleEvents();

        ProcessDeviceEvents();
    }
    // SystemLayerSocketsLoop().EventLoopEnds();

    Impl()->UnlockChipStack();
}

template <class ImplClass>
void GenericPlatformManagerImpl_Nrfconnect<ImplClass>::EventLoopTaskMain(void * thisPtr, void *, void *)
{
    ChipLogProgress(DeviceLayer, "CHIP task running");
    static_cast<GenericPlatformManagerImpl_Nrfconnect<ImplClass> *>(thisPtr)->Impl()->RunEventLoop();
}

template <class ImplClass>
CHIP_ERROR GenericPlatformManagerImpl_Nrfconnect<ImplClass>::_StartEventLoopTask(void)
{
    if (!mChipThreadStack)
        return CHIP_ERROR_UNINITIALIZED;

    const auto tid = k_thread_create(&mChipThread, mChipThreadStack, CHIP_DEVICE_CONFIG_CHIP_TASK_STACK_SIZE, EventLoopTaskMain,
                                     this, nullptr, nullptr, CHIP_DEVICE_CONFIG_CHIP_TASK_PRIORITY, 0, K_NO_WAIT);

#ifdef CONFIG_THREAD_NAME
    k_thread_name_set(tid, CHIP_DEVICE_CONFIG_CHIP_TASK_NAME);
#else
    IgnoreUnusedVariable(tid);
#endif

    return CHIP_NO_ERROR;
}

// Fully instantiate the generic implementation class in whatever compilation unit includes this file.
// NB: This must come after all templated class members are defined.
template class GenericPlatformManagerImpl_Nrfconnect<PlatformManagerImpl>;

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

#endif // GENERIC_PLATFORM_MANAGER_IMPL_ZEPHYR_CPP
