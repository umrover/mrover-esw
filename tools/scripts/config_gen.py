import argparse
from pathlib import Path

from esw.config import generator

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate Config Struct from yaml")
    parser.add_argument(
        "--name", "-n", type=str, required=True, help="Name of project source directory to generate config for"
    )
    parser.add_argument(
        "--input",
        "-i",
        type=Path,
        required=True,
        help="Path to yaml file describing config",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        required=True,
        help="Path to file where config will be generated",
    )
    parser.add_argument(
        "--tabsize",
        type=int,
        required=False,
        default=4,
        help="Tab size in output file",
    )
    parser.add_argument(
        "--template-dir",
        type=Path,
        required=False,
        default=Path.cwd(),
        help="directory containing config_header_hpp.j2 (default: current directory)",
    )

    args = parser.parse_args()

    gen: generator.ConfigGen = generator.ConfigGen(args.name, args.tabsize, args.template_dir)

    gen.generate_config_header(args.input, args.output)
