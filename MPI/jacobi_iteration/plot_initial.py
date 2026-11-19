import numpy as np
import matplotlib.pyplot as plt

rows, cols = 512, 512

# 用 numpy 向量化重现 C++ 公式
i = np.arange(rows).reshape(-1, 1)      # (rows, 1)
j = np.arange(cols).reshape(1, -1)      # (1, cols)
# 注意：C++ 里 i/121 是整数除法，numpy 里用 // 才等价
data = ((i // 121) + (j // 121)) % 2

print('shape:', data.shape)
print('unique values:', np.unique(data))
print('value counts:', np.bincount(data.astype(int).ravel()))

plt.figure(figsize=(7, 6))
plt.imshow(data, cmap='gray_r', origin='upper', aspect='equal',
           vmin=0, vmax=1, interpolation='nearest')
plt.colorbar(ticks=[0, 1], label='Value')
plt.title(f'Initial Matrix ({rows}x{cols})')
plt.xlabel('Column')
plt.ylabel('Row')
# 标出 121 处的分割线，直观看出块边界
for k in range(121, cols, 121):
    plt.axvline(k - 0.5, color='red', linewidth=0.5, alpha=0.5)
for k in range(121, rows, 121):
    plt.axhline(k - 0.5, color='red', linewidth=0.5, alpha=0.5)
plt.tight_layout()
plt.savefig('initial_matrix.png', dpi=150)
print('saved initial_matrix.png')
