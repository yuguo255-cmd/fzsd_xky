录制指定话题：在命令后列出你想录制的话题名称即可
ros2 bag record /topic_name1 /topic_name2

指定输出文件名：使用 -o 参数可以自定义保存的文件名或路径
ros2 bag record -o my_bag_record /topic_name

录制所有话题：如果想录制当前系统中所有活跃话题的数据，可以使用 --all 参数
ros2 bag record --all

不指定格式的时候默认为 SQLite

录制时指定格式：在 ros2 bag record 命令中，通过 -s mcap（或 --storage mcap）参数来指定使用 MCAP 格式
ros2 bag record -s mcap /topic_name

 MCAP 的高级特性与配置

MCAP 格式之所以更现代，是因为它支持一些灵活的配置，尤其是在性能和文件大小之间做权衡。

    预设配置（Preset Profiles）：比如 fastwrite 预设，它通过牺牲一些校验和索引功能来获得最高的写入速度，非常适合在计算资源受限的机器人上使用。
    bash

    ros2 bag record -s mcap --all --storage-preset-profile fastwrite

    自定义配置文件（Storage Config File）：你可以通过一个 YAML 文件来精细控制压缩算法、数据块大小等参数。例如，使用 Zstd 压缩算法并设置为快速模式。
    bash

    # 创建一个配置文件 my_storage_config.yaml
    ros2 bag record -s mcap --all --storage-config-file my_storage_config.yaml

总的来说，可以把 ros2 bag record 看作是“录像机”，而 MCAP 则是它可以使用的一种“高清、高效的录像带格式”。你通过 -s mcap 这个选项告诉“录像机”使用这种“录像带”。