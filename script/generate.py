# Script to generate libpng-loader.h
# Usage:
#   1. Put "png.h" and "libpng-loader-base.h" into the same directory as generate.py
#   2. Run "python generate.py" (requires python 3.10 or later)
#   3. It generates "libpng-loader-generated.h" in the same directory as generate.py

import argparse
import os


def split_in_two(line, c):
    ret = line.split(c, 1)
    if len(ret) == 1:
        return ret[0].strip(), ""
    return ret[0].strip(), ret[1].strip()


def rsplit_in_two(line, c):
    ret = line.rsplit(c, 1)
    if len(ret) == 1:
        return ret[0].strip(), ""
    return ret[0].strip(), ret[1].strip()


def contain_one_of_items(s, items):
    for i in items:
        if i in s:
            return True
    return False


def trim_space_and_comment(file, line):
    line = line.replace('\t', ' ')
    line, _ = split_in_two(line, "//")
    if line.count("\"") % 2 != 0:
        raise RuntimeError("There is \"//\" in a C string. This is unexpected.")

    if "/*" in line:
        line, rest = rsplit_in_two(line, "/*")
        if "*/" in rest:
            # The comment block (/* */) is a single line
            _, rest = split_in_two(line, "*/")
            if rest != "":
                raise RuntimeError(
                    "The last line of multi-line comment does not end with \"*/\". "
                    "This is unexpected.")
        else:
            while True:
                # skip multi-line comment
                comment = file.readline()
                if comment == "":
                    break
                if "*/" in comment:
                    _, rest = split_in_two(comment, "*/")
                    if rest != "":
                        raise RuntimeError(
                            "The last line of multi-line comment does not end with \"*/\". "
                            "This is unexpected.")
                    break
    line = line.strip()
    return line


def skip_to_endif(file):
    # skip to #endif
    while True:
        line = file.readline()
        if line == "":
            break
        line = line.strip()
        if line.startswith("#endif"):
            break


def read_lines_to_char(file, first_line, c):
    string = first_line
    while True:
        if string.endswith(c):
            break
        # Read more lines
        line = file.readline()
        line = trim_space_and_comment(file, line)
        if line == "":
            continue
        string += " " + line
    return string


def read_lines_to_semicolon(file, first_line):
    return read_lines_to_char(file, first_line, ";")


def read_lines_to_right_paren(file, first_line):
    return read_lines_to_char(file, first_line, ")")


def type_normalize(t):
    if t.endswith("charp"):
        t = t[:-1] + " *"
    if t.endswith("colorp"):
        t = t[:-1] + " *"
    if t.endswith("point_p"):
        t = t[:-2] + " *"
    if t.endswith("rp"):
        t = t[:-2] + " *"
    if t.endswith("pp"):
        t = t[:-2] + " **"
    if t.endswith("p"):
        t = t[:-1] + " *"
    if t.startswith("png_const_"):
        t = "const png_" + t[10:]
    if t.endswith(" PNG_RESTRICT"):
        t = t[:-13]
    return t


class Macro:
    def __init__(self):
        self.first_line = ""
        self.lines = []

    def read(self, file, first_line):
        # first_line = "#define ..."
        #           or "#  define ..."
        if first_line.startswith("#  "):
            first_line = "#" + first_line[3:]
        if not first_line.endswith("\\"):
            # single line macro
            first_line = first_line[8:]
            name, rest = split_in_two(first_line, " ")
            if rest == "":
                self.first_line = f"#define {name}"
            else:
                self.first_line = f"#define {name} {rest}"
            return
        # multi-line macro
        self.first_line = first_line[:-1].strip() + " \\"
        while True:
            line = file.readline()
            if line == "":
                break
            line = line.strip().replace(" * ", " *")
            last_line = not line.endswith("\\")
            if not last_line:
                line = line[:-1].strip()
            if line.endswith("*/"):
                line, _ = rsplit_in_two(line, "/*")
            self.lines.append(line)
            if last_line:
                break

    def to_str(self):
        string = self.first_line + "\n"
        if len(self.lines) == 0:
            return string
        for l in self.lines[:-1]:
            string += f"    {l} \\\n"
        string += f"    {self.lines[-1]}\n"
        return string


