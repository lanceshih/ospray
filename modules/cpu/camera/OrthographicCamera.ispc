// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.ih"
#include "common/Embree.h"
#include "common/FeatureFlags.ih"
// c++ shared
#include "OrthographicCameraShared.h"

#include "rkcommon/math/AffineSpace.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL void OrthographicCamera_initRay(const Camera *uniform _self,
    varying Ray &ray,
    varying RayCone &rayCone,
    const varying CameraSample &sample,
    const uniform FeatureFlagsHandler &ffh)
{
  const OrthographicCamera *uniform self =
      (const OrthographicCamera *uniform)_self;

  const vec2f screen = Camera_subRegion(_self, sample.screen);
  const float time = Camera_shutterTime(_self, screen, sample.time);

  vec3f dir;
  vec3f org;
  const uniform FeatureFlagsOther ffo = getFeatureFlagsOther(ffh);
  if (self->super.motionBlur && (ffo & FFO_CAMERA_MOTION_BLUR)) {
    AffineSpace3f xfm;
    rtcGetGeometryTransformFromScene(
        self->super.scene, 0, time, RTC_FORMAT_FLOAT3X4_COLUMN_MAJOR, &xfm);

    // we cannot just transform the final org & dir, because interpolated
    // transforms can scale (even if original transforms are without scale)
    dir = normalize(xfmVector(xfm, self->dir));
    const vec3f up = xfmVector(xfm, self->dv_up);
    org = xfmPoint(xfm, self->org);

    const vec3f du = normalize(cross(dir, up));
    const vec3f dv = cross(du, dir);
    const float x = (screen.x - 0.5f) * self->du_size.x;
    const float y = (screen.y - 0.5f) * self->du_size.y;
    org = org + x * du + y * dv;
  } else {
    dir = self->dir;
    org = self->org + screen.x * self->du_size + screen.y * self->dv_up;
  }

  setRay(ray, org, dir, self->super.nearClip, inf, time);
  const uniform vec2f subImageSize = box_size(self->super.subImage);
  rayCone.width = self->height * abs(subImageSize.y);
  rayCone.dwdt = 0.f;
}

// Exports (called from C++) //////////////////////////////////////////////////

export void *uniform OrthographicCamera_initRay_addr()
{
  return (void *uniform)OrthographicCamera_initRay;
}

OSPRAY_END_ISPC_NAMESPACE
