# gen_csv.py —— 生成巡逻航点文件 patrol.csv
#
# 用法（两步）：
#   1) 改下面 points 里的坐标（每行一个航点，格式：x, y, 到达后等待秒数）
#   2) 在终端里运行：  python3 gen_csv.py
#      运行完会生成 patrol.csv

# 每个点是 (x坐标, y坐标, 到达后停几秒)
points = [
    (0.0, 0.0, 0.0),     # 第 1 个航点
    (1.5, -1.0, 3.0),    # 第 2 个航点：到达后停 3 秒
    (3.0, -0.5, 0.0),    # 第 3 个航点
]

with open('patrol.csv', 'w') as f:
    # 第一行必须是表头（代码会跳过它）
    f.write("id,pose_x,pose_y,pose_z,rot_x,rot_y,rot_z,rot_w,command,wait_sec\n")
    # 逐点写出行
    for i, (x, y, wait) in enumerate(points):
        f.write(f"{i},{x},{y},0.0,0.0,0.0,0.0,1.0,,{wait}\n")

print("已生成 patrol.csv，共", len(points), "个航点")
