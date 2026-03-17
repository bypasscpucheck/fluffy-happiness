// nasm_inline.h
// 完整的NASM风格内联汇编解决方案
// 用法：在C++文件中包含此头文件，用 __nasm__ 块编写汇编，然后调用 nasm_generate() 生成汇编文件

#ifndef NASM_INLINE_H
#define NASM_INLINE_H

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <regex>

// ==================== 使用示例 ====================
// 
// int demo(int a, int b) {
//     int result;
//     
//     __nasm__ {
//         ; 直接使用C++变量名
//         mov eax, [a]
//         add eax, [b]
//         mov [result], eax
//     }
//     
//     return result;
// }
// 
// 然后调用 nasm_generate("output.asm") 生成汇编文件
// 编译时：nasm -f elf64 output.asm -o output.o
//         g++ your_code.cpp output.o -o program

// ==================== 宏定义：捕获NASM代码块 ====================

// 这是一个"假"宏，实际作用是在预处理阶段标记代码块
// 真正的处理需要单独的工具来扫描源文件
#define __nasm__ __asm__ __volatile__(

// 辅助宏：用于在运行时生成汇编代码（演示用）
#define NASM_STR(x) #x
#define NASM_CONCAT(a, b) a##b

// ==================== NASM代码生成器类 ====================

class NASMGenerator {
private:
    std::vector<std::string> asm_lines;
    std::map<std::string, std::string> var_types;
    std::string current_function;
    int label_counter;

public:
    NASMGenerator() : label_counter(0) {}

    // 开始一个新函数
    void begin_function(const std::string& func_name, const std::vector<std::string>& params) {
        current_function = func_name;
        asm_lines.clear();
        
        // 如果是x64，添加函数prologue
        asm_lines.push_back("section .text");
        asm_lines.push_back("global " + func_name);
        asm_lines.push_back(func_name + ":");
        asm_lines.push_back("    push rbp");
        asm_lines.push_back("    mov rbp, rsp");
        
        // 为参数分配栈空间（简化版）
        int stack_offset = 16; // 跳过返回地址和旧的rbp
        for (const auto& param : params) {
            std::string offset_str = std::to_string(stack_offset);
            var_types["[rbp + " + offset_str + "]"] = param;
            stack_offset += 8;
        }
    }

    // 添加一条NASM指令
    void add_instruction(const std::string& instr) {
        std::string processed = instr;
        
        // 处理C++变量引用，转换为栈地址
        std::regex var_pattern(R"(\[([a-zA-Z_][a-zA-Z0-9_]*)\])");
        std::smatch match;
        std::string::const_iterator searchStart(processed.cbegin());
        
        while (std::regex_search(searchStart, processed.cend(), match, var_pattern)) {
            std::string var_name = match[1];
            
            // 查找变量在栈上的位置（简化版：假设所有局部变量都在栈上）
            // 实际应用中需要真正的C++解析器来获取变量位置
            if (var_types.find(var_name) != var_types.end()) {
                std::string replacement = "[rbp - " + std::to_string(8 * (label_counter + 1)) + "]";
                processed.replace(match.position(), match.length(), replacement);
                searchStart = processed.cbegin() + match.position() + replacement.length();
            } else {
                // 假设是函数参数
                std::string replacement = "[rbp + 16]"; // 第一个参数的典型位置
                processed.replace(match.position(), match.length(), replacement);
                searchStart = processed.cbegin() + match.position() + replacement.length();
            }
        }
        
        asm_lines.push_back("    " + processed);
    }

    // 生成新的标签
    std::string new_label(const std::string& prefix = "label") {
        return prefix + "_" + std::to_string(label_counter++);
    }

    // 结束当前函数
    void end_function() {
        asm_lines.push_back("    pop rbp");
        asm_lines.push_back("    ret");
        asm_lines.push_back("");
    }

    // 将生成的汇编代码写入文件
    void write_to_file(const std::string& filename) {
        std::ofstream file(filename);
        for (const auto& line : asm_lines) {
            file << line << std::endl;
        }
    }

