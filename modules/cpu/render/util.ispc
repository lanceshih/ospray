// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/FeatureFlagsEnum.h"
#include "common/World.ih"
#include "math/random.ih"
#include "math/sampling.ih"
#include "render/util.ih"

OSPRAY_BEGIN_ISPC_NAMESPACE

SYCL_EXTERNAL float computeAO(const Renderer *uniform,
    const World *uniform world,
    const varying DifferentialGeometry &dg,
    const uniform int sampleCnt,
    const uniform float aoRadius,
    const varying vec3i &sampleID,
    const uniform FeatureFlagsHandler &ffh)
{
  const uniform int accumID = reduce_max(sampleID.z) * sampleCnt;

  // init TEA RNG //
  RandomTEA rng_state;
  varying RandomTEA *const uniform rng = &rng_state;
  RandomTEA_Constructor(rng, sampleID.x, sampleID.y);
  const vec2f rot = RandomTEA_getFloats(rng);

  int hits = 0;

  const linear3f localToWorld = frame(dg.Ns);

  for (uniform int i = 0; i < sampleCnt; i++) {
    // Use base-5 and base-7 Halton to avoid correlation with sample position
    const vec2f halton = make_vec2f(
        PermHalton_sample5(accumID + i), PermHalton_sample7(accumID + i));
    const vec2f r = CranleyPattersonRotation(halton, rot);
    const vec3f local_ao_dir = cosineSampleHemisphere(r);
    const vec3f ao_dir = localToWorld * local_ao_dir;

    if (dot(ao_dir, dg.Ns) < 0.01f) { // check below surface
      hits++;
      continue;
    }

    Ray ao_ray;
    setRay(ao_ray, dg.P, ao_dir, dg.epsilon, aoRadius);
    if (isOccluded(world, ao_ray, ffh))
      hits++;
  }

  // the cosTheta of cosineSampleHemispherePDF and dot(shadingNormal, ao_dir)
  // cancel
  return 1.0f - (hits / (float)sampleCnt);
}

OSPRAY_END_ISPC_NAMESPACE
