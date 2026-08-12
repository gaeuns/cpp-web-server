#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
    workers_.reserve(numThreads);   // 스레드 수만큼 메모리 공간 확보
    for (size_t i = 0; i < numThreads; ++i) {
        // 스레드들은 무한루프 함수로 들어가서 대기
        workers_.emplace_back(&ThreadPool::workerLoop, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

// 처리할 작업을 큐(tasks_)에 넣고 대기중인 스레드 하나를 깨움
void ThreadPool::enqueue(std::function<void()> task) {
    {
        // 큐 잠금(mutex) : 나만 건드릴 수 있게
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stop_) return;
        // 큐에 작업 추가
        tasks_.push(std::move(task));
    } // 중괄호 닫히면 자동으로 잠금 해제
    condition_.notify_one();    // 대기중인 스레드 중 하나만 깨움
}

// 각 워커 스레드는 workerLoop()를 돌면서 : 큐가 비어있으면 잠들어있다가(condition_variable),
// 작업이 들어오면 꺼내서 실행
void ThreadPool::workerLoop() {
    while (true) {
        std::function<void()> task;
        {
            // 큐 잠금
            std::unique_lock<std::mutex> lock(queueMutex_);
            // 작업이 들어올 때까지 대기
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) return;

            // 작업이 있으면 맨 앞 작업을 뽑아옴
            task = std::move(tasks_.front());
            tasks_.pop();
        } // 잠금 해제
        task(); // 작업 실행
    }
}

// 종료 신호를 보내고 모든 스레드가 끝날 때까지 기다림(join)
void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stop_) return;
        stop_ = true;
    }
    condition_.notify_all();    // 대기중인 스레드 전부 깨움

    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();   // 모든 스레드의 작업이 끝날 때까지 대기
    }
}
