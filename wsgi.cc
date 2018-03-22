#include "wsgi.hh"
#include <iostream>

struct wsgi_input {
    PyObject_HEAD
};

struct wsgi_error {
    PyObject_HEAD
};

static PyMethodDef wsgi_input_methods[] = {
//    { "read", &wsgi_input::read, METH_VARARGS, 0},
//    { "read", &wsgi_input::read, METH_VARARGS, 0},
//    { "read", &wsgi_input::read, METH_VARARGS, 0},
    { 0,      0,                 0,            0}
};

static void dealloc(PyObject *obj) {
    PyObject_Del(obj);
}

static PyTypeObject wsgi_input_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "seawsgi._input",                   /* tp_name              */
    sizeof(wsgi_input),                 /* tp_basicsize         */
    0,                                  /* tp_itemsize          */
    dealloc,                       /* tp_dealloc           */
    0,                                  /* tp_print             */
    0,                                  /* tp_getattr           */
    0,                                  /* tp_setattr           */
    0,                                  /* tp_compare           */
    0,                                  /* tp_repr              */
    0,                                  /* tp_as_number         */
    0,                                  /* tp_as_sequence       */
    0,                                  /* tp_as_mapping        */
    0,                                  /* tp_hash              */
    0,                                  /* tp_call              */
    0,                                  /* tp_str               */
    0,                                  /* tp_getattro          */
    0,                                  /* tp_setattro          */
    0,                                  /* tp_as_buffer         */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags             */
    "seawsgi input type",               /* tp_doc               */
    0,                                  /* tp_traverse          */
    0,                                  /* tp_clear             */
    0,                                  /* tp_richcompare       */
    0,                                  /* tp_weaklistoffset    */
    0,                                  /* tp_iter              */
    0,                                  /* tp_iternext          */
    wsgi_input_methods,                 /* tp_methods           */
    0,                                  /* tp_members           */
    0,                                  /* tp_getset            */
    0,                                  /* tp_base              */
    0,                                  /* tp_dict              */
    0,                                  /* tp_descr_get         */
    0,                                  /* tp_descr_set         */
    0,                                  /* tp_dictoffset        */
    0,                                  /* tp_init              */
    0,                                  /* tp_alloc             */
    0,                                  /* tp_new               */
    0,                                  /* tp_free              */
    0,                                  /* tp_is_gc             */
    0,                                  /* tp_bases             */
    0,                                  /* tp_mro - method resolution order */
    0,                                  /* tp_cache             */
    0,                                  /* tp_subclasses        */
    0,                                  /* tp_weaklist          */
    0,                                  /* tp_del               */
    0,                                  /* tp_version_tag       */
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION > 3
    0,                                  /* tp_finalize          */
#endif
};


future<std::unique_ptr<reply>> wsgi_handler::handle(const sstring &path,
                                                    std::unique_ptr<request> req, std::unique_ptr<reply> rep) {
    static sstring content_type("text/plain");
    static sstring body("hello");
    rep->set_status(reply::status_type::ok, "ok");
    rep->set_content_type(content_type);
    rep->write_body(content_type, body);
    auto env = build_environ(req.get());

    seastar_logger.debug("wsgi handling path {}", path);

    pyobj application = pyobj(PyObject_GetAttrString(_module.get(), "application"));

    if (!application.get()) {
        seastar_logger.error("no application found in module");
        engine().exit(1);
    }

    if (!PyCallable_Check(application.get())) {
        seastar_logger.error("the application of module is not callable");
        engine().exit(1);
    }

    pyobj args = pyobj(PyTuple_New(2));
    PyTuple_SetItem(args.get(), 0, env.get());
    PyTuple_SetItem(args.get(), 1, Py_True);
    PyObject_Call(application.get(), args.get(), NULL);

    return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
}

