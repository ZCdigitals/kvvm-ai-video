import argparse

def int2hex(num):
    if num < 0:
        num = (1<<32)+num
    return hex(num)

def main():
    parser = argparse.ArgumentParser(description='将整数转换为十六进制')
    parser.add_argument('number', type=int, help='要转换的整数')

    args = parser.parse_args()

    num = args.number

    print(f"{num} -> {int2hex(num)}")

if __name__ == "__main__":
    main()
