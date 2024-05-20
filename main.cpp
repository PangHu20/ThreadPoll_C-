#include "thread_pool.h"

int main() {
    // Example usage of ThreadPool
    ThreadPool pool(4); // Create a thread pool with 4 worker threads
    pool.init(); // Initialize the pool

    // Submit tasks to the pool
    auto result1 = pool.submit([]() { return 1 + 2; });
    auto result2 = pool.submit([](int x, int y) { return x * y; }, 4, 5);

    // Get results from the tasks
    std::cout << "Result of task 1: " << result1.get() << std::endl;
    std::cout << "Result of task 2: " << result2.get() << std::endl;

    // Shutdown the pool
    pool.shutdown();

    return 0;
}