pyobj wsgi_handler::build_environ(request *req) {
    auto env = pyobj(PyDict_New());


    auto version = pyobj(Py_BuildValue("(ii)", 1, 0));
    PyDict_SetItemString(env.get(), "wsgi.version", version.get());

    PyDict_SetItemString(env.get(), "wsgi.multithread", Py_True);
    PyDict_SetItemString(env.get(), "wsgi.multiprocess", Py_False);
    PyDict_SetItemString(env.get(), "wsgi.run_once", Py_False);
    auto url_schema = pyobj(PyUnicode_FromString("http"));
    PyDict_SetItemString(env.get(), "wsgi.url_schema", url_schema.get());
    auto input = pyobj(PyObject_New(PyObject, &wsgi_input_type));
    PyDict_SetItemString(env.get(), "wsgi.input", input.get());

    pyobj method = pyobj(PyUnicode_FromString(req->_method.c_str()));
    PyDict_SetItemString(env.get(), "REQUEST_METHOD", method.get());

    pyobj script_path = pyobj(PyUnicode_FromString(_script_name.c_str()));
    PyDict_SetItemString(env.get(), "SCRIPT_PATH", script_path.get());

    auto url = req->_url;
    auto pos = url.find('?');
    pyobj path_info, query_string;
    for (size_t i = 0; i < _script_name.size(); i++) {
        if (_script_name[i] != url[i + 1]) {
            assert(false && "the initial part of the url should be the same as _script_name");
        }
    }
    if (pos != seastar::sstring::npos) {
        url.data()[pos] = '\0';
        path_info = pyobj(PyUnicode_FromString(url.data() + _script_name.size() + 1));
        query_string = pyobj(PyUnicode_FromString(url.data() + pos + 1));
    } else {
        path_info = pyobj(PyUnicode_FromString(url.data() + _script_name.size() + 1));
        query_string = pyobj(PyUnicode_FromString(""));
    }

    PyDict_SetItemString(env.get(), "PATH_INFO", path_info.get());
    PyDict_SetItemString(env.get(), "QUERY_STRING", query_string.get());

    auto ctype = req->get_header("Content-Type");
    if (!ctype.empty()) {
        pyobj content_type = pyobj(PyUnicode_FromString(ctype.c_str()));
        PyDict_SetItemString(env.get(), "Content-Type", content_type.get());
    }

    auto clength = req->get_header("Content-Length");
    if (!clength.empty()) {
        pyobj content_length = pyobj(PyUnicode_FromString(clength.c_str()));
        PyDict_SetItemString(env.get(), "Content-Length", content_length.get());
    }

    pyobj server_name = pyobj(PyUnicode_FromString("seawsgi"));
    PyDict_SetItemString(env.get(), "SERVER_NAME", server_name.get());

    pyobj server_port = pyobj(PyUnicode_FromString(_port.c_str()));
    PyDict_SetItemString(env.get(), "SERVER_PORT", server_port.get());

    pyobj server_protocol = pyobj(PyUnicode_FromString(req->_method.c_str()));
    PyDict_SetItemString(env.get(), "SERVER_PROTOCOL", server_protocol.get());

    for (auto &&e : req->_headers) {
        static constexpr const char CONTENT_TYPE[] = "CONTENT-TYPE";
        static constexpr const char CONTENT_LENGTH[] = "CONTENT-LENGTH";
        static constexpr const char HTTP_[] = "HTTP_";
        auto &field_name = e.first;
        if (field_name.size() == strlen(CONTENT_TYPE)) {
            bool found = true;
            for (size_t i = 0; i < field_name.size(); i++) {
                if (field_name[i] - CONTENT_TYPE[i] != 0 && field_name[i] - CONTENT_TYPE[i] != 'a' - 'A') {
                    found = false;
                    break;
                }
            }
            if (found) {
                pyobj content_type = pyobj(PyUnicode_FromString(e.second.c_str()));
                PyDict_SetItemString(env.get(), CONTENT_TYPE, content_type.get());
            }
        } else if (field_name.size() == strlen(CONTENT_LENGTH)) {
            bool found = true;
            for (size_t i = 0; i < field_name.size(); i++) {
                if (field_name[i] - CONTENT_LENGTH[i] != 0 && field_name[i] - CONTENT_LENGTH[i] != 'a' - 'A') {
                    found = false;
                    break;
                }
            }
            if (found) {
                pyobj content_length = pyobj(PyUnicode_FromString(e.second.c_str()));
                PyDict_SetItemString(env.get(), CONTENT_LENGTH, content_length.get());
            }
        } else {
            bool isHTTP_ = true;
            for (size_t i = 0; i < std::min(size_t(4), field_name.size()); i++) {
                if (field_name[i] - HTTP_[i] != 0 && field_name[i] - HTTP_[i] != 'a' - 'A') {
                    isHTTP_ = false;
                }
            }
            if (isHTTP_) {
                pyobj http_ = pyobj(PyUnicode_FromString(e.second.c_str()));
                PyDict_SetItemString(env.get(), e.first.c_str(), http_.get());
            }
        }
    }

    return std::move(env);
}
