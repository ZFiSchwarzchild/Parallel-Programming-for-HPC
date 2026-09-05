CXX = g++
CC = g++          # 强制使用g++作为链接器
CXXFLAGS = -std=c++17 -O3 -mavx -mfma -msse3 -Wall -Wextra -Iinclude  #添加 -mfma
LDFLAGS = -lstdc++   # 显式链接C++标准库
DEPFLAGS = -MMD -MP

# 目录设置
BUILD_DIR = build
SRC_DIR = .
INC_DIR = include

# 如果有额外的源文件目录，可以添加：
# SRC_DIRS = .src

# 源文件
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
# 如果只想编译特定文件，可以用：
# SOURCES = matrix_matrix_mult.cpp
# 如果有多个源文件
# SOURCES = matrix_matrix_mult.cpp other.cpp
# SOURCES += $(wildcard $(SRC_DIRS)/*.cpp)

# 生成目标文件和依赖文件路径
OBJECTS = $(addprefix $(BUILD_DIR)/, $(notdir $(SOURCES:.cpp=.o)))
DEPS = $(OBJECTS:.o=.d)

# 最终目标
TARGET = $(BUILD_DIR)/matrix_matrix_mult

# 默认目标
all: $(BUILD_DIR) $(TARGET)

# 创建build目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 链接 - 使用g++而不是cc
$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS)  -o $@ $^ $(LDFLAGS)

# 编译每个.cpp文件到.o文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

# 运行
run: $(TARGET)
	@echo "运行程序..."
	@./$(TARGET)

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
	@echo "  Matrix Matrix Multiplication"
	@echo "====================================="
	@echo "可用目标:"
	@echo "  make          - 编译程序到 build/ 目录"
	@echo "  make run      - 编译并运行"
	@echo "  make debug    - 编译调试版本 (build/)"
	@echo "  make profile  - 编译性能分析版本 (build/)"
	@echo "  make clean    - 删除 build/ 目录"
	@echo "  make help     - 显示此帮助"
	@echo "====================================="
	@echo "构建产物位置: $(TARGET)"

.PHONY: all run clean debug profile help