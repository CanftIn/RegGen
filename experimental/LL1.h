#ifndef REGGEN_EXPERIMENTAL_LL1_H
#define REGGEN_EXPERIMENTAL_LL1_H

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

// 产生式
class Prod {
  friend class LL1;

 private:
  std::string prod;                 // 产生式
  char noTerminal;                  // 产生式左部非终结符名字
  std::set<std::string> selection;  // 候选式集合
  std::set<char> Vn;                // 非终结符
  std::set<char> Vt;                // 终结符
  auto cut(int i, int j) -> std::string {
    return std::string(prod.begin() + i, prod.begin() + j);
  }
  friend auto operator==(const Prod& a, const char& c) -> bool {
    return a.noTerminal == c;
  }

  bool isValid;  // 产生式是否合法

 public:
  Prod(const std::string& in);
  // 分割终结符、非终结符、产生式集合、左部非终结符名字，返回分割是否成功
  auto split() -> bool;
};

class LL1 {
 private:
  std::vector<Prod> G;                             // 文法G
  std::set<char> VN;                               // 非终结符
  std::set<char> VT;                               // 终结符
  std::map<char, std::set<char> > FIRST;           // first集
  std::map<char, std::set<char> > FOLLOW;          // follow集
  std::map<std::pair<char, char>, std::string> M;  // 分析表
  auto first(const std::string& s) -> std::set<char>;
  void follow();
  std::vector<char> parse;   // 分析栈
  std::vector<char> indata;  // 输入表达式栈
  void parseTable();

 public:
  auto addProd(const Prod& prod) -> bool;  // 添加产生式
  void info();                             // 输出相关结果
  void tableInfo();                        // 输出表
  void build();                           // 建立first、follow集、分析表
  void showIndataStack();                 // 输出输入串内容
  void showParseStack();                  // 输出分析栈内容
  void loadIndata(const std::string& s);  // 输入串入栈
  void parser();                          // LL1预测分析
  void error(int step);                   // 错误处理
  void run();                             // 运行LL1
};

#endif