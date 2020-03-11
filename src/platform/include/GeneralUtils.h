/*
 *
 *    <COPYRIGHT>
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
 *          General utility functions available on all platforms.
 */

namespace chip {
namespace DeviceLayer {

extern CHIP_ERROR ParseCompilerDateStr(const char * dateStr, uint16_t & year, uint8_t & month, uint8_t & dayOfMonth);
extern CHIP_ERROR Parse24HourTimeStr(const char * timeStr, uint8_t & hour, uint8_t & minute, uint8_t & second);
extern const char * CharacterizeIPv6Address(const ::Inet::IPAddress & ipAddr);
extern const char * CharacterizeIPv6Prefix(const Inet::IPPrefix & inPrefix);
extern void RegisterDeviceLayerErrorFormatter(void);
extern bool FormatDeviceLayerError(char * buf, uint16_t bufSize, int32_t err);

} // namespace DeviceLayer
} // namespace chip
