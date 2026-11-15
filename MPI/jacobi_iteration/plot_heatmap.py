import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt('out_seq')

plt.figure(figsize=(8, 7))
plt.imshow(data, cmap='viridis', origin='upper', aspect='equal')
plt.colorbar(label='Value')
plt.title('Jacobi 512x512 (1 processes)')
plt.xlabel('Column')
plt.ylabel('Row')
plt.tight_layout()
plt.savefig('heatmap_seq.png', dpi=150)
print('saved heatmap_seq.png')