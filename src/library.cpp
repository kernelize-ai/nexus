
#include <nexus/log.h>

#include "_info_impl.h"
#include "_kernel_impl.h"
#include "_library_impl.h"
#include "_runtime_impl.h"

#define NEXUS_LOG_MODULE "library"
#undef NEXUS_LOG_DEPTH
#define NEXUS_LOG_DEPTH 34

using namespace nexus;
using namespace nexus::detail;

/// @brief Construct a Platform for the current system
LibraryImpl::LibraryImpl(Impl base) : Impl(base) {
  NEXUS_LOG(NXS_LOG_NOTE, "CTOR: ", getId());
}

LibraryImpl::LibraryImpl(Impl base, Info info) : Impl(base), info(info) {
  // auto name = info.get<std::string_view>("Name");
  NEXUS_LOG(NXS_LOG_NOTE, "CTOR: ", getId());
  // Iterate over all functions and kernels
  try {
    if (auto functions = info.getNode({"Functions"})) {
      for (auto &kernel : *functions) {
        auto name = kernel.at("Symbol").get<std::string>();
        Info::Node node(kernel);
        getKernel(name, Info(node));
      }
    }
  } catch (...) {
    NEXUS_LOG(NXS_LOG_ERROR, "  LibraryImpl: ERROR loading functions");
  }
}

LibraryImpl::~LibraryImpl() {
  NEXUS_LOG(NXS_LOG_NOTE, "DTOR: ", getId());
  release();
}

void LibraryImpl::release() {
  kernels.clear();
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int kid = rt->runAPIFunction<NF_nxsReleaseLibrary>(getId());
}

std::optional<Property> detail::LibraryImpl::getProperty(nxs_int prop) const {
  auto *rt = getParentOfType<RuntimeImpl>();
  return rt->getAPIProperty<NF_nxsGetLibraryProperty>(prop, getId());
}

Kernel LibraryImpl::getKernel(const std::string &kernelName, Info info) {
  NEXUS_LOG(NXS_LOG_NOTE, "  getKernel: ", kernelName);
  auto it = kernelMap.find(kernelName);
  if (it != kernelMap.end())
    return it->second;
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int kid =
      rt->runAPIFunction<NF_nxsGetKernel>(getId(), kernelName.c_str());
  Kernel kern(Impl(this, kid), kernelName, info);
  kernels.add(kern);
  kernelMap[kernelName] = kern;
  return kern;
}

///////////////////////////////////////////////////////////////////////////////
Library::Library(detail::Impl base) : Object(base) {}

Library::Library(detail::Impl base, Info info) : Object(base, info) {}

Info Library::getInfo() const { NEXUS_OBJ_MCALL(Info(), getInfo); }

std::optional<Property> Library::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Kernel Library::getKernel(const std::string &kernelName, Info info) {
  NEXUS_OBJ_MCALL(Kernel(), getKernel, kernelName, info);
}

Kernels Library::getKernels() const { NEXUS_OBJ_MCALL(Kernels(), getKernels); }