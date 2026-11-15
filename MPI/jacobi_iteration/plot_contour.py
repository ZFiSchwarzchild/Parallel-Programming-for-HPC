import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt('out_seq')
n = data.shape[0]

plt.figure(figsize=(8, 7))
x = np.arange(n)
y = np.arange(n)
X, Y = np.meshgrid(x, y)

cf = plt.contourf(X, Y, data, levels=20, cmap='viridis')
plt.colorbar(cf, label='Value')
plt.contour(X, Y, data, levels=10, colors='black', linewidths=0.5, alpha=0.5)

plt.title('Jacobi 512x512 Contour (1 processes)')
plt.xlabel('Column')
plt.ylabel('Row')
plt.gca().set_aspect('equal')
plt.tight_layout()
plt.savefig('contour_seq.png', dpi=150)
print('saved contour_seq.png')