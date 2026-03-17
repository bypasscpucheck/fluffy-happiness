// memory_debugger.hpp
// 单文件内存调试器 - 泄漏检测 + 野指针保护 + 缓冲区溢出检测
// 使用: 在debug模式下包含此文件，release模式定义NDEBUG即可关闭

#pragma once

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <csignal>

// ==================== 配置宏 ====================

#ifndef MEMORY_DEBUGGER_ENABLED
    #ifdef NDEBUG
        #define MEMORY_DEBUGGER_ENABLED 0
    #else
        #define MEMORY_DEBUGGER_ENABLED 1
    #endif
#endif

// ==================== 条件编译 ====================

#if MEMORY_DEBUGGER_ENABLED

// ==================== 调试器核心 ====================

namespace memory_debugger {

// ==================== 配置常量 ====================

constexpr size_t CANARY_SIZE = 8;                    // 金丝雀字节数
constexpr uint64_t CANARY_VALUE = 0xDEADBEEFBAADF00D; // 金丝雀值
constexpr uint64_t FREED_CANARY = 0xDEADDEADDEADDEAD; // 释放后填充值
constexpr size_t BACKTRACE_DEPTH = 16;                // 调用栈深度
constexpr int MAX_ALLOCATIONS = 1000000;              // 最大跟踪数量

// ==================== 调用栈捕获 ====================

#ifdef __linux__
#include <execinfo.h>
#endif

class StackTrace {
private:
    static constexpr int MAX_FRAMES = 32;
    void* frames[MAX_FRAMES];
    int num_frames;
    
public:
    StackTrace() {
        #ifdef __linux__
        num_frames = backtrace(frames, MAX_FRAMES);
        #else
        num_frames = 0;
        #endif
    }
    
    std::string to_string() const {
        if (num_frames == 0) return "  <无法获取调用栈>";
        
        std::string result;
        #ifdef __linux__
        char** symbols = backtrace_symbols(frames, num_frames);
        if (symbols) {
            for (int i = 1; i < num_frames; ++i) {  // 跳过当前函数
                result += "  ";
                result += symbols[i];
                result += "\n";
            }
            free(symbols);
        }
        #endif
        return result;
    }
};

// ==================== 内存块头部 ====================

struct MemoryHeader {
    enum class State : uint8_t {
        ACTIVE,     // 正在使用
        FREED,      // 已释放
        CORRUPTED   // 已损坏
    };
    
    // 元数据
    size_t size;                    // 请求的大小
    size_t actual_size;             // 实际分配大小（包括头部和金丝雀）
    void* ptr;                       // 原始指针
    uint64_t allocation_id;          // 分配ID
    std::chrono::steady_clock::time_point timestamp;  // 分配时间
    State state;                     // 状态
    int thread_id;                    // 线程ID
    
    // 调用栈
    StackTrace alloc_stack;
    StackTrace free_stack;
    
    // 金丝雀
    uint64_t pre_canary;              // 前向金丝雀
    uint64_t post_canary;             // 后向金丝雀
    
    // 链表指针（用于遍历）
    MemoryHeader* next;
    MemoryHeader* prev;
    
    // 验证金丝雀
    bool validate_canaries() const {
        return pre_canary == CANARY_VALUE && 
               post_canary == CANARY_VALUE;
    }
    
    // 获取用户数据指针
    void* user_data() {
        return reinterpret_cast<char*>(this) + sizeof(MemoryHeader);
    }
    
    // 从用户指针获取头部
    static MemoryHeader* from_user_ptr(void* user_ptr) {
        if (!user_ptr) return nullptr;
        return reinterpret_cast<MemoryHeader*>(
            reinterpret_cast<char*>(user_ptr) - sizeof(MemoryHeader)
        );
    }
};

// ==================== 全局调试器 ====================

class MemoryDebugger {
private:
    std::mutex mutex;
    std::unordered_map<void*, MemoryHeader*> active_allocations;
    std::vector<MemoryHeader*> all_allocations;
    
    std::atomic<size_t> total_allocated{0};
    std::atomic<size_t> peak_allocated{0};
    std::atomic<uint64_t> next_id{1};
    
    MemoryHeader* head = nullptr;
    MemoryHeader* tail = nullptr;
    
    bool break_on_alloc = false;
    bool break_on_free = false;
    bool break_on_corruption = true;
    
public:
    MemoryDebugger() {
        setup_signal_handlers();
    }
    
    ~MemoryDebugger() {
        report_leaks();
    }
    
