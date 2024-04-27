import re
import sys

def extract_args(param_string):
    if param_string.strip() == "void":
        return ""
    args = []
    params = param_string.split(',')
    for param in params:
        name = re.match(".*?([A-Za-z_][A-Za-z0-9_]*)$", param.strip())
        if not name:
            print(param)
        args.append(name.group(1).strip())
    return ', '.join(args)

# Define the regex pattern
pattern = r'^\s*([\w\*\s]+)\s+(\w+)\s*\(([^)]*)\)\s*;'

# Replacement function
def replace_function(match):
    returntype = match.group(1).strip()
    func_name = match.group(2)
    params = match.group(3).strip()
    args = extract_args(params)
    ret = "return"
    if returntype == "void":
        ret = ""
    return f'PROC({returntype}, {func_name}, ({params}), ({args}),{ret});'

# Read and write files
with open(sys.argv[1], 'r') as file:
    for line in file:
        if not re.match(pattern, line):
            continue
        transformed_line = re.sub(pattern, replace_function, line).strip()
        print(transformed_line)
