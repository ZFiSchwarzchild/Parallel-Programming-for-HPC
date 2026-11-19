# 正确性验证（重点：不整除的情况）
# 行数选 250、1000、1024 三种：250 和 1000 对多数进程数不整除
for R in 250 350 512 1024; do
  echo "===== rows=$R ====="

  # 串行基线
  ./build/jacobi_seq input $R 512 out_seq_$R 1e-6

  # 8 进程（250 % 8 = 2，1000 % 8 = 0，1024 % 8 = 0）
  mpirun --mca btl self,vader --mca pml ob1 -np 8 \
    ./build/jacobi_1D_block input $R 512 out_p8_$R 1e-6

  python3 -c "
import numpy as np
a = np.loadtxt('out_seq_$R')
b = np.loadtxt('out_p8_$R')
diff = np.abs(a - b)
print(f'rows=$R: max abs diff = {diff.max():.3e}, elements > 1e-4 = {(diff > 1e-4).sum()}')
"
done

# 特别验证不整除时的负载均衡：rows=250, np=8 时，前 2 个进程各 32 行、后 6 个进程各 31 行。可以临时在代码里加一行：
# std::cout << "rank " << myId << " myRows = " << myRows << std::endl;