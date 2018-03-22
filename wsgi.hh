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
    std::string _script_name;
    std::string _port;
    pyobj _module;

    pyobj build_environ(request *req);

public:
    wsgi_handler(std::string script_name, std::string port_s, PyObject *module):
            _script_name(std::move(script_name)),
            _port(std::move(port_s))
    {
        Py_IncRef(module);
        _module.reset(module);
    }

    virtual future<std::unique_ptr<reply>> handle(const sstring &path,
                                                  std::unique_ptr<request> req, std::unique_ptr<reply> rep) override;
};
