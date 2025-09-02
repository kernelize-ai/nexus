
#include <nexus/log.h>

#include "_library_impl.h"

#define NEXUS_LOG_MODULE "library"
#undef NEXUS_LOG_DEPTH
#define NEXUS_LOG_DEPTH 34

using namespace nexus;
using namespace nexus::detail;

/// @brief Construct a Platform for the current system
LibraryImpl::LibraryImpl(Impl base) : Impl(base) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "CTOR: " << getId());
}

LibraryImpl::LibraryImpl(Impl base, const Properties &info)
    : Impl(base), info(info) {
  // auto name = info.get<std::string_view>("Name");
  NEXUS_LOG(NEXUS_STATUS_NOTE, "CTOR: " << getId());
}

LibraryImpl::~LibraryImpl() {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "DTOR: " << getId());
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

Kernel LibraryImpl::getKernel(const std::string &kernelName) {
  auto *rt = getParentOfType<RuntimeImpl>();
  nxs_int kid =
      rt->runAPIFunction<NF_nxsGetKernel>(getId(), kernelName.c_str());
  Kernel kern(Impl(this, kid), kernelName);
  kernels.add(kern);
  return kern;
}

///////////////////////////////////////////////////////////////////////////////
Library::Library(Impl base) : Object(base) {}

Library::Library(Impl base, Properties info) : Object(base, info) {
  NEXUS_LOG(NEXUS_STATUS_NOTE, "CTOR: " << getId());
}

Properties Library::getInfo() const { NEXUS_OBJ_MCALL(Properties(), getInfo); }

std::optional<Property> Library::getProperty(nxs_int prop) const {
  NEXUS_OBJ_MCALL(std::nullopt, getProperty, prop);
}

Kernel Library::getKernel(const std::string &kernelName) {
  NEXUS_OBJ_MCALL(Kernel(), getKernel, kernelName);
}
