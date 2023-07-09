// LzyServer.cpp: 定义应用程序的入口点。
//

#include "LzyAsyncNetIo/LzyAsyncNetIo/header/LzyAsyncNet.h"
#include "LzyLog/LzyLog.hpp"
#include "LzyHttp/LzyHttp.hpp"
#include "LzyCoroutine/LzyCoroutine.hpp"

#include <atomic>
auto logger = Lzy::Log::Logger<Lzy::Log::Outter::Console>::get_instance();
using namespace std::literals::string_literals;

Lzy::Http::ResponseHead OKHEAD{ .version{1.1}, .statusCode{200}, .comment{"OK"} };
Lzy::Http::ResponseHead NOTFOUNDHEAD{ .version{1.1}, .statusCode{404}, .comment{"NOTFOUND"} };

Lzy::Coroutine::Task<bool> HTTP(SOCKET socket, std::span<char> buffer) {
    Lzy::Http::Request request;
    Lzy::Http::Response response;


    if (!request.parser({ buffer.begin(), buffer.end() })) {
        logger.info("not Http");
        co_return false;
    }
    response.headers = { "Access-Control-Allow-Origin: *", "Access-Control-Max-Age: 1728000" };
   

    std::filesystem::path path = std::filesystem::path("..\\HTML");
    path /= request.head.url.substr(1);
    if(std::filesystem::exists(path))
    {
        response.head = OKHEAD;
        std::ifstream file(path);
        response.content.resize(std::filesystem::file_size(path));
        file.read(response.content.data(), std::filesystem::file_size(path));
    }
    else {
        logger.info("error page:", path.string());
        response.head = NOTFOUNDHEAD;
    }
    
    auto response_buffer = response.to_string();
    if (auto error = co_await Lzy::Async::send(socket, response_buffer); error != 0) {
        co_return false;
    }
    co_return true;
}

Lzy::Coroutine::Task<> socket_listener(std::atomic<size_t>& numOfAcceptor) {
    using namespace std::literals::chrono_literals;
    logger.info("listening unit start");
    std::array<char, 952> buffer{};
    int res = 0;

    while (true)
    {
        numOfAcceptor++;
        auto socket = co_await Lzy::Async::accept();
        numOfAcceptor--;
        if (numOfAcceptor == 0) {
            logger.info("listening unit added");
            auto task = socket_listener(numOfAcceptor);
            task._handle.resume();
        }
        logger.info("socket =", socket);

        if (auto res = co_await Lzy::Async::recv(socket, buffer); res == -1) {
            break;
        }
        else {
            co_await Lzy::Coroutine::Join(HTTP(socket, buffer));
        }
       
        closesocket(socket);
    }

    logger.info("listening unit end");
    co_return;
}


int main()
{
    using namespace Lzy::Log::Formatter;
    logger["console"].first.level = Lzy::Log::Level::error | Lzy::Log::Level::info | Lzy::Log::Level::warning;
    logger.info("server started");
    Lzy::Async::NetSchdeler<int>::get_instance().run();

    std::atomic<size_t> numOfClient = 0;
    std::atomic<size_t> numOfAcceptor{0};

    auto task = socket_listener(numOfAcceptor);
    task._handle.resume();

    char a{};
    while (true) {
        a = getchar();
        if (a == 'q')
            return 0;
        if (a == 'h') {
            logger.info("Helper");
        }
        if (a == 'n') {
            logger.info("now accept = ", numOfAcceptor.load());
        }
    }

    return 0;
}
