// Copyright 2018 The Amber Authors.
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

#ifndef AMBER_AMBER_VULKAN_H_
#define AMBER_AMBER_VULKAN_H_

#include <limits>
#include <string>
#include <vector>

#include "amber/amber.h"
#include "amber/vulkan_header.h"

namespace amber {

/// Configuration for the Vulkan Engine. The following are all required when
/// when using the vulkan backend.
struct VulkanEngineConfig : public EngineConfig {
  /// The Vulkan instance procedure loader.
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

  /// The VkInstance to use.
  VkInstance instance;

  /// The VkPhysicalDevice to use.
  VkPhysicalDevice physical_device;

  /// Physical device features available for |physical_device|.
  VkPhysicalDeviceFeatures available_features;

  /// Physical device extensions available for |physical_device|.
  std::vector<std::string> available_extensions;

  /// The given queue family index to use.
  uint32_t queue_family_index;

  /// The VkDevice to use.
  VkDevice device;

  /// The VkQueue to use.
  VkQueue queue;
};

}  // namespace amber

#endif  // AMBER_AMBER_VULKAN_H_
