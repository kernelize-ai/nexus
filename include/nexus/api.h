#ifndef _NEXUS_API_H
#define _NEXUS_API_H

namespace nexus {

enum NXS_Object_Type {
  NT_Buffer,
  NT_Command,
  NT_Device,

};

typedef enum NXS_Object_Type nxs_type;
}  // namespace nexus

#endif  // _NEXUS_API_H
