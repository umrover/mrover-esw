from esw import esw_logger
from esw.cubemx import run_cubemx_script


def list_available_mcu():
    script = f"""
load STM32IMNOTREAL
exit
"""
    esw_logger.debug(script)
    result, output = run_cubemx_script(script, return_output_on_failure=True)
    if result:
        err = f"STM32IMNOTREAL exists?!?! cool new chip ig :shrug:"
        esw_logger.error(err)
        raise RuntimeError(err)

    print(output)