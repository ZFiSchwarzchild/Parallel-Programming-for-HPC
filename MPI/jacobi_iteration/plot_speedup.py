import matplotlib.pyplot as plt

ns = [1, 2, 4, 8, 16]
t_baseline = 53.78
times = [48.70, 26.55, 15.17, 8.65, 8.89]
speedup = [t_baseline / t for t in times]

plt.figure(figsize=(8, 5))
plt.plot(ns, speedup, 'o-', label='Measured speedup', linewidth=2, markersize=8)
plt.plot(ns, ns, '--', color='gray', label='Ideal linear')
plt.axvline(x=8, color='red', linestyle=':', alpha=0.6, label='Physical cores = 8')
plt.xlabel('Number of MPI processes', fontsize=12)
plt.ylabel('Speedup', fontsize=12)
plt.title('Jacobi 512x512 (i7-10700, 8C/16T)', fontsize=13)
plt.legend(fontsize=11)
plt.grid(alpha=0.3)
plt.tight_layout()
plt.savefig('speedup.png', dpi=150)
print('saved speedup.png')