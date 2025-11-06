#include <Python.h>
#include <nexus-api.h>
#include <nexus.h>
#include <pybind11/stl.h>

#include <iostream>
#include <pybind11_json/pybind11_json.hpp>

#include "../src/_info_impl.h"
#include "pynexus.h"

namespace py = pybind11;

using namespace nexus;

struct DevPtr {
  char *ptr;
  size_t size;
  std::string runtime_name;
  nxs_int device_id;
};

static DevPtr getPointer(PyObject *obj) {
  DevPtr result{nullptr, 0, "", -1};
  if (obj == Py_None) {
    return result;
  }
  PyObject *data_ptr_m = PyObject_GetAttrString(obj, "data_ptr");
  if (data_ptr_m == nullptr) {
    data_ptr_m = PyObject_GetAttrString(obj, "tobytes");
  }
  PyObject *nbytes_ret = PyObject_GetAttrString(obj, "nbytes");
  if (data_ptr_m && nbytes_ret) {
    PyObject *empty_tuple = PyTuple_New(0);
    PyObject *data_ret = PyObject_Call(data_ptr_m, empty_tuple, NULL);
    Py_DECREF(empty_tuple);
    Py_DECREF(data_ptr_m);
    if (!data_ret || *((nxs_long*)&data_ret) == -1) {
      PyErr_SetString(
          PyExc_TypeError,
          "data_ptr method of Pointer object must return 64-bit int");
      return result;
    }
    PyObject *device_m = PyObject_GetAttrString(obj, "device");
    if (device_m) {
      PyObject *runtime_name_m = PyObject_GetAttrString(device_m, "type");
      if (runtime_name_m) {
        result.runtime_name = PyUnicode_AsUTF8(runtime_name_m);
        Py_DECREF(runtime_name_m);
      }
      PyObject *device_id_m = PyObject_GetAttrString(device_m, "index");
      if (device_id_m && PyLong_Check(device_id_m)) {
        result.device_id = PyLong_AsLong(device_id_m);
        Py_DECREF(device_id_m);
      } else if (!result.runtime_name.empty()) {
        result.device_id = 0;
      }
      Py_DECREF(device_m);
    }
    result.ptr = (char *)PyLong_AsUnsignedLongLong(data_ret);
    result.size = PyLong_AsUnsignedLongLong(nbytes_ret);
    //Py_DECREF(data_ret);
    Py_DECREF(nbytes_ret);
  }
  return result;
}

static Buffer make_buffer(py::object tensor) {
  // TODO: track ownership of the py::object tensor (release on destruction of
  // Buffer)
  auto data_ptr = getPointer(tensor.ptr());
  if (data_ptr.size == 0) {
    throw std::runtime_error("Invalid buffer");
  }
  if (!data_ptr.runtime_name.empty() && data_ptr.device_id != -1) {
    auto runtime = nexus::getSystem().getRuntime(data_ptr.runtime_name);
    if (runtime) {
      auto device = runtime.getDevice(data_ptr.device_id);
      if (device) {
        return device.createBuffer(data_ptr.size, data_ptr.ptr, NXS_BufferSettings_OnDevice);
      }
    } else {
      throw std::runtime_error("Runtime not found: " +
                               std::string(data_ptr.runtime_name));
    }
  }
  return nexus::getSystem().createBuffer(data_ptr.size, data_ptr.ptr);
}

//////////////////////////////////////////////////////////////////////////
// Property key string conversion
static std::string get_key_str(const std::string &key) {
  return key;
}

static std::string get_key_str(const nxs_int &key) {
  return nxsGetPropName(key);
}

static std::string get_key_str(const std::vector<std::string_view> &key) {
  std::string str;
  bool first = true;
  for (const auto &k : key) {
    if (!first) {
      str += ".";
    }
    str += k;
    first = false;
  }
  return str;
}

static std::string get_key_str(const std::vector<nxs_int> &key) {
  std::string str;
  bool first = true;
  for (const auto &k : key) {
    if (!first) {
      str += ".";
    }
    str += std::string(nxsGetPropName(k));
    first = false;
  }
  return str;
}

template <typename T, typename Tobj>
static T get_prop(Tobj &self, const nxs_property prop) {
  if (prop != NXS_PROPERTY_INVALID) {
    auto pval = self.getProperty(prop);
    if (pval)
      return pval->template getValue<T>();
  }
  auto str = std::string(nxsGetPropName(prop));
  throw std::runtime_error("Invalid property: " + str); 
}


