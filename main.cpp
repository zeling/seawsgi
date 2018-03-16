#include <core/app-template.hh>
#include <core/reactor.hh>
#include <http/httpd.hh>
#include <Python.h>
#include "wsgi.hh"

namespace bpo = boost::program_options;

int main(int argc, char **argv) {
    Py_Initialize();
    seastar::app_template app;
    app.add_options()
            ("help", "how help")
            ("module", bpo::value<std::string>()->default_value("app"), "the module that is intended as the web server")
            ("port", bpo::value<uint16_t>()->default_value(8080), "port listening on")
            ("addr", bpo::value<std::string>()->default_value("0.0.0.0"), "address to bind to");

    auto *server = new seastar::http_server_control;
    app.run_deprecated(argc, argv, [&app, server] {
        auto &&conf = app.configuration();
        std::string module = conf["module"].as<std::string>();
        ipv4_addr bind_addr(conf["addr"].as<std::string>(), conf["port"].as<uint16_t>());
        auto *hdl = new wsgi_handler(std::move(module));
        server->start().then([server, hdl]{
            return server->set_routes([hdl](seastar::routes &r) {
            r.add(seastar::operation_type::GET, seastar::url("/wsgi"), hdl);
          });
        }).then([server, bind_addr] {
            return server->listen(bind_addr);
        }).then([server, hdl] {
            engine().at_exit([server, hdl] {
                return server->stop().then([server, hdl] {
                    delete server;
                    delete hdl;
                });
            });
        });
    });
}