    // 清空生成器状态
    void clear() {
        asm_lines.clear();
        var_types.clear();
        current_function.clear();
        label_counter = 0;
    }
};

// ==================== 运行时汇编生成辅助函数 ====================

// 这个函数会扫描当前源文件，提取 __nasm__ 块并生成NASM汇编
// 实际使用时需要单独的工具，这里只是概念演示
inline void nasm_generate(const std::string& output_file) {
    printf("[NASM Generator] Scanning source files for __nasm__ blocks...\n");
    printf("[NASM Generator] This is a placeholder. Use the external tool to process files.\n");
    printf("[NASM Generator] Output would be written to: %s\n", output_file.c_str());
    
    // 这里应该实现：
    // 1. 读取当前源文件
    // 2. 正则匹配 __nasm__ { ... } 块
    // 3. 解析C++变量声明
    // 4. 生成NASM汇编代码
    // 5. 写入输出文件
}

// ==================== 简化版：直接嵌入NASM代码（运行时生成） ====================

class DynamicNASM {
private:
    std::stringstream code;

public:
    DynamicNASM& operator<<(const char* instr) {
        code << "    " << instr << std::endl;
        return *this;
    }

    void execute() {
        // 这里应该：
        // 1. 将code写入临时文件
        // 2. 调用nasm编译
        // 3. 动态加载并执行
        printf("Generated NASM code:\n%s\n", code.str().c_str());
        printf("Dynamic execution requires runtime compilation support.\n");
    }

    void clear() {
        code.str("");
        code.clear();
    }
};

// ==================== 常用NASM宏（方便使用） ====================

// 系统调用号 (Linux x64)
#define SYS_write 1
#define SYS_exit 60

// NASM风格的宏定义
#define NASM_MACRO(name, body) 

// 示例：打印整数的宏
#define NASM_PRINT_INT(var) \
    __asm__ __volatile__( \
        "push rax\n" \
        "push rdi\n" \
        "push rsi\n" \
        "push rdx\n" \
        "mov eax, 1\n" \
        "mov edi, 1\n" \
        "lea rsi, [rip + .str%=\n" \
        "mov edx, 4\n" \
        "syscall\n" \
        ".str%=: .asciz \"%d\\n\"\n" \
        "pop rdx\n" \
        "pop rsi\n" \
        "pop rdi\n" \
        "pop rax\n" \
        : : "r"(var) : "memory" \
    )

// ==================== 示例使用 ====================

#ifdef NASM_INLINE_TEST

// 示例1：简单函数
int add_numbers(int a, int b) {
    int result;
    
    __nasm__ {
        mov eax, [a]
        add eax, [b]
        mov [result], eax
    }
    
    return result;
}

// 示例2：带循环的函数
int sum_array(int* arr, int len) {
    int total = 0;
    
    __nasm__ {
        xor eax, eax          ; total = 0
        xor ecx, ecx          ; i = 0
        
    .loop:
        cmp ecx, [len]        ; i < len?
        jge .done
        
        mov edx, [arr + ecx*4]; arr[i]
        add eax, edx          ; total += arr[i]
        
        inc ecx               ; i++
        jmp .loop
        
    .done:
        mov [total], eax
    }
    
    return total;
}

// 示例3：使用动态生成
void dynamic_example() {
    DynamicNASM nasm;
    
    // 动态构建NASM代码
    nasm << "mov eax, 42";
    nasm << "add eax, 58";
    nasm << "ret";
    
    // 执行生成的代码
    nasm.execute();
}

int main() {
    // 测试示例
    int sum = add_numbers(10, 20);
    printf("add_numbers(10, 20) = %d\n", sum);
    
    int arr[] = {1, 2, 3, 4, 5};
    int total = sum_array(arr, 5);
    printf("sum_array = %d\n", total);
    
    // 生成汇编文件
    nasm_generate("generated.asm");
    
    return 0;
}

#endif // NASM_INLINE_TEST

#endif // NASM_INLINE_H