//////////////////////////////////////////////////////////////////////////
// Object class generation
template <typename T>
static py::class_<T> make_object_class(py::module &m, const std::string &name) {
  return py::class_<T>(m, name.c_str(), py::module_local())
      .def("__bool__", [](T &self) { return (bool)self; })
      .def("get_property_str",
           [](T &self, const std::string &name) {
             return get_prop<std::string>(self, nxsGetPropEnum(name.c_str()));
           })
      .def("get_property_str",
           [](T &self, nxs_property prop) {
             return get_prop<std::string>(self, prop);
           })
      .def("get_property_int",
           [](T &self, const std::string &name) {
             return get_prop<nxs_long>(self, nxsGetPropEnum(name.c_str()));
           })
      .def("get_property_int",
           [](T &self, nxs_property prop) {
             return get_prop<nxs_long>(self, prop);
           })
      .def("get_property_flt",
           [](T &self, const std::string &name) {
             return get_prop<nxs_double>(self, nxsGetPropEnum(name.c_str()));
           })
      .def("get_property_flt",
           [](T &self, nxs_property prop) {
             return get_prop<nxs_double>(self, prop);
           })
      .def("get_property_int_vec",
           [](T &self, const std::string &name) {
             return get_prop<std::vector<nxs_long>>(
                 self, nxsGetPropEnum(name.c_str()));
           })
      .def("get_property_int_vec",
           [](T &self, nxs_property prop) {
             return get_prop<std::vector<nxs_long>>(self, prop);
           })
      .def("get_property_keys", [](T &self) {
        auto keys = get_prop<std::vector<nxs_long>>(self, NP_Keys);
        std::vector<nxs_property> props;
        for (auto key : keys) {
          props.push_back((nxs_property)key);
        }
        return props;
      });
}

template <typename T>
static py::class_<Objects<T>> make_objects_class(py::module &m, const std::string &name) {
  return py::class_<Objects<T>>(m, name.c_str(), py::module_local())
      .def("__bool__", [](Objects<T> &self) { return (bool)self; })
      .def("__getitem__", [](Objects<T> &self, int idx) { return self.get(idx); })
      .def("__len__", [](Objects<T> &self) { return self.size(); })
      .def(
          "__iter__",
          [](const Objects<T> &rts) {
            return py::make_iterator(rts.begin(), rts.end());
          },
          py::keep_alive<0, 1>() /* Essential: keep object alive */)
      .def("size", [](Objects<T> &self) { return self.size(); });
}

static Command create_command(Schedule &schedule, Kernel kernel, std::vector<py::object> params,
             nxs_dim3 global_size, nxs_dim3 local_size, size_t shared_memory_size, nxs_uint settings) {
  auto cmd = schedule.createCommand(kernel);
  if (cmd) {
    int idx = 0;
    for (auto buf : params) {
      if (buf.is_none()) {
        auto none_buf = nexus::getSystem().createBuffer(0, nullptr, NXS_BufferSettings_OnDevice);
        cmd.setArgument(idx++, none_buf);
      } else if (PyLong_Check(buf.ptr())) {
        nxs_long value = PyLong_AsLong(buf.ptr());
        cmd.setArgument(idx++, value);
      } else if (PyFloat_Check(buf.ptr())) {
        nxs_double value = PyFloat_AsDouble(buf.ptr());
        cmd.setArgument(idx++, value);
      } else {
        auto buf_obj = make_buffer(buf);
        cmd.setArgument(idx++, buf_obj);
      }
    }
    if (global_size.x > 0 && local_size.x > 0) {
      cmd.finalize(global_size, local_size, shared_memory_size);
    }
  }
  return cmd;
}


