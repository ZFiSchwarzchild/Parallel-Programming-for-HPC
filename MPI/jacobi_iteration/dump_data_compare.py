import numpy as np
a = np.loadtxt('out_seq')
b = np.loadtxt('out_p8')
diff = np.abs(a - b)
print('max  abs diff =', diff.max())
print('mean abs diff =', diff.mean())
print('elements > 1e-4:', (diff > 1e-4).sum())