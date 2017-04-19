// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#undef NDEBUG // do all assertions in this file

#include "mpiCommon/MPICommon.h"
#include "mpi/MPIDistributedDevice.h"
#include "mpi/render/MPILoadBalancer.h"

#ifdef OPEN_MPI
# include <thread>
//# define _GNU_SOURCE
# include <sched.h>
#endif

namespace ospray {
  namespace mpi {

    // MPIDistributedDevice definitions ///////////////////////////////////////
    
    MPIDistributedDevice::MPIDistributedDevice()
    {
    }

    MPIDistributedDevice::~MPIDistributedDevice()
    {
      //TODO
    }

    void MPIDistributedDevice::commit()
    {
      //TODO: is it necessary to track if we've initialized the device yet?
#if 0
      if (initialized)
        return;
#endif

      Device::commit();

      initialized = true;

      int _ac = 2;
      const char *_av[] = {"ospray_mpi_worker", "--osp:mpi"};

      mpi::init(&_ac, _av);

      masterRank = getParam1i("masterRank", 0);

      std::string mode = getParamString("mode", "distributed");

      if (mode == "distributed") {
        postErrorMsg() << "#dmpi: device commit() setting mode to " << mode;
      } else {
        throw std::runtime_error("#dmpi: bad device mode ['" + mode + "]");
      }

      // TODO: implement 'staticLoadBalancer::Distributed'
      TiledLoadBalancer::instance = make_unique<staticLoadBalancer::Master>();
    }

    OSPFrameBuffer 
    MPIDistributedDevice::frameBufferCreate(const vec2i &size,
                                            const OSPFrameBufferFormat mode,
                                            const uint32 channels)
    {
      NOT_IMPLEMENTED;
    }


    const void*
    MPIDistributedDevice::frameBufferMap(OSPFrameBuffer _fb,
                                         OSPFrameBufferChannel channel)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::frameBufferUnmap(const void *mapped,
                                                OSPFrameBuffer _fb)
    {
      NOT_IMPLEMENTED;
    }

    OSPModel MPIDistributedDevice::newModel()
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::commit(OSPObject _object)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::addGeometry(OSPModel _model,
                                           OSPGeometry _geometry)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::addVolume(OSPModel _model, OSPVolume _volume)
    {
      NOT_IMPLEMENTED;
    }

    OSPData MPIDistributedDevice::newData(size_t nitems, OSPDataType format,
                                          void *init, int flags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVoidPtr(OSPObject _object,
                                          const char *bufName,
                                          void *v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeParam(OSPObject object, const char *name)
    {
      NOT_IMPLEMENTED;
    }

    int MPIDistributedDevice::setRegion(OSPVolume _volume, const void *source,
                                        const vec3i &index, const vec3i &count)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setString(OSPObject _object,
                                         const char *bufName,
                                         const char *s)
    {
      NOT_IMPLEMENTED;
    }

    int MPIDistributedDevice::loadModule(const char *name)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setFloat(OSPObject _object,
                                        const char *bufName,
                                        const float f)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setInt(OSPObject _object,
                                      const char *bufName,
                                      const int i)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec2f(OSPObject _object,
                                        const char *bufName,
                                        const vec2f &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec3f(OSPObject _object,
                                        const char *bufName,
                                        const vec3f &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec4f(OSPObject _object,
                                        const char *bufName,
                                        const vec4f &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec2i(OSPObject _object,
                                        const char *bufName,
                                        const vec2i &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setVec3i(OSPObject _object,
                                        const char *bufName,
                                        const vec3i &v)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setObject(OSPObject _target,
                                         const char *bufName,
                                         OSPObject _value)
    {
      NOT_IMPLEMENTED;
    }

    OSPPixelOp MPIDistributedDevice::newPixelOp(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setPixelOp(OSPFrameBuffer _fb, OSPPixelOp _op)
    {
      NOT_IMPLEMENTED;
    }

    OSPRenderer MPIDistributedDevice::newRenderer(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPCamera MPIDistributedDevice::newCamera(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPVolume MPIDistributedDevice::newVolume(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPGeometry MPIDistributedDevice::newGeometry(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPMaterial MPIDistributedDevice::newMaterial(OSPRenderer _renderer,
                                                  const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPTransferFunction
    MPIDistributedDevice::newTransferFunction(const char *type)
    {
      NOT_IMPLEMENTED;
    }

    OSPLight MPIDistributedDevice::newLight(OSPRenderer _renderer,
                                            const char *type)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::frameBufferClear(OSPFrameBuffer _fb,
                                                const uint32 fbChannelFlags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeGeometry(OSPModel _model,
                                              OSPGeometry _geometry)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::removeVolume(OSPModel _model, OSPVolume _volume)
    {
      NOT_IMPLEMENTED;
    }

    float MPIDistributedDevice::renderFrame(OSPFrameBuffer _fb,
                                            OSPRenderer _renderer,
                                            const uint32 fbChannelFlags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::release(OSPObject _obj)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::setMaterial(OSPGeometry _geometry,
                                           OSPMaterial _material)
    {
      NOT_IMPLEMENTED;
    }

    OSPTexture2D MPIDistributedDevice::newTexture2D(const vec2i &sz,
        const OSPTextureFormat type, void *data, const uint32 flags)
    {
      NOT_IMPLEMENTED;
    }

    void MPIDistributedDevice::sampleVolume(float **results,
                                            OSPVolume volume,
                                            const vec3f *worldCoordinates,
                                            const size_t &count)
    {
      NOT_IMPLEMENTED;
    }

    ObjectHandle MPIDistributedDevice::allocateHandle() const
    {
      if (currentApiMode != OSPD_MODE_MASTERED)
        throw std::runtime_error("Can only alloc handles in MASTERED mode!");

      return ObjectHandle();
    }

    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed_device);
    OSP_REGISTER_DEVICE(MPIDistributedDevice, mpi_distributed);

  } // ::ospray::mpi
} // ::ospray
