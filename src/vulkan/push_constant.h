// Copyright 2019 The Amber Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_VULKAN_PUSH_CONSTANT_H_
#define SRC_VULKAN_PUSH_CONSTANT_H_

#include <memory>
#include <vector>

#include "amber/result.h"
#include "amber/vulkan_header.h"
#include "src/command.h"
#include "src/vulkan/resource.h"

namespace amber {
namespace vulkan {

class Device;

// Class to handle push constant.
class PushConstant : public Resource {
 public:
  // |max_push_constant_size| must be the same value with
  // maxPushConstantsSize of VkPhysicalDeviceLimits, which is an
  // element of VkPhysicalDeviceProperties getting from
  // vkGetPhysicalDeviceProperties().
  PushConstant(Device* device, uint32_t max_push_constant_size);
  ~PushConstant() override;

  // Return a VkPushConstantRange structure whose shader stage flag
  // is VK_SHADER_STAGE_ALL, offset is minimum |offset| among elements
  // in |push_constant_data_| rounded down by 4, and size is maximum
  // |offset| + |size_in_bytes| among elements in |push_constant_data_|
  // rounded up by 4.
  VkPushConstantRange GetPushConstantRange();

  // Call vkCmdPushConstants() to record a command for push constant
  // if size in bytes of push constant is not larger than
  // |max_push_constant_size_|. |command_buffer| is a Vulkan command
  // buffer that keeps the recorded command and |pipeline_layout| is
  // the graphics / compute pipeline that it currently uses.
  Result RecordPushConstantVkCommand(VkCommandBuffer command_buffer,
                                     VkPipelineLayout pipeline_layout);

  // Add a new set of values in an offset range to the push constants
  // to be used on the next pipeline execution.
  Result AddBufferData(const BufferCommand* command);

  // Resource
  VkDeviceMemory GetHostAccessMemory() const override { return VK_NULL_HANDLE; }
  Result CopyToHost(VkCommandBuffer) override {
    return Result("Vulkan: should not call CopyToHost() for PushConstant");
  }
  void Shutdown() override {}

 private:
  // maxPushConstantsSize of VkPhysicalDeviceLimits, which is an
  // element of VkPhysicalDeviceProperties getting from
  // vkGetPhysicalDeviceProperties().
  uint32_t max_push_constant_size_ = 0;

  // Keep the information of what and how to conduct push constant.
  // These are applied from lowest index to highest index, so that
  // if address ranges overlap, then the later values take effect.
  std::vector<BufferInput> push_constant_data_;

  std::unique_ptr<uint8_t> memory_;
};

}  // namespace vulkan
}  // namespace amber

#endif  // SRC_VULKAN_PUSH_CONSTANT_H_
