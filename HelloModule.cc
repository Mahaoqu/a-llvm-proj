#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

// int max(int val1, int val2) {
//     int c;

//     if (val1 > val2) {
//         c = val1;
//     } else {
//         c = val2;
//     }

//     return c;
// }

using namespace llvm;

static std::unique_ptr<LLVMContext> my_Context;
static std::unique_ptr<Module> my_Module;
static std::unique_ptr<IRBuilder<>> my_Builder;

static void InitializeModule() {
  my_Context = std::make_unique<LLVMContext>();
  my_Module = std::make_unique<Module>("IR_function", *my_Context);
  my_Builder = std::make_unique<IRBuilder<>>(*my_Context);

  my_Module->setDataLayout(
      R"(e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128)");

  my_Module->setTargetTriple(R"(x86_64-pc-linux-gnu)");
}

Function *createFunction(Type *Ret_type, ArrayRef<Type *> Params,
                         std::string Name, bool isVarArg = false) {
  // 参数列表 int ->(int int) 不是可变参数
  auto func_Type = FunctionType::get(Ret_type, Params, isVarArg);

  auto func = Function::Create(func_Type, Function::ExternalLinkage, Name,
                               my_Module.get());

  return func;
}

void setFuncArgs(Function *Func, SmallVector<std::string> FuncArgs) {
  int idx = 0;
  for (auto it = Func->arg_begin(); it != Func->arg_end(); ++it, ++idx) {
    it->setName(FuncArgs[idx]);
  }
}

void createBasicBlock(Function *func) {
  // 创建基本块
  auto entry = BasicBlock::Create(*my_Context, "entry", func);
  // then else 基本快
  auto then_Build = BasicBlock::Create(*my_Context, "then", func);
  auto else_Build = BasicBlock::Create(*my_Context, "else", func);
  // ret 基本块
  auto ret_Build = BasicBlock::Create(*my_Context, "ret", func);

  my_Builder->SetInsertPoint(entry);
  // 局部变量c
  auto c = my_Builder->CreateAlloca(my_Builder->getInt32Ty(), nullptr, "c");
  auto cmp_ret = my_Builder->CreateICmpSLT(func->getArg(0), func->getArg(1));
  my_Builder->CreateCondBr(cmp_ret, then_Build, else_Build);

  my_Builder->SetInsertPoint(then_Build);
  my_Builder->CreateStore(func->getArg(0), c);
  my_Builder->CreateBr(ret_Build);

  my_Builder->SetInsertPoint(else_Build);
  my_Builder->CreateStore(func->getArg(1), c);
  my_Builder->CreateBr(ret_Build);

  // 创建返回值
  my_Builder->SetInsertPoint(ret_Build);
  auto c_var = my_Builder->CreateLoad(my_Builder->getInt32Ty(), c);
  my_Builder->CreateRet(c_var);
}

int main(int argc, char *argv[]) {
  // 初始化
  InitializeModule();

  auto my_int32_t = my_Builder->getInt32Ty();

  // 参数类型
  SmallVector<Type *, 2> Args;

  for (int i = 0; i < 2; i++) {
    Args.push_back(my_int32_t);
  }
  // 创建函数
  auto func = createFunction(my_int32_t, Args, "max");

  // 设置参数名
  SmallVector<std::string> param{"val1", "val2"};
  setFuncArgs(func, param);

  // 创建基本快
  createBasicBlock(func);

  // 校验
  verifyFunction(*func);

  my_Module->print(outs(), nullptr);

  return 0;
}