//////////////////////////////////////////////////////////////////////////
// pynexus::init_system_bindings -- add bindings for system objects
// - this is the main entry point for the system module
void pynexus::init_system_bindings(py::module &m) {
  using ret = py::return_value_policy;
  using namespace pybind11::literals;

  //////////////////////////////////////////////////////////////////////////
  // Generate python enum for nxs_status
  // - added to `status` submodule for scoping
  auto mstatus = m.def_submodule("status");
  auto statusEnum =
      py::enum_<nxs_status>(mstatus, "nxs_status", py::module_local());
  for (nxs_int i = NXS_STATUS_MIN; i <= NXS_STATUS_MAX; ++i) {
    nxs_status status = (nxs_status)i;
    const char *str = nxsGetStatusName(i);
    if (str && *str) statusEnum.value(str, status);
  }
  statusEnum.export_values();

  //////////////////////////////////////////////////////////////////////////
  // Generate python enum for nxs_property
  // - added to `property` submodule for scoping
  auto mprop = m.def_submodule("property");

  auto propEnum =
      py::enum_<nxs_property>(mprop, "nxs_property", py::module_local());
  for (nxs_int i = 0; i <= NXS_PROPERTY_CNT; ++i) {
    nxs_property prop = (nxs_property)i;
    const char *str = nxsGetPropName(i);
    if (str && *str) propEnum.value(str, prop);
  }
  propEnum.export_values();

  mprop.def("get_count", []() { return NXS_PROPERTY_CNT; })
      .def("get_name", [](nxs_int prop) { return nxsGetPropName(prop); })
      .def("get_enum",
           [](const std::string &name) { return nxsGetPropEnum(name.c_str()); })
      .def("get",
           [](nxs_int prop) {
             if (prop < 0 || prop >= NXS_PROPERTY_CNT) {
               throw std::runtime_error("Invalid property");
             }
             return (nxs_property)prop;
           })
      .def("get_type", [](nxs_int prop) {
        if (prop < 0 || prop >= NXS_PROPERTY_CNT) {
          throw std::runtime_error("Invalid property");
        }
        switch (nxs_property_type_map[prop]) {
          case NPT_INT:
            return "int";
          case NPT_FLT:
            return "flt";
          case NPT_STR:
            return "str";
          case NPT_INT_VEC:
            return "int_vec";
          case NPT_FLT_VEC:
            return "flt_vec";
          case NPT_STR_VEC:
            return "str_vec";
          case NPT_OBJ_VEC:
            return "obj_vec";
          case NPT_UNK:
            return "unk";
          default:
            return "unk";
        }
      });

  //////////////////////////////////////////////////////////////////////////
  // Generate python enum for nxs_event_type
  // - added to `event_type` submodule for scoping
  auto meventType = m.def_submodule("event_type");
  auto eventTypeEnum = py::enum_<nxs_event_type>(meventType, "nxs_event_type", py::module_local());
  eventTypeEnum.value("Shared", NXS_EventType_Shared);
  eventTypeEnum.value("Signal", NXS_EventType_Signal);
  eventTypeEnum.value("Fence", NXS_EventType_Fence);
  eventTypeEnum.export_values();

  //////////////////////////////////////////////////////////////////////////
  // Add Nexus Object types and methods
  //////////////////////////////////////////////////////////////////////////

  // Properties Object
  py::class_<Info>(m, "_info", py::module_local())
      .def("__bool__", [](Info &self) { return (bool)self; })
      .def(
          "get",
          [](Info &self, const std::vector<std::string_view> &path) {
            if (auto node = self.getNode(path)) {
              return node->getJson();
            } else {
              throw std::runtime_error("Property not found: " +
                                       get_key_str(path));
            }
            return json::object();
          },
          py::arg("path") = std::vector<std::string_view>());

  make_object_class<Buffer>(m, "_buffer")
      .def("copy", [](Buffer &self, py::object tensor) {
        auto data_ptr = getPointer(tensor.ptr());
        if (!data_ptr.runtime_name.empty() && data_ptr.runtime_name != "cpu") {
          assert(0);
        }
        auto local = self.getLocal();
        if (data_ptr.ptr != nullptr && local.getData() != nullptr &&
            data_ptr.size == self.getSize()) {
          return local.copy(data_ptr.ptr);
        }
        return NXS_InvalidDevice;
      });
  make_objects_class<Buffer>(m, "_buffers");

  make_object_class<Kernel>(m, "_kernel").def("get_info", [](Kernel &self) {
    return self.getInfo();
  });
  make_objects_class<Kernel>(m, "_kernels");

  make_object_class<Library>(m, "_library")
      .def("get_info", [](Library &self) { return self.getInfo(); })
      .def("get_kernel",
           [](Library &self, const std::string &name) {
             return self.getKernel(name);
           })
      .def("get_kernels", [](Library &self) { return self.getKernels(); });

  make_object_class<Stream>(m, "_stream");
  make_object_class<Event>(m, "_event")
      .def("signal", [](Event &self, int signal_value) { return self.signal(signal_value); }, py::arg("signal_value") = 1)
      .def("wait", [](Event &self, int wait_value) { return self.wait(wait_value); }, py::arg("wait_value") = 1);

  make_object_class<Command>(m, "_command")
      .def("get_event", [](Command &self) { return self.getEvent(); })
      .def("get_kernel", [](Command &self) { return self.getKernel(); })
      .def("set_arg", [](Command &self, int index,
                         Buffer buf) { return self.setArgument(index, buf); })
      .def("set_arg",
           [](Command &self, int index, nxs_int value) {
             return self.setArgument(index, value);
           })
      .def("set_arg",
           [](Command &self, int index, nxs_uint value) {
             return self.setArgument(index, value);
           })
      .def("set_arg",
           [](Command &self, int index, nxs_long value) {
             return self.setArgument(index, value);
           })
      .def("set_arg",
           [](Command &self, int index, nxs_ulong value) {
             return self.setArgument(index, value);
           })
      .def("set_arg",
           [](Command &self, int index, nxs_float value) {
             return self.setArgument(index, value);
           })
      .def("set_arg",
           [](Command &self, int index, nxs_double value) {
             return self.setArgument(index, value);
           })
      .def("set_arg",
           [](Command &self, int index, py::object value) {
             if (value.is_none()) {
               auto none_buf = nexus::getSystem().createBuffer(0, nullptr, NXS_BufferSettings_OnDevice);
               return self.setArgument(index, none_buf);
             } else {
               return self.setArgument(index, make_buffer(value));
             }
           })
      .def("finalize", [](Command& self, py::list grid, py::list block, size_t shared_memory_size) {
          auto list_to_dim3 = [](const py::list& l) -> nxs_dim3 {
              nxs_uint x = l.size() > 0 ? l[0].cast<nxs_uint>() : 1;
              nxs_uint y = l.size() > 1 ? l[1].cast<nxs_uint>() : 1;
              nxs_uint z = l.size() > 2 ? l[2].cast<nxs_uint>() : 1;
              return nxs_dim3{ x, y, z };
          };
          return self.finalize(list_to_dim3(grid), list_to_dim3(block), shared_memory_size);
        }, py::arg("grid"), py::arg("block"), py::arg("shared_memory_size") = 0
      );

  make_objects_class<Command>(m, "_commands");

  make_object_class<Schedule>(m, "_schedule")
      .def(
          "create_command",
          [](Schedule &self, Kernel kernel, std::vector<Buffer> buffers,
             std::vector<nxs_dim3> dims) {
            auto cmd = self.createCommand(kernel);
            if (cmd) {
              int idx = 0;
              for (auto &buf : buffers) {
                cmd.setArgument(idx++, buf);
              }
              if (dims.size() == 2 && dims[0].x > 0 && dims[1].x > 0) {
                cmd.finalize(dims[0], dims[1], 0);
              }
            }
            return cmd;
          },
          py::arg("kernel"), py::arg("buffers") = std::vector<Buffer>(),
          py::arg("dims") = std::vector<nxs_dim3>())
      .def(
          "create_command",
          [](Schedule &self, Kernel kernel, std::vector<py::object> buffers,
             std::vector<nxs_dim3> dims) {
              nxs_dim3 global_size = dims.size() > 0 ? dims[0] : nxs_dim3{1, 1, 1};
              nxs_dim3 local_size = dims.size() > 1 ? dims[1] : nxs_dim3{1, 1, 1};
              size_t shared_memory_size = 0;
              nxs_uint settings = 0;
            auto cmd = create_command(self, kernel, buffers, global_size, local_size, shared_memory_size, settings);
            return cmd;
          },
          py::arg("kernel"), py::arg("buffers") = std::vector<py::object>(),
          py::arg("dims") = std::vector<int>())
      .def(
          "create_signal",
          [](Schedule &self, Event event, int signal_value) {
            return self.createSignalCommand(event, signal_value);
          },
          py::arg("event") = Event(), py::arg("signal_value") = 1)
      .def(
          "create_wait",
          [](Schedule &self, Event event, int wait_value) {
            return self.createWaitCommand(event, wait_value);
          },
          py::arg("event"), py::arg("wait_value") = 1)
      .def("get_commands", [](Schedule &self) { return self.getCommands(); })
      .def(
          "run",
          [](Schedule &self, Stream &stream, nxs_bool blocking) {
            return self.run(stream, blocking);
          },
          py::arg("stream") = Stream(), py::arg("blocking") = true);

  // Object Containers
  make_objects_class<Library>(m, "_libraries");
  make_objects_class<Schedule>(m, "_schedules");
  make_objects_class<Stream>(m, "_streams");
  make_objects_class<Event>(m, "_events");

  make_object_class<Device>(m, "_device")
      .def("get_info", [](Device &self) { return self.getInfo(); })
      .def("create_buffer",
           [](Device &self, py::object tensor) {
             auto devp = getPointer(tensor.ptr());
             bool on_device = false;
             if (devp.runtime_name.empty() || devp.device_id == -1) {
               // if it is this device, then on_device is true
               if (self.getId() == devp.device_id) {
                 on_device = true;
               } else {
                 // make copy
               }
             }
             return self.createBuffer(devp.size, devp.ptr, on_device);
           })
      .def("create_buffer",
           [](Device &self, size_t size) { return self.createBuffer(size); })
      .def("copy_buffer",
           [](Device &self, Buffer buf) { return self.copyBuffer(buf); })
      .def("get_buffers", [](Device &self) { return self.getBuffers(); })
      .def("load_library",
           [](Device &self, const char *data, size_t size) {
             return self.createLibrary((void *)data, size);
           })
      .def("load_library",
           [](Device &self, Info catalog, const std::string &libraryName) {
             return self.loadLibrary(catalog, libraryName);
           })
      .def("load_library",
           [](Device &self, const std::string &filepath) {
             return self.createLibrary(filepath);
           })
      .def("get_libraries", [](Device &self) { return self.getLibraries(); })
      .def(
          "create_event",
          [](Device &self, nxs_event_type event_type) {
            return self.createEvent(event_type);
          },
          py::arg("event_type") = NXS_EventType_Shared)
      .def("get_events", [](Device &self) { return self.getEvents(); })
      .def("create_stream", [](Device &self) { return self.createStream(); })
      .def("get_streams", [](Device &self) { return self.getStreams(); })
      .def("create_schedule",
           [](Device &self) { return self.createSchedule(); })
      .def("get_schedules", [](Device &self) { return self.getSchedules(); })
      .def(
        "create_command",
        [](Device &self, Kernel kernel, std::vector<py::object> buffers,
           std::vector<nxs_dim3> dims) {
            auto schedule = self.createSchedule();
            nxs_dim3 global_size = dims.size() > 0 ? dims[0] : nxs_dim3{1, 1, 1};
            nxs_dim3 local_size = dims.size() > 1 ? dims[1] : nxs_dim3{1, 1, 1};
            size_t shared_memory_size = 0;
            nxs_uint settings = 0;
            auto cmd = create_command(schedule, kernel, buffers, global_size, local_size, shared_memory_size, settings);
            return schedule.run();
        },
        py::arg("kernel"), py::arg("buffers") = std::vector<py::object>(),
        py::arg("dims") = std::vector<int>());

  make_objects_class<Device>(m, "_devices");
  make_objects_class<Runtime>(m, "_runtimes");

  make_object_class<Runtime>(m, "_runtime")
      .def("get_device",
           [](Runtime &self, nxs_int id) { return self.getDevice(id); })
      .def("get_devices", [](Runtime &self) { return self.getDevices(); });

  make_objects_class<Info>(m, "_infos");

  // query
  m.def("get_runtime", [](const std::string &name) { return nexus::getSystem().getRuntime(name); });
  m.def("get_runtimes", []() { return nexus::getSystem().getRuntimes(); });
  m.def("get_device_info", []() { return *nexus::getDeviceInfoDB(); });
  m.def("lookup_device_info",
        [](const std::string &name) { return nexus::lookupDeviceInfo(name); });

  m.def("load_catalog", [](const std::string &catalog_path) {
    return nexus::getSystem().loadCatalog(catalog_path);
  });
  m.def("get_catalogs", []() { return nexus::getSystem().getCatalogs(); });

  // create System Buffers
  m.def("create_buffer",
        [](size_t size) { return nexus::getSystem().createBuffer(size); });
  m.def("create_buffer", [](py::object tensor) { return make_buffer(tensor); });
}
