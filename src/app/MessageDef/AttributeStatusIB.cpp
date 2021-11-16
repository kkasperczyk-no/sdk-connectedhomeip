/**
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include "AttributeStatusIB.h"
#include "MessageDefHelper.h"
#include "StructBuilder.h"
#include "StructParser.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>

#include <app/AppBuildConfig.h>

namespace chip {
namespace app {
#if CHIP_CONFIG_IM_ENABLE_SCHEMA_CHECK
CHIP_ERROR AttributeStatusIB::Parser::CheckSchemaValidity() const
{
    CHIP_ERROR err      = CHIP_NO_ERROR;
    int TagPresenceMask = 0;
    TLV::TLVReader reader;

    PRETTY_PRINT("AttributeStatusIB =");
    PRETTY_PRINT("{");

    // make a copy of the reader
    reader.Init(mReader);

    while (CHIP_NO_ERROR == (err = reader.Next()))
    {
        VerifyOrReturnError(TLV::IsContextTag(reader.GetTag()), CHIP_ERROR_INVALID_TLV_TAG);
        uint32_t tagNum = TLV::TagNumFromTag(reader.GetTag());
        switch (tagNum)
        {
        case to_underlying(Tag::kPath):
            // check if this tag has appeared before
            VerifyOrReturnError(!(TagPresenceMask & (1 << to_underlying(Tag::kPath))), CHIP_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << to_underlying(Tag::kPath));
            {
                AttributePathIB::Parser path;
                ReturnErrorOnFailure(path.Init(reader));

                PRETTY_PRINT_INCDEPTH();
                ReturnErrorOnFailure(path.CheckSchemaValidity());
                PRETTY_PRINT_DECDEPTH();
            }
            break;
        case to_underlying(Tag::kErrorStatus):
            // check if this tag has appeared before
            VerifyOrReturnError(!(TagPresenceMask & (1 << to_underlying(Tag::kErrorStatus))), CHIP_ERROR_INVALID_TLV_TAG);
            TagPresenceMask |= (1 << to_underlying(Tag::kErrorStatus));
            {
                StatusIB::Parser errorStatus;
                ReturnErrorOnFailure(errorStatus.Init(reader));

                PRETTY_PRINT_INCDEPTH();
                ReturnErrorOnFailure(errorStatus.CheckSchemaValidity());
                PRETTY_PRINT_DECDEPTH();
            }
            break;
        default:
            PRETTY_PRINT("Unknown tag num %" PRIu32, tagNum);
            break;
        }
    }

    PRETTY_PRINT("},");
    PRETTY_PRINT("");

    if (CHIP_END_OF_TLV == err)
    {
        const int RequiredFields = (1 << to_underlying(Tag::kPath)) | (1 << to_underlying(Tag::kErrorStatus));

        if ((TagPresenceMask & RequiredFields) == RequiredFields)
        {
            err = CHIP_NO_ERROR;
        }
    }

    ReturnErrorOnFailure(err);
    ReturnErrorOnFailure(reader.ExitContainer(mOuterContainerType));
    return CHIP_NO_ERROR;
}
#endif // CHIP_CONFIG_IM_ENABLE_SCHEMA_CHECK

CHIP_ERROR AttributeStatusIB::Parser::GetPath(AttributePathIB::Parser * const apPath) const
{
    TLV::TLVReader reader;
    ReturnErrorOnFailure(mReader.FindElementWithTag(TLV::ContextTag(to_underlying(Tag::kPath)), reader));
    ReturnErrorOnFailure(apPath->Init(reader));
    return CHIP_NO_ERROR;
}

CHIP_ERROR AttributeStatusIB::Parser::GetErrorStatus(StatusIB::Parser * const apErrorStatus) const
{
    TLV::TLVReader reader;
    ReturnErrorOnFailure(mReader.FindElementWithTag(TLV::ContextTag(to_underlying(Tag::kErrorStatus)), reader));
    ReturnErrorOnFailure(apErrorStatus->Init(reader));
    return CHIP_NO_ERROR;
}

AttributePathIB::Builder & AttributeStatusIB::Builder::CreatePath()
{
    if (mError == CHIP_NO_ERROR)
    {
        mError = mPath.Init(mpWriter, to_underlying(Tag::kPath));
    }
    return mPath;
}

StatusIB::Builder & AttributeStatusIB::Builder::CreateErrorStatus()
{
    if (mError == CHIP_NO_ERROR)
    {
        mError = mErrorStatus.Init(mpWriter, to_underlying(Tag::kErrorStatus));
    }
    return mErrorStatus;
}

AttributeStatusIB::Builder & AttributeStatusIB::Builder::EndOfAttributeStatusIB()
{
    EndOfContainer();
    return *this;
}
} // namespace app
} // namespace chip
