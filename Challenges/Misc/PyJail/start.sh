#!/bin/sh

echo "FLAG文件失踪在某dir了你能找到它吗，我记得最后一次修改2025年5月1日……">/flag.txt

mkdir /tmp/.\\x0a\\x0b\\x00hidden
cd /tmp/.\\x0a\\x0b\\x00hidden

touch flag1 flag2 flag3 flag4 flag5
echo "miniLCTF{this_is_a_real_flag_1}" > flag1
echo "miniLCTF{this_is_a_real_flag_guess_123}" > flag2
echo "miniLCTF{this_is_the_real_flag_abc}" > flag3
echo "miniLCTF{decoy_flag_you_lost}" > flag4
echo "miniLCTF{almost_but_not_quite}" > flag5

touch -d "2025-04-20" flag1
touch -d "2025-04-25" flag2
touch -d "2025-05-02" flag3
touch -d "2025-03-15" flag4
touch -d "2025-04-30" flag5

# 创建真正的 flag 文件（藏在特定时间）
touch flagD
echo $FLAG > flagD
touch -d "2025-05-01" flagD

# 创建一些更多干扰 flag 文件
echo "miniLCTF{this_is_not_flag}" > flagA
echo "miniLCTF{definitely_flag_flag}" > flagB
echo "miniLCTF{this_is_the_real_flag}" > flagC
touch -d "2025-04-30" flagA
touch -d "2025-05-02" flagB
touch -d "2025-05-05" flagC

export FLAG=''
cd /app
python /app/app.py