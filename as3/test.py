import random

max = 212000

size1_count = 4000   # More of Category 1
size2_count = 7000   # Less of Category 2
size3_count = 11000  # Less of Category 3

size1_list = [random.randint(1, 255) for _ in range(size1_count)]
size2_list = [random.randint(256, 1023) for _ in range(size2_count)]
size3_list = [random.randint(1024, 7273) for _ in range(size3_count)]

result_list = size1_list + size2_list + size3_list
random.shuffle(result_list)

for i in result_list:
    print(i)