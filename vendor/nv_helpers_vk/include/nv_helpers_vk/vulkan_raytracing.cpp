/*
 * Copyright (c) 2018, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#include "vulkan/vulkan.h"

static VkDevice s_global_device = VK_NULL_HANDLE;

VKAPI_ATTR VkResult VKAPI_CALL
                    vkCreateAccelerationStructureNV(VkDevice                                   device,
                                                    const VkAccelerationStructureCreateInfoNV* pCreateInfo,
                                                    const VkAllocationCallbacks*               pAllocator,
                                                    VkAccelerationStructureNV*                 pAccelerationStructure)
{
  s_global_device        = device;
  static const auto call = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(
      vkGetDeviceProcAddr(device, "vkCreateAccelerationStructureNV"));
  return call(device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL
                vkDestroyAccelerationStructureNV(VkDevice                     device,
                                                 VkAccelerationStructureNV    accelerationStructure,
                                                 const VkAllocationCallbacks* pAllocator)
{
  static const auto call = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(
      vkGetDeviceProcAddr(device, "vkDestroyAccelerationStructureNV"));
  return call(device, accelerationStructure, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsNV(
    VkDevice                                               device,
    const VkAccelerationStructureMemoryRequirementsInfoNV* pInfo,
    VkMemoryRequirements2KHR*                              pMemoryRequirements)
{
  static const auto call = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(
      vkGetDeviceProcAddr(device, "vkGetAccelerationStructureMemoryRequirementsNV"));
  return call(device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR VkResult VKAPI_CALL
                    vkBindAccelerationStructureMemoryNV(VkDevice                                       device,
                                                        uint32_t                                       bindInfoCount,
                                                        const VkBindAccelerationStructureMemoryInfoNV* pBindInfos)
{
  static const auto call = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(
      vkGetDeviceProcAddr(device, "vkBindAccelerationStructureMemoryNV"));
  return call(device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL
                vkCmdBuildAccelerationStructureNV(VkCommandBuffer                      commandBuffer,
                                                  const VkAccelerationStructureInfoNV* pInfo,
                                                  VkBuffer                             instanceData,
                                                  VkDeviceSize                         instanceOffset,
                                                  VkBool32                             update,
                                                  VkAccelerationStructureNV            dst,
                                                  VkAccelerationStructureNV            src,
                                                  VkBuffer                             scratch,
                                                  VkDeviceSize                         scratchOffset)
{
  static const auto call = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(
      vkGetDeviceProcAddr(s_global_device, "vkCmdBuildAccelerationStructureNV"));
  return call(commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch,
              scratchOffset);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureNV(VkCommandBuffer           cmdBuf,
                                                            VkAccelerationStructureNV dst,
                                                            VkAccelerationStructureNV src,
                                                            VkCopyAccelerationStructureModeNV mode)
{
  static const auto call = reinterpret_cast<PFN_vkCmdCopyAccelerationStructureNV>(
      vkGetDeviceProcAddr(s_global_device, "vkCmdCopyAccelerationStructureNV"));
  return call(cmdBuf, dst, src, mode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysNV(VkCommandBuffer commandBuffer,
                                            VkBuffer        raygenShaderBindingTableBuffer,
                                            VkDeviceSize    raygenShaderBindingOffset,
                                            VkBuffer        missShaderBindingTableBuffer,
                                            VkDeviceSize    missShaderBindingOffset,
                                            VkDeviceSize    missShaderBindingStride,
                                            VkBuffer        hitShaderBindingTableBuffer,
                                            VkDeviceSize    hitShaderBindingOffset,
                                            VkDeviceSize    hitShaderBindingStride,
                                            VkBuffer        callableShaderBindingTableBuffer,
                                            VkDeviceSize    callableShaderBindingOffset,
                                            VkDeviceSize    callableShaderBindingStride,
                                            uint32_t        width,
                                            uint32_t        height,
                                            uint32_t        depth)
{
  static const auto call = reinterpret_cast<PFN_vkCmdTraceRaysNV>(
      vkGetDeviceProcAddr(s_global_device, "vkCmdTraceRaysNV"));
  return call(commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset,
              missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride,
              hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride,
              callableShaderBindingTableBuffer, callableShaderBindingOffset,
              callableShaderBindingStride, width, height, depth);
}

VKAPI_ATTR VkResult VKAPI_CALL
                    vkCreateRayTracingPipelinesNV(VkDevice                                device,
                                                  VkPipelineCache                         pipelineCache,
                                                  uint32_t                                createInfoCount,
                                                  const VkRayTracingPipelineCreateInfoNV* pCreateInfos,
                                                  const VkAllocationCallbacks*            pAllocator,
                                                  VkPipeline*                             pPipelines)
{
  s_global_device        = device;
  static const auto call = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(
      vkGetDeviceProcAddr(device, "vkCreateRayTracingPipelinesNV"));
  return call(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesNV(VkDevice   device,
                                                                   VkPipeline pipeline,
                                                                   uint32_t   firstGroup,
                                                                   uint32_t   groupCount,
                                                                   size_t     dataSize,
                                                                   void*      pData)
{
  static const auto call = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(
      vkGetDeviceProcAddr(device, "vkGetRayTracingShaderGroupHandlesNV"));
  return call(device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL
                    vkGetAccelerationStructureHandleNV(VkDevice                  device,
                                                       VkAccelerationStructureNV accelerationStructure,
                                                       size_t                    dataSize,
                                                       void*                     pData)
{
  static const auto call = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(
      vkGetDeviceProcAddr(device, "vkGetAccelerationStructureHandleNV"));
  return call(device, accelerationStructure, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesNV(
    VkCommandBuffer                  commandBuffer,
    uint32_t                         accelerationStructureCount,
    const VkAccelerationStructureNV* pAccelerationStructures,
    VkQueryType                      queryType,
    VkQueryPool                      queryPool,
    uint32_t                         firstQuery)
{
  static const auto call = reinterpret_cast<PFN_vkCmdWriteAccelerationStructuresPropertiesNV>(
      vkGetDeviceProcAddr(s_global_device, "vkCmdWriteAccelerationStructuresPropertiesNV"));
  return call(commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType,
              queryPool, firstQuery);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCompileDeferredNV(VkDevice   device,
                                                   VkPipeline pipeline,
                                                   uint32_t   shader)
{
  static const auto call = reinterpret_cast<PFN_vkCompileDeferredNV>(
      vkGetDeviceProcAddr(s_global_device, "vkCompileDeferredNV"));
  return call(device, pipeline, shader);
}
