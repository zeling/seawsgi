#pragma once

#include <http/httpd.hh>

using namespace seastar;
using namespace httpd;

class wsgi_handler final : public handler_base {

    std::string _module;

public:
    wsgi_handler(std::string module): _module(std::move(module)) {}
    virtual future<std::unique_ptr<reply>> handle(const sstring& path,
                                                   std::unique_ptr<request> req, std::unique_ptr<reply> rep) override;
};