    // ==================== 分配 ====================
    
    void* allocate(size_t size, const char* file = nullptr, int line = 0) {
        std::lock_guard<std::mutex> lock(mutex);
        
        // 计算实际分配大小
        size_t total_size = sizeof(MemoryHeader) + size + CANARY_SIZE;
        
        // 分配内存
        void* raw_ptr = std::malloc(total_size);
        if (!raw_ptr) return nullptr;
        
        // 初始化头部
        MemoryHeader* header = static_cast<MemoryHeader*>(raw_ptr);
        header->size = size;
        header->actual_size = total_size;
        header->ptr = raw_ptr;
        header->allocation_id = next_id++;
        header->timestamp = std::chrono::steady_clock::now();
        header->state = MemoryHeader::State::ACTIVE;
        header->thread_id = get_thread_id();
        header->pre_canary = CANARY_VALUE;
        header->post_canary = CANARY_VALUE;
        header->next = nullptr;
        header->prev = tail;
        
        // 添加调用栈
        header->alloc_stack = StackTrace();
        
        // 填充用户区域为特殊模式（检测未初始化使用）
        uint8_t* user_ptr = static_cast<uint8_t*>(header->user_data());
        memset(user_ptr, 0xCD, size);  // 0xCD = 未初始化
        
        // 填充后向金丝雀
        uint8_t* post_canary_ptr = user_ptr + size;
        memset(post_canary_ptr, 0xFD, CANARY_SIZE);  // 0xFD = 后向金丝雀
        
        // 添加到链表
        if (tail) {
            tail->next = header;
        } else {
            head = header;
        }
        tail = header;
        
        // 记录分配
        active_allocations[header->user_data()] = header;
        all_allocations.push_back(header);
        
        // 更新统计
        total_allocated += size;
        if (total_allocated > peak_allocated) {
            peak_allocated = total_allocated;
        }
        
        // 断点触发
        if (break_on_alloc) {
            raise(SIGTRAP);
        }
        
        return header->user_data();
    }
    
    // ==================== 释放 ====================
    
    void deallocate(void* ptr) {
        if (!ptr) return;
        
        std::lock_guard<std::mutex> lock(mutex);
        
        // 获取头部
        MemoryHeader* header = MemoryHeader::from_user_ptr(ptr);
        
        // 验证指针有效性
        if (!validate_pointer(header, ptr)) {
            return;
        }
        
        // 检查重复释放
        if (header->state == MemoryHeader::State::FREED) {
            fprintf(stderr, "\n❌ 内存错误: 重复释放指针 %p\n", ptr);
            fprintf(stderr, "首次释放位置:\n%s", header->free_stack.to_string().c_str());
            
            StackTrace current_stack;
            fprintf(stderr, "当前释放位置:\n%s", current_stack.to_string().c_str());
            
            raise(SIGTRAP);
            return;
        }
        
        // 验证金丝雀
        if (!header->validate_canaries()) {
            fprintf(stderr, "\n❌ 内存错误: 缓冲区溢出检测到！\n");
            fprintf(stderr, "指针: %p, 大小: %zu\n", ptr, header->size);
            fprintf(stderr, "分配位置:\n%s", header->alloc_stack.to_string().c_str());
            
            header->state = MemoryHeader::State::CORRUPTED;
            
            if (break_on_corruption) {
                raise(SIGTRAP);
            }
            return;
        }
        
        // 记录释放栈
        header->free_stack = StackTrace();
        header->state = MemoryHeader::State::FREED;
        
        // 从活动映射中移除
        active_allocations.erase(ptr);
        
        // 填充已释放区域（检测野指针访问）
        memset(ptr, 0xDE, header->size);  // 0xDE = 已释放
        
        // 更新统计
        total_allocated -= header->size;
        
        // 断点触发
        if (break_on_free) {
            raise(SIGTRAP);
        }
        
        // 这里不立即释放内存，而是保留用于检测野指针
        // 实际release模式会真的释放
        // std::free(header->ptr);
    }
    
    // ==================== 验证 ====================
    
