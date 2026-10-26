CXX = g++
CC = g++          # 强制使用g++作为链接器
CXXFLAGS = -std=c++17 -O3 -pthread -Wl,--no-as-needed -mavx -mfma -msse3 -Wall -Wextra -Iinclude -latomic #添加 -mfma
LDFLAGS = -lstdc++   # 显式链接C++标准库
DEPFLAGS = -MMD -MP

OPENMP ?= 1
ifeq ($(OPENMP),1)	
CXXFLAGS += -fopenmp
$(info OpenMP support enabled)
endif

# 目录设置
BUILD_DIR = build
SRC_DIR = .
INC_DIR = include
# 如果有额外的源文件目录，可以添加：
# SRC_DIRS = .src

# 1. 扫描所有 .cpp 文件 -> 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

# 2. 为每个 .cpp 生成对应的目标可执行文件路径
TARGET = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.cpp=))) #(例如 traditional.cpp -> build/traditional)

# 生成目标文件和依赖文件路径
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.cpp=.o)))
DEPS = $(OBJECTS:.o=.d)

# 默认目标：编译所有源文件
all: $(BUILD_DIR) $(TARGET)

# 创建build目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 3. 链接规则：每个可执行文件只依赖它自己的 .o 文件 (利用 % 模式匹配)
$(BUILD_DIR)/%: $(BUILD_DIR)/%.o 
	@echo "正在链接: $< -> $@"
	$(CXX) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# 4. 编译规则：每个 .cpp 编译成对应的 .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "正在编译: $< -> $@"
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# 运行指定的程序 (默认运行 traditional，也可以 make run EXEC=promise_future)
EXEC ?= traditional
run: $(BUILD_DIR)/$(EXEC)
	@echo "运行程序: $(EXEC)..."
	@./$(BUILD_DIR)/$(EXEC)

# 清理
clean:
	@echo "清理构建目录..."
	@rm -rf $(BUILD_DIR)

# 调试版本
debug: CXXFLAGS += -g -O0 -DDEBUG
debug: clean $(TARGET)

# 带性能分析的版本
profile:CXXFLAGS += -pg
profile: clean $(TARGET)

# 包含依赖文件
-include $(DEPS)

# 帮助信息
help:
	@echo "====================================="
	@echo "traditional makefile for C++ project"
	@echo "====================================="
	@echo "可用目标:"
	@echo "  make                - 编译所有 .cpp 到 build/ 目录"
	@echo "  make run            - 编译并运行 build/traditional"
	@echo "  make run EXEC=xxx   - 运行指定的程序 (如: make run EXEC=promise_future)"
	@echo "  make debug          - 编译调试版本"
	@echo "  make profile        - 编译性能分析版本"
	@echo "  make clean          - 删除 build/ 目录"
	@echo "====================================="
	@echo "当前可编译的程序: $(notdir $(SOURCES:.cpp=))"

.PHONY: all run clean debug profile help