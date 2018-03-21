#include "wsgi.hh"


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
    return make_ready_future<std::unique_ptr<reply>>(std::move(rep));
}

pyobj wsgi_handler::build_environ(request *req) {
    auto env = pyobj(PyDict_New());


    auto version = pyobj(Py_BuildValue("(ii)", 1, 0));
    PyDict_SetItemString(env.get(), "wsgi.version", version.get());

    PyDict_SetItemString(env.get(), "wsgi.multithread", Py_False);
    PyDict_SetItemString(env.get(), "wsgi.multiprocess", Py_False);
    PyDict_SetItemString(env.get(), "wsgi.run_once", Py_False);
    auto url_schema = pyobj(PyBytes_FromString("http"));
    PyDict_SetItemString(env.get(), "wsgi.url_schema", url_schema.get());
    auto input = pyobj(PyObject_New(PyObject, &wsgi_input_type));
    PyDict_SetItemString(env.get(), "wsgi.input", input.get());

    pyobj method = pyobj(PyBytes_FromString(req->_method.c_str()));
    PyDict_SetItemString(env.get(), "REQUEST_METHOD", method.get());

    pyobj script_path = pyobj(PyBytes_FromString(_script_name.c_str()));
    PyDict_SetItemString(env.get(), "SCRIPT_PATH", script_path.get());

    return std::move(env);
}