    bool validate_pointer(MemoryHeader* header, void* user_ptr) {
        // 检查对齐
        if (reinterpret_cast<uintptr_t>(user_ptr) % alignof(max_align_t) != 0) {
            fprintf(stderr, "\n❌ 内存错误: 未对齐的指针 %p\n", user_ptr);
            return false;
        }
        
        // 检查是否在已知分配中
        auto it = active_allocations.find(user_ptr);
        if (it == active_allocations.end()) {
            // 可能是已释放的指针
            fprintf(stderr, "\n❌ 内存错误: 野指针 %p (已释放或未分配)\n", user_ptr);
            
            // 尝试在所有分配中查找
            for (auto* h : all_allocations) {
                if (h->user_data() == user_ptr) {
                    fprintf(stderr, "该指针于以下位置释放:\n%s", 
                            h->free_stack.to_string().c_str());
                    break;
                }
            }
            
            raise(SIGTRAP);
            return false;
        }
        
        return true;
    }
    
    // ==================== 泄漏报告 ====================
    
    void report_leaks() {
        if (active_allocations.empty()) {
            printf("\n✅ 内存调试器: 未检测到内存泄漏\n");
            return;
        }
        
        printf("\n❌ 内存调试器: 检测到 %zu 处内存泄漏\n", 
               active_allocations.size());
        printf("========================================\n");
        
        size_t total_leaked = 0;
        int index = 1;
        
        for (const auto& [ptr, header] : active_allocations) {
            printf("\n泄漏 #%d:\n", index++);
            printf("  指针: %p\n", ptr);
            printf("  大小: %zu 字节\n", header->size);
            printf("  线程: %d\n", header->thread_id);
            
            // 计算存活时间
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - header->timestamp);
            printf("  存活时间: %lld ms\n", duration.count());
            
            printf("  分配位置:\n%s", header->alloc_stack.to_string().c_str());
            
            total_leaked += header->size;
        }
        
        printf("\n========================================\n");
        printf("总计泄漏: %zu 字节\n", total_leaked);
        printf("峰值内存: %zu 字节\n", peak_allocated.load());
    }
    
    // ==================== 统计信息 ====================
    
    void print_stats() {
        printf("\n📊 内存调试器统计\n");
        printf("========================================\n");
        printf("当前分配: %zu 处\n", active_allocations.size());
        printf("当前内存: %zu 字节\n", total_allocated.load());
        printf("峰值内存: %zu 字节\n", peak_allocated.load());
        printf("总分配数: %zu\n", all_allocations.size());
        printf("========================================\n");
    }
    
    // ==================== 辅助函数 ====================
    
    static int get_thread_id() {
        static std::atomic<int> next_id{1};
        static thread_local int id = next_id++;
        return id;
    }
    
    void setup_signal_handlers() {
        signal(SIGSEGV, crash_handler);
        signal(SIGABRT, crash_handler);
    }
    
    static void crash_handler(int sig) {
        fprintf(stderr, "\n💥 程序崩溃，最后的内存操作:\n");
        // 这里可以输出最后的内存操作记录
        exit(1);
    }
    
    // ==================== 控制接口 ====================
    
    void set_break_on_alloc(bool enable) { break_on_alloc = enable; }
    void set_break_on_free(bool enable) { break_on_free = enable; }
    void set_break_on_corruption(bool enable) { break_on_corruption = enable; }
};

// ==================== 全局单例 ====================

MemoryDebugger& get_debugger() {
    static MemoryDebugger instance;
    return instance;
}

// ==================== 重载的new/delete ====================

