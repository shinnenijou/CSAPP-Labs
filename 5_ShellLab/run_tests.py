import os

pwd = os.path.dirname(__file__)
ref_file = os.path.join(pwd, "ref-trace")
stu_file = os.path.join(pwd, "stu-trace")

def report_not_pass(msg):
    print(msg)
    exit(0)

def replace_pid(line : str):
    ret = line.replace("rtest", "test", 100)
    ret = ret.replace("tshref", "tsh", 100)
    ret = ret.replace("ref-trace", "stu-trace", 100)

    for i in range(len(ret) - 7):
        segment = ret[i : i + 7]
        if segment.isnumeric():
            ret = ret.replace(segment, "xxxxxxx")

    return ret

def get_lines(filename):
    with open(filename, 'r', encoding='utf-8') as file:
        return file.readlines()



def run_test(test: str):
    if os.path.isfile(ref_file):
        os.remove(ref_file)

    if os.path.isfile(stu_file):
        os.remove(stu_file)

    os.system(f"make r{test} > {ref_file}")
    os.system(f"make {test} > {stu_file}")

    ref_lines = get_lines(ref_file)
    stu_lines = get_lines(stu_file)

    if (len(ref_lines) != len(stu_lines)):
        report_not_pass("lines not equal")
    
    for i in range(4, len(ref_lines)):
        ref_line = replace_pid(ref_lines[i])
        stu_line = replace_pid(stu_lines[i])
        if ref_line != stu_line:
            report_not_pass(f"at {test} {i} line\n{ref_line} <- ref\n{stu_line} <- student")

    print(f"{test} passed")

    if os.path.isfile(ref_file):
        os.remove(ref_file)

    if os.path.isfile(stu_file):
        os.remove(stu_file) 

if __name__ == "__main__":
    run_test("test01")
    run_test("test02")
    run_test("test03")
    run_test("test04")
    run_test("test05")
    run_test("test06")
    run_test("test07")
    run_test("test08")
    run_test("test09")
    run_test("test10")
    run_test("test11")
    run_test("test12")
    run_test("test13")
    run_test("test14")
    run_test("test15")
    run_test("test16")