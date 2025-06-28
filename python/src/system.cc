#include <nexus-api.h>
#include <nexus.h>
#include <pybind11/stl.h>
#include <Python.h>

#include <iostream>

#include "pynexus.h"

namespace py = pybind11;

using namespace nexus;

struct DevPtr {
  char *ptr;
  size_t size;
};

static DevPtr getPointer(PyObject *obj) {
  DevPtr result = {nullptr, 0};
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
    result.ptr = (char *)PyLong_AsUnsignedLongLong(data_ret);
    result.size = PyLong_AsUnsignedLongLong(nbytes_ret);
    //Py_DECREF(data_ret);
    Py_DECREF(nbytes_ret);
  }
  return result;
}

#if 0
static DevPtr getPointer(py::object obj) {
    DevPtr result = { nullptr, 0 };
    if (obj.is_none()) {
        return result;
    }
    py::object data_ptr_m = obj.attr("data_ptr");
    py::object nbytes_ret = obj.attr("nbytes");
    if (!data_ptr_m.is_none() && !nbytes_res.is_none()) {
        py::object data_ret = data_ptr_m();
        if (!PyLong_Check(data_ret)) {
            PyErr_SetString(PyExc_TypeError, "data_ptr method of Pointer object must return 64-bit int");
            return result;
        }
        result.ptr = (void *)data_ret.cast<int64_t>();
        result.size = PyLong_AsUnsignedLongLong(nbytes_ret);
    }
    return result;
}
#endif

//////////////////////////////////////////////////////////////////////////
// Property key string conversion
static std::string get_key_str(const std::string &key) {
  return key;
}

static std::string get_key_str(const nxs_int &key) {
  return nxsGetPropName(key);
}

static std::string get_key_str(const std::vector<std::string> &key) {
  std::string str;
  for (const auto &k : key) {
    str += k + ".";
  }
  return str;
}

static std::string get_key_str(const std::vector<nxs_int> &key) {
  std::string str;
  for (const auto &k : key) {
    str += std::string(nxsGetPropName(k)) + ".";
  }
  return str;
}

template <typename T, typename Tkey>
static T get_info(Properties &self, const Tkey &key) {
  if (auto pval = self.getProperty(key))
    return pval->template getValue<T>();
  auto str = get_key_str(key);
  throw std::runtime_error("Property not found: " + str);
}

