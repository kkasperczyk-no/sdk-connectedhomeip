/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include "MessagingContext.h"

#include <support/CodeUtils.h>
#include <support/ErrorStr.h>

namespace chip {
namespace Test {

CHIP_ERROR MessagingContext::Init(nlTestSuite * suite, TransportMgrBase * transport)
{
    ReturnErrorOnFailure(IOContext::Init(suite));

    mFabrics.Reset();

    chip::Transport::FabricInfo * srcNodeFabric = mFabrics.AssignFabricIndex(mSrcFabricIndex, GetSourceNodeId());
    VerifyOrReturnError(srcNodeFabric != nullptr, CHIP_ERROR_NO_MEMORY);

    chip::Transport::FabricInfo * destNodeFabric = mFabrics.AssignFabricIndex(mDestFabricIndex, GetDestinationNodeId());
    VerifyOrReturnError(destNodeFabric != nullptr, CHIP_ERROR_NO_MEMORY);

    ReturnErrorOnFailure(
        mSecureSessionMgr.Init(GetSourceNodeId(), &GetSystemLayer(), transport, &mFabrics, &mMessageCounterManager));

    ReturnErrorOnFailure(mExchangeManager.Init(&mSecureSessionMgr));
    ReturnErrorOnFailure(mMessageCounterManager.Init(&mExchangeManager));

    ReturnErrorOnFailure(mSecureSessionMgr.NewPairing(mPeer, GetDestinationNodeId(), &mPairingLocalToPeer,
                                                      SecureSession::SessionRole::kInitiator, mSrcFabricIndex));

    return mSecureSessionMgr.NewPairing(mPeer, GetSourceNodeId(), &mPairingPeerToLocal, SecureSession::SessionRole::kResponder,
                                        mDestFabricIndex);
}

// Shutdown all layers, finalize operations
CHIP_ERROR MessagingContext::Shutdown()
{
    mExchangeManager.Shutdown();
    return IOContext::Shutdown();
}

SecureSessionHandle MessagingContext::GetSessionLocalToPeer()
{
    // TODO: temporarily create a SecureSessionHandle from node id, will be fixed in PR 3602
    return { GetDestinationNodeId(), GetPeerKeyId(), GetFabricIndex() };
}

SecureSessionHandle MessagingContext::GetSessionPeerToLocal()
{
    // TODO: temporarily create a SecureSessionHandle from node id, will be fixed in PR 3602
    return { GetSourceNodeId(), GetLocalKeyId(), mDestFabricIndex };
}

Messaging::ExchangeContext * MessagingContext::NewExchangeToPeer(Messaging::ExchangeDelegate * delegate)
{
    // TODO: temprary create a SecureSessionHandle from node id, will be fix in PR 3602
    return mExchangeManager.NewContext(GetSessionLocalToPeer(), delegate);
}

Messaging::ExchangeContext * MessagingContext::NewExchangeToLocal(Messaging::ExchangeDelegate * delegate)
{
    // TODO: temprary create a SecureSessionHandle from node id, will be fix in PR 3602
    return mExchangeManager.NewContext(GetSessionPeerToLocal(), delegate);
}

} // namespace Test
} // namespace chip