class Var:
    def __init__(self, ty, name):
        self.ty = type_normalize(ty)
        if name.startswith("**"):
            self.ty += " **"
            name = name[2:]
        if name.startswith("*"):
            self.ty += " *"
            name = name[1:]
        self.name = name

    def to_str(self):
        if self.name == "":
            return self.ty
        if self.ty.endswith("*"):
            return self.ty + self.name
        return f"{self.ty} {self.name}"


class StructDef:
    def __init__(self):
        self.struct_name = ""
        self.name = ""
        self.members = []
        self.macros = []

    def read(self, file, first_line):
        # typedef struct struct_name
        # {
        # } name, ***;
        self.struct_name = first_line[14:].strip()
        _ = file.readline()

        while True:
            line = file.readline()
            if line == "":
                break
            line = trim_space_and_comment(file, line)
            if line == "":
                continue
            if line.startswith("#define") or line.startswith("#  define"):
                macro = Macro()
                macro.read(file, line)
                self.macros.append(macro)
                continue
            line = read_lines_to_semicolon(file, line)
            if line.startswith("}"):
                # line = "} struct_name;" or "} name, name_p;"
                _, line = split_in_two(line[:-1], " ")
                self.name, _ = split_in_two(line, ",")
                if " " in self.name:
                    raise RuntimeError(f"Unexpected struct name. ({self.name})")
                return

            # line = "member_type member_name;"
            member_type, member_name = rsplit_in_two(line[:-1], " ")
            self.members.append(Var(member_type, member_name))

    def to_str(self):
        member_str = "".join([f"    {member.to_str()};\n" for member in self.members])
        if self.struct_name == "":
            first_line = "typedef struct {\n"
        else:
            first_line = "typedef struct " + self.struct_name + " {\n"
        return first_line + member_str + "} " + self.name + ";\n"


class FuncDef:
    def __init__(self):
        self.return_type = ""
        self.name = ""
        self.args = []

    def read(self, file, first_line):
        # typedef PNG_CALLBACK(return_type, name, (arg_type0 arg_name0, arg_type1 arg_name1, ...));"
        # PNG_EXPORT(ord, return_type, name, (arg_type0 arg_name0, arg_type1 arg_name1, ...));"
        # PNG_EXPORTA(ord, return_type, name, (arg_type0 arg_name0, arg_type1 arg_name1, ...), attr);"
        # PNG_FP_EXPORT(ord, return_type, name, (arg_type0 arg_name0, arg_type1 arg_name1, ...))"
        # PNG_FIXDED_EXPORT(ord, return_type, name, (arg_type0 arg_name0, arg_type1 arg_name1, ...))"

        if first_line.startswith("PNG_FP_") or first_line.startswith("PNG_FIXED_"):
            def_str = read_lines_to_right_paren(file, first_line)
        else:
            def_str = read_lines_to_semicolon(file, first_line)
            def_str = def_str[:-1]
        def_str = def_str[:-1]
        if first_line.startswith("PNG_EXPORTA") or first_line.startswith("PNG_FUNCTION"):
            def_str, _ = rsplit_in_two(def_str, ",")
        _, def_str = split_in_two(def_str, "(")
        if not first_line.startswith("typedef") and not first_line.startswith("PNG_FUNCTION"):
            _, def_str = split_in_two(def_str, ",")

        # read return type
        # def_str = "return_type, name, (arg_type0 arg_name0, arg_type1 arg_name1, ...)"
        ret_type, def_str = split_in_two(def_str, ",")
        self.return_type = type_normalize(ret_type)

        # read function name
        name, def_str = split_in_two(def_str, ",")
        if (name.startswith("(")):
            _, name = split_in_two(name[:-1], " ")
        self.name = name
        def_str = def_str[1:-1]

        # def_str = "arg_type0 arg_name0, arg_type1 arg_name1, ..."
        while True:
            arg, def_str = split_in_two(def_str, ",")
            if arg == "":
                break
            arg_type, arg_name = rsplit_in_two(arg, " ")
            if "[" in arg_name:
                arg_name, _ = split_in_two(arg_name, "[")
                arg_name = "*" + arg_name
            self.args.append(Var(arg_type, arg_name))

    def to_str(self):
        args_as_str = ", ".join([arg.to_str() for arg in self.args])
        return f"{self.return_type} {self.name}({args_as_str});"

    def to_callback_str(self):
        args_as_str = ", ".join([arg.ty for arg in self.args])
        return f"typedef {self.return_type} ({self.name})({args_as_str});"

    def to_pfn_str(self):
        args_as_str = ", ".join([arg.ty for arg in self.args])
        return f"typedef {self.return_type} (*PFN_{self.name})({args_as_str});"


