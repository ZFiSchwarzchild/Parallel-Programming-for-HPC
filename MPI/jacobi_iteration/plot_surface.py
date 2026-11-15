import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

data = np.loadtxt('out_p8')
n = data.shape[0]

# 降采样（512x512 画 3D 太慢，每 4 个取 1 个）
stride = 4
d = data[::stride, ::stride]
m = d.shape[0]

X, Y = np.meshgrid(np.arange(m), np.arange(m))

fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')
surf = ax.plot_surface(X, Y, d, cmap='viridis', linewidth=0, antialiased=True)
fig.colorbar(surf, shrink=0.6, label='Value')
ax.set_title('Jacobi 512x512 Surface (8 processes)')
ax.set_xlabel('Column')
ax.set_ylabel('Row')
ax.set_zlabel('Value')
ax.view_init(elev=30, azim=-60)
plt.tight_layout()
plt.savefig('surface_p8.png', dpi=150)
print('saved surface_p8.png')
