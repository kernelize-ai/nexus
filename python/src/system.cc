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
  char *ptr = nullptr;
  Shape shape;
  std::string runtime_name;
  nxs_int device_id = -1;
  nxs_data_type dtype = NXS_DataType_Undefined;
};

static DevPtr getPointer(PyObject *obj) {
  DevPtr result;
  if (obj == Py_None || PyLong_Check(obj) || PyFloat_Check(obj)) {
    return result;
  }
  PyObject *data_ptr_m = PyObject_GetAttrString(obj, "data_ptr");
  if (data_ptr_m == nullptr) {
    data_ptr_m = PyObject_GetAttrString(obj, "tobytes");
  }
  PyObject *nbytes_ret = PyObject_GetAttrString(obj, "nbytes");
  if (data_ptr_m && nbytes_ret) {
    PyObject *data_ret = PyObject_CallNoArgs(data_ptr_m);
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
    // get element type
    PyObject *dtype_tuple = PyTuple_New(1);
    PyTuple_SetItem(dtype_tuple, 0, obj);
    Py_INCREF(obj); // is this necessary? crashes without it
    PyObject *nexus_module = PyImport_ImportModule("nexus");
    PyObject *get_data_type_func = PyObject_GetAttrString(nexus_module, "get_data_type");
    PyObject *dtype_ret = PyObject_CallObject(get_data_type_func, dtype_tuple);
    Py_DECREF(nexus_module);
    Py_DECREF(get_data_type_func);
    Py_DECREF(dtype_tuple);
    if (dtype_ret) {
      result.dtype = (nxs_data_type)PyLong_AsLong(dtype_ret);
      Py_DECREF(dtype_ret);
    } else {
      PyErr_Print();
      throw std::runtime_error("Failed to get data type");
    }

    result.ptr = (char *)PyLong_AsUnsignedLongLong(data_ret);
    PyObject *shape_m = PyObject_GetAttrString(obj, "shape");
    if (shape_m) {
      PyObject *builtins = PyImport_ImportModule("builtins");
      PyObject *func = PyObject_GetAttrString(builtins, "len");
      PyObject *shape_len = PyObject_CallFunction(func, "(s)", shape_m);
      Py_DECREF(func);
      assert(shape_len);
      nxs_shape _shape;
      _shape.rank = PyLong_AsUnsignedLongLong(shape_len);
      assert(_shape.rank < NXS_MAX_DIMS);
      for (nxs_int i = 0; i < _shape.rank; i++) {
        PyObject *index = PyLong_FromLong(i);
        PyObject *item = PyObject_GetItem(shape_m, index);  
        Py_DECREF(index);
        _shape.dims[i] = PyLong_AsUnsignedLongLong(item);
        Py_DECREF(item);
      }
      result.shape = Shape(_shape);
      Py_DECREF(shape_len);
      Py_DECREF(shape_m);
    } else {
      result.shape = Shape(PyLong_AsUnsignedLongLong(nbytes_ret));
    }
    Py_DECREF(data_ret);
    Py_DECREF(nbytes_ret);
  }
  return result;
}

PyObject *import_from(const char *module_name, const char *var_name) {
  py::object var = py::module_::import(module_name).attr(var_name);
  return var.release().ptr();
}