template <typename T, typename Tkey>
static std::vector<T> get_info_vec(Properties &self, const Tkey &key) {
  if (auto pval = self.getProperty(key))
    return pval->template getValueVec<T>();
  auto str = get_key_str(key);
  throw std::runtime_error("Property not found: " + str);
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
      .def("get_property_int", [](T &self, nxs_property prop) {
        return get_prop<nxs_long>(self, prop);
      })
      .def("get_property_flt",
           [](T &self, const std::string &name) {
             return get_prop<nxs_double>(self, nxsGetPropEnum(name.c_str()));
           })
      .def("get_property_flt", [](T &self, nxs_property prop) {
        return get_prop<nxs_double>(self, prop);
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

  //////////////////////////////////////////////////////////////////////////
  // Add Nexus Object types and methods
  //////////////////////////////////////////////////////////////////////////

  // Properties Object
  py::class_<Properties>(m, "_properties", py::module_local())
      .def("__bool__", [](Properties &self) { return (bool)self; })
      .def("get_str",
           [](Properties &self, const std::string &name) {
             return get_info<std::string>(self, name);
           })
      .def("get_str",
           [](Properties &self, nxs_property prop) {
             return self.getProp<std::string>(prop);
           })
      .def("get_int",
           [](Properties &self, const std::string &name) {
             return get_info<nxs_long>(self, name);
           })
      .def("get_int",
           [](Properties &self, nxs_property prop) {
             return self.getProp<nxs_long>(prop);
           })
      .def("get_str",
           [](Properties &self, const std::vector<std::string> &path) {
             return get_info<std::string>(self, path);
           })
      .def("get_str",
           [](Properties &self, const std::vector<nxs_int> &path) {
             return get_info<std::string>(self, path);
           })
      .def("get_int",
           [](Properties &self, const std::vector<std::string> &path) {
             return get_info<nxs_long>(self, path);
           })
      .def("get_int",
           [](Properties &self, const std::vector<nxs_int> &path) {
             return get_info<nxs_long>(self, path);
           })
      .def("get_str_vec",
           [](Properties &self, const std::vector<std::string> &path) {
             return get_info_vec<std::string>(self, path);
           })
      .def("get_str_vec",
           [](Properties &self, const std::vector<nxs_int> &path) {
             return get_info_vec<std::string>(self, path);
           });

  make_object_class<Buffer>(m, "_buffer")
      .def("copy", [](Buffer &self, py::object tensor) {
        auto local = self.getLocal();
        auto devp = getPointer(tensor.ptr());
        if (devp.ptr != nullptr && local.getData() != nullptr &&
            devp.size == self.getSize()) {
          return local.copy(devp.ptr);
        }
        return NXS_InvalidDevice;
      });

  make_object_class<Kernel>(m, "_kernel");

  make_object_class<Library>(m, "_library")
      .def("get_kernel", [](Library &self, const std::string &name) {
        return self.getKernel(name);
      });

  make_object_class<Command>(m, "_command")
      .def("set_buffer",
           [](Command &self, int index, Buffer buf) {
             return self.setArgument(index, buf);
           })
      .def("finalize", [](Command &self, int groupSize, int gridSize) {
        return self.finalize(groupSize, gridSize);
      });

  make_object_class<Schedule>(m, "_schedule")
      .def("create_command",
           [](Schedule &self, Kernel kernel) {
             return self.createCommand(kernel);
           })
      .def("run", [](Schedule &self) { return self.run(); });

  make_object_class<Stream>(m, "_stream");

  make_objects_class<Buffer>(m, "_buffers");
  make_objects_class<Kernel>(m, "_kernels");
  make_objects_class<Library>(m, "_libraries");
  make_objects_class<Command>(m, "_commands");
  make_objects_class<Schedule>(m, "_schedules");
  make_objects_class<Stream>(m, "_streams");

  make_object_class<Device>(m, "_device")
      .def("get_info", [](Device &self) { return self.getInfo(); })
      .def("create_buffer",
           [](Device &self, py::object tensor) {
             auto devp = getPointer(tensor.ptr());
             return self.createBuffer(devp.size, devp.ptr);
           })
      .def("create_buffer",
           [](Device &self, size_t size) { return self.createBuffer(size); })
      .def("copy_buffer",
           [](Device &self, Buffer buf) { return self.copyBuffer(buf); })
      .def("get_buffers", [](Device &self) { return self.getBuffers(); })
      .def("load_library",
           [](Device &self, const std::string &filepath) {
             return self.createLibrary(filepath);
           })
      .def("get_libraries", [](Device &self) { return self.getLibraries(); })
      .def("create_stream", [](Device &self) { return self.createStream(); })
      .def("get_streams", [](Device &self) { return self.getStreams(); })
      .def("create_schedule",
           [](Device &self) { return self.createSchedule(); })
      .def("get_schedules", [](Device &self) { return self.getSchedules(); });

  make_objects_class<Device>(m, "_devices");
  make_objects_class<Runtime>(m, "_runtimes");

  make_object_class<Runtime>(m, "_runtime")
      .def("get_device",
           [](Runtime &self, nxs_int id) { return self.getDevice(id); })
      .def("get_devices", [](Runtime &self) { return self.getDevices(); });

  // query
  m.def("get_runtimes", []() { return nexus::getSystem().getRuntimes(); });
  m.def("get_device_info", []() { return *nexus::getDeviceInfoDB(); });
  m.def("lookup_device_info",
        [](const std::string &name) { return nexus::lookupDeviceInfo(name); });

  // create System Buffers
  m.def("create_buffer",
        [](size_t size) { return nexus::getSystem().createBuffer(size); });
  m.def("create_buffer", [](py::object tens) {
    auto devp = getPointer(tens.ptr());
    return nexus::getSystem().createBuffer(devp.size, devp.ptr);
  });

}