class HeaderGenerator:
    def __init__(self, optional_keywords, remove_keywords, optional_functions, remove_functions):
        self.clear()
        self.optional_keywords = optional_keywords
        self.remove_keywords = remove_keywords
        self.optional_functions = optional_functions
        self.remove_functions = remove_functions

    def clear(self):
        self.macros = []
        self.structs = []
        self.callbacks = []
        self.functions = []

    def read_png_h(self, png_h):
        self.clear()
        with open(png_h, "r", encoding="utf-8") as file:
            print(f"reading {png_h}...")
            while True:
                line = file.readline()
                if line == "":
                    break
                line = trim_space_and_comment(file, line)
                if line.startswith("#else"):
                    skip_to_endif(file)
                elif line.startswith("#define") or line.startswith("#  define"):
                    macro = Macro()
                    macro.read(file, line)
                    self.macros.append(macro)
                elif "typedef struct" in line and not line.endswith(";"):
                    struct = StructDef()
                    struct.read(file, line)
                    self.macros += struct.macros
                    self.structs.append(struct)
                if (line.startswith("PNG_FUNCTION(") or
                    line.startswith("typedef PNG_CALLBACK(")):
                    func = FuncDef()
                    func.read(file, line)
                    self.callbacks.append(func)
                if (line.startswith("PNG_EXPORT(") or
                    line.startswith("PNG_EXPORTA(") or
                    line.startswith("PNG_FP_EXPORT(") or
                    line.startswith("PNG_FIXED_EXPORT(")):
                    func = FuncDef()
                    func.read(file, line)
                    self.functions.append(func)

        print(f"Found {len(self.macros)} macros.")
        print(f"Found {len(self.structs)} structs.")
        print(f"Found {len(self.callbacks)} callbacks.")
        print(f"Found {len(self.functions)} functions.")

    def write_loader_h(self, basepath, outpath):
        with open(basepath, "r", encoding="utf-8") as infile:
            print(f"reading {basepath}...")
            base_lines = infile.readlines()[1:]

        with open(outpath, "w", encoding="utf-8") as outfile:
            print(f"writing {outpath}...")
            outfile.write("// This file was generated by script/generate.py\n")
            for i in range(0, len(base_lines)):
                line = base_lines[i]
                if line.startswith("// ------ Macros"):
                    break
                outfile.write(line)
            macro_line = base_lines[i]
            base_lines = base_lines[i + 1:]

            outfile.write("// public structs\n")
            for st in self.structs:
                outfile.write(st.to_str())
                outfile.write(
                    "\n"
                    f"typedef {st.name} *{st.name}p;  // deprecated\n"
                    "\n"
                )

            outfile.write(macro_line + "\n")

            for m in self.macros:
                outfile.write(m.to_str())

            for i in range(0, len(base_lines)):
                line = base_lines[i]
                if line.startswith("// ------ Function Pointers"):
                    break
                outfile.write(line)
            base_lines = base_lines[i:]
            outfile.write(
                "    REMOVE_API(png_plz_ignore_this_line)"
            )

            def is_optional(name):
                return name in self.optional_functions or contain_one_of_items(name, self.optional_keywords)

            def is_removed(name):
                return name in self.remove_functions or contain_one_of_items(name, self.remove_keywords)

            for func in self.functions:
                if is_removed(func.name):
                    # libpng-loader should not load this function
                    outfile.write(f" \\\n    REMOVE_API({func.name})")
                elif is_optional(func.name):
                    # libpng-loader should not force this function to be exist
                    outfile.write(f" \\\n    LIBPNG_OPT({func.name})")
                else:
                    outfile.write(f" \\\n    LIBPNG_MAP({func.name})")
            outfile.write(
                "\n"
                "\n"
                "// ------ Callback Types ------\n"
                "\n"
            )
            for func in self.callbacks:
                outfile.write(func.to_callback_str())
                outfile.write("\n")
            outfile.write(
                "\n"
                "// ------ Function Types ------\n"
                "\n"
            )
            for func in self.functions:
                outfile.write(func.to_pfn_str())
                outfile.write("\n")

            outfile.write("\n")
            for line in base_lines:
                outfile.write(line)


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--png_h", default=None, type=str, help="path to png.h")
    parser.add_argument("--base_h", default=None, type=str, help="path to libpng-loader-base.h")
    parser.add_argument("--generated_h", default=None, type=str, help="path to libpng-loader-generated.h")
    parser.add_argument("--overwrite_loader", action='store_true',
        help="overwrite libpng-loader.h instead of generating libpng-loader-generated.h")

    default_optional_keywords = [
        "eXIf", # added at 1.6.31 (July, 2017)
        "cICP", # added at 1.6.45 (Jan, 2025)
        "mDCV", # added at 1.6.46 (Jan, 2025)
        "cLLI", # added at 1.6.46 (Jan, 2025)
    ]
    parser.add_argument(
        "--optional_keywords",
        default=",".join(default_optional_keywords), type=str,
        help="comma separeted list of keywords to make APIs optional which contain one of the keywords"
    )
    default_remove_keywords = [
        # We should not pass FILE* to libpng because it can cause invalid memory access
        # https://learn.microsoft.com/en-us/cpp/c-runtime-library/potential-errors-passing-crt-objects-across-dll-boundaries?view=msvc-170
        "stdio",
        "init_io",
        # We should not call longjmp from libpng because it can cause invalid memory access
        "jmp",
    ]
    parser.add_argument(
        "--remove_keywords",
        default=",".join(default_remove_keywords), type=str,
        help="comma separeted list of keywords to remove APIs which contain one of the keywords"
    )
    default_optional_functions = [
        # Following functions are not included in common libpng binaries.
        "png_err",
        "png_set_strip_error_numbers",
    ]
    parser.add_argument(
        "--optional_functions",
        default=",".join(default_optional_functions), type=str,
        help="comma separeted list of functions to make them optional"
    )
    parser.add_argument(
        "--remove_functions", default="", type=str,
        help="comma separeted list of functions to remove them from libpng-loader"
    )
    return parser.parse_args()

if __name__ == "__main__":
    # Get args
    args = get_args()
    png_h = args.png_h
    base_h = args.base_h
    generated_h = args.generated_h
    optional_keywords = [s for s in args.optional_keywords.split(",") if len(s) > 0]
    remove_keywords = [s for s in args.remove_keywords.split(",") if len(s) > 0]
    optional_functions = [s for s in args.optional_functions.split(",") if len(s) > 0]
    remove_functions = [s for s in args.remove_functions.split(",") if len(s) > 0]

    # Set default paths
    base_dir = os.path.dirname(os.path.abspath(__file__))
    if png_h is None:
        png_h = os.path.join(base_dir, "libpng", "png.h")
    if base_h is None:
        base_h = os.path.join(base_dir, "libpng-loader-base.h")
    if generated_h is None:
        if args.overwrite_loader:
            generated_h = os.path.join(os.path.dirname(base_dir), "libpng-loader.h")
        else:
            generated_h = os.path.join(base_dir, "libpng-loader-generated.h")

    # Generate libpng-loader-generated.h
    generator = HeaderGenerator(
        optional_keywords, remove_keywords,
        optional_functions, remove_functions)
    generator.read_png_h(png_h)
    generator.write_loader_h(base_h, generated_h)
    print("done!")
