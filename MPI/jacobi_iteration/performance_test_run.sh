# 性能测试——与旧版本对比
# jacobi_1D_block_simple_modify（旧版本，整除 + swap + Allreduce 降频），核心对比是：
# 性能差异：Scatterv/Gatherv 的启动开销 vs Scatter/Gather
# 不整除时的可运行性：旧版本 rows=250, np=8 直接 abort，新版本能跑

# 固定 MCA 参数，避免代理/WSL 干扰
MPIRUN="mpirun --use-hwthread-cpus --mca btl self,vader --mca pml ob1"

# MPIRUN= "mpirun \                       # ① OpenMPI 启动器
#         --use-hwthread-cpus \           # ② 按硬件线程数算槽位（允许超过物理核）等价写法：--oversubscribe
#         --mca btl self,vader \          # ③ 只用共享内存传输层，禁用 TCP
#         --mca pml ob1"                  # ④ 用最基础的点对点协议层


# 串行基线
./build/jacobi_seq input 512 512 out_seq 1e-6

# 新版本（Scatterv/Gatherv）
for np in 1 2 4 8 16; do
  echo "--- block(Scatterv) np=$np ---"
  $MPIRUN -np $np ./build/jacobi_1D_block input 512 512 out_v2_np$np 1e-6  2>&1 | tee log_v2_np8.txt
done

# 旧版本（Scatter/Gather）对照
for np in 1 2 4 8 16; do
  echo "--- simple_modify(Scatter) np=$np ---"
  $MPIRUN -np $np ./build/jacobi_1D_block_simple_modify input 512 512 out_v1_np$np 1e-6 2>&1 | tee log_v1_np8.txt
done

# 两个版本都会打印时间，用 tee 保存：
# $MPIRUN -np 8 ./build/jacobi_1D_block input 512 512 out_v2_np8 1e-6 2>&1 | tee log_v2_np8.txt
# 从日志1里提取时间：
# grep -oP 'Time.*?:\s*\K[\d.]+' log_v1_np8.txt
# 从日志2里提取时间：
# grep -oP 'Time.*?:\s*\K[\d.]+' log_v2_np8.txt