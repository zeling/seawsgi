#pragma once

#include <http/httpd.hh>
#include <Python.h>

using namespace seastar;
using namespace httpd;

struct pyobj_deleter {
    void operator()(void *obj) {
        Py_XDECREF(obj);
    }
};

/* thing wrapper over vanilla PyObject * */
using pyobj = std::unique_ptr<PyObject, pyobj_deleter>;

class wsgi_handler final : public handler_base {
    std::string _module_name;
    std::string _script_name;
    std::string _port;
    pyobj _module;

    pyobj build_environ(request *req);

public:
    wsgi_handler(std::string module_name, std::string script_name, std::string port_s):
            _module_name(std::move(module_name)),
            _script_name(std::move(script_name)),
            _port(std::move(port_s))
//            _module(PyImport_ImportModule(_module_name.c_str())) {
    {}

    virtual future<std::unique_ptr<reply>> handle(const sstring &path,
                                                  std::unique_ptr<request> req, std::unique_ptr<reply> rep) override;
};
