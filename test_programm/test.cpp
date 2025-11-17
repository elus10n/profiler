#include <vector>
#include <cmath>
#include <random>
#include <algorithm>
#include <memory>
#include <thread>
#include <atomic>

class CPUStressTest {
private:
    std::vector<double> data;
    std::vector<int> int_data;
    std::mt19937 rng;
    size_t data_size;
    
public:
    CPUStressTest(size_t size = 1000000) : data_size(size), rng(std::random_device{}()) {
        data.resize(data_size);
        int_data.resize(data_size);
        initialize_data();
    }
    
    void initialize_data() {
        std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
        for (size_t i = 0; i < data_size; ++i) {
            data[i] = dist(rng);
            int_data[i] = static_cast<int>(data[i] * 100);
        }
    }
    
    // 1. Интенсивные математические вычисления
    void heavy_math_computations() {
        double sum = 0.0;
        for (size_t i = 0; i < data_size; ++i) {
            sum += std::sin(data[i]) * std::cos(data[i]) + 
                   std::log(std::abs(data[i]) + 1.0) * 
                   std::sqrt(std::abs(data[i]));
        }
        
        // Дополнительные тяжелые операции
        for (size_t i = 0; i < data_size / 2; ++i) {
            data[i] = std::pow(data[i], 2.5) + 
                      std::tan(data[i]) * std::atan(data[i]);
        }
    }
    
    // 2. Работа с памятью - частые аллокации/деаллокации
    void memory_intensive_operations() {
        for (int cycle = 0; cycle < 100; ++cycle) {
            std::vector<std::vector<double>> matrix(1000);
            for (auto& row : matrix) {
                row.resize(500);
                std::fill(row.begin(), row.end(), 1.0);
            }
            
            // Симуляция работы с матрицей
            for (int i = 0; i < 100; ++i) {
                for (auto& row : matrix) {
                    for (auto& elem : row) {
                        elem *= 1.0001;
                    }
                }
            }
        }
    }
    
    // 3. Сортировки и алгоритмическая нагрузка
    void algorithmic_stress() {
        // Многократные сортировки
        for (int i = 0; i < 50; ++i) {
            std::vector<int> temp = int_data;
            std::sort(temp.begin(), temp.end());
            std::shuffle(temp.begin(), temp.end(), rng);
            std::partial_sort(temp.begin(), temp.begin() + temp.size()/2, temp.end());
        }
        
        // Поисковые операции
        int search_count = 0;
        std::uniform_int_distribution<int> dist(0, data_size - 1);
        for (int i = 0; i < 100000; ++i) {
            int target = int_data[dist(rng)];
            if (std::find(int_data.begin(), int_data.end(), target) != int_data.end()) {
                ++search_count;
            }
        }
    }
    
    // 4. Работа со строками и копирования
    void string_manipulation_stress() {
        std::vector<std::string> strings(10000);
        for (auto& str : strings) {
            str.resize(100, 'A');
        }
        
        for (int cycle = 0; cycle < 100; ++cycle) {
            // Модификация строк
            for (auto& str : strings) {
                for (char& c : str) {
                    c = (c + 1) % 128;
                }
                std::reverse(str.begin(), str.end());
            }
            
            // Создание новых строк (конкатенация)
            std::vector<std::string> new_strings;
            for (size_t i = 0; i < strings.size() - 1; i += 2) {
                new_strings.push_back(strings[i] + strings[i + 1]);
            }
        }
    }
    
    // 5. Рекурсивные вычисления
    double recursive_computation(int depth, double x) {
        if (depth <= 0) return x;
        double result = 0.0;
        for (int i = 0; i < 5; ++i) {
            result += recursive_computation(depth - 1, x + i) / (i + 1);
        }
        return std::sin(result);
    }
    
    void run_recursive_stress() {
        double total = 0.0;
        for (int i = 0; i < 100; ++i) {
            total += recursive_computation(12, static_cast<double>(i) * 0.1);
        }
    }
    
    // 6. Параллельная нагрузка
    void parallel_stress() {
        std::vector<std::thread> threads;
        std::atomic<int> counter{0};
        
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([this, &counter]() {
                for (int i = 0; i < 1000; ++i) {
                    heavy_math_computations();
                    counter.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }
    
    // Главный метод - запускает все виды нагрузки
    void run_all_stress_tests() {
        while (true) {
            heavy_math_computations();
            memory_intensive_operations();
            algorithmic_stress();
            string_manipulation_stress();
            run_recursive_stress();
            parallel_stress();
            
            // Переинициализация данных для разнообразия
            initialize_data();
        }
    }
};

int main() {
    // Создаем большую нагрузку
    CPUStressTest stress_test(2000000);
    
    // Бесконечный цикл нагрузки
    stress_test.run_all_stress_tests();
    
    return 0;
}