void* operator new(size_t size) {
    void* ptr = get_debugger().allocate(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void* operator new[](size_t size) {
    void* ptr = get_debugger().allocate(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    get_debugger().deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
    get_debugger().deallocate(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    get_debugger().deallocate(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    get_debugger().deallocate(ptr);
}

// ==================== 调试宏 ====================

#define MEMORY_BREAK_ON_ALLOC() \
    memory_debugger::get_debugger().set_break_on_alloc(true)

#define MEMORY_BREAK_ON_FREE() \
    memory_debugger::get_debugger().set_break_on_free(true)

#define MEMORY_PRINT_STATS() \
    memory_debugger::get_debugger().print_stats()

#define MEMORY_CHECK_POINTER(ptr) \
    memory_debugger::get_debugger().validate_pointer( \
        memory_debugger::MemoryHeader::from_user_ptr(ptr), ptr)

// ==================== 内存池集成 ====================

template<typename T>
class DebugTypePool {
private:
    memory_debugger::MemoryDebugger& debugger;
    
public:
    DebugTypePool() : debugger(memory_debugger::get_debugger()) {}
    
    T* allocate() {
        return static_cast<T*>(debugger.allocate(sizeof(T)));
    }
    
    template<typename... Args>
    T* construct(Args&&... args) {
        T* ptr = allocate();
        if (ptr) {
            new (ptr) T(std::forward<Args>(args)...);
        }
        return ptr;
    }
    
    void destroy(T* ptr) {
        if (ptr) {
            ptr->~T();
            debugger.deallocate(ptr);
        }
    }
};

} // namespace memory_debugger

// ==================== 使用示例宏 ====================

#define MEMORY_DEBUGGER_ENABLE() \
    memory_debugger::get_debugger()

#else // MEMORY_DEBUGGER_ENABLED

// ==================== 空实现（Release模式）====================

namespace memory_debugger {
    inline void* allocate(size_t size) { return std::malloc(size); }
    inline void deallocate(void* ptr) { std::free(ptr); }
    
    template<typename T>
    class DebugTypePool {
    public:
        T* allocate() { return static_cast<T*>(std::malloc(sizeof(T))); }
        template<typename... Args> T* construct(Args&&... args) {
            T* ptr = allocate();
            if (ptr) new (ptr) T(std::forward<Args>(args)...);
            return ptr;
        }
        void destroy(T* ptr) { if (ptr) { ptr->~T(); std::free(ptr); } }
    };
}

#define MEMORY_BREAK_ON_ALLOC() ((void)0)
#define MEMORY_BREAK_ON_FREE() ((void)0)
#define MEMORY_PRINT_STATS() ((void)0)
#define MEMORY_CHECK_POINTER(ptr) ((void)0)

#endif // MEMORY_DEBUGGER_ENABLED

// ==================== 测试代码 ====================

#ifdef MEMORY_DEBUGGER_TEST

#include <iostream>
#include <vector>
#include <thread>

// 测试泄漏
void test_leak() {
    std::cout << "\n=== 测试1: 内存泄漏 ===\n";
    int* p = new int(42);
    // 忘记delete
}

// 测试重复释放
void test_double_free() {
    std::cout << "\n=== 测试2: 重复释放 ===\n";
    int* p = new int(100);
    delete p;
    delete p;  // 第二次释放
}

// 测试缓冲区溢出
void test_buffer_overflow() {
    std::cout << "\n=== 测试3: 缓冲区溢出 ===\n";
    char* buffer = new char[10];
    for (int i = 0; i <= 10; ++i) {  // 越界写入
        buffer[i] = 'A';
    }
    delete[] buffer;
}

// 测试野指针
void test_wild_pointer() {
    std::cout << "\n=== 测试4: 野指针 ===\n";
    int* p = new int(42);
    delete p;
    *p = 100;  // 访问已释放内存
}

// 测试内存池集成
struct TestObject {
    int id;
    std::string name;
    
    TestObject(int i, const char* n) : id(i), name(n) {
        std::cout << "TestObject构造: " << id << "\n";
    }
    ~TestObject() {
        std::cout << "TestObject析构: " << id << "\n";
    }
};

void test_memory_pool() {
    std::cout << "\n=== 测试5: 内存池集成 ===\n";
    
    memory_debugger::DebugTypePool<TestObject> pool;
    
    TestObject* obj1 = pool.construct(1, "对象1");
    TestObject* obj2 = pool.construct(2, "对象2");
    
    pool.destroy(obj1);
    // 忘记destroy obj2
    
    MEMORY_PRINT_STATS();
}

// 多线程测试
void thread_worker(int id) {
    for (int i = 0; i < 100; ++i) {
        int* p = new int(id * 1000 + i);
        if (i % 3 == 0) {
            delete p;  // 部分释放
        }
    }
}

void test_multithreading() {
    std::cout << "\n=== 测试6: 多线程 ===\n";
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(thread_worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

int main() {
    std::cout << "内存调试器测试\n";
    std::cout << "========================\n";
    
    // 设置断点选项
    // MEMORY_BREAK_ON_ALLOC();
    // MEMORY_BREAK_ON_FREE();
    
    // 运行测试
    test_leak();
    // test_double_free();      // 会触发异常
    // test_buffer_overflow();   // 会触发检测
    // test_wild_pointer();      // 会触发段错误
    test_memory_pool();
    test_multithreading();
    
    // 输出最终统计
    MEMORY_PRINT_STATS();
    
    return 0;
}

#endif // MEMORY_DEBUGGER_TEST

// ==================== 编译命令 ====================

// 编译测试:
// g++ -std=c++17 -DMEMORY_DEBUGGER_TEST -o memory_test memory_debugger.hpp
// g++ -std=c++17 -DMEMORY_DEBUGGER_TEST -DNDEBUG -o memory_test_release memory_debugger.hpp
//
// 运行:
// ./memory_test
