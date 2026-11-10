/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoSDL.h"

#include <mutex>

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoSDL::Create()
{
  return new CProcessInfoSDL();
}

void CProcessInfoSDL::Register()
{
  CProcessInfo::RegisterProcessControl("SDL", CProcessInfoSDL::Create);
}

void CProcessInfoSDL::SetSwDeinterlacingMethods()
{
  CProcessInfo::SetSwDeinterlacingMethods();

  std::list<EINTERLACEMETHOD> methods;
  {
    std::unique_lock<CCriticalSection> lock(m_videoCodecSection);
    methods = m_deintMethods;
  }
  methods.push_back(EINTERLACEMETHOD::VS_INTERLACEMETHOD_RENDER_BOB);
  UpdateDeinterlacingMethods(methods);
}

std::vector<AVPixelFormat> CProcessInfoSDL::GetRenderFormats()
{
  // NV12 is what libpmhwdec's hardware decoders output; GLES renders both directly.
  return {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NV12};
}
