#include <iostream>
#include <coroutine>
#include <string>


struct GuessGame {
    struct promise_type {
        int current_guess = 0;
        int feedback = 0; 

        GuessGame get_return_object() {
            return { std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_always initial_suspend() { return std::suspend_always{}; }
        std::suspend_always final_suspend() noexcept { return std::suspend_always{}; }
        void unhandled_exception() { std::terminate(); }
        void return_void() {}

       
        std::suspend_always yield_value(int value) {
            current_guess = value;
            return {};
        }
    };

    std::coroutine_handle<promise_type> handle;

    GuessGame(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~GuessGame() { if (handle) handle.destroy(); }

    
    void set_feedback(int f) {
        handle.promise().feedback = f;
    }

    int get_current_guess() {
        return handle.promise().current_guess;
    }

    bool next() {
        handle.resume();
        return !handle.done();
    }
};


GuessGame guesser_coroutine() {
    int low = 1;
    int high = 100;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        co_yield mid;
    }
}


struct GetFeedback {
    int* feedback_ptr;
    bool await_ready() { return false; } 
    void await_suspend(std::coroutine_handle<GuessGame::promise_type> h) {
        feedback_ptr = &h.promise().feedback;
    }
    int await_resume() { return *feedback_ptr; }
};

GuessGame interactive_guesser() {
    int low = 1;
    int high = 100;
    int feedback = 0;

    while (low <= high) {
        int mid = low + (high - low) / 2;

        co_yield mid;

       
        feedback = co_await GetFeedback{};

        if (feedback == 0) co_return; 
        if (feedback == -1) { 
            low = mid + 1;
        }
        else if (feedback == 1) { 
            high = mid - 1;
        }
    }
}

int main() {
    setlocale(LC_ALL, "Ukrainian");

    std::cout << "--- Guess the number ---\n";
    std::cout << " from 1 to 100.\n";
    

    auto game = interactive_guesser();
    int attempt = 1;

    while (game.next()) {
        int current_guess = game.get_current_guess();
        std::cout << "[" << attempt << "] guess: " << current_guess << ". reaction: ";

        int user_response;
        while (!(std::cin >> user_response) || user_response < -1 || user_response > 1) {
            std::cout << "enter -1, 0 або 1: ";
            std::cin.clear();
            std::cin.ignore(10000, '\n');
        }

        if (user_response == 0) {
            std::cout << "\nfinale " << current_guess << " for " << attempt << " attemt(s).\n";
            break;
        }

        game.set_feedback(user_response);
        attempt++;
    }

    if (attempt > 7 && !std::cin.eof()) {
        std::cout << "\neror\n";
    }

    return 0;
}