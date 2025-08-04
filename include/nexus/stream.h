#ifndef NEXUS_STREAM_H
#define NEXUS_STREAM_H

#include <nexus-api.h>
#include <nexus/object.h>

namespace nexus {

namespace detail {
class StreamImpl;
}  // namespace detail

// System class
class Stream : public Object<detail::StreamImpl> {
 public:
  Stream(detail::Impl base);
  using Object::Object;

  std::optional<Property> getProperty(nxs_int prop) const override;

};

typedef Objects<Stream> Streams;

}  // namespace nexus

#endif  // NEXUS_STREAM_H