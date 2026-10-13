#####################################################################
# run __ONE__ of the following commands:
#
# pip install --user tensorflow (if you have no CUDA-enabled GPU)
# pip install --user tensorflow-gpu
#
# Numpy should come bundled with tensorflow. Run this file et voila!
#####################################################################

import array as ar
import numpy as np

# everyone has tensorflow installed nowadays :D
# from tensorflow.examples.tutorials.mnist import input_data
# mnist = input_data.read_data_sets('MNIST_data', one_hot=False)
# merge = np.vstack(( mnist.train.images, mnist.test.images))

# 1. 引入现代 TensorFlow
# 替换为以下新代码（TF 2.x 标准写法）
import tensorflow as tf

# 2. 加载 MNIST 数据集 (替代已废弃的 input_data.read_data_sets)
# 返回的图片数据形状为 (60000, 28, 28) 和 (10000, 28, 28)
# 加载 MNIST 数据集
(train_images, train_labels), (test_images, test_labels) = tf.keras.datasets.mnist.load_data()

# 3. 【核心修改】数据类型转换与归一化
# Keras 默认加载的是 0-255 的 uint8 类型
# 而原版代码通过 input_data 加载会自动转为 0-1 之间的 float32
# 为了保证 C++ 代码计算距离时数值正确，这里必须手动除以 255.0
# ⚠️ 极其重要：数据归一化
# tf.keras 加载的图片是 0-255 的 uint8 类型，而原代码 input_data 会自动归一化为 0-1
# 你的 C++ 代码期望的是 float32 类型，所以这里必须手动除以 255.0
# 归一化 (0-1) 并转为 float32
train_images = train_images.astype('float32') / 255.0
test_images = test_images.astype('float32') / 255.0

# 4. 将训练集和测试集垂直堆叠，总共 70000 张图片
# 合并
merge = np.vstack((train_images, test_images))

# 5. 【核心修改】修改输出文件 "./data/mnist_65000_28_28_32.bin" 保持一致
# 写入二进制文件
with open("mnist_65000_28_28_32.bin", "wb") as f:
    f.write(ar.array("f", merge.flatten()))

print("数据导出成功！文件已保存为 mnist_65000_28_28_32.bin")