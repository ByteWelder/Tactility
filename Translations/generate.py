import csv
import sys
from pathlib import Path
from typing import List

def get_project_root():
    return Path(__file__).parent.parent.resolve()

def load_csv(path: str, delimiter: str = ",", quotechar: str = '"', encoding: str = "utf-8", skip_header: bool = False) -> List[List[str]]:
    """
    Load a CSV file into a list of rows, where each row is a list of strings.

    Args:
        path: Path to the CSV file.
        delimiter: Field delimiter character.
        quotechar: Quote character.
        encoding: File encoding.
        skip_header: If True, skip the first row (header).

    Returns:
        List of rows (list of lists of strings).
    """
    rows: List[List[str]] = []
    with open(path, "r", encoding=encoding, newline="") as f:
        reader = csv.reader(f, delimiter=delimiter, quotechar=quotechar)
        if skip_header:
            next(reader, None)
        for row in reader:
            rows.append(row)
    return rows

def print_help():
    print("Usage: python generate.py [csv_file] [header_file_path] [header_namespace] [i18n_directory]\n\n")
    print("\t[csv_file]            the CSV file containing the translations, exported from the .ods file")
    print("\t[header_file_path]    the path to the header file to be generated")
    print("\t[header_namespace]    the C++ namespace to use for the generated header file")
    print("\t[i18n_directory]      the directory where the .i18n files will be generated")

def open_i18n_files(row, i18n_path):
    result = []
    for i in range(1, len(row)):
        filepath = f"{i18n_path}/{row[i]}.i18n"
        print(f"Opening {filepath}")
        file = open(filepath, "w")
        result.append(file)
    return result

def close_i18n_files(files):
    for file in files:
        file.close()

def generate_header(filepath, namespace, rows):
    file = open(filepath, "w")
    file.write("#pragma once\n\n")
    file.write("#include \"Tactility/i18n/TextResources.h\"\n\n")
    file.write("// WARNING: This file is auto-generated. Do not edit manually.\n\n")
    file.write(f"namespace {namespace}")
    file.write(" {\n\n")
    file.write("enum class Text {\n")
    for i in range(1, len(rows)):
        key = rows[i][0].upper()
        file.write(f"    {key} = {i - 1},\n")
    file.write("};\n")
    file.write("\n}\n")
    file.close()

def translate(rows, language_index, file):
    for i in range(1, len(rows)):
        value = rows[i][language_index]
        if value == "":
            value = f"{rows[i][0]}_untranslated"
        file.write(value)
        file.write("\n")

if __name__ == "__main__":
    if "--help" in sys.argv:
        print_help()
        sys.exit()
    if len(sys.argv) != 5:
        print_help()
        sys.exit()
    project_root_path = get_project_root()
    csv_file = f"{project_root_path}/Translations/{sys.argv[1]}"
    header_path = f"{project_root_path}/{sys.argv[2]}"
    header_namespace = sys.argv[3]
    i18n_path = f"{project_root_path}/{sys.argv[4]}"
    rows = load_csv(csv_file)
    if len(rows) == 0:
        print("Error: CSV file is empty.")
        sys.exit(1)
    generate_header(header_path, header_namespace, rows)
    i18n_files = open_i18n_files(rows[0], i18n_path)
    for i in range(0, len(i18n_files)):
        i18n_file = i18n_files[i]
        translate(rows, i + 1, i18n_file)
    close_i18n_files(i18n_files)
