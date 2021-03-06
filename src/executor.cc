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

#include "src/executor.h"

#include <cassert>
#include <vector>

#include "src/engine.h"
#include "src/script.h"
#include "src/shader_compiler.h"

namespace amber {

Executor::Executor() = default;

Executor::~Executor() = default;

Result Executor::Execute(Engine* engine,
                         const amber::Script* script,
                         const ShaderMap& shader_map) {
  engine->SetEngineData(script->GetEngineData());

  if (script->GetPipelines().empty())
    return Result("no pipelines defined");

  // TODO(jaebaek): Support multiple pipelines.
  if (script->GetPipelines().size() > 1)
    return Result("only single pipeline supported currently");

  auto* pipeline = script->GetPipelines()[0].get();

  // Process Shaders
  for (const auto& shader_info : pipeline->GetShaders()) {
    ShaderCompiler sc(script->GetSpvTargetEnv());

    Result r;
    std::vector<uint32_t> data;
    std::tie(r, data) = sc.Compile(shader_info.GetShader(), shader_map);
    if (!r.IsSuccess())
      return r;

    r = engine->SetShader(shader_info.GetShaderType(), data);
    if (!r.IsSuccess())
      return r;
  }

  // Handle Image and Depth buffers early so they are available when we call
  // the CreatePipeline method.
  for (const auto& info : pipeline->GetColorAttachments()) {
    Result r = engine->SetBuffer(info.type, static_cast<uint8_t>(info.location),
                                 info.buffer->AsFormatBuffer()->GetFormat(),
                                 info.buffer->GetData());
    if (!r.IsSuccess())
      return r;
  }

  if (pipeline->GetDepthBuffer().buffer) {
    const auto& info = pipeline->GetDepthBuffer();
    Result r = engine->SetBuffer(info.type, static_cast<uint8_t>(info.location),
                                 info.buffer->AsFormatBuffer()->GetFormat(),
                                 info.buffer->GetData());
    if (!r.IsSuccess())
      return r;
  }

  Result r = engine->CreatePipeline(pipeline->GetType());
  if (!r.IsSuccess())
    return r;

  for (const auto& info : pipeline->GetVertexBuffers()) {
    r = engine->SetBuffer(info.type, static_cast<uint8_t>(info.location),
                          info.buffer->IsFormatBuffer()
                              ? info.buffer->AsFormatBuffer()->GetFormat()
                              : Format(),
                          info.buffer->GetData());
    if (!r.IsSuccess())
      return r;
  }

  if (pipeline->GetIndexBuffer()) {
    auto* buf = pipeline->GetIndexBuffer();
    r = engine->SetBuffer(
        buf->GetBufferType(), 0,
        buf->IsFormatBuffer() ? buf->AsFormatBuffer()->GetFormat() : Format(),
        buf->GetData());
    if (!r.IsSuccess())
      return r;
  }

  // TODO(dsinclair) pipeline->GetBuffers() ....

  // Process Commands
  for (const auto& cmd : script->GetCommands()) {
    if (cmd->IsProbe()) {
      ResourceInfo info;

      r = engine->DoProcessCommands();
      if (!r.IsSuccess())
        return r;

      // This must come after processing commands because we require
      // the frambuffer buffer to be mapped into host memory and have
      // a valid host-side pointer.
      r = engine->GetFrameBufferInfo(&info);
      if (!r.IsSuccess())
        return r;
      assert(info.cpu_memory != nullptr);

      r = verifier_.Probe(cmd->AsProbe(), info.image_info.texel_format,
                          info.image_info.texel_stride,
                          info.image_info.row_stride, info.image_info.width,
                          info.image_info.height, info.cpu_memory);
    } else if (cmd->IsProbeSSBO()) {
      auto probe_ssbo = cmd->AsProbeSSBO();
      ResourceInfo info;
      r = engine->GetDescriptorInfo(probe_ssbo->GetDescriptorSet(),
                                    probe_ssbo->GetBinding(), &info);
      if (!r.IsSuccess())
        return r;

      r = engine->DoProcessCommands();
      if (!r.IsSuccess())
        return r;

      r = verifier_.ProbeSSBO(probe_ssbo, info.size_in_bytes, info.cpu_memory);
    } else if (cmd->IsClear()) {
      r = engine->DoClear(cmd->AsClear());
    } else if (cmd->IsClearColor()) {
      r = engine->DoClearColor(cmd->AsClearColor());
    } else if (cmd->IsClearDepth()) {
      r = engine->DoClearDepth(cmd->AsClearDepth());
    } else if (cmd->IsClearStencil()) {
      r = engine->DoClearStencil(cmd->AsClearStencil());
    } else if (cmd->IsDrawRect()) {
      r = engine->DoDrawRect(cmd->AsDrawRect());
    } else if (cmd->IsDrawArrays()) {
      r = engine->DoDrawArrays(cmd->AsDrawArrays());
    } else if (cmd->IsCompute()) {
      r = engine->DoCompute(cmd->AsCompute());
    } else if (cmd->IsEntryPoint()) {
      r = engine->DoEntryPoint(cmd->AsEntryPoint());
    } else if (cmd->IsPatchParameterVertices()) {
      r = engine->DoPatchParameterVertices(cmd->AsPatchParameterVertices());
    } else if (cmd->IsBuffer()) {
      r = engine->DoBuffer(cmd->AsBuffer());
    } else {
      return Result("Unknown command type");
    }

    if (!r.IsSuccess())
      return r;
  }
  return {};
}

}  // namespace amber
