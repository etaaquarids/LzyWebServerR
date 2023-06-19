// LzyServer.cpp: 定义应用程序的入口点。
//

#include "../LzyAsyncNetIo/LzyAsyncNetIo/header/LzyAsyncNet.h"
#include "../LzyLog/LzyLog.hpp"
#include <atomic>
auto logger = Lzy::Log::Logger<Lzy::Log::Outter::Console>::get_instance();

using namespace std::literals::string_literals;
struct Task {
    struct promise_type {
        promise_type() {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            logger.debug("promise_type create handle:", *(int*)&(handle));
        }
        promise_type(promise_type&& v) = delete;
        ~promise_type() {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            logger.debug("promise_type deleted handle:", *(int*)&(handle));
            
            logger.debug("func == nullptr:", func.operator bool());
        }
        std::function<void()> func{ [&]() { logger.debug("init_func"); } };
        std::suspend_always initial_suspend() {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            logger.debug("initial_suspend handle:", *(int*)&(handle));
            return {};
        };
        std::suspend_never final_suspend() noexcept {
            auto handle = std::coroutine_handle<promise_type>::from_promise(*this);
            logger.debug("final_suspend handle:", *(int*)&(handle));
            func();
            logger.debug("finnal resumed handle:", *(int*)&(handle));
            return {};
        };
        Task get_return_object() {
            return Task{ std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        void return_void() {
            logger.debug("return_void");
        }
        void unhandled_exception() {
            logger.debug("unhandled_exception");
        };
    };

    Task(std::coroutine_handle<promise_type> handle) {
        _handle = handle;
        logger.debug("task create handle:", *(int*)&(_handle));
    }
    Task(Task&& task):_handle(std::exchange(task._handle, {})) {
        logger.debug("task moved from handle:", *(int*)&(_handle));
        logger.debug("to handle:", *(int*)&(task._handle));
    }
   
    void then(std::function<void()>&& functor) {
        _handle.promise().func = std::move(functor);
    }
    std::coroutine_handle<promise_type> _handle;
};
struct Join {
    Join(Task&& task):task { std::move(task) } {
    }
    bool await_ready() {
        return false;
    }
    void await_suspend(std::coroutine_handle<> handle) {
        logger.debug("await_suspend handle:", *(int*)&handle);
        // handle is not ptr, pass by value
        task.then([handle]() {
            logger.debug("Join resume handle:", *(int*)&handle);
            handle.resume(); 
            });
        task._handle.resume();
    }
    void await_resume() {
        return;
    }
    Task task;
};
Task Echo(SOCKET socket, std::array<char, 952> buffer) {
    while (true) {
        
        if (auto error = co_await Lzy::Async::send(socket, buffer); error != 0) {
            break;
        }
        if (auto res = co_await Lzy::Async::recv(socket, buffer); res == -1) {
            break;
        }
        logger.info("recvd:", buffer.data());
        if (buffer.data()[0] == 'q') {
            break;
        }
    }
    //co_await Lzy::Async::recv(socket, buffer);
    co_return;
}

Task HTTP(SOCKET socket, std::span<char> buffer) {
   
    co_return;
}

Task socket_listener(std::atomic<size_t>& numOfAcceptor) {
    using namespace std::literals::chrono_literals;
    logger.info("listening unit start");
    std::array<char, 952> buffer{};
    int res = 0;

    while (true)
    {
        numOfAcceptor++;
        logger.info("now accept = ", numOfAcceptor);
        auto socket = co_await Lzy::Async::accept();
        numOfAcceptor--;
        logger.info("now accept = ", numOfAcceptor);
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
            logger.info("recvd:", buffer.data());
            co_await Join(HTTP(socket, buffer));
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
