#include <iostream>
#include <iomanip> // 引入 iomanip 以使用 std::setw

int main() {
    int number1 = 42;
    double number2 = 3.14159;
    
    // 设置宽度为10，对齐默认是右对齐
    std::cout << "Number 1: " << std::setw(10) << number1 << std::endl;
    
    // 设置宽度为10，输出浮点数
    std::cout << "Number 2: " << std::setw(10) << number2 << std::endl;
    
    // 不再使用 std::setw，输出宽度恢复默认
    std::cout << "No width setting: " << number2 << std::endl;

    double number3 = 3.14159;
    double number4 = 2.71828;

    // 使用 std::fixed 和 std::setprecision(2) 控制输出格式
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Formatted output: " << number3 << std::endl;

    // 恢复默认的输出格式
    std::cout.unsetf(std::ios::fixed);

    // 之后的输出不受影响，使用默认格式
    std::cout << "Default output: " << number4 << std::endl;

    return 0;
}
