// LzyServer.cpp: 定义应用程序的入口点。
//

#include "LzyAsyncNetIo/LzyAsyncNetIo/header/LzyAsyncNet.h"
#include "LzyLog/LzyLog.hpp"
#include "LzyHttp/LzyHttp.hpp"
#include "LzyCoroutine/LzyCoroutine.hpp"

#include <atomic>
auto logger = Lzy::Log::Logger<Lzy::Log::Outter::Console>::get_instance();
using namespace std::literals::string_literals;

Lzy::Coroutine::Task<bool> Echo(SOCKET socket, std::array<char, 952> buffer) {
    logger.info("Echo recvd:\n", buffer.data());
    while (true) {
        
        if (auto error = co_await Lzy::Async::send(socket, buffer); error != 0) {
            break;
        }
        if (auto res = co_await Lzy::Async::recv(socket, buffer); res == -1) {
            break;
        }
        //logger.info("recvd:\n", buffer.data());
        if (buffer.data()[0] == 'q') {
            break;
        }
    }
    //co_await Lzy::Async::recv(socket, buffer);
    co_return true;
}

Lzy::Coroutine::Task<bool> HTTP(SOCKET socket, std::span<char> buffer) {
    Lzy::Http::Request request;
    Lzy::Http::Response response;
    response.headers = { "Accept:*","" };
    if (!request.parser({ buffer.begin(), buffer.end() })) {
        logger.info("not Http");
        co_return false;
    }
    response.content = request.content;
    auto response_buffer = response.to_string();
    if (auto error = co_await Lzy::Async::send(socket, response_buffer); error != 0) {
        co_return false;
    }
    co_return true;
}

Lzy::Coroutine::Task<bool> RPC(SOCKET socket, std::span<char> buffer) {
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
        //logger.info("now accept = ", numOfAcceptor);
        auto socket = co_await Lzy::Async::accept();
        numOfAcceptor--;
        //logger.info("now accept = ", numOfAcceptor);
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
       
            std::vector<Lzy::Coroutine::Task<bool>> tasks;
            tasks.reserve(2);
            tasks.emplace_back(HTTP(socket, buffer));
            tasks.emplace_back(Echo(socket, buffer));
            for (bool finished; auto & task : tasks) {
                finished = co_await Lzy::Coroutine::Join(std::move(task));
                if (finished) break;
            }
              
        }
       
        closesocket(socket);
        logger.info("socket closed");
    }

    logger.info("listening unit end");
    co_return;
}


int main()
{
    using namespace Lzy::Log::Formatter;
    logger["console"].first.level = Lzy::Log::Level::error | Lzy::Log::Level::info | Lzy::Log::Level::error;
    logger.info("server started");
    Lzy::Async::NetSchdeler<int>::get_instance().run();
    //test();
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
