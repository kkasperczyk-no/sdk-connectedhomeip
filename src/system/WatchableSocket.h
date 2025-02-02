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
 *      This file declares the WatchableSocket abstraction of socket (file descriptor) events.
 */

#pragma once

// Include configuration headers
#include <system/SystemConfig.h>

#if CHIP_SYSTEM_CONFIG_USE_SOCKETS

#include <errno.h>
#include <unistd.h>

#include <core/CHIPError.h>
#include <support/BitFlags.h>
#include <support/CodeUtils.h>
#include <system/SystemError.h>

namespace chip {

namespace System {

class Layer;
class WatchableEventManager;

enum class SocketEventFlags : uint8_t
{
    kRead   = 0x1, /**< Bit flag indicating if there is a read event on a socket. */
    kWrite  = 0x2, /**< Bit flag indicating if there is a write event on a socket. */
    kExcept = 0x4, /**< Bit flag indicating if there is an exceptional condition on a socket (e.g. out-of-band data). */
    kError  = 0x8, /**< Bit flag indicating if there is an error event on a socket. */
};

using SocketEvents = BitFlags<SocketEventFlags>;

/**
 * @class WatchableSocket
 *
 * Users of a WatchableSocket should interact with it using the methods defined by WatchableSocketBasis.
 *
 * Implementations of WatchableSocket must inherit `public WatchableSocketBasis<WatchableSocket>`
 * and provide the following methods, which are invoked by the corresponding WatchableSocketBasis functions:
 *
 *  void OnInit()
 *  CHIP_ERROR OnAttach()
 *  CHIP_ERROR OnRelease()
 *  CHIP_ERROR OnRequestCallbackOnPendingRead()
 *  CHIP_ERROR OnRequestCallbackOnPendingWrite()
 *  CHIP_ERROR OnClearCallbackOnPendingRead()
 *  CHIP_ERROR OnClearCallbackOnPendingWrite()
 *
 */
class WatchableSocket;

/**
 * @class WatchableSocketBasis
 *
 * This class provides the interface used by platform-independent parts of the CHIP stack.
 *
 * The general pattern for using a WatchableSocket s is:
 *
 *  s.Init(WatchableEventManager)
 *  s.Attach(fd)
 *  s.SetCallback(callbackFunction, callbackData)
 *  s.{Request|Clear}CallbackOnPending{Read|Write}()
 *   ...
 *  s.Close()
 *
 */
template <class Impl>
class WatchableSocketBasis
{
public:
    enum : int
    {
        kInvalidFd = -1
    };

    /**
     * Initialize a WatchableSocket.
     *
     * @param[in] manager       Reference to shared socket-event state (which must already have been initialized).
     */
    void Init(WatchableEventManager & manager)
    {
        mFD = kInvalidFd;
        mPendingIO.ClearAll();
        mCallback     = nullptr;
        mCallbackData = 0;
        mSharedState  = &manager;
        static_cast<Impl *>(this)->OnInit();
    }

    /**
     * Associate this WatchableSocket with a file descriptor.
     *
     * @param[in]   fd          An open file descriptor.
     */
    CHIP_ERROR Attach(int fd)
    {
        mFD = fd;
        return static_cast<Impl *>(this)->OnAttach();
    }

    /**
     * Disassociate this WatchableSocket from its file descriptor.
     *
     * @returns the file descriptor.
     */
    int ReleaseFD()
    {
        static_cast<Impl *>(this)->OnRelease();
        const int fd = mFD;
        mFD          = kInvalidFd;
        return fd;
    }

    /**
     * Close the associated file descriptor.
     */
    void Close() { VerifyOrDie(close(ReleaseFD()) == 0); }

    /**
     * Test whether there is an associated open file descriptor.
     */
    bool HasFD() const { return mFD >= 0; }

    /**
     * Get the associated open file descriptor.
     */
    int GetFD() const { return mFD; }

