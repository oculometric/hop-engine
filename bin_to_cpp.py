import sys

with open(sys.argv[2],'wb') as result_file:
    result_file.write(b'unsigned char %s[] = {' % sys.argv[3].encode('utf-8'))
    binary_file = open(sys.argv[1], 'rb')
    binary_size = 0
    for b in binary_file.read():
        result_file.write(b'0x%02X,' % b)
        binary_size += 1
    result_file.write(b'};\n')
    result_file.write('unsigned long long {0}_size = {1}ull;'.format(sys.argv[3], binary_size).encode('utf-8'))