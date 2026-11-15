import numpy as np
import matplotlib.pyplot as plt

a = np.loadtxt('out_seq')      # 串行
b = np.loadtxt('out_p8')       # 8 进程
diff = np.abs(a - b)

fig, axes = plt.subplots(1, 3, figsize=(18, 6))

# 串行
im0 = axes[0].imshow(a, cmap='viridis', origin='upper', aspect='equal')
axes[0].set_title('Sequential')
plt.colorbar(im0, ax=axes[0], fraction=0.046)

# 并行
im1 = axes[1].imshow(b, cmap='viridis', origin='upper', aspect='equal')
axes[1].set_title('MPI 8 processes')
plt.colorbar(im1, ax=axes[1], fraction=0.046)

# 差异（用对数色标，1e-6 量级的差也能看清分布）
im2 = axes[2].imshow(diff, cmap='hot', origin='upper', aspect='equal')
axes[2].set_title(f'|diff|  max = {diff.max():.2e}')
plt.colorbar(im2, ax=axes[2], fraction=0.046)

for ax in axes:
    ax.set_xlabel('Column')
    ax.set_ylabel('Row')

plt.tight_layout()
plt.savefig('compare.png', dpi=150)
print(f'saved compare.png')
print(f'max abs diff = {diff.max():.3e}')