    /**
     * Indicate that the socket-event system should invoke the registered callback when the file descriptor is ready to read.
     */
    CHIP_ERROR RequestCallbackOnPendingRead() { return static_cast<Impl *>(this)->OnRequestCallbackOnPendingRead(); }

    /**
     * Indicate that the socket-event system should invoke the registered callback when the file descriptor is ready to write.
     */
    CHIP_ERROR RequestCallbackOnPendingWrite(bool request = true)
    {
        return static_cast<Impl *>(this)->OnRequestCallbackOnPendingWrite();
    }

    /**
     * Indicate that the socket-event system need not invoke the registered callback when the file descriptor is ready to read.
     */
    CHIP_ERROR ClearCallbackOnPendingRead() { return static_cast<Impl *>(this)->OnClearCallbackOnPendingRead(); }

    /**
     * Indicate that the socket-event system need not invoke the registered callback when the file descriptor is ready to write.
     */
    CHIP_ERROR ClearCallbackOnPendingWrite() { return static_cast<Impl *>(this)->OnClearCallbackOnPendingWrite(); }

    /**
     * The callback is passed a reference to the WatchableSocket for which the requested event(s) are ready.
     */
    using Callback = void (*)(WatchableSocket & socket);

    /**
     * Register a callback function.
     *
     * The callback will be invoked (with the CHIP stack lock held) when requested event(s) are ready.
     *
     * @param[in]   callback        Function invoked when event(s) are ready.
     * @param[in]   data            Arbitrary data accessible within a callback function.
     */
    void SetCallback(Callback callback, intptr_t data)
    {
        mCallback     = callback;
        mCallbackData = data;
    }

    /**
     * Retrieve callback data.
     *
     * @returns the pointer supplied to SetCallback().
     */
    intptr_t GetCallbackData() const { return mCallbackData; }

    /**
     * Inside a callback function, get the pending SocketEvents.
     */
    SocketEvents GetPendingEvents() const { return mPendingIO; }

    /**
     * Inside a callback function, test whether the file descriptor is ready to read.
     */
    bool HasPendingRead() const { return mPendingIO.Has(SocketEventFlags::kRead); }

    /**
     * Inside a callback function, test whether the file descriptor is ready to write.
     */
    bool HasPendingWrite() const { return mPendingIO.Has(SocketEventFlags::kWrite); }

    /**
     * Inside a callback function, test whether there is an exceptional condition (e.g. out-of-band data)
     * associated with the file descriptor.
     */
    bool HasPendingException() const { return mPendingIO.Has(SocketEventFlags::kExcept); }

    /**
     * Inside a callback function, test whether there is an error condition associated with the file descriptor.
     */
    bool HasPendingError() const { return mPendingIO.Has(SocketEventFlags::kError); }

    /**
     * Inside a callback function, reset the set of pending events.
     */
    void ClearPendingIO() { mPendingIO.ClearAll(); }

protected:
    void InvokeCallback()
    {
        if (mCallback != nullptr)
        {
            mCallback(static_cast<Impl &>(*this));
        }
    }

    int mFD;
    SocketEvents mPendingIO;
    Callback mCallback;
    intptr_t mCallbackData;
    WatchableEventManager * mSharedState;
};

} // namespace System
} // namespace chip

#define INCLUDING_CHIP_SYSTEM_WATCHABLE_SOCKET_CONFIG_FILE 1
#ifdef CHIP_SYSTEM_WATCHABLE_SOCKET_CONFIG_FILE
#include CHIP_SYSTEM_WATCHABLE_SOCKET_CONFIG_FILE
#else // CHIP_SYSTEM_WATCHABLE_SOCKET_CONFIG_FILE
#include <system/WatchableSocketSelect.h>
#endif // CHIP_SYSTEM_WATCHABLE_SOCKET_CONFIG_FILE
#undef INCLUDING_CHIP_SYSTEM_WATCHABLE_SOCKET_CONFIG_FILE

#endif // CHIP_SYSTEM_CONFIG_USE_SOCKETS
