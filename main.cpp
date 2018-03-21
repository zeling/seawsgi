#include <core/app-template.hh>
#include <core/reactor.hh>
#include <http/httpd.hh>
#include <Python.h>
#include "wsgi.hh"

namespace bpo = boost::program_options;

int main(int argc, char **argv) {
    Py_Initialize();
    PyEval_InitThreads();
    seastar::app_template app;
    app.add_options()
            ("help", "how help")
            ("module", bpo::value<std::string>()->default_value("app"), "the module that is intended as the web server")
            ("name", bpo::value<std::string>()->default_value(""), "script name for wsgi applications")
            ("port", bpo::value<uint16_t>()->default_value(8080), "port listening on")
            ("addr", bpo::value<std::string>()->default_value("0.0.0.0"), "address to bind to");

    auto *server = new seastar::http_server_control;
    app.run_deprecated(argc, argv, [&app, server] {
        auto &&conf = app.configuration();
        ipv4_addr bind_addr(conf["addr"].as<std::string>(), conf["port"].as<uint16_t>());
        server->start().then([server, &conf] {
            return seastar::make_ready_future<>();
//            return server->server().invoke_on_others([](http_server &) {
//                PyEval_InitThreads();
////                PyEval_ReleaseLock();
//            });
        }).then([server, &conf]{
            return server->set_routes([&conf](seastar::routes &r) {

                auto module = conf["module"].as<std::string>();
                auto script_path = conf["name"].as<std::string>();
                auto port_s = std::to_string(conf["port"].as<uint16_t>());

                using seastar::operation_type;
                std::array<operation_type, operation_type::NUM_OPERATION> ops = {
                        operation_type::GET,
                        operation_type::POST,
                        operation_type::PUT,
                        operation_type::DELETE,
                };

                for (auto op : ops) {
                    r.add(op, seastar::url(script_path), new wsgi_handler(module, script_path, port_s));
                }
            });
        }) .then([server, bind_addr] {
            return server->listen(bind_addr);
        }).then([server] {
            engine().at_exit([server] {
                return server->stop().then([server] {
                    delete server;
                    Py_Finalize();
                });
            });
        });
    });
}