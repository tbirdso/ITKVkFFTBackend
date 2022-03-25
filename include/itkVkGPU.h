/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkVkGPU_h
#  define itkVkGPU_h

#  include "VkFFTBackendExport.h"
#include "itkLightObject.h"
#  include "vkFFT.h"

namespace itk
{

class VkGPU : public LightObject
{
public:
  cl_platform_id   platform = 0;
  cl_device_id     device = 0;
  cl_context       context = 0;
  cl_command_queue commandQueue = 0;
  uint64_t         device_id;

  static uint64_t m_DefaultDeviceID;

  static void
      SetDefaultDeviceID(const int device_id)
  {
    m_DefaultDeviceID = device_id;
  }

  VkGPU::VkGPU() { device_id = m_DefaultDeviceID; };
  ~VkGPU() = default;

  bool
  operator!=(const VkGPU & rhs) const
  {
    return this->platform != rhs.platform || this->device != rhs.device || this->context != rhs.context ||
           this->commandQueue != rhs.commandQueue || this->device_id != rhs.device_id;
  }

  void
  operator=(const VkGPU & rhs)
  {
    this->platform = rhs.platform;
    this->device = rhs.device;
    this->context = rhs.context;
    this->commandQueue = rhs.commandQueue;
    this->device_id = rhs.device_id;
  }

};
uint64_t VkGPU::m_DefaultDeviceID = 0;

} // namespace itk

#endif // itkVkGPU_h