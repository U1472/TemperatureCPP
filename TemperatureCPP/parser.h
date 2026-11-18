#pragma once

#include <cmath>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <unordered_map>

class Parser
{
private: std::unordered_map<std::string, int> prior = {
    {"(",    0},
    {"-",    1},
    {"+",    1},
    {"*",    2},
    {"/",    2},
    {"^",    3},
    {"~",    4},
    {"lg",   4},
    {"ln",   4},
    {"sqrt", 4}
    };

private: enum class OpCode {
    PUSH_NUM,
    PUSH_T,
    ADD, SUB, MUL, DIV, POW,
    NEG,
    LG, LN, SQRT
};

private: struct Instruction {
    OpCode op;
    double val = 0.0;
};

private: bool isFunction(const std::string & token) {
        return token == "lg" || token == "ln" || token == "sqrt";
    }

private: bool isPushesOut(const std::string& ejector, const std::string& ejected) {
        if (ejected == "(") return false;
        if (ejector == "^") {
            return prior[ejector] < prior[ejected];
        }
        return prior[ejector] <= prior[ejected];
    }

private: std::string readNumber(int& i, const std::string& input) {
        std::string val;
        bool hasDot = false;
        bool hasDigit = false;

        while (i < input.length() && (isdigit(static_cast<unsigned char>(input[i])) || input[i] == '.')) {
            if (input[i] == '.') {
                if (hasDot) {
                    throw std::invalid_argument("Некорректное число (несколько точек): " + val + ".");
                }
                hasDot = true;
            }
            else {
                hasDigit = true;
            }
            val += input[i];
            i++;
        }
        i--;

        if (!hasDigit) {
            throw std::invalid_argument("Некорректное число: одинокая точка");
        }

        return val;
    }

public: std::vector<std::string> shuntingYard(std::string input) {
        transform(input.begin(), input.end(), input.begin(), [](unsigned char c) {
            return tolower(c);
            });

        std::vector<std::string> output;
        std::vector<std::string> stack;
        bool isUnar = true; // true = ожидаем операнд (число, t, функцию, '(', унарный знак)

        for (int i = 0; i < input.length(); i++) {
            if (isspace(static_cast<unsigned char>(input[i]))) {
                continue;
            }

            if (isdigit(static_cast<unsigned char>(input[i])) || input[i] == '.') {
                if (!isUnar) {
                    throw std::invalid_argument("Пропущен оператор перед числом");
                }
                output.push_back(readNumber(i, input));
                isUnar = false;
            }
            else if (input[i] == 't') {
                if (!isUnar) {
                    throw std::invalid_argument("Пропущен оператор перед переменной t");
                }
                output.push_back("t");
                isUnar = false;
            }
            else if (input[i] == '(') {
                if (!isUnar) {
                    throw std::invalid_argument("Пропущен оператор перед '('");
                }
                stack.push_back("(");
                isUnar = true;
            }
            else if (input[i] == ')') {
                if (isUnar) {
                    // Поймает "()" и "(2 + )"
                    throw std::invalid_argument("Пустые скобки или оператор перед ')'");
                }

                bool foundedOpenBracket = false;
                while (!stack.empty() && !foundedOpenBracket) {
                    if (stack.back() == "(") {
                        foundedOpenBracket = true;
                    }
                    else {
                        output.push_back(stack.back());
                    }
                    stack.pop_back();
                }

                if (!foundedOpenBracket) {
                    throw std::invalid_argument("Пропущена открывающая скобка");
                }

                if (!stack.empty() && isFunction(stack.back())) {
                    output.push_back(stack.back());
                    stack.pop_back();
                }

                isUnar = false;
            }
            else {
                std::string val;

                if ((input[i] == '-' || input[i] == '+') && isUnar) {
                    if (input[i] == '-') {
                        stack.push_back("~");
                    }
                    // Если унарный '+', просто игнорируем его (например, "+5" -> "5")
                    isUnar = true;
                    continue;
                }
                else if (input.substr(i, 2) == "lg") {
                    val = "lg";
                    i += 1;
                }
                else if (input.substr(i, 2) == "ln") {
                    val = "ln";
                    i += 1;
                }
                else if (input.substr(i, 4) == "sqrt") {
                    val = "sqrt";
                    i += 3;
                }
                else {
                    val = std::string(1, input[i]);
                }

                if (prior.find(val) == prior.end()) {
                    throw std::invalid_argument("Неизвестный оператор или символ: " + val);
                }

                if (isFunction(val)) {
                    if (!isUnar) {
                        throw std::invalid_argument("Пропущен оператор перед функцией: " + val);
                    }
                    // Проверяем, что сразу после функции (с учетом пробелов) идет '('
                    int nextIdx = i + 1;
                    while (nextIdx < input.length() && isspace(static_cast<unsigned char>(input[nextIdx]))) {
                        nextIdx++;
                    }
                    if (nextIdx >= input.length() || input[nextIdx] != '(') {
                        throw std::invalid_argument("После функции ожидается '(': " + val);
                    }
                    stack.push_back(val);
                }
                else {
                    // Сюда попадают только бинарные операторы: +, -, *, /, ^
                    if (isUnar) {
                        // Поймает "* 2", "2 + * 3", "( * 2 )"
                        throw std::invalid_argument("Неожиданный бинарный оператор: " + val);
                    }
                    while (!stack.empty() && isPushesOut(val, stack.back())) {
                        output.push_back(stack.back());
                        stack.pop_back();
                    }
                    stack.push_back(val);
                }

                isUnar = true;
            }
        }

        if (isUnar) {
            // Поймает пустую строку, строку из пробелов и висячий оператор в конце ("2 +", "-")
            throw std::invalid_argument("Неполное или пустое выражение");
        }

        while (!stack.empty()) {
            if (stack.back() == "(") {
                throw std::invalid_argument("Присутствует незакрытая скобка");
            }
            output.push_back(stack.back());
            stack.pop_back();
        }

        return output;
    }

public: std::function<double(double)> compileRPN(const std::vector<std::string>& rpn) {
        std::vector<Instruction> code;
        code.reserve(rpn.size());

        for (const auto& token : rpn) {
            if (token == "t") {
                code.push_back({ OpCode::PUSH_T });
            }
            else if (token == "+") {
                code.push_back({ OpCode::ADD });
            }
            else if (token == "-") {
                code.push_back({ OpCode::SUB });
            }
            else if (token == "*") {
                code.push_back({ OpCode::MUL });
            }
            else if (token == "/") {
                code.push_back({ OpCode::DIV });
            }
            else if (token == "^") {
                code.push_back({ OpCode::POW });
            }
            else if (token == "~") {
                code.push_back({ OpCode::NEG });
            }
            else if (token == "lg") {
                code.push_back({ OpCode::LG });
            }
            else if (token == "ln") {
                code.push_back({ OpCode::LN });
            }
            else if (token == "sqrt") {
                code.push_back({ OpCode::SQRT });
            }
            else {
                code.push_back({ OpCode::PUSH_NUM, std::stod(token) });
            }
        }

        return [code = std::move(code)](double t) -> double {
            double stack[64];
            int sp = 0;

            for (const auto& instr : code) {
                switch (instr.op) {
                case OpCode::PUSH_NUM:
                    stack[sp++] = instr.val;
                    break;
                case OpCode::PUSH_T:
                    stack[sp++] = t;
                    break;
                case OpCode::ADD:
                    stack[sp - 2] += stack[sp - 1];
                    sp--;
                    break;
                case OpCode::SUB:
                    stack[sp - 2] -= stack[sp - 1];
                    sp--;
                    break;
                case OpCode::MUL:
                    stack[sp - 2] *= stack[sp - 1];
                    sp--;
                    break;
                case OpCode::DIV:
                    stack[sp - 2] /= stack[sp - 1];
                    sp--;
                    break;
                case OpCode::POW:
                    stack[sp - 2] = std::pow(stack[sp - 2], stack[sp - 1]);
                    sp--;
                    break;
                case OpCode::NEG:
                    stack[sp - 1] = -stack[sp - 1];
                    break;
                case OpCode::LG:
                    stack[sp - 1] = std::log10(stack[sp - 1]);
                    break;
                case OpCode::LN:
                    stack[sp - 1] = std::log(stack[sp - 1]);
                    break;
                case OpCode::SQRT:
                    stack[sp - 1] = std::sqrt(stack[sp - 1]);
                    break;
                }
            }
            return stack[0];
            };
    }

};

