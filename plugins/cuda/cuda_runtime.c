/* OpenCL runtime library: clGetPlatformIDs()

   Copyright (c) 2011 Kalle Raiskila 
   
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <assert.h>
#include <string.h>
#include <sys/utsname.h>

int uname(struct utsname *buf);

#include <nexus-api.h>


/*
 * Get the number of supported platforms on this system. 
 * On POCL, this trivially reduces to 1 - POCL itself.
 */ 
NXS_API_ENTRY nxs_int NXS_API_CALL
nxsGetRuntimeProperty(
  nxs_uint runtime_property_id,
  void *property_value,
  size_t* property_value_size
) NXS_API_SUFFIX__VERSION_1_0
{

  /* lookup HIP equivalent */
  /* return value size */
  /* return value */
  switch (runtime_property_id) {
  case NXS_RUNTIME_NAME:
    // hipDeviceGetName()
    if (property_value != NULL) {
      strncpy(property_value, "hip", 4);
    } else if (property_value_size != NULL) {
      *property_value_size = 4;
    }
    break;
  
  default:
    break;
  }
  return NXS_Success;
}
/* POsym(clGetPlatformIDs) */


#ifdef __GNUC__
#pragma GCC visibility push(hidden)
#endif

#if 0
struct _nxs_plugin_table hip_runtime_plugin = {
  &nxsGetRuntimeProperty,

};

#endif

#ifdef __GNUC__
#pragma GCC visibility pop
#endif

