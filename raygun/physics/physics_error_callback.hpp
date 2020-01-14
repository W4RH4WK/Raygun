// The MIT License (MIT)
//
// Copyright (c) 2019,2020 The Raygun Authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#pragma once

#include "raygun/logging.hpp"

namespace raygun::physics {

class ErrorCallback : public physx::PxErrorCallback {
  public:
    ErrorCallback() : m_logger(logger->clone("PhysX")) {}

    void reportError(physx::PxErrorCode::Enum code, const char* msg, const char* file, int line) override
    {
        using namespace physx;

#ifdef NDEBUG
        const auto fmt = "{3} {2}";
#else
        const auto fmt = "{3} {2}\n\t{0}:{1}";
#endif

        switch(code) {
        case PxErrorCode::eNO_ERROR:
            m_logger->info(fmt, file, line, "", msg);
            break;
        case PxErrorCode::eINVALID_PARAMETER:
            m_logger->error(fmt, file, line, "invalid parameter", msg);
            break;
        case PxErrorCode::eINVALID_OPERATION:
            m_logger->error(fmt, file, line, "invalid operation", msg);
            break;
        case PxErrorCode::eOUT_OF_MEMORY:
            m_logger->error(fmt, file, line, "out of memory", msg);
            break;
        case PxErrorCode::eDEBUG_INFO:
            m_logger->info(fmt, file, line, "", msg);
            break;
        case PxErrorCode::eDEBUG_WARNING:
            m_logger->warn(fmt, file, line, "", msg);
            break;
        case PxErrorCode::ePERF_WARNING:
            m_logger->warn(fmt, file, line, "performance", msg);
            break;
        case PxErrorCode::eABORT:
            m_logger->error(fmt, file, line, "abort", msg);
            break;
        case PxErrorCode::eINTERNAL_ERROR:
            m_logger->error(fmt, file, line, "internal error", msg);
            break;
        case PxErrorCode::eMASK_ALL:
            m_logger->error(fmt, file, line, "unknown error", msg);
            break;
        }
    }

  private:
    std::shared_ptr<spdlog::logger> m_logger;
};

} // namespace raygun::physics
