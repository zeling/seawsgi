#include <core/app-template.hh>
#include <core/reactor.hh>
#include <iostream>
#include <Python.h>

int main(int argc, char **argv) {
    Py_Initialize();
    seastar::app_template app;
    app.run(argc, argv, [] {
        std::cout << "Hello world\n";
        return seastar::make_ready_future<>();
    });
}