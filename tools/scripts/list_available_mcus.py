import argparse

from esw.cubemx.packages import list_available_mcu

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="List Boards/MCUs Available to CubeMX")
    args = parser.parse_args()

    list_available_mcu()