static Buffer make_buffer(py::object tensor, Device device = Device()) {
  // TODO: track ownership of the py::object tensor (release on destruction of
  // Buffer)
  static auto nexus_buffer = import_from("nexus", "buffer");
  if (PyObject_IsInstance(tensor.ptr(), nexus_buffer)) {
    // TODO: check for matching device
    auto buffer = tensor.cast<Buffer>();
    if (device && device != buffer.getParentImpl()) {
      return device.copyBuffer(buffer);
    }
    return buffer;
  }

  auto data_ptr = getPointer(tensor.ptr());
  nxs_uint settings = data_ptr.dtype;
  if (data_ptr.shape.getRank() == 0) { // is size 0 legal?
    return Buffer();
  }
  if (!data_ptr.runtime_name.empty() && data_ptr.device_id != -1) {
    auto buffer_runtime = nexus::getSystem().getRuntime(data_ptr.runtime_name);
    if (buffer_runtime) {
      auto dp_device = buffer_runtime.getDevice(data_ptr.device_id);
      if (!dp_device) {
        throw std::runtime_error("Device not found: " + std::string(data_ptr.runtime_name) + " " + std::to_string(data_ptr.device_id));
      }
      auto buf = dp_device.createBuffer(data_ptr.shape, data_ptr.ptr, settings | NXS_BufferSettings_OnDevice);
      if (device && device != dp_device) {
        return device.copyBuffer(buf);
      }
      return buf;
    }
    return Buffer();
  }
  if (device) {
    return device.createBuffer(data_ptr.shape, data_ptr.ptr, settings);
  }
  return nexus::getSystem().createBuffer(data_ptr.shape, data_ptr.ptr, settings);
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

static nxs_status set_argument(Command &self, int index, py::object value,
                               const char *name = "", nxs_data_type data_type = NXS_DataType_Undefined,
                               bool is_const = false) {
  nxs_uint settings = data_type | (is_const ? NXS_CommandArgType_Constant : 0);
  if (py::isinstance<Buffer>(value)) {
    auto buf = value.cast<Buffer>();
    return self.setArgument(index, buf, name, settings);
  }
  else if (Buffer buffer = make_buffer(value)) {
    return self.setArgument(index, buffer, name, settings);
  }
  // Test for bool (check before int, since bool is subclass of int in Python)
  else if (py::isinstance<py::bool_>(value)) {
    bool val = value.cast<bool>();
    return self.setArgument(index, val, name, settings);
  }
  // Test for int
  else if (py::isinstance<py::int_>(value)) {
    nxs_int val = value.cast<nxs_int>();
    if (data_type == NXS_DataType_Undefined)
      settings |= NXS_DataType_I32;
    return self.setArgument(index, val, name, settings);
  }
  // Test for float
  else if (py::isinstance<py::float_>(value)) {
    nxs_float val = value.cast<nxs_float>();
    if (data_type == NXS_DataType_Undefined)
      settings |= NXS_DataType_F32;
    return self.setArgument(index, val, name, settings);
  }
  else if (value.is_none()) {
    auto none_buf = nexus::getSystem().createBuffer(0, nullptr, NXS_BufferSettings_OnDevice);
    return self.setArgument(index, none_buf, name, settings);
  }
  return NXS_InvalidArgValue;
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
  // Generate python enum for nxs_data_type
  // - added to `data_type` submodule for scoping
  // - TODO: confirm match with torch dtype names
  auto mdataType = m.def_submodule("data_type");
  auto dataTypeEnum = py::enum_<nxs_data_type>(mdataType, "nxs_data_type", py::module_local());
  dataTypeEnum.value("undefined", NXS_DataType_Undefined);
  dataTypeEnum.value("float32", NXS_DataType_F32);
  dataTypeEnum.value("float16", NXS_DataType_F16);
  dataTypeEnum.value("bfloat16", NXS_DataType_BF16);
  dataTypeEnum.value("float8", NXS_DataType_F8); // 
  dataTypeEnum.value("bfloat8", NXS_DataType_BF8);
  dataTypeEnum.value("float4", NXS_DataType_F4);
  dataTypeEnum.value("bfloat4", NXS_DataType_BF4);
  dataTypeEnum.value("int32", NXS_DataType_I32);
  dataTypeEnum.value("uint32", NXS_DataType_U32);
  dataTypeEnum.value("int16", NXS_DataType_I16);
  dataTypeEnum.value("uint16", NXS_DataType_U16);
  dataTypeEnum.value("int8", NXS_DataType_I8);
  dataTypeEnum.value("uint8", NXS_DataType_U8);
  dataTypeEnum.value("int4", NXS_DataType_I4);
  dataTypeEnum.value("uint4", NXS_DataType_U4);
  dataTypeEnum.value("float64", NXS_DataType_F64);
  dataTypeEnum.value("int64", NXS_DataType_I64);
  dataTypeEnum.value("uint64", NXS_DataType_U64);
  dataTypeEnum.value("bool", NXS_DataType_Bool);
  dataTypeEnum.export_values();

  //////////////////////////////////////////////////////////////////////////
  // Add Nexus Object types and methods
  //////////////////////////////////////////////////////////////////////////

  // Properties Object
  py::class_<Info>(m, "info", py::module_local())
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

    py::class_<Shape>(m, "shape", py::module_local())
      .def("rank", [](Shape &self) { return self.getRank(); })
      .def_property_readonly("rank", [](Shape &self) { return self.getRank(); })
      .def("dim", [](Shape &self, size_t index) { return self.getDim(index); })
      //.def("stride", [](Shape &self, size_t index) { return self.getStride(index); })
      .def("numel", [](Shape &self) { return self.getNumElements(); })
      .def_property_readonly("numel", [](Shape &self) { return self.getNumElements(); });
    
    make_object_class<Buffer>(m, "buffer")
      .def("shape", [](Buffer &self) { return self.getShape(); })
      .def("numel", [](Buffer &self) { return self.getNumElements(); })
      .def("element_size", [](Buffer &self) { return self.getElementSizeBits(); })
      .def("data_type", [](Buffer &self) { return self.getDataType(); })
      .def_property_readonly("size_bytes", [](Buffer &self) { return self.getSizeBytes(); })
      .def_property_readonly("nbytes", [](Buffer &self) { return self.getSizeBytes(); })
      .def_property_readonly("dtype", [](Buffer &self) { return self.getDataType(); })
      .def("data_ptr", [](Buffer &self) -> intptr_t { return reinterpret_cast<intptr_t>(self.getData()); }) // TODO: get Device pointer
      .def("copy", [](Buffer &self, py::object tensor) {
        auto data_ptr = getPointer(tensor.ptr());
        if (!data_ptr.runtime_name.empty() && data_ptr.runtime_name != "cpu") {
          assert(0);
        }
        return self.copy(data_ptr.ptr);
      });
  make_objects_class<Buffer>(m, "buffers");

  make_object_class<Kernel>(m, "kernel").def("get_info", [](Kernel &self) {
    return self.getInfo();
  });
  make_objects_class<Kernel>(m, "kernels");

  make_object_class<Library>(m, "library")
      .def("get_info", [](Library &self) { return self.getInfo(); })
      .def("get_kernel",
           [](Library &self, const std::string &name) {
             return self.getKernel(name);
           })
      .def("get_kernels", [](Library &self) { return self.getKernels(); });

  make_object_class<Stream>(m, "stream");
  make_object_class<Event>(m, "event")
      .def("signal", [](Event &self, int signal_value) { return self.signal(signal_value); }, py::arg("signal_value") = 1)
      .def("wait", [](Event &self, int wait_value) { return self.wait(wait_value); }, py::arg("wait_value") = 1);

  make_object_class<Command>(m, "command")
      .def("get_event", [](Command &self) { return self.getEvent(); })
      .def("get_kernel", [](Command &self) { return self.getKernel(); })
      .def("set_arg", [](Command &self, int index, py::object value, const char *name, nxs_data_type data_type) -> nxs_status {
          return set_argument(self, index, value, name, data_type, false);
        }, py::arg("index"), py::arg("value"), py::arg("name") = "", py::arg("data_type") = NXS_DataType_Undefined)
      .def("set_const", [](Command &self, int index, py::object value, const char *name, nxs_data_type data_type) -> nxs_status {
          return set_argument(self, index, value, name, data_type, true);
        }, py::arg("index"), py::arg("value"), py::arg("name") = "", py::arg("data_type") = NXS_DataType_Undefined)
      .def("finalize", [](Command& self, py::list grid, py::list block, size_t shared_memory_size) {
          auto list_to_dim3 = [](const py::list& l) -> nxs_dim3 {
              nxs_uint x = l.size() > 0 ? l[0].cast<nxs_uint>() : 1;
              nxs_uint y = l.size() > 1 ? l[1].cast<nxs_uint>() : 1;
              nxs_uint z = l.size() > 2 ? l[2].cast<nxs_uint>() : 1;
              return nxs_dim3{ x, y, z };
          };
          return self.finalize(list_to_dim3(grid), list_to_dim3(block), shared_memory_size);
        }, py::arg("grid"), py::arg("block") = py::list{}, py::arg("shared_memory_size") = 0
      );

  make_objects_class<Command>(m, "commands");

  make_object_class<Schedule>(m, "schedule")
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
            auto cmd = self.createCommand(kernel);
            if (cmd) {
              int idx = 0;
              for (auto buf : buffers) {
                set_argument(cmd, idx++, buf);
              }
              if (dims.size() == 2 && dims[0].x > 0 && dims[1].x > 0) {
                cmd.finalize(dims[0], dims[1], 0);
              }
            }
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
  make_objects_class<Library>(m, "librarys");
  make_objects_class<Schedule>(m, "schedules");
  make_objects_class<Stream>(m, "streams");
  make_objects_class<Event>(m, "events");

  make_object_class<Device>(m, "device")
      .def("get_info", [](Device &self) { return self.getInfo(); })
      .def("create_buffer",
           [](Device &self, py::object tensor) {
             return make_buffer(tensor, self);
           })
      .def("create_buffer",
           [](Device &self, std::vector<size_t> shape, nxs_uint settings) {
             Shape s(shape.data(), shape.size());
             return self.createBuffer(s, nullptr, settings);
          }, py::arg("shape"), py::arg("settings") = 0)
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
      .def("get_schedules", [](Device &self) { return self.getSchedules(); });

  make_objects_class<Device>(m, "devices");
  make_objects_class<Runtime>(m, "runtimes");

  make_object_class<Runtime>(m, "runtime")
      .def("get_device",
           [](Runtime &self, nxs_int id) { return self.getDevice(id); })
      .def("get_devices", [](Runtime &self) { return self.getDevices(); });

  make_objects_class<Info>(m, "infos");